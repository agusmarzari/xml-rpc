#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
import re
import base64
import xmlrpc.client
import getpass
from dataclasses import dataclass

@dataclass
class Session:
    url: str
    user: str
    password: str
    next_id: int = 1

def build_tipo_valor(s: str):
    if re.fullmatch(r"\d+", s or ""):
        return "int", s
    if re.fullmatch(r"-?\d+(\.\d+)?", s or ""):
        return "double", s
    return "string", s

def serialize_message(sess: Session, payload: str):
    tipo, valor = build_tipo_valor(payload)
    msg = f"{sess.next_id}|{sess.user}|{sess.password}|{tipo}|{valor}"
    sess.next_id += 1
    return msg

def parse_mensaje(serializado: str):
    parts = serializado.split("|", 4)
    if len(parts) != 5:
        return None
    return {"ID": parts[0], "usuario": parts[1], "clave": parts[2], "tipo": parts[3], "valor": parts[4]}

def normalize(cmd: str) -> str:
    c = cmd.strip()
    if c.lower().startswith(("move ", "upload ", "run ", "admin log ")):
        return c
    c = c.lower()
    if c in ("help", "?", "comandos", "ayuda"):
        return "comandos"
    if c == "on":  return "encender motores"
    if c == "off": return "apagar motores"
    if c == "home": return "homing"
    if c in ("rep", "status"): return "reporte"
    if c == "grip on":  return "activar gripper"
    if c == "grip off": return "desactivar gripper"
    if c in ("abs", "g90"): return "modo absoluto"
    if c in ("rel", "g91"): return "modo relativo"
    if c == "admin acceso on" or c == "admin acceso off": return c
    if c.startswith("move"):
        out = "mover brazo"
        m = re.search(r"x\s*=\s*([-\d\.]+)", c)
        if m: out += f" x={m.group(1)}"
        m = re.search(r"y\s*=\s*([-\d\.]+)", c)
        if m: out += f" y={m.group(1)}"
        m = re.search(r"z\s*=\s*([-\d\.]+)", c)
        if m: out += f" z={m.group(1)}"
        return out or c
    return c

def b64_of_file(path: str) -> str:
    with open(path, "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")

def print_help():
    print(r"""
Comandos disponibles (cliente Python):
  on / off / home
  grip on / grip off
  status / rep / reporte
  abs / rel
  move x=<num> y=<num> z=<num>
  upload <archivo.gcode>
  run <archivo.gcode>
  comandos / ayuda / help
  admin acceso on | admin acceso off
  admin log N
  salir
""")

def main():
    if len(sys.argv) != 3:
        print("Uso: python3 client_rpc.py <ip> <puerto>")
        sys.exit(1)
    ip, port = sys.argv[1], int(sys.argv[2])
    url = f"http://{ip}:{port}"
    proxy = xmlrpc.client.ServerProxy(url, allow_none=True)

    print("=== Cliente Python TPI ===")
    print("Servidor:", url)
    user = input("Usuario: ").strip()
    pwd = getpass.getpass("Clave: ")

    sess = Session(url=url, user=user, password=pwd)
    print("\nEscribí 'help' para ver comandos.")

    while True:
        try:
            raw = input("\n> ").strip()
        except EOFError:
            break
        if not raw:
            continue
        if raw.lower() == "salir":
            break

        if raw.lower().startswith("upload "):
            path = raw[7:].strip()
            if (path.startswith('"') and path.endswith('"')) or (path.startswith("'") and path.endswith("'")):
                path = path[1:-1]
            try:
                b64 = b64_of_file(path)
            except Exception as e:
                print(f"[upload] No pude leer el archivo: {e}")
                continue
            import os
            fname = os.path.basename(path)
            payload = f"upload filename={fname} data={b64}"
            msg = serialize_message(sess, payload)
            try:
                res = proxy.RecibirMensaje(msg)
                print("Servidor:", res)
            except Exception as e:
                print("[RPC] Error:", e)
            continue

        if raw.lower().startswith("run "):
            fname = raw[4:].strip()
            if not fname:
                print("Uso: run <archivo.gcode>")
                continue
            payload = f"run filename={fname}"
            msg = serialize_message(sess, payload)
            try:
                res = proxy.RecibirMensaje(msg)
                print("Servidor:", res)
            except Exception as e:
                print("[RPC] Error:", e)
            continue

        peticion = normalize(raw)
        msg = serialize_message(sess, peticion)
        try:
            res = proxy.RecibirMensaje(msg)
        except Exception as e:
            print("[RPC] Error:", e)
            continue

        if peticion == "reporte" and isinstance(res, str):
            parsed = parse_mensaje(res)
            if parsed and parsed.get("tipo") == "string":
                print("\n==== REPORTE ====")
                print(parsed.get("valor", ""))
                print("=================")
                continue

        if peticion in ("comandos", "ayuda", "help"):
            print(res)
            continue

        print("Servidor:", res)

    print("Cerrando cliente Python. ¡Chau!")

if __name__ == "__main__":
    main()
