// Backend nativo de Nintendo Switch para FTL: Faster Than Light
// Implementa la interfaz platform.h utilizando libnx, EGL y SDL2 Audio.

#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>

#include <switch.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <SDL2/SDL.h>

#include "port_log.h"
#include "port_main.h"
#include "ux/ux.h"
#include "fw/fw.h"
#include "fw/present.h"

// Configuración de libnx: aumentamos la pila del hilo principal a 8 MB
// (el valor por defecto de libnx es de solo 128 KB, lo que provocaba desbordamiento de pila
// inmediato en LoadAnimations al reservar 68 KB en un único frame de llamada).
size_t __nx_stack_size = 8 * 1024 * 1024;

// Dimensiones de pantalla de Switch: 720p en modo portátil, 1080p en dock
static int g_sw = 1280;
static int g_sh = 720;

// Estado EGL
static EGLDisplay g_dpy = EGL_NO_DISPLAY;
static EGLConfig g_cfg;
static EGLSurface g_surf = EGL_NO_SURFACE;
static EGLContext g_ctx = EGL_NO_CONTEXT;

// Estado de Audio
static SDL_AudioDeviceID g_audio_dev = 0;
static plat_audio_cb g_audio_cb = NULL;
static void *g_audio_ud = NULL;
static int g_audio_rate = 44100;
static int g_audio_channels = 2;
static bool g_audio_paused = true;

// Estado de Mandos (Joy-Con / Pro Controller)
static PadState g_pad;

// Estado de Pantalla Táctil
#define MAX_TOUCHES 10
typedef struct {
    int id;
    float x;
    float y;
    bool active;
} SwitchTouch;
static SwitchTouch g_touches[MAX_TOUCHES];

// Códigos de botón compatibles con ux/pad.c
enum {
    K_DPAD_UP = 19, K_DPAD_DOWN = 20, K_DPAD_LEFT = 21, K_DPAD_RIGHT = 22,
    K_A = 96, K_B = 97, K_X = 99, K_Y = 100,
    K_L1 = 102, K_R1 = 103, K_L2 = 104, K_R2 = 105,
    K_L3 = 106, K_R3 = 107, K_START = 108, K_SELECT = 109,
};

static double current_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
}

// ---------------------------------------------------------------------------
// Ciclo de vida y bombeo de eventos
// ---------------------------------------------------------------------------
void plat_main_thread_started(void) {
    LOGI("plat_switch: hilo principal iniciado");
}

void plat_wake_main(void) {}

