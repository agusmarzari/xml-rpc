#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys, os, re, base64, xmlrpc.client, getpass
from dataclasses import dataclass

@dataclass
class Session:
    proxy: xmlrpc.client.ServerProxy
    user: str
    password: str
    next_id: int = 1

def build_tipo_valor(s: str):
    if re.fullmatch(r"\d+", s or ""): return "int", s
    if re.fullmatch(r"-?\d+(\.\d+)?", s or ""): return "double", s
    return "string", s

def serialize_message(sess: Session, payload: str):
    tipo, valor = build_tipo_valor(payload)
    msg = f"{sess.next_id}|{sess.user}|{sess.password}|{tipo}|{valor}"
    sess.next_id += 1
    return msg

def send(sess: Session, payload: str):
    return sess.proxy.RecibirMensaje(serialize_message(sess, payload))

def parse_mensaje(s: str):
    parts = s.split("|", 4)
    if len(parts) != 5: return None
    return {"ID": parts[0], "usuario": parts[1], "clave": parts[2], "tipo": parts[3], "valor": parts[4]}

def ask_float(prompt: str):
    while True:
        s = input(prompt).strip()
        if s == "": return None
        try: return float(s)
        except ValueError: print("  Valor inválido. Dejá vacío para no enviar o escribí un número.")

def upload_file(sess: Session, path: str):
    if (path.startswith('"') and path.endswith('"')) or (path.startswith("'") and path.endswith("'")):
        path = path[1:-1]
    try:
        with open(path, "rb") as f: b64 = base64.b64encode(f.read()).decode("ascii")
    except Exception as e:
        return f"[upload] No pude leer el archivo: {e}"
    fname = os.path.basename(path)
    return send(sess, f"upload filename={fname} data={b64}")

def run_file(sess: Session, fname: str):
    return send(sess, f"run filename={fname}")

def show_menu():
    print(r"""
========= MENÚ =========
1) Encender motores (M17)
2) Apagar motores (M18)
3) Homing (G28)
4) Mover brazo (G0 X/Y/Z)
5) Gripper ON (M3)
6) Gripper OFF (M5)
7) Modo ABS (G90)
8) Modo REL (G91)
9) Estado / Reporte (M114)
10) Subir archivo GCODE (upload)
11) Ejecutar archivo GCODE (run)
12) Admin: acceso ON
13) Admin: acceso OFF
14) Admin: ver últimas N líneas de log
h) Ayuda de comandos (servidor)
q) Salir
========================
""")

def main():
    if len(sys.argv) != 3:
        print("Uso: python3 client_cli.py <ip> <puerto>"); sys.exit(1)
    ip, port = sys.argv[1], int(sys.argv[2])
    url = f"http://{ip}:{port}"
    proxy = xmlrpc.client.ServerProxy(url, allow_none=True)
    print("=== Cliente Python (CLI) ==="); print("Servidor:", url)
    user = input("Usuario: ").strip(); pwd = getpass.getpass("Clave: ")
    sess = Session(proxy=proxy, user=user, password=pwd)
    while True:
        try:
            show_menu(); opt = input("Elegí una opción: ").strip().lower()
        except EOFError:
            print(); break
        if opt in ("q", "0"): break
        try:
            if opt == "1": print(send(sess, "encender motores"))
            elif opt == "2": print(send(sess, "apagar motores"))
            elif opt == "3": print(send(sess, "homing"))
            elif opt == "4":
                x = ask_float("  X (vacío = no enviar): ")
                y = ask_float("  Y (vacío = no enviar): ")
                z = ask_float("  Z (vacío = no enviar): ")
                cmd = "mover brazo"; 
                if x is not None: cmd += f" x={x}"
                if y is not None: cmd += f" y={y}"
                if z is not None: cmd += f" z={z}"
                print(send(sess, cmd))
            elif opt == "5": print(send(sess, "activar gripper"))
            elif opt == "6": print(send(sess, "desactivar gripper"))
            elif opt == "7": print(send(sess, "modo absoluto"))
            elif opt == "8": print(send(sess, "modo relativo"))
            elif opt == "9":
                res = send(sess, "reporte")
                if isinstance(res, str):
                    parsed = parse_mensaje(res)
                    if parsed and parsed.get("tipo") == "string":
                        print("\n==== REPORTE ===="); print(parsed.get("valor", "")); print("=================")
                    else: print(res)
                else: print(res)
            elif opt == "10":
                path = input("Ruta del .gcode a subir: ").strip(); print(upload_file(sess, path))
            elif opt == "11":
                fname = input("Nombre del .gcode (en 'uploads/'): ").strip(); print(run_file(sess, fname))
            elif opt == "12": print(send(sess, "admin acceso on"))
            elif opt == "13": print(send(sess, "admin acceso off"))
            elif opt == "14":
                n = input("¿Cuántas líneas querés ver? [50]: ").strip() or "50"
                if not re.fullmatch(r"\d+", n): print("Debe ser un número entero.")
                else: print(send(sess, f"admin log {n}"))
            elif opt == "h": print(send(sess, "comandos"))
            else: print("Opción inválida.")
        except Exception as e:
            print("[RPC] Error:", e)
    print("Cerrando cliente CLI. ¡Chau!")

if __name__ == "__main__":
    main()
