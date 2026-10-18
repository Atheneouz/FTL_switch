import os
import sys
import shutil
import subprocess
from PIL import Image

def resolver_entorno_nton():
    # Detectamos la ubicación del paquete nton según el entorno de Python activo
    base_nton = None
    try:
        import nton
        base_nton = os.path.dirname(nton.__file__)
    except ImportError:
        pass

    if not base_nton:
        rutas_candidatas = [
            os.path.expandvars(r"%APPDATA%\Python\Python311\site-packages\nton"),
            os.path.expandvars(r"%APPDATA%\Python\Python312\site-packages\nton"),
            os.path.expandvars(r"%APPDATA%\Python\Python310\site-packages\nton"),
            os.path.expanduser("~/.local/lib/python3.11/site-packages/nton"),
            os.path.expanduser("~/.local/lib/python3.12/site-packages/nton"),
        ]
        for r in rutas_candidatas:
            if os.path.isdir(r):
                base_nton = r
                break

    hacbrewpack = shutil.which("hacbrewpack")
    if not hacbrewpack and base_nton:
        exe_path = os.path.join(base_nton, "bin", "hacbrewpack.exe" if os.name == "nt" else "hacbrewpack")
        if os.path.exists(exe_path):
            hacbrewpack = exe_path

    hptnacp = shutil.which("hptnacp")
    if not hptnacp and base_nton:
        nacp_exe = os.path.join(base_nton, "bin", "hptnacp.exe" if os.name == "nt" else "hptnacp")
        if os.path.exists(nacp_exe):
            hptnacp = nacp_exe

    exefs = os.path.join(base_nton, "assets", "exefs") if base_nton else None
    logo = os.path.join(base_nton, "assets", "logo") if base_nton else None
    control_xml = os.path.join(base_nton, "assets", "control.nacp.xml") if base_nton else None

    prod_keys = os.path.expanduser("~/.switch/prod.keys")
    if not os.path.exists(prod_keys) and os.name == "nt":
        alt_keys = os.path.expandvars(r"%USERPROFILE%\.switch\prod.keys")
        if os.path.exists(alt_keys):
            prod_keys = alt_keys

    return {
        "base": base_nton,
        "hacbrewpack": hacbrewpack,
        "hptnacp": hptnacp,
        "exefs": exefs,
        "logo": logo,
        "control_xml": control_xml,
        "keys": prod_keys
    }

ENV_NTON = resolver_entorno_nton()

TITLE_ID = "0100F71016210000"
TITLE_NAME = "FTL: Faster Than Light"
TITLE_PUBLISHER = "Subset Games"
TITLE_VERSION = "1.6.21"

def preparar_icono(origen_jpg, destino_dat):
    # Convertimos y normalizamos el icono a JPEG 256x256 sin metadatos
    im = Image.open(origen_jpg)
    if im.size != (256, 256):
        im = im.resize((256, 256), Image.Resampling.LANCZOS)
    if im.mode != "RGB":
        im = im.convert("RGB")
    clean = Image.new("RGB", (256, 256))
    clean.paste(im)
    clean.save(destino_dat, format="JPEG", quality=98)
    im.close()
    clean.close()

