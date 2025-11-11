import unittest
from unittest.mock import patch, MagicMock, mock_open
import base64

from cliente.usuario import Usuario
from cliente.cliente import ClienteRPC

class TestClienteRPC(unittest.TestCase):
    @patch("cliente.cliente.xmlrpc.client.ServerProxy")
    def test_comando_envia_mensaje_serializado(self, MockProxy):
        # Arrange
        proxy = MockProxy.return_value
        proxy.RecibirMensaje.return_value = "ok"
        cli = ClienteRPC("http://127.0.0.1:8080", Usuario("agus","1234"))

        # Act
        res = cli.comando("on")  # Interfaz.normalizar -> 'encender motores'

        # Assert
        self.assertEqual(res, "ok")
        self.assertTrue(proxy.RecibirMensaje.called)
        arg = proxy.RecibirMensaje.call_args[0][0]
        # formato: ID|usuario|clave|tipo|valor
        parts = arg.split("|", 4)
        self.assertEqual(parts[1], "agus")
        self.assertEqual(parts[2], "1234")
        self.assertEqual(parts[3], "string")
        self.assertEqual(parts[4], "encender motores")

    @patch("cliente.cliente.xmlrpc.client.ServerProxy")
    def test_run_filename_payload(self, MockProxy):
        proxy = MockProxy.return_value
        proxy.RecibirMensaje.return_value = "ok"
        cli = ClienteRPC("http://127.0.0.1:8080", Usuario("agus","1234"))

        res = cli.run("prueba.gcode")
        self.assertEqual(res, "ok")
        arg = proxy.RecibirMensaje.call_args[0][0]
        self.assertTrue(arg.endswith("|string|run filename=prueba.gcode"))

    @patch("cliente.cliente.open", new_callable=mock_open, read_data=b"G28\n")
    @patch("cliente.cliente.xmlrpc.client.ServerProxy")
    def test_upload_base64_payload(self, MockProxy, mopen):
        proxy = MockProxy.return_value
        proxy.RecibirMensaje.return_value = "ok"
        cli = ClienteRPC("http://127.0.0.1:8080", Usuario("agus","1234"))

        res = cli.upload("/tmp/prueba.gcode")
        self.assertEqual(res, "ok")

        # validar que el payload contiene filename y data=base64(...)
        arg = proxy.RecibirMensaje.call_args[0][0]
        self.assertIn("upload filename=prueba.gcode data=", arg)
        b64 = arg.split("data=",1)[1]
        self.assertEqual(b64, base64.b64encode(b"G28\n").decode("ascii"))

if __name__ == "__main__":
    unittest.main()

