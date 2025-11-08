# -*- coding: utf-8 -*-
import re

class Interfaz:
    """
    Normaliza entradas humanas -> texto que entiende el servidor.
    (equivale a la “capa de UI” del cliente)
    """

    @staticmethod
    def normalizar(cmd: str) -> str:
        c = cmd.strip()
        if c.lower().startswith(("move ", "upload ", "run ", "admin log ")):
            return c
        c = c.lower()
        if c in ("help", "?", "comandos", "ayuda"): return "comandos"
        if c == "on": return "encender motores"
        if c == "off": return "apagar motores"
        if c == "home": return "homing"
        if c in ("rep", "status"): return "reporte"
        if c == "grip on": return "activar gripper"
        if c == "grip off": return "desactivar gripper"
        if c in ("abs", "g90"): return "modo absoluto"
        if c in ("rel", "g91"): return "modo relativo"
        if c.startswith("move"):
            out = "mover brazo"
            m = re.search(r"x\s*=\s*([-\d\.]+)", c)
            if m: out += f" x={m.group(1)}"
            m = re.search(r"y\s*=\s*([-\d\.]+)", c)
            if m: out += f" y={m.group(1)}"
            m = re.search(r"z\s*=\s*([-\d\.]+)", c)
            if m: out += f" z={m.group(1)}"
            return out or c
        if c in ("admin acceso on", "admin acceso off"): return c
        return c

