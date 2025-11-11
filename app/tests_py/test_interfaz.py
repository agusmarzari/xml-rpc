import unittest

# Intentamos usar la misma función que usa la GUI:
#   from client_rpc import normalize
# Si no está, caemos a cliente.interfaz.Interfaz.normalizar
try:
    from client_rpc import normalize as _normalize
    _NORMALIZER_NAME = "client_rpc.normalize"
except Exception:
    _normalize = None
    _NORMALIZER_NAME = None

if _normalize is None:
    try:
        from cliente.interfaz import Interfaz
        def _normalize(cmd: str) -> str:
            return Interfaz.normalizar(cmd)
        _NORMALIZER_NAME = "cliente.interfaz.Interfaz.normalizar"
    except Exception as e:
        raise RuntimeError(
            "No pude encontrar ni client_rpc.normalize ni cliente.interfaz.Interfaz.normalizar. "
            f"Error: {e}"
        )

class TestInterfaz(unittest.TestCase):
    def setUp(self):
        self.norm = _normalize  # alias corto

    def assertAlias(self, entrada, esperados):
        """Comprueba que normalizar(entrada) esté en el conjunto de salidas esperadas."""
        out = self.norm(entrada)
        self.assertIn(
            out.lower(),
            {e.lower() for e in esperados},
            msg=f"{_NORMALIZER_NAME}('{entrada}') -> '{out}' no está en {esperados}"
        )

    def test_alias_basicos(self):
        self.assertAlias("help", {"comandos", "ayuda", "help"})
        self.assertAlias("?", {"comandos", "ayuda", "help"})

        # on/off
        self.assertAlias("on", {"encender motores", "on"})
        self.assertAlias("off", {"apagar motores", "off"})

        # homing
        self.assertAlias("home", {"homing", "home"})

        # status / rep -> reporte (o passthrough)
        self.assertAlias("status", {"reporte", "status"})
        self.assertAlias("rep", {"reporte", "rep"})

        # gripper
        self.assertAlias("grip on", {"activar gripper", "grip on"})
        self.assertAlias("grip off", {"desactivar gripper", "grip off"})

        # modos
        self.assertAlias("abs", {"modo absoluto", "abs", "g90"})
        self.assertAlias("rel", {"modo relativo", "rel", "g91"})

    def test_move_xyz(self):
        out = self.norm("move x=10 y=20 z=30")
        low = out.lower()
        # Aceptamos ambas variantes: "mover brazo ..." o "move ..."
        self.assertTrue(
            low.startswith("mover brazo") or low.startswith("move"),
            msg=f"Se esperaba que normalizar('move ...') comience con 'mover brazo' o 'move', pero fue: '{out}'"
        )
        # Debe conservar los ejes y valores
        self.assertIn("x=10", low)
        self.assertIn("y=20", low)
        self.assertIn("z=30", low)

    def test_passthrough_upload_run(self):
        out_u = self.norm("upload prueba.gcode")
        out_r = self.norm("run prueba.gcode")
        # Aceptamos passthrough (lo usual)
        self.assertEqual(out_u, "upload prueba.gcode")
        self.assertEqual(out_r, "run prueba.gcode")

if __name__ == "__main__":
    unittest.main()

