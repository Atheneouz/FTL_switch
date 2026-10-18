import os
import sys
import shutil
import subprocess
from PIL import Image

# Rutas de herramientas y claves
SITE_PACKAGES = os.path.expandvars(r"%APPDATA%\Python\Python311\site-packages")
HACBREWPACK = os.path.join(SITE_PACKAGES, "nton", "bin", "hacbrewpack.exe")
HPTNACP = os.path.join(SITE_PACKAGES, "nton", "bin", "hptnacp.exe")
EXEFS_TEMPLATE = os.path.join(SITE_PACKAGES, "nton", "assets", "exefs")
LOGO_TEMPLATE = os.path.join(SITE_PACKAGES, "nton", "assets", "logo")
PROD_KEYS = os.path.expanduser(r"~/.switch/prod.keys")

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
    template_path = os.path.join(SITE_PACKAGES, "nton", "assets", "control.nacp.xml")
    with open(template_path, "r", encoding="utf-8") as f:
        template = f.read()

    xml_content = (
        template.replace("<Name>Name</Name>", f"<Name>{TITLE_NAME}</Name>")
        .replace("<Publisher>Publisher</Publisher>", f"<Publisher>{TITLE_PUBLISHER}</Publisher>")
        .replace("<DisplayVersion>1.0.0</DisplayVersion>", f"<DisplayVersion>{TITLE_VERSION}</DisplayVersion>")
    )
    xml_path = os.path.join(tmp_dir, "control.nacp.xml")
    with open(xml_path, "w", encoding="utf-8") as f:
        f.write(xml_content)

    cmd = [HPTNACP, "-a", "createnacp", "-i", xml_path, "-o", destino_nacp]
    subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def construir_standalone_nsp(nro_path, romfs_dir, out_dir):
    print("=" * 60)
    print("Construyendo Standalone NSP (Embebido en RomFS)")
    print("=" * 60)

    staging = os.path.abspath("temp_standalone_build")
    if os.path.exists(staging):
        shutil.rmtree(staging)
    os.makedirs(staging, exist_ok=True)

    try:
        # 1. Preparamos exefs y logo desde las plantillas
        exefs_dir = os.path.join(staging, "exefs")
        shutil.copytree(EXEFS_TEMPLATE, exefs_dir)
        logo_dir = os.path.join(staging, "logo")
        shutil.copytree(LOGO_TEMPLATE, logo_dir)

        # 2. Preparamos control
        control_dir = os.path.join(staging, "control")
        os.makedirs(control_dir, exist_ok=True)
        generar_nacp(os.path.join(control_dir, "control.nacp"), staging)
        preparar_icono("assets/icon.jpg", os.path.join(control_dir, "icon_AmericanEnglish.dat"))
        preparar_icono("assets/icon.jpg", os.path.join(control_dir, "icon_Spanish.dat"))

        # 3. Preparamos romfs
        target_romfs = os.path.join(staging, "romfs")
        print(f"Copiando recursos RomFS desde {romfs_dir}...")
        shutil.copytree(romfs_dir, target_romfs)

        # Copiamos el ejecutable ftl.nro dentro de romfs como app.nro
        dest_app_nro = os.path.join(target_romfs, "app.nro")
        shutil.copy2(nro_path, dest_app_nro)
        print(f"[OK] Incrustado {nro_path} como romfs:/app.nro")

        # Escribimos los archivos de redireccion para el cargador de ExeFS
        with open(os.path.join(target_romfs, "nextNroPath"), "w", encoding="utf-8") as f:
            f.write("romfs:/app.nro")
        with open(os.path.join(target_romfs, "nextArgv"), "w", encoding="utf-8") as f:
            f.write("romfs:/app.nro")

        # 4. Empaquetamos mediante hacbrewpack
        os.makedirs(out_dir, exist_ok=True)
        nsp_temp_out = os.path.join(staging, "nsp_out")
        os.makedirs(nsp_temp_out, exist_ok=True)

        print("Empaquetando con hacbrewpack...")
        cmd = [
            HACBREWPACK,
            "--titleid", TITLE_ID,
            "--titlename", TITLE_NAME,
            "--titlepublisher", TITLE_PUBLISHER,
            "--nspdir", nsp_temp_out,
            "-k", PROD_KEYS
        ]
        res = subprocess.run(cmd, cwd=staging, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if res.returncode != 0:
            print("[ERROR] Fallo al generar con hacbrewpack:")
            print(res.stdout)
            return False

        # Renombramos y movemos el NSP resultante a instalacion/
        built_nsp = os.path.join(nsp_temp_out, f"{TITLE_ID}.nsp")
        if not os.path.exists(built_nsp):
            # Buscamos cualquier .nsp generado
            for item in os.listdir(nsp_temp_out):
                if item.endswith(".nsp"):
                    built_nsp = os.path.join(nsp_temp_out, item)
                    break

        final_nsp = os.path.join(out_dir, f"FTL_Standalone_RomFS_[{TITLE_ID}].nsp")
        shutil.copy2(built_nsp, final_nsp)
        print(f"\n[EXITO] Standalone NSP generado correctamente:")
        print(f"  -> {os.path.abspath(final_nsp)} ({os.path.getsize(final_nsp)/(1024*1024):.2f} MB)")
        return True

    finally:
        if os.path.exists(staging):
            shutil.rmtree(staging)

def construir_forwarder_nsp(out_dir):
    print("\n" + "=" * 60)
    print("Construyendo Forwarder NSP (Acceso directo a MicroSD)")
    print("=" * 60)

    staging = os.path.abspath("temp_forwarder_build")
    if os.path.exists(staging):
        shutil.rmtree(staging)
    os.makedirs(staging, exist_ok=True)

    try:
        exefs_dir = os.path.join(staging, "exefs")
        shutil.copytree(EXEFS_TEMPLATE, exefs_dir)
        logo_dir = os.path.join(staging, "logo")
        shutil.copytree(LOGO_TEMPLATE, logo_dir)

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
            HACBREWPACK,
            "--titleid", TITLE_ID,
            "--titlename", TITLE_NAME,
            "--titlepublisher", TITLE_PUBLISHER,
            "--nspdir", nsp_temp_out,
            "-k", PROD_KEYS
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
    nro = sys.argv[1] if len(sys.argv) > 1 else "bin/ftl.nro"
    romfs = sys.argv[2] if len(sys.argv) > 2 else "romfs"
    salida = sys.argv[3] if len(sys.argv) > 3 else "instalacion"

    construir_standalone_nsp(nro, romfs, salida)
    construir_forwarder_nsp(salida)