void plat_pump_events(void) {
    // Comprobamos si el usuario solicita salir mediante la interfaz de Horizon
    if (!appletMainLoop()) {
        LOGI("plat_switch: señal de cierre recibida de Horizon");
        ux_quit();
        return;
    }

    // Comprobamos cambios en el modo de funcionamiento (portátil vs dock)
    // Mantenemos 1280x720 de forma constante ya que FTL de iPad renderiza internamente a 720p
    // y el sistema Horizon escala automáticamente la imagen a 1080p en el televisor.
    AppletOperationMode op_mode = appletGetOperationMode();
    (void)op_mode;

    // 1. Lectura de mandos
    padUpdate(&g_pad);
    uint64_t buttons_down = padGetButtonsDown(&g_pad);
    uint64_t buttons_up = padGetButtonsUp(&g_pad);
    uint64_t buttons_held = padGetButtons(&g_pad);

    struct { uint64_t mask; int keycode; } button_map[] = {
        { HidNpadButton_A,      K_A },
        { HidNpadButton_B,      K_B },
        { HidNpadButton_X,      K_X },
        { HidNpadButton_Y,      K_Y },
        { HidNpadButton_L,      K_L1 },
        { HidNpadButton_R,      K_R1 },
        { HidNpadButton_ZL,     K_L2 },
        { HidNpadButton_ZR,     K_R2 },
        { HidNpadButton_StickL, K_L3 },
        { HidNpadButton_StickR, K_R3 },
        { HidNpadButton_Plus,   K_START },
        { HidNpadButton_Minus,  K_SELECT },
        { HidNpadButton_Up,     K_DPAD_UP },
        { HidNpadButton_Down,   K_DPAD_DOWN },
        { HidNpadButton_Left,   K_DPAD_LEFT },
        { HidNpadButton_Right,  K_DPAD_RIGHT },
    };

    for (size_t i = 0; i < sizeof(button_map) / sizeof(button_map[0]); i++) {
        if (buttons_down & button_map[i].mask) {
            ux_pad_button(button_map[i].keycode, true);
        }
        if (buttons_up & button_map[i].mask) {
            ux_pad_button(button_map[i].keycode, false);
        }
    }

    // Leemos los sticks analógicos (normalizamos al rango -1.0 a 1.0)
    HidAnalogStickState stick_l = padGetStickPos(&g_pad, 0);
    HidAnalogStickState stick_r = padGetStickPos(&g_pad, 1);

    float axes[8] = {0};
    axes[0] = (float)stick_l.x / 32767.0f;
    axes[1] = -(float)stick_l.y / 32767.0f; // Invertimos Y para coordenadas de pantalla
    axes[2] = (float)stick_r.x / 32767.0f;
    axes[3] = -(float)stick_r.y / 32767.0f;
    axes[4] = (buttons_held & HidNpadButton_ZL) ? 1.0f : 0.0f;
    axes[5] = (buttons_held & HidNpadButton_ZR) ? 1.0f : 0.0f;
    axes[6] = ((buttons_held & HidNpadButton_Right) ? 1.0f : 0.0f) - ((buttons_held & HidNpadButton_Left) ? 1.0f : 0.0f);
    axes[7] = ((buttons_held & HidNpadButton_Down) ? 1.0f : 0.0f) - ((buttons_held & HidNpadButton_Up) ? 1.0f : 0.0f);

    ux_pad_axes(axes);

    // 2. Lectura de pantalla táctil
    HidTouchScreenState touch_state;
    if (hidGetTouchScreenStates(&touch_state, 1) > 0) {
        double now = current_time_seconds();
        bool seen[MAX_TOUCHES] = {false};

        for (int32_t i = 0; i < touch_state.count && i < MAX_TOUCHES; i++) {
            HidTouchState *t = &touch_state.touches[i];
            int slot = -1;
            for (int s = 0; s < MAX_TOUCHES; s++) {
                if (g_touches[s].active && g_touches[s].id == (int)t->finger_id) {
                    slot = s;
                    break;
                }
            }
            if (slot == -1) {
                // Nuevo toque
                for (int s = 0; s < MAX_TOUCHES; s++) {
                    if (!g_touches[s].active) {
                        slot = s;
                        g_touches[s].active = true;
                        g_touches[s].id = (int)t->finger_id;
                        g_touches[s].x = (float)t->x;
                        g_touches[s].y = (float)t->y;
                        ux_handle_touch(0, t->finger_id, (float)t->x, (float)t->y, now); // TOUCH_BEGAN
                        break;
                    }
                }
            } else {
                // Toque continuo
                g_touches[slot].x = (float)t->x;
                g_touches[slot].y = (float)t->y;
                ux_handle_touch(1, t->finger_id, (float)t->x, (float)t->y, now); // TOUCH_MOVED
            }
            if (slot >= 0) seen[slot] = true;
        }

        // Finalizamos toques que ya no están presentes
        for (int s = 0; s < MAX_TOUCHES; s++) {
            if (g_touches[s].active && !seen[s]) {
                g_touches[s].active = false;
                ux_handle_touch(3, g_touches[s].id, g_touches[s].x, g_touches[s].y, now); // TOUCH_ENDED
            }
        }
    }

    // 3. Tick del sistema de interfaz
    ux_tick();
}

