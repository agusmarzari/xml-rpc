import unittest
from cliente.mensaje import Mensaje, tipar

class TestMensaje(unittest.TestCase):
    def test_serializar_deserializar(self):
        m = Mensaje(1, "agus", "1234", "string", "encender motores")
        s = m.serializar()
        self.assertEqual(s, "1|agus|1234|string|encender motores")

        m2 = Mensaje.deserializar(s)
        self.assertEqual(m2.id, 1)
        self.assertEqual(m2.usuario, "agus")
        self.assertEqual(m2.clave, "1234")
        self.assertEqual(m2.tipo, "string")
        self.assertEqual(m2.valor, "encender motores")

    def test_deserializar_invalido(self):
        with self.assertRaises(ValueError):
            Mensaje.deserializar("mal|formado|falta")

    def test_tipar(self):
        self.assertEqual(tipar("42"), "int")
        self.assertEqual(tipar("-12.5"), "double")
        self.assertEqual(tipar("home"), "string")
        self.assertEqual(tipar(""), "string")

if __name__ == "__main__":
    unittest.main()

