#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
interfaz_cl.py

Interfaz gráfica del cliente XML-RPC.
Usa el paquete `cliente` (ClienteRPC, Usuario) para conectarse al servidor.

Requiere:
    - cliente/cliente.py
    - cliente/usuario.py
    - cliente/mensaje.py
    - cliente/interfaz.py

Ejecución:
    python interfaz_cl.py
"""

import os
import traceback
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext

# Importar la versión modular del cliente
from cliente.cliente import ClienteRPC
from cliente.usuario import Usuario


class InterfazCL:
    """GUI del cliente RPC (Tkinter). Permite conectar, enviar comandos,
    subir y ejecutar archivos GCODE."""

    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Cliente RPC - GUI")
        self.root.geometry("760x520")

        self.cliente = None  # instancia de ClienteRPC

        self._build_widgets()

    # ---------------------- Construcción de interfaz ----------------------

    def _build_widgets(self):
        frm = ttk.Frame(self.root, padding=10)
        frm.pack(fill=tk.BOTH, expand=True)

        # --- Conexión ---
        conn = ttk.LabelFrame(frm, text="Conexión")
        conn.pack(fill=tk.X, padx=4, pady=4)

        ttk.Label(conn, text="IP:").grid(row=0, column=0, sticky=tk.W, padx=4, pady=2)
        self.ip_var = tk.StringVar(value="127.0.0.1")
        ttk.Entry(conn, textvariable=self.ip_var, width=18).grid(row=0, column=1, padx=4)

        ttk.Label(conn, text="Puerto:").grid(row=0, column=2, sticky=tk.W, padx=4)
        self.port_var = tk.StringVar(value="8000")
        ttk.Entry(conn, textvariable=self.port_var, width=8).grid(row=0, column=3, padx=4)

        ttk.Label(conn, text="Usuario:").grid(row=0, column=4, sticky=tk.W, padx=4)
        self.user_var = tk.StringVar()
        ttk.Entry(conn, textvariable=self.user_var, width=12).grid(row=0, column=5, padx=4)

        ttk.Label(conn, text="Clave:").grid(row=0, column=6, sticky=tk.W, padx=4)
        self.pwd_var = tk.StringVar()
        ttk.Entry(conn, textvariable=self.pwd_var, show="*", width=12).grid(row=0, column=7, padx=4)

        self.connect_btn = ttk.Button(conn, text="Conectar", command=self.connect)
        self.connect_btn.grid(row=0, column=8, padx=6)

        self.status_lbl = ttk.Label(conn, text="No conectado", foreground="red")
        self.status_lbl.grid(row=0, column=9, padx=6)

        # --- Comandos ---
        cmdfrm = ttk.LabelFrame(frm, text="Comandos / Acciones")
        cmdfrm.pack(fill=tk.X, padx=4, pady=6)

        ttk.Label(cmdfrm, text="Comando:").grid(row=0, column=0, sticky=tk.W, padx=4)
        self.cmd_entry = ttk.Entry(cmdfrm, width=70)
        self.cmd_entry.grid(row=0, column=1, padx=4, pady=4, columnspan=6)

        self.send_btn = ttk.Button(cmdfrm, text="Enviar", command=self.send_command, state=tk.DISABLED)
        self.send_btn.grid(row=0, column=7, padx=4)

        # --- Upload / Run ---
        self.upload_btn = ttk.Button(cmdfrm, text="Upload archivo...", command=self.upload_file, state=tk.DISABLED)
        self.upload_btn.grid(row=1, column=1, pady=6)

        ttk.Label(cmdfrm, text="Run filename:").grid(row=1, column=2, sticky=tk.W)
        self.run_fname = tk.StringVar()
        ttk.Entry(cmdfrm, textvariable=self.run_fname, width=30).grid(row=1, column=3, columnspan=2)
        self.run_btn = ttk.Button(cmdfrm, text="Run", command=self.run_file, state=tk.DISABLED)
        self.run_btn.grid(row=1, column=5)

        # --- Log / Respuestas ---
        logfrm = ttk.LabelFrame(frm, text="Registro / Respuestas")
        logfrm.pack(fill=tk.BOTH, expand=True, padx=4, pady=6)

        self.log = scrolledtext.ScrolledText(logfrm, wrap=tk.WORD, height=18)
        self.log.pack(fill=tk.BOTH, expand=True)

        # --- Footer ---
        foot = ttk.Frame(frm)
        foot.pack(fill=tk.X, pady=6)
        ttk.Button(foot, text="Limpiar log", command=lambda: self.log.delete("1.0", tk.END)).pack(side=tk.LEFT)
        ttk.Button(foot, text="Cerrar", command=self.on_close).pack(side=tk.RIGHT)

    # ---------------------- Funciones auxiliares ----------------------

    def append_log(self, *lines: object):
        """Agrega texto al área de log."""
        for l in lines:
            self.log.insert(tk.END, str(l) + "\n")
        self.log.see(tk.END)

    # ---------------------- Conexión ----------------------

    def connect(self):
        ip = self.ip_var.get().strip()
        port = self.port_var.get().strip()
        user = self.user_var.get().strip()
        pwd = self.pwd_var.get()

        if not ip or not port:
            messagebox.showwarning("Datos faltantes", "IP y Puerto son requeridos")
            return

        try:
            port_i = int(port)
        except ValueError:
            messagebox.showerror("Puerto inválido", "El puerto debe ser un número entero")
            return

        url = f"http://{ip}:{port_i}"
        try:
            usuario = Usuario(user, pwd)
            self.cliente = ClienteRPC(url, usuario)

            self.send_btn.config(state=tk.NORMAL)
            self.upload_btn.config(state=tk.NORMAL)
            self.run_btn.config(state=tk.NORMAL)
            self.status_lbl.config(text=f"Conectado a {url}", foreground="green")
            self.append_log(f"Conectado a {url} como '{user}'")

        except Exception as e:
            tb = traceback.format_exc()
            messagebox.showerror("Error conexión", f"No pude conectarme: {e}\n\n{tb}")
            self.append_log(f"ERROR conexión: {e}")

    # ---------------------- Envío de comandos ----------------------

    def send_command(self):
        if not self.cliente:
            messagebox.showwarning("No conectado", "Conecte primero al servidor")
            return

        raw = self.cmd_entry.get().strip()
        if not raw:
            return

        self.append_log(f"> {raw}")
        try:
            res = self.cliente.comando(raw)
            self.append_log("Servidor:", res)
        except Exception as e:
            tb = traceback.format_exc()
            self.append_log(f"[RPC] Error: {e}\n{tb}")

    # ---------------------- Subir archivo ----------------------

    def upload_file(self):
        if not self.cliente:
            messagebox.showwarning("No conectado", "Conecte primero al servidor")
            return

        path = filedialog.askopenfilename(title="Seleccionar archivo a subir")
        if not path:
            return

        self.append_log(f"> upload {os.path.basename(path)}")
        try:
            res = self.cliente.upload(path)
            self.append_log("Servidor:", res)
        except Exception as e:
            self.append_log(f"[RPC] Error: {e}")

    # ---------------------- Ejecutar archivo ----------------------

    def run_file(self):
        if not self.cliente:
            messagebox.showwarning("No conectado", "Conecte primero al servidor")
            return

        fname = self.run_fname.get().strip()
        if not fname:
            messagebox.showinfo("Uso", "Ingrese el nombre del archivo a ejecutar")
            return

        self.append_log(f"> run {fname}")
        try:
            res = self.cliente.run(fname)
            self.append_log("Servidor:", res)
        except Exception as e:
            self.append_log(f"[RPC] Error: {e}")

    # ---------------------- Salida ----------------------

    def on_close(self):
        try:
            self.root.destroy()
        except Exception:
            pass

    def run(self):
        """Inicia el bucle principal de la interfaz."""
        self.root.mainloop()


if __name__ == "__main__":
    gui = InterfazCL()
    gui.run()
