import os
import sys
import shutil
import zipfile

def localizar_ipa(ruta_sugerida=None):
    if ruta_sugerida and os.path.exists(ruta_sugerida):
        return ruta_sugerida
    if os.path.isdir("ipa"):
        for arch in os.listdir("ipa"):
            if arch.lower().endswith(".ipa"):
                return os.path.join("ipa", arch)
    for arch in os.listdir("."):
        if arch.lower().endswith(".ipa"):
            return arch
    return None

def preparar_carpeta_romfs(origen_sd="sd_card/switch/ftl", ipa_path=None, output_dir="romfs"):
    print("=" * 60)
    print("Preparando carpeta RomFS para Standalone NSP (FTL Switch)")
    print("=" * 60)

    # Creamos el directorio de salida para la particion RomFS
    os.makedirs(output_dir, exist_ok=True)

    dest_bin = os.path.join(output_dir, "FTL")
    dest_app = os.path.join(output_dir, "FTL.app")

    # 1. Intentamos obtener los archivos desde la carpeta ya preparada en sd_card/
    if os.path.exists(os.path.join(origen_sd, "FTL")) and os.path.exists(os.path.join(origen_sd, "FTL.app")):
        print(f"Copiando recursos desde {origen_sd}...")

        # Copiamos el binario Mach-O si no existe ya
        if not os.path.exists(dest_bin):
            shutil.copy2(os.path.join(origen_sd, "FTL"), dest_bin)
            print(f"[OK] Copiado ejecutable Mach-O -> {dest_bin}")
        else:
            print(f"[INFO] Ejecutable ya presente en {dest_bin}")

        # Copiamos la carpeta FTL.app completa con ftl.dat y recursos
        if not os.path.exists(dest_app):
            shutil.copytree(os.path.join(origen_sd, "FTL.app"), dest_app)
            print(f"[OK] Copiada carpeta de recursos -> {dest_app}")
        else:
            print(f"[INFO] Carpeta de recursos ya presente en {dest_app}")

    # 2. Si no esta en sd_card, intentamos extraer directamente del archivo IPA
    else:
        ipa_detectado = localizar_ipa(ipa_path)
        if not ipa_detectado:
            print("[ERROR] No localizamos la carpeta sd_card/switch/ftl ni ningún archivo IPA en 'ipa/'.")
            return False

        print(f"Extrayendo recursos directamente desde {ipa_detectado}...")
        with zipfile.ZipFile(ipa_detectado, 'r') as z:
            app_prefix = None
            for name in z.namelist():
                if name.endswith("FTL.app/") or name.endswith("FTL.app/FTL"):
                    app_prefix = name.split("FTL.app/")[0] + "FTL.app/"
                    break

            if not app_prefix:
                print("[ERROR] No se localizó 'FTL.app' dentro del paquete IPA.")
                return False

            os.makedirs(dest_app, exist_ok=True)
            extraidos = 0
            for info in z.infolist():
                if info.filename.startswith(app_prefix):
                    rel = info.filename[len(app_prefix):]
                    if not rel:
                        continue
                    target = os.path.join(dest_app, rel)
                    if info.is_dir():
                        os.makedirs(target, exist_ok=True)
                    else:
                        os.makedirs(os.path.dirname(target), exist_ok=True)
                        with z.open(info) as src, open(target, "wb") as dst:
                            shutil.copyfileobj(src, dst)
                        extraidos += 1
            print(f"[OK] Extraídos {extraidos} archivos en {dest_app}")

            app_bin = os.path.join(dest_app, "FTL")
            if os.path.exists(app_bin):
                shutil.copy2(app_bin, dest_bin)
                print(f"[OK] Copiado binario Mach-O -> {dest_bin}")

    # Verificamos que los componentes criticos esten presentes
    dat_path = os.path.join(dest_app, "ftl.dat")
    if not os.path.exists(dat_path):
        print(f"[ALERTA] No se encuentra ftl.dat en {dat_path}")
        return False

    tam_dat_mb = os.path.getsize(dat_path) / (1024 * 1024)
    print(f"\n[EXITO] Recursos RomFS listos en: {os.path.abspath(output_dir)}")
    print(f"  - Binario Mach-O: {dest_bin} ({os.path.getsize(dest_bin)/(1024*1024):.2f} MB)")
    print(f"  - Paquete FTL.app: {dest_app} (ftl.dat = {tam_dat_mb:.2f} MB)")
    print("\nPara empaquetar el juego en Standalone NSP:")
    print("  python tools/build_nsp.py")
    return True

if __name__ == "__main__":
    origen = sys.argv[1] if len(sys.argv) > 1 else "sd_card/switch/ftl"
    ipa = sys.argv[2] if len(sys.argv) > 2 else None
    salida = sys.argv[3] if len(sys.argv) > 3 else "romfs"
    preparar_carpeta_romfs(origen, ipa, salida)

