import os
import sys
import zipfile
import shutil

def localizar_ipa(ruta_sugerida=None):
    # Verificamos si la ruta indicada manualmente existe
    if ruta_sugerida and os.path.exists(ruta_sugerida):
        return ruta_sugerida

    # Buscamos cualquier archivo .ipa en la carpeta ipa/
    if os.path.isdir("ipa"):
        for arch in os.listdir("ipa"):
            if arch.lower().endswith(".ipa"):
                return os.path.join("ipa", arch)

    # Buscamos en el directorio raíz como alternativa
    for arch in os.listdir("."):
        if arch.lower().endswith(".ipa"):
            return arch

    return None

def localizar_nro(ruta_sugerida=None):
    # Verificamos si la ruta del nro indicada existe
    if ruta_sugerida and os.path.exists(ruta_sugerida):
        return ruta_sugerida

    candidatos = [
        "bin/ftl.nro",
        "build/ftl.nro",
        "ftl.nro",
        "build/ftl.elf.nro"
    ]
    for c in candidatos:
        if os.path.exists(c):
            return c

    # Buscamos si existe dentro de carpetas de artefactos de compilación
    for item in os.listdir("."):
        if item.startswith("build_artifact") and os.path.isdir(item):
            for raiz, _, archivos in os.walk(item):
                for f in archivos:
                    if f == "ftl.nro":
                        return os.path.join(raiz, f)
    return None

def preparar_carpeta_switch(ipa_path=None, nro_path=None, output_dir="sd_card"):
    print("=" * 60)
    print("Preparando archivos para Nintendo Switch (FTL Homebrew)")
    print("=" * 60)

    # Resolvemos automáticamente la ruta del paquete IPA
    ipa_encontrado = localizar_ipa(ipa_path)
    if not ipa_encontrado:
        print("[ERROR] No localizamos ningun archivo .ipa en la carpeta 'ipa/' ni en el directorio actual.")
        print("Por favor, coloca tu archivo 'FTL_1.6.21_ios_8.0.ipa' (o cualquier .ipa de FTL) dentro de 'ipa/'.")
        return False
    print(f"[OK] Archivo IPA detectado: {ipa_encontrado}")

    # Creamos el directorio destino para la estructura de MicroSD
    destino_ftl = os.path.join(output_dir, "switch", "ftl")
    os.makedirs(destino_ftl, exist_ok=True)

    # Resolvemos y copiamos el ejecutable Homebrew ftl.nro si está disponible
    nro_encontrado = localizar_nro(nro_path)
    if nro_encontrado:
        dest_nro = os.path.join(destino_ftl, "ftl.nro")
        shutil.copy2(nro_encontrado, dest_nro)
        print(f"[OK] Copiado ejecutable Homebrew: {nro_encontrado} -> {dest_nro}")
    else:
        print("[AVISO] No localizamos ftl.nro en 'bin/' ni en 'build/'.")
        print("        Recuerda descargarlo de GitHub Actions (artefacto 'ftl-switch-build')")
        print("        o compilarlo con CMake, y colocarlo en 'sdmc:/switch/ftl/ftl.nro'.")

    # Extraemos el ejecutable y los recursos del archivo IPA
    print(f"Extrayendo recursos del juego desde {ipa_encontrado}...")
    with zipfile.ZipFile(ipa_encontrado, 'r') as z:
        app_prefix = None
        for name in z.namelist():
            if name.endswith("FTL.app/") or name.endswith("FTL.app/FTL"):
                app_prefix = name.split("FTL.app/")[0] + "FTL.app/"
                break

        if not app_prefix:
            print("[ERROR] No se localizó 'FTL.app' dentro del paquete IPA.")
            return False

        destino_app = os.path.join(destino_ftl, "FTL.app")
        os.makedirs(destino_app, exist_ok=True)

        extraidos = 0
        for info in z.infolist():
            if info.filename.startswith(app_prefix):
                rel_path = info.filename[len(app_prefix):]
                if not rel_path:
                    continue

                target_path = os.path.join(destino_app, rel_path)
                if info.is_dir():
                    os.makedirs(target_path, exist_ok=True)
                else:
                    os.makedirs(os.path.dirname(target_path), exist_ok=True)
                    with z.open(info) as src, open(target_path, "wb") as dst:
                        shutil.copyfileobj(src, dst)
                    extraidos += 1

        print(f"[OK] Extraidos {extraidos} recursos en {destino_app}")

        # El cargador de Switch espera el binario Mach-O en sdmc:/switch/ftl/FTL
        app_bin = os.path.join(destino_app, "FTL")
        dest_bin = os.path.join(destino_ftl, "FTL")
        if os.path.exists(app_bin):
            shutil.copy2(app_bin, dest_bin)
            print(f"[OK] Copiado binario Mach-O -> {dest_bin}")
        else:
            print("[ERROR] No se encontró el binario 'FTL' dentro de FTL.app.")
            return False

    # Creamos las carpetas de partidas guardadas, cache y registros
    os.makedirs(os.path.join(destino_ftl, "save"), exist_ok=True)
    os.makedirs(os.path.join(destino_ftl, "cache"), exist_ok=True)
    os.makedirs(os.path.join(destino_ftl, "logs"), exist_ok=True)

    print("\n" + "=" * 60)
    print("ESTRUCTURA DE MICROSD PREPARADA CON EXITO")
    print("=" * 60)
    print(f"Carpeta de salida: {os.path.abspath(output_dir)}")
    print("\nPasos para jugar en tu Nintendo Switch:")
    print("1. Copiamos la carpeta 'switch' de 'sd_card/' a la raiz de la MicroSD.")
    print("2. Instalamos el archivo 'instalacion/FTL_Forwarder_[0100F71016210000].nsp' con DBI o Goldleaf.")
    print("3. Abrimos 'FTL: Faster Than Light' directamente desde el Menu HOME de la consola.")
    print("=" * 60)
    return True

if __name__ == "__main__":
    ipa_arg = sys.argv[1] if len(sys.argv) > 1 else None
    nro_arg = sys.argv[2] if len(sys.argv) > 2 else None
    out_arg = sys.argv[3] if len(sys.argv) > 3 else "sd_card"
    preparar_carpeta_switch(ipa_arg, nro_arg, out_arg)

