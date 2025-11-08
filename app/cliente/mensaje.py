# -*- coding: utf-8 -*-
from dataclasses import dataclass
import re

@dataclass
class Mensaje:
    id: int
    usuario: str
    clave: str
    tipo: str   # "string" | "int" | "double"
    valor: str  # crudo; el servidor decide cómo tratarlo

    def serializar(self) -> str:
        # Formato: ID|usuario|clave|tipo|valor
        return f"{self.id}|{self.usuario}|{self.clave}|{self.tipo}|{self.valor}"

    @staticmethod
    def deserializar(s: str) -> "Mensaje":
        partes = s.split("|", 4)
        if len(partes) != 5:
            raise ValueError("Mensaje inválido")
        return Mensaje(int(partes[0]), partes[1], partes[2], partes[3], partes[4])

def tipar(valor: str) -> str:
    if re.fullmatch(r"\d+", valor or ""):
        return "int"
    if re.fullmatch(r"-?\d+(\.\d+)?", valor or ""):
        return "double"
    return "string"