// ---------------------------------------------------------------------------
// Gráficos (EGL / OpenGL ES)
// ---------------------------------------------------------------------------
void *plat_gl_create_context(int es_major, void *share) {
    EGLint attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, es_major,
        EGL_NONE
    };
    EGLContext ctx = eglCreateContext(g_dpy, g_cfg, share ? (EGLContext)share : EGL_NO_CONTEXT, attrs);
    if (ctx == EGL_NO_CONTEXT) {
        LOGE("plat_switch: eglCreateContext falló: %#x", eglGetError());
        return NULL;
    }
    g_ctx = ctx;
    return ctx;
}

bool plat_gl_make_current(void *ctx) {
    if (!ctx) return eglMakeCurrent(g_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    bool ok = eglMakeCurrent(g_dpy, g_surf, g_surf, (EGLContext)ctx);
    if (ok) {
        LOGI("plat_switch: GL activado: %s (%s)", glGetString(GL_RENDERER), glGetString(GL_VERSION));
    }
    return ok;
}

void plat_gl_present(void) {
    if (g_surf != EGL_NO_SURFACE) {
        eglSwapBuffers(g_dpy, g_surf);
    }
}

bool plat_gl_has_surface(void) {
    return g_surf != EGL_NO_SURFACE;
}

void plat_gl_surface_size(int *w, int *h) {
    if (w) *w = g_sw;
    if (h) *h = g_sh;
}

void *plat_gl_get_proc(const char *name) {
    return (void *)eglGetProcAddress(name);
}

unsigned plat_gl_window_fbo(void) {
    // 0 representa el framebuffer nativo de la ventana EGL
    return 0;
}

// ---------------------------------------------------------------------------
// Audio (SDL2)
// ---------------------------------------------------------------------------
static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata;
    if (g_audio_paused || !g_audio_cb) {
        memset(stream, 0, len);
        return;
    }
    int frames = len / (g_audio_channels * sizeof(int16_t));
    g_audio_cb(g_audio_ud, (int16_t *)stream, frames);
}

bool plat_audio_open(int rate, int channels, plat_audio_cb cb, void *ud) {
    g_audio_rate = rate;
    g_audio_channels = channels;
    g_audio_cb = cb;
    g_audio_ud = ud;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        LOGE("plat_switch: SDL_InitSubSystem(AUDIO) falló: %s", SDL_GetError());
        return false;
    }

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = rate;
    want.format = AUDIO_S16SYS;
    want.channels = (Uint8)channels;
    want.samples = 1024;
    want.callback = sdl_audio_callback;
    want.userdata = ud;

    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (g_audio_dev == 0) {
        LOGE("plat_switch: SDL_OpenAudioDevice falló: %s", SDL_GetError());
        return false;
    }

    LOGI("plat_switch: audio abierto: %d Hz, %d canales", have.freq, have.channels);
    g_audio_paused = false;
    SDL_PauseAudioDevice(g_audio_dev, 0);
    return true;
}

void plat_audio_pause(bool paused) {
    g_audio_paused = paused;
    if (g_audio_dev) {
        SDL_PauseAudioDevice(g_audio_dev, paused ? 1 : 0);
    }
}

int plat_audio_native_rate(void) {
    // La tasa nativa del hardware de audio de Switch es 48000 Hz
    return 48000;
}

// ---------------------------------------------------------------------------
// Servicios de UI y periféricos
// ---------------------------------------------------------------------------
void plat_request_text_input(const char *prompt, const char *initial) {
    // Invocamos el teclado por software oficial de Switch (swkbd)
    SwkbdConfig kbd;
    Result rc = swkbdCreate(&kbd, 0);
    if (R_SUCCEEDED(rc)) {
        swkbdConfigMakePresetDefault(&kbd);
        if (prompt) swkbdConfigSetGuideText(&kbd, prompt);
        if (initial) swkbdConfigSetInitialText(&kbd, initial);
        char out_str[256] = {0};
        swkbdShow(&kbd, out_str, sizeof(out_str));
        swkbdClose(&kbd);
        LOGI("plat_switch: texto introducido en swkbd: %s", out_str);
    }
}

