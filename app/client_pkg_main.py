#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys, getpass
from cliente.usuario import Usuario
from cliente.cliente import ClienteRPC

def main():
    if len(sys.argv) != 3:
        print("Uso: python3 client_pkg_main.py <ip> <puerto>")
        sys.exit(1)

    ip, port = sys.argv[1], int(sys.argv[2])
    url = f"http://{ip}:{port}"
    print("Servidor:", url)

    user = input("Usuario: ").strip()
    pwd = getpass.getpass("Clave: ")
    cli = ClienteRPC(url, Usuario(user, pwd))

    print("Escribí 'help' para ver comandos; 'salir' para terminar.")
    while True:
        try:
            s = input("> ").strip()
        except EOFError:
            break
        if not s:
            continue
        if s.lower() == "salir":
            break

        # === Comando especial: upload ===
        if s.lower().startswith("upload "):
            path = s[7:].strip()
            try:
                print("Servidor:", cli.upload(path))
            except Exception as e:
                print("[upload] Error:", e)
            continue

        # === Comando especial: run ===
        if s.lower().startswith("run "):
            print("Servidor:", cli.run(s[4:].strip()))
            continue

        # === NUEVOS comandos de grabación ===
        if s.lower().startswith("guardar trayectoria"):
            try:
                print("Servidor:", cli.comando(s))
            except Exception as e:
                print("[grabar trayectoria] Error:", e)
            continue

        if s.lower() == "fin trayectoria":
            try:
                print("Servidor:", cli.comando(s))
            except Exception as e:
                print("[fin trayectoria] Error:", e)
            continue

        # === Comando general (otros casos) ===
        try:
            res = cli.comando(s)
            print("Servidor:", res)
        except Exception as e:
            print("[RPC] Error:", e)


if __name__ == "__main__":
    main()


