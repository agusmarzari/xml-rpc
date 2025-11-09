#ifndef CONTROLADOR_H
#define CONTROLADOR_H

#include <string>
#include <vector>
#include <termios.h>

// ============================================================================
// Clase Controlador
// Gestiona la comunicación serie con el Arduino a través de un puerto /dev/tty*
// ============================================================================

class Controlador {
private:
    int fd = -1;                 // Descriptor de archivo del puerto serie
    std::string port;            // Nombre del puerto, ej: /dev/ttyUSB0
    int baud;                    // Baud rate, normalmente 115200
    int timeout_ms;              // Tiempo máximo de espera para respuesta

    // Convierte el baud rate en la constante termios correspondiente
    static speed_t to_termios_baud(int b);

    // Configura parámetros del puerto (modo raw, 8N1, sin control de flujo)
    bool configurarPuerto();

public:
    // Constructor y destructor
    Controlador(const std::string& portName = "", int baud_rate = 115200, int timeout = 1000);
    ~Controlador();

    // Intenta establecer conexión serial con el Arduino
    bool conectar();

    // Cierra la conexión
    void desconectar();

    // Envía un comando G-code (por ejemplo "M114") y devuelve la respuesta
    std::string enviarComandoGcode(const std::string& cmd);

    // Detección automática de puertos serie disponibles
    static std::vector<std::string> detectarPuertos();
};

#endif // CONTROLADOR_H