void plat_cancel_text_input(void) {}

void plat_show_alert(const char *title, const char *msg, const char **buttons, int nbuttons) {
    LOGI("plat_switch: ALERTA [%s]: %s (opciones: %d)", title ? title : "", msg ? msg : "", nbuttons);
    fw_alert_result(0);
}

void plat_open_url(const char *url) {
    LOGI("plat_switch: solicitud de apertura de URL: %s", url);
}

void plat_request_quit(void) {
    ux_quit();
}

void plat_keep_screen_on(bool on) {
    (void)on;
}

void plat_haptic(int kind) {
    (void)kind;
}

void plat_pad_rumble(float strength, int ms) {
    (void)strength;
    (void)ms;
}

// ---------------------------------------------------------------------------
// Sistema de registro a archivo en SD y consola
// ---------------------------------------------------------------------------
static FILE *s_logfile = NULL;

void switch_log_init(const char *path) {
    if (s_logfile) {
        fclose(s_logfile);
        s_logfile = NULL;
    }
    s_logfile = fopen(path, "w");
}

void switch_log(const char *prefix, const char *fmt, ...) {
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s%s\n", prefix, buf);
    if (s_logfile) {
        fprintf(s_logfile, "%s%s\n", prefix, buf);
        fflush(s_logfile);
    }
}