def generar_nacp(destino_nacp, tmp_dir):
    xml_template = ENV_NTON.get("control_xml")
    if not xml_template or not os.path.exists(xml_template):
        raise FileNotFoundError("No se encontró control.nacp.xml en los recursos de nton.")

    with open(xml_template, "r", encoding="utf-8") as f:
        template = f.read()

    xml_content = (
        template.replace("<Name>Name</Name>", f"<Name>{TITLE_NAME}</Name>")
        .replace("<Publisher>Publisher</Publisher>", f"<Publisher>{TITLE_PUBLISHER}</Publisher>")
        .replace("<DisplayVersion>1.0.0</DisplayVersion>", f"<DisplayVersion>{TITLE_VERSION}</DisplayVersion>")
    )
    xml_path = os.path.join(tmp_dir, "control.nacp.xml")
    with open(xml_path, "w", encoding="utf-8") as f:
        f.write(xml_content)

    hptnacp_bin = ENV_NTON.get("hptnacp")
    cmd = [hptnacp_bin, "-a", "createnacp", "-i", xml_path, "-o", destino_nacp]
    subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def construir_forwarder_nsp(out_dir="instalacion"):
    print("=" * 60)
    print("Construyendo Forwarder NSP (Acceso directo a MicroSD)")
    print("=" * 60)

    hacbrewpack_bin = ENV_NTON.get("hacbrewpack")
    exefs_template = ENV_NTON.get("exefs")
    logo_template = ENV_NTON.get("logo")
    keys_path = ENV_NTON.get("keys")

    if not hacbrewpack_bin or not os.path.exists(hacbrewpack_bin):
        print("[ERROR] No se localizó 'hacbrewpack'. Asegúrate de tener instalado 'nton' (pip install nton).")
        return False

    if not keys_path or not os.path.exists(keys_path):
        print(f"[ERROR] No se encontraron las claves prod.keys en {keys_path}.")
        return False

    staging = os.path.abspath("temp_forwarder_build")
    if os.path.exists(staging):
        shutil.rmtree(staging)
    os.makedirs(staging, exist_ok=True)

    try:
        exefs_dir = os.path.join(staging, "exefs")
        shutil.copytree(exefs_template, exefs_dir)
        logo_dir = os.path.join(staging, "logo")
        shutil.copytree(logo_template, logo_dir)

        control_dir = os.path.join(staging, "control")
        os.makedirs(control_dir, exist_ok=True)
        generar_nacp(os.path.join(control_dir, "control.nacp"), staging)
        preparar_icono("assets/icon.jpg", os.path.join(control_dir, "icon_AmericanEnglish.dat"))
        preparar_icono("assets/icon.jpg", os.path.join(control_dir, "icon_Spanish.dat"))

        target_romfs = os.path.join(staging, "romfs")
        os.makedirs(target_romfs, exist_ok=True)
        with open(os.path.join(target_romfs, "nextNroPath"), "w", encoding="utf-8") as f:
            f.write("sdmc:/switch/ftl/ftl.nro")
        with open(os.path.join(target_romfs, "nextArgv"), "w", encoding="utf-8") as f:
            f.write("sdmc:/switch/ftl/ftl.nro")

        os.makedirs(out_dir, exist_ok=True)
        nsp_temp_out = os.path.join(staging, "nsp_out")
        os.makedirs(nsp_temp_out, exist_ok=True)

        cmd = [
            hacbrewpack_bin,
            "--titleid", TITLE_ID,
            "--titlename", TITLE_NAME,
            "--titlepublisher", TITLE_PUBLISHER,
            "--nspdir", nsp_temp_out,
            "-k", keys_path
        ]
        res = subprocess.run(cmd, cwd=staging, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if res.returncode != 0:
            print("[ERROR] Fallo al generar Forwarder NSP:")
            print(res.stdout)
            return False

        built_nsp = os.path.join(nsp_temp_out, f"{TITLE_ID}.nsp")
        if not os.path.exists(built_nsp):
            for item in os.listdir(nsp_temp_out):
                if item.endswith(".nsp"):
                    built_nsp = os.path.join(nsp_temp_out, item)
                    break

        final_nsp = os.path.join(out_dir, f"FTL_Forwarder_[{TITLE_ID}].nsp")
        shutil.copy2(built_nsp, final_nsp)
        print(f"[EXITO] Forwarder NSP generado correctamente:")
        print(f"  -> {os.path.abspath(final_nsp)} ({os.path.getsize(final_nsp)/(1024*1024):.2f} MB)")
        return True

    finally:
        if os.path.exists(staging):
            shutil.rmtree(staging)

if __name__ == "__main__":
    salida = sys.argv[1] if len(sys.argv) > 1 else "instalacion"
    construir_forwarder_nsp(salida)
