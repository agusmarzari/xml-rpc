#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys, getpass, re
import xmlrpc.client

def send(proxy, msg):
    return proxy.RecibirMensaje(msg)

def build_msg(i, user, pwd, valor):
    tipo = "string"
    return f"{i}|{user}|{pwd}|{tipo}|{valor}"

def main():
    if len(sys.argv) != 3:
        print("Uso: python3 server_admin.py <ip> <puerto>")
        sys.exit(1)
    ip, port = sys.argv[1], int(sys.argv[2])
    url = f"http://{ip}:{port}"
    proxy = xmlrpc.client.ServerProxy(url, allow_none=True)

    print("== Interfaz de Servidor (Admin) ==")
    admin = input("Usuario admin: ").strip()
    pwd = getpass.getpass("Clave: ")
    i = 1

    while True:
        print("""
1) Acceso remoto ON
2) Acceso remoto OFF
3) Ver últimas N líneas de log
4) Mostrar catálogo de comandos (help)
5) Reporte
q) Salir
""")
        op = input("Opción: ").strip().lower()
        if op == "q": break
        try:
            if op == "1":
                print(send(proxy, build_msg(i, admin, pwd, "admin acceso on"))); i+=1
            elif op == "2":
                print(send(proxy, build_msg(i, admin, pwd, "admin acceso off"))); i+=1
            elif op == "3":
                n = input("N [50]: ").strip() or "50"
                if not re.fullmatch(r"\d+", n): print("Debe ser entero."); continue
                print(send(proxy, build_msg(i, admin, pwd, f"admin log {n}"))); i+=1
            elif op == "4":
                print(send(proxy, build_msg(i, admin, pwd, "comandos"))); i+=1
            elif op == "5":
                print(send(proxy, build_msg(i, admin, pwd, "reporte"))); i+=1
            else:
                print("Opción inválida.")
        except Exception as e:
            print("[RPC] Error:", e)

if __name__ == "__main__":
    main()