// ---------------------------------------------------------------------------
// Punto de entrada principal (main)
// ---------------------------------------------------------------------------
int main(int argc, char **argv) {
    (void)argc; (void)argv;

    // Creamos directorios de trabajo iniciales y abrimos el registro
    mkdir("sdmc:/switch", 0755);
    mkdir("sdmc:/switch/ftl", 0755);
    mkdir("sdmc:/switch/ftl/logs", 0755);
    mkdir("sdmc:/switch/ftl/save", 0755);
    mkdir("sdmc:/switch/ftl/cache", 0755);
    switch_log_init("sdmc:/switch/ftl/logs/ftl.log");

    LOGI("==================================================");
    LOGI("  FTL: Faster Than Light - Nintendo Switch Port   ");
    LOGI("==================================================");

    AppletType at = appletGetAppletType();
    LOGI("plat_switch: Modo de ejecucion: AppletType=%d (%s)", (int)at,
         (at == AppletType_Application || at == AppletType_SystemApplication)
             ? "Title Override / Full RAM"
             : "Modo Applet / RAM Limitada");

    Handle proc = envGetOwnProcessHandle();
    LOGI("plat_switch: envGetOwnProcessHandle = 0x%08x", proc);

    u64 mem_available = 0, mem_used = 0;
    svcGetInfo(&mem_available, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
    svcGetInfo(&mem_used, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0);
    LOGI("plat_switch: Memoria del proceso: Total=%llu MB, Usada=%llu MB, Libre=%llu MB",
         mem_available / (1024 * 1024), mem_used / (1024 * 1024),
         (mem_available > mem_used ? (mem_available - mem_used) : 0) / (1024 * 1024));

    // Rutas del juego en la tarjeta MicroSD (con '/' inicial para compatibilidad con código POSIX de FTL)
    const char *base_dir = "/switch/ftl";
    char bin_path[512], bundle_path[512], docs_path[512], cache_path[512], log_path[512];

    snprintf(bin_path, sizeof(bin_path), "%s/FTL", base_dir);
    snprintf(bundle_path, sizeof(bundle_path), "%s/FTL.app", base_dir);
    snprintf(docs_path, sizeof(docs_path), "%s/save", base_dir);
    snprintf(cache_path, sizeof(cache_path), "%s/cache", base_dir);
    snprintf(log_path, sizeof(log_path), "%s/logs", base_dir);

    // Comprobamos la presencia de los archivos del juego
    struct stat st;
    if (stat(bin_path, &st) != 0) {
        char alt_bin[512];
        snprintf(alt_bin, sizeof(alt_bin), "sdmc:%s", bin_path);
        if (stat(alt_bin, &st) == 0) {
            // El archivo es accesible a través del dispositivo sdmc:
        } else if (stat("FTL", &st) == 0) {
            snprintf(bin_path, sizeof(bin_path), "FTL");
            snprintf(bundle_path, sizeof(bundle_path), "FTL.app");
            snprintf(docs_path, sizeof(docs_path), "save");
            snprintf(cache_path, sizeof(cache_path), "cache");
            snprintf(log_path, sizeof(log_path), "logs");
        } else {
            port_fatal("No se encuentra el binario ejecutable 'FTL' en:\n  %s\n\n"
                       "Asegúrate de copiar la carpeta 'ftl' completa dentro de sdmc:/switch/.", bin_path);
            return 1;
        }
    }
    LOGI("plat_switch: Binario 'FTL' encontrado (%lld bytes)", (long long)st.st_size);

    if (stat(bundle_path, &st) != 0) {
        char alt_bundle[512];
        snprintf(alt_bundle, sizeof(alt_bundle), "sdmc:%s", bundle_path);
        if (stat(alt_bundle, &st) != 0) {
            port_fatal("No se encuentra la carpeta de recursos 'FTL.app' en:\n  %s\n\n"
                       "Asegúrate de copiar FTL.app dentro de sdmc:/switch/ftl/.", bundle_path);
            return 1;
        }
    }
    LOGI("plat_switch: Carpeta de recursos 'FTL.app' encontrada");

    // Cambiamos el directorio activo para resolución de rutas relativas
    if (chdir(bundle_path) == 0 || chdir("sdmc:/switch/ftl/FTL.app") == 0) {
        LOGI("plat_switch: Directorio activo cambiado a '%s'", bundle_path);
    } else {
        chdir(base_dir);
        LOGW("plat_switch: No se pudo cambiar a '%s', usando '%s'", bundle_path, base_dir);
    }

    // Inicializamos la lectura de mandos y pantalla táctil
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);
    hidInitializeTouchScreen();

    // Fijamos 1280x720 (720p nativo del binario de iPad) para evitar desajustes de búfer
    g_sw = 1280;
    g_sh = 720;

    // Configuramos la ventana y contexto EGL
    NWindow *win = nwindowGetDefault();
    nwindowSetDimensions(win, g_sw, g_sh);

    g_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_dpy == EGL_NO_DISPLAY) {
        port_fatal("plat_switch: eglGetDisplay falló");
        return 1;
    }

    EGLint maj, min;
    if (!eglInitialize(g_dpy, &maj, &min)) {
        port_fatal("plat_switch: eglInitialize falló");
        return 1;
    }

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint cfg_attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_NONE
    };

    EGLint num_configs = 0;
    if (!eglChooseConfig(g_dpy, cfg_attrs, &g_cfg, 1, &num_configs) || num_configs < 1) {
        port_fatal("plat_switch: eglChooseConfig falló");
        return 1;
    }

    g_surf = eglCreateWindowSurface(g_dpy, g_cfg, (EGLNativeWindowType)win, NULL);
    if (g_surf == EGL_NO_SURFACE) {
        port_fatal("plat_switch: eglCreateWindowSurface falló");
        return 1;
    }

    LOGI("plat_switch: EGL inicializado (%d.%d), superficie %dx%d", maj, min, g_sw, g_sh);

    // Registramos el manejador de fallos de la librería de port
    port_install_crash_handler();

    // Configuramos la capa UX del juego (densidad de pantalla y márgenes seguros)
    float layout[8] = {0, 0, 0, 0, 300, (float)g_sh, 0};
    ux_configure(g_sw, g_sh, 60, layout);
    ux_pad_connected(true, "Nintendo Switch Pad");

    port_config cfg = {
        .binary_path = bin_path,
        .bundle_dir  = bundle_path,
        .docs_dir    = docs_path,
        .cache_dir   = cache_path,
        .hw_machine  = "iPad4,1",
        .verbose     = 1,
        .mods_index  = NULL,
        .hyperspace  = NULL,
        .log_dir     = log_path
    };

    LOGI("plat_switch: iniciando FTL desde %s ...", bin_path);
    int ret = port_run_game(&cfg);

    // Limpieza al salir
    port_on_game_exit(ret);
    svcExitProcess();
    return ret;
}

