# -*- coding: utf-8 -*-
import base64, os
import xmlrpc.client
from .usuario import Usuario
from .mensaje import Mensaje, tipar
from .interfaz import Interfaz

class ClienteRPC:
    def __init__(self, url: str, usuario: Usuario):
        self.proxy = xmlrpc.client.ServerProxy(url, allow_none=True)
        self.usuario = usuario
        self._next_id = 1

    def _enviar(self, valor: str) -> str:
        tipo = tipar(valor)
        msg = Mensaje(self._next_id, self.usuario.nombre, self.usuario.clave, tipo, valor)
        self._next_id += 1
        return self.proxy.RecibirMensaje(msg.serializar())

    # API de alto nivel para la UI:
    def comando(self, cmd_usuario: str) -> str:
        return self._enviar(Interfaz.normalizar(cmd_usuario))

    def upload(self, path: str) -> str:
        with open(path, "rb") as f:
            b64 = base64.b64encode(f.read()).decode("ascii")
        fname = os.path.basename(path)
        payload = f"upload filename={fname} data={b64}"
        return self._enviar(payload)

    def run(self, fname: str) -> str:
        payload = f"run filename={fname}"
        return self._enviar(payload)

