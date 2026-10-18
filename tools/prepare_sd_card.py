import os
import sys
import zipfile
import shutil

def preparar_carpeta_switch(ipa_path, nro_path, output_dir):
    print("=" * 60)
    print("Preparando archivos para Nintendo Switch (FTL Homebrew)")
    print("=" * 60)
    
    destino_ftl = os.path.join(output_dir, "switch", "ftl")
    os.makedirs(destino_ftl, exist_ok=True)

    # 1. Copiamos el archivo ejecutable homebrew ftl.nro
    if os.path.exists(nro_path):
        dest_nro = os.path.join(destino_ftl, "ftl.nro")
        shutil.copy2(nro_path, dest_nro)
        print(f"[OK] Copiado ftl.nro -> {dest_nro}")
    else:
        print(f"[ERROR] No se encuentra el archivo {nro_path}")
        return False

    # 2. Extraemos el ejecutable y recursos del IPA
    if not os.path.exists(ipa_path):
        print(f"[ERROR] No se encuentra el archivo IPA: {ipa_path}")
        return False

    print(f"Extrayendo recursos desde {ipa_path}...")
    with zipfile.ZipFile(ipa_path, 'r') as z:
        # Buscamos la carpeta Payload/FTL.app/
        app_prefix = None
        for name in z.namelist():
            if name.endswith("FTL.app/") or name.endswith("FTL.app/FTL"):
                app_prefix = name.split("FTL.app/")[0] + "FTL.app/"
                break
        
        if not app_prefix:
            print("[ERROR] No se localizo FTL.app dentro del archivo .ipa")
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

        print(f"[OK] Extraidos {extraidos} archivos a {destino_app}")

        # El ejecutable Mach-O debe estar en sdmc:/switch/ftl/FTL
        app_bin = os.path.join(destino_app, "FTL")
        dest_bin = os.path.join(destino_ftl, "FTL")
        if os.path.exists(app_bin):
            shutil.copy2(app_bin, dest_bin)
            print(f"[OK] Copiado binario Mach-O -> {dest_bin}")
        else:
            print("[ERROR] No se encontro el binario FTL dentro de FTL.app")
            return False

    # Creamos las carpetas de guardado y logs
    os.makedirs(os.path.join(destino_ftl, "save"), exist_ok=True)
    os.makedirs(os.path.join(destino_ftl, "cache"), exist_ok=True)
    os.makedirs(os.path.join(destino_ftl, "logs"), exist_ok=True)

    print("\n" + "=" * 60)
    print("ESTRUCTURA PREPARADA CON EXITO!")
    print(f"Carpeta lista: {os.path.abspath(output_dir)}")
    print("Instrucciones para la Nintendo Switch:")
    print("1. Conecta la tarjeta MicroSD al PC o usa DBI (MTP Responder).")
    print("2. Copia la carpeta 'switch' generada en la raiz de tu MicroSD (sdmc:/).")
    print("3. Abre el Homebrew Menu (hbmenu) en tu Switch y pulsa en 'FTL: Faster Than Light'.")
    print("=" * 60)
    return True

if __name__ == "__main__":
    ipa = sys.argv[1] if len(sys.argv) > 1 else "ipa/FTL_1.6.21_ios_8.0.ipa"
    nro = sys.argv[2] if len(sys.argv) > 2 else "bin/ftl.nro"
    out = sys.argv[3] if len(sys.argv) > 3 else "sd_card"
    preparar_carpeta_switch(ipa, nro, out)