void port_report_fatal(const char *msg) {
    LOGE("plat_switch: ERROR FATAL: %s", msg ? msg : "desconocido");
    if (s_logfile) {
        fflush(s_logfile);
        fclose(s_logfile);
        s_logfile = NULL;
    }

    if (g_surf != EGL_NO_SURFACE) { eglDestroySurface(g_dpy, g_surf); g_surf = EGL_NO_SURFACE; }
    if (g_ctx != EGL_NO_CONTEXT) { eglDestroyContext(g_dpy, g_ctx); g_ctx = EGL_NO_CONTEXT; }
    if (g_dpy != EGL_NO_DISPLAY) { eglTerminate(g_dpy); g_dpy = EGL_NO_DISPLAY; }

    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("\x1b[2J\x1b[H\n");
    printf(" \x1b[31;1m========================================================\x1b[0m\n");
    printf(" \x1b[31;1m             ERROR FATAL AL INICIAR FTL                 \x1b[0m\n");
    printf(" \x1b[31;1m========================================================\x1b[0m\n\n");
    printf(" \x1b[37;1mDetalle del error:\x1b[0m\n");
    printf("   %s\n\n", msg ? msg : "(Sin descripcion disponible)");
    printf(" \x1b[33;1mComprobaciones recomendadas:\x1b[0m\n");
    printf("   1. Iniciar con Title Override: mantener pulsado (R) al\n");
    printf("      abrir cualquier juego comercial de Nintendo Switch.\n");
    printf("   2. Verificar que 'FTL' y 'FTL.app' esten en 'sdmc:/switch/ftl/'.\n");
    printf("   3. Revisar el archivo de registro en:\n");
    printf("      \x1b[36;1msdmc:/switch/ftl/logs/ftl.log\x1b[0m\n\n");
    printf(" \x1b[32;1mPulsa (+) para volver al Homebrew Menu...\x1b[0m\n");
    consoleUpdate(NULL);

    u64 start_tick = armGetSystemTick();
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);
        if (kDown & HidNpadButton_Plus) break;
        if (armTicksToNs(armGetSystemTick() - start_tick) > 30ULL * 1000000000ULL) break;
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
}

void port_on_game_exit(int code) {
    static bool s_exited = false;
    if (s_exited) return;
    s_exited = true;

    LOGI("plat_switch: salida del juego con código %d", code);
    if (g_audio_dev) {
        SDL_CloseAudioDevice(g_audio_dev);
        g_audio_dev = 0;
    }
    SDL_Quit();

    if (g_surf != EGL_NO_SURFACE) { eglDestroySurface(g_dpy, g_surf); g_surf = EGL_NO_SURFACE; }
    if (g_ctx != EGL_NO_CONTEXT) { eglDestroyContext(g_dpy, g_ctx); g_ctx = EGL_NO_CONTEXT; }
    if (g_dpy != EGL_NO_DISPLAY) { eglTerminate(g_dpy); g_dpy = EGL_NO_DISPLAY; }

    if (s_logfile) {
        fflush(s_logfile);
        fclose(s_logfile);
        s_logfile = NULL;
    }
}

