#include "Controlador.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <glob.h>
#include <cstring>
#include <sys/select.h>

// ============================================================================
// Conversión de baud rate a constantes termios
// ============================================================================
speed_t Controlador::to_termios_baud(int b) {
    switch (b) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return B115200;
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================
Controlador::Controlador(const std::string& portName, int baud_rate, int timeout)
    : port(portName), baud(baud_rate), timeout_ms(timeout) {}

Controlador::~Controlador() {
    desconectar();
}

// ============================================================================
// Configuración de parámetros del puerto serie
// ============================================================================
bool Controlador::configurarPuerto() {
    termios tio{};
    if (tcgetattr(fd, &tio) != 0) {
        std::perror("tcgetattr");
        return false;
    }

    cfmakeraw(&tio);
    cfsetispeed(&tio, to_termios_baud(baud));
    cfsetospeed(&tio, to_termios_baud(baud));

    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~PARENB;
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag &= ~CRTSCTS;

    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        std::perror("tcsetattr");
        return false;
    }

    // 🔹 Espera breve tras configurar (permite estabilizar handshake)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return true;
}

// ============================================================================
// Intento de conexión al puerto serie
// ============================================================================
bool Controlador::conectar() {
    // Detección automática si no se especifica puerto
    if (port.empty()) {
        auto puertos = detectarPuertos();
        if (puertos.empty()) {
            std::cerr << "No se detectaron puertos disponibles.\n";
            return false;
        }
        port = puertos.front();
    }

    int intentos = 0;
    while (intentos < 3) {
        fd = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd >= 0) break;

        std::perror("open");
        std::cerr << "Error al abrir el puerto " << port << ", reintentando...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        intentos++;
    }

    if (fd < 0) {
        std::cerr << "No se pudo abrir el puerto serial después de varios intentos.\n";
        return false;
    }

    std::cout << "Esperando inicialización del Arduino...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));  // 🔹 tiempo extendido CH340

    if (!configurarPuerto()) {
        ::close(fd);
        fd = -1;
        return false;
    }

    std::cout << "Conexión establecida con Arduino en " << port << "\n";
    return true;
}

// ============================================================================
// Desconexión
// ============================================================================
void Controlador::desconectar() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
        std::cout << "Conexión serial cerrada.\n";
    }
}

// ============================================================================
// Envío de comando G-code y lectura de respuesta
// ============================================================================
std::string Controlador::enviarComandoGcode(const std::string& cmd) {
    if (fd < 0) return "ERROR: No conectado al Arduino.";

    tcflush(fd, TCIOFLUSH);
    std::string payload = cmd + "\r\n";
    ::write(fd, payload.data(), payload.size());
    tcdrain(fd);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::string buffer;
    char tmp[256];
    
    // Timeout dinámico según el comando G-code
    int tmo = timeout_ms;  // valor base definido en Controlador.h

    if (cmd.find("G28") != std::string::npos) {
    tmo = 5000;  // Homing puede tardar varios segundos
    }  
    else if (cmd.find("M3") != std::string::npos || cmd.find("M5") != std::string::npos) {
    tmo = 2000;  // Gripper
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(tmo);


    while (true) {
        int available = 0;
        ioctl(fd, FIONREAD, &available);

        if (available > 0) {
            ssize_t r = ::read(fd, tmp, sizeof(tmp));
            if (r > 0) buffer.append(tmp, tmp + r);
        }

        if (std::chrono::steady_clock::now() > deadline) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Limpieza de CR/LF
    while (!buffer.empty() && (buffer.back() == '\r' || buffer.back() == '\n'))
        buffer.pop_back();

    return buffer.empty() ? "SIN RESPUESTA" : buffer;
}

// ============================================================================
// Detección automática de puertos serie
// ============================================================================
std::vector<std::string> Controlador::detectarPuertos() {
    std::vector<std::string> found;
    const char* patterns[] = {"/dev/ttyUSB*", "/dev/ttyACM*"};
    for (const char* pat : patterns) {
        glob_t g{};
        if (glob(pat, 0, nullptr, &g) == 0) {
            for (size_t i = 0; i < g.gl_pathc; ++i)
                found.emplace_back(g.gl_pathv[i]);
        }
        globfree(&g);
    }

    if (found.empty())
        std::cerr << "No se detectaron puertos serie disponibles.\n";
    else
        std::cout << "Puertos detectados: " << found.front() << std::endl;

    return found;
}


