#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
interfaz_cl.py

Provides class `interfaz_cl` — a simple tkinter GUI client that uses the
helpers in `client_rpc.py` (Session, serialize_message, parse_mensaje,
normalize, b64_of_file) to connect to the XML-RPC server and send/receive
messages. This file does NOT modify `client_rpc.py` and imports from it.

Usage: python interfaz_cl.py
"""
from __future__ import annotations

import os
import traceback
import xmlrpc.client
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext

# Import helpers from existing client_rpc.py (file must remain unchanged)
from client_rpc import Session, serialize_message, parse_mensaje, normalize, b64_of_file


class interfaz_cl:
    """GUI client to connect to the XML-RPC server and send commands.

    Public methods:
    - run(): start the Tk mainloop

    The GUI collects IP, port, username and password then allows sending
    commands, uploading a file and requesting the server to run a file.
    """

    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Interfaz Cliente RPC")
        self.root.geometry("760x520")

        self.proxy = None
        self.session = None

        self._build_widgets()

    def _build_widgets(self):
        frm = ttk.Frame(self.root, padding=10)
        frm.pack(fill=tk.BOTH, expand=True)

        # Connection frame
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

        # Commands frame
        cmdfrm = ttk.LabelFrame(frm, text="Comandos / Acciones")
        cmdfrm.pack(fill=tk.X, padx=4, pady=6)

        ttk.Label(cmdfrm, text="Comando:").grid(row=0, column=0, sticky=tk.W, padx=4)
        self.cmd_entry = ttk.Entry(cmdfrm, width=70)
        self.cmd_entry.grid(row=0, column=1, padx=4, pady=4, columnspan=6)

        self.send_btn = ttk.Button(cmdfrm, text="Enviar", command=self.send_command, state=tk.DISABLED)
        self.send_btn.grid(row=0, column=7, padx=4)

        # Upload / Run
        self.upload_btn = ttk.Button(cmdfrm, text="Upload archivo...", command=self.upload_file, state=tk.DISABLED)
        self.upload_btn.grid(row=1, column=1, pady=6)

        ttk.Label(cmdfrm, text="Run filename:").grid(row=1, column=2, sticky=tk.W)
        self.run_fname = tk.StringVar()
        ttk.Entry(cmdfrm, textvariable=self.run_fname, width=30).grid(row=1, column=3, columnspan=2)
        self.run_btn = ttk.Button(cmdfrm, text="Run", command=self.run_file, state=tk.DISABLED)
        self.run_btn.grid(row=1, column=5)

        # Log / responses
        logfrm = ttk.LabelFrame(frm, text="Registro / Respuestas")
        logfrm.pack(fill=tk.BOTH, expand=True, padx=4, pady=6)

        self.log = scrolledtext.ScrolledText(logfrm, wrap=tk.WORD, height=18)
        self.log.pack(fill=tk.BOTH, expand=True)

        # Footer
        foot = ttk.Frame(frm)
        foot.pack(fill=tk.X, pady=6)
        ttk.Button(foot, text="Limpiar log", command=lambda: self.log.delete("1.0", tk.END)).pack(side=tk.LEFT)
        ttk.Button(foot, text="Cerrar", command=self.on_close).pack(side=tk.RIGHT)

    def append_log(self, *lines: object):
        for l in lines:
            self.log.insert(tk.END, str(l) + "\n")
        self.log.see(tk.END)

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
            proxy = xmlrpc.client.ServerProxy(url, allow_none=True)
            # quick ping: try a harmless call - RecibirMensaje requires message; we skip calling it here
            # Build session
            sess = Session(url=url, user=user, password=pwd)

            # Save
            self.proxy = proxy
            self.session = sess

            # enable UI
            self.send_btn.config(state=tk.NORMAL)
            self.upload_btn.config(state=tk.NORMAL)
            self.run_btn.config(state=tk.NORMAL)
            self.status_lbl.config(text=f"Conectado a {url}", foreground="green")
            self.append_log(f"Conectado a {url} como usuario '{user}'")
        except Exception as e:
            tb = traceback.format_exc()
            messagebox.showerror("Error conexión", f"No pude conectarme: {e}\n\n{tb}")
            self.append_log(f"ERROR conexión: {e}")

    def send_command(self):
        if not self.proxy or not self.session:
            messagebox.showwarning("No conectado", "Conecte primero al servidor")
            return

        raw = self.cmd_entry.get().strip()
        if not raw:
            return

        # Normalize similar to client_rpc's CLI
        peticion = normalize(raw)

        msg = serialize_message(self.session, peticion)
        self.append_log(f"> {peticion} (serialized id={self.session.next_id-1})")
        try:
            res = self.proxy.RecibirMensaje(msg)
            # Special handling for 'reporte' if server returns a serialized message
            if peticion == "reporte" and isinstance(res, str):
                parsed = parse_mensaje(res)
                if parsed and parsed.get("tipo") == "string":
                    self.append_log("==== REPORTE ====")
                    self.append_log(parsed.get("valor", ""))
                    self.append_log("=================")
                    return

            # For help/commands the server may return the help text
            self.append_log("Servidor:", res)
        except Exception as e:
            tb = traceback.format_exc()
            self.append_log(f"[RPC] Error: {e}")
            self.append_log(tb)

    def upload_file(self):
        if not self.proxy or not self.session:
            messagebox.showwarning("No conectado", "Conecte primero al servidor")
            return

        path = filedialog.askopenfilename(title="Seleccionar archivo a subir")
        if not path:
            return

        try:
            b64 = b64_of_file(path)
        except Exception as e:
            messagebox.showerror("Archivo", f"No pude leer el archivo: {e}")
            return

        fname = os.path.basename(path)
        payload = f"upload filename={fname} data={b64}"
        msg = serialize_message(self.session, payload)
        self.append_log(f"> upload {fname} (serialized id={self.session.next_id-1})")
        try:
            res = self.proxy.RecibirMensaje(msg)
            self.append_log("Servidor:", res)
        except Exception as e:
            self.append_log(f"[RPC] Error: {e}")

    def run_file(self):
        if not self.proxy or not self.session:
            messagebox.showwarning("No conectado", "Conecte primero al servidor")
            return
        fname = self.run_fname.get().strip()
        if not fname:
            messagebox.showinfo("Uso", "Ingrese el nombre del archivo a ejecutar en 'Run filename'")
            return

        payload = f"run filename={fname}"
        msg = serialize_message(self.session, payload)
        self.append_log(f"> run {fname} (serialized id={self.session.next_id-1})")
        try:
            res = self.proxy.RecibirMensaje(msg)
            self.append_log("Servidor:", res)
        except Exception as e:
            self.append_log(f"[RPC] Error: {e}")

    def on_close(self):
        try:
            self.root.destroy()
        except Exception:
            pass

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    gui = interfaz_cl()
    gui.run()
