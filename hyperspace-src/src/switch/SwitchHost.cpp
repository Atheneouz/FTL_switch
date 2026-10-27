// Punto de entrada y capa host de Hyperspace para Nintendo Switch.
// El port principal (ftl.nro) invoca hs_switch_init una vez mapeado el binario
// Mach-O antes de ejecutar los constructores estáticos.

#include "SwitchHost.h"
#include "Global.h"
#include "SigScan.h"
#include "zhl/Arm64Detour.h"
#include "zhl_internal.h"
#include "zhl_private.h"

#ifdef __SWITCH__
    #include <switch.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>

static hs_host_info g_host;
static std::string g_logDir;
static std::string g_zhlLogPath;
static std::string g_lastError;

// Enrutamos los mensajes de diagnóstico al callback de log del port
static void HostLog(int level, const char *fmt, ...)
{
    char msg[1024];
    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, sizeof(msg), fmt, va);
    va_end(va);
    if (g_host.log)
    {
        g_host.log(level, msg);
    }
    else
    {
        FILE *out = (level >= 2) ? stderr : stdout;
        fprintf(out, "[Hyperspace:%s] %s\n", level >= 2 ? "ERROR" : level == 1 ? "WARN" : "INFO", msg);
        fflush(out);
    }
}

void PAL_HostShowError(const char *title, const char *message)
{
    HostLog(2, "Hyperspace: %s: %s", title, message);
    g_lastError = std::string(title) + ": " + message;
}

std::string HS_LogPath(const char *name)
{
    if (g_logDir.empty())
        return std::string("sdmc:/switch/ftl/") + name;
    return g_logDir + "/" + name;
}

// Recorremos la cadena de frame pointers de ARM64 para diagnosticar llamadas
static void LogCallers()
{
    uintptr_t *fp = (uintptr_t *)__builtin_frame_address(0);
    uintptr_t game = (uintptr_t)g_host.mach_header;
    for (int i = 0; i < 16 && fp; i++)
    {
        uintptr_t ret = fp[1];
        if (!ret) break;
        if (game && ret >= game && ret < game + g_host.image_size)
        {
            HostLog(2, "  from FTL %#lx (link %#lx)", (unsigned long)ret, (unsigned long)(ret - g_host.slide));
        }
        else
        {
            HostLog(2, "  from %#lx", (unsigned long)ret);
        }
        uintptr_t *next = (uintptr_t *)fp[0];
        if (next <= fp) break;
        fp = next;
    }
}

// Trampa para funciones no presentes en la versión de iOS 1.6.21
static void MissingFunctionCalled(const char *name)
{
    HostLog(2, "Hyperspace: llamada a %s, no mapeada en la versión de iPad", name);
    ZHL::Log("Called %s, which isn't found in the iPad game yet\n", name);
    LogCallers();
    abort();
}

static void *MakeMissingStub(const char *name)
{
    return Arm64Detour::MakeTrap(name, MissingFunctionCalled);
}

// Búsqueda de información de desenrollado para libunwind
extern "C" int __ftl_find_macho_unwind(uintptr_t pc, uintptr_t *mh, uintptr_t *textLen, uintptr_t *cu, size_t *cuLen,
                                       uintptr_t *eh, size_t *ehLen)
{
    if (!g_host.find_macho_unwind) return 0;
    return g_host.find_macho_unwind(pc, mh, textLen, cu, cuLen, eh, ehLen);
}

// Inicialización de Hyperspace en Nintendo Switch
extern "C" __attribute__((visibility("default"))) int hs_switch_init(const hs_host_info *info, char *error, size_t errorSize)
{
    if (!info || info->version != HS_HOST_VERSION || info->size < sizeof(hs_host_info))
    {
        snprintf(error, errorSize, "Versión de interfaz de Hyperspace incompatible (%u, esperada %u).",
                 info ? info->version : 0u, (unsigned)HS_HOST_VERSION);
        return 1;
    }
    g_host = *info;
    g_logDir = info->log_dir ? info->log_dir : "sdmc:/switch/ftl";

    // Configuramos el directorio de trabajo para settings_hs.ini y hyperspace.xml
    if (info->data_dir && chdir(info->data_dir) != 0)
        HostLog(1, "Hyperspace: no se pudo fijar %s como directorio de trabajo", info->data_dir);

    // Rotamos logs de ejecuciones anteriores
    std::rename(HS_LogPath("zhl.log").c_str(), HS_LogPath("zhl.log.bak").c_str());
    std::rename(HS_LogPath("FTL_HS.log").c_str(), HS_LogPath("FTL_HS.log.bak").c_str());
    g_zhlLogPath = HS_LogPath("zhl.log");
    ZHL::SetLogPath(g_zhlLogPath.c_str());

    // Vinculamos la imagen Mach-O en el escáner de firmas
    SigScan::SetGameImage(info->mach_header, info->slide);
    if (!SigScan::IsKnownGame())
    {
        snprintf(error, errorSize, "El ejecutable no coincide con la versión 1.6.21 de FTL para iPad.");
        HostLog(2, "Hyperspace: %s", error);
        return 1;
    }

    // Registramos el rango de memoria ejecutable del juego para saltos relativos
    Arm64Detour::SetGameRange((uintptr_t)info->mach_header, info->image_size);
    ZHL::SetMissingFunctionStub(MakeMissingStub);

    // Inicializamos ZHL e instalamos los ganchos en el juego
    if (!ZHL::Init())
    {
        snprintf(error, errorSize, "%s", g_lastError.empty() ? "Fallo al iniciar ZHL (consultar zhl.log)." : g_lastError.c_str());
        return 1;
    }
    size_t poolSize = 0, poolUsed = Arm64Detour::PoolUsed(&poolSize);
    HostLog(0, "Hyperspace: ganchos instalados (%zu KB de %zu KB en trampolines)", poolUsed >> 10, poolSize >> 10);

    // Inicializamos el estado global de Hyperspace
    G_->Initialize();
    HostLog(0, "Hyperspace inicializado correctamente en Nintendo Switch");
    return 0;
}

// Alias de compatibilidad
extern "C" __attribute__((visibility("default"))) int hs_android_init(const hs_host_info *info, char *error, size_t errorSize)
{
    return hs_switch_init(info, error, errorSize);
}
