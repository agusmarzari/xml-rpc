#include "PALogger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <vector>

// ======= Singleton =======
PALogger& PALogger::getInstance() {
    static PALogger instance;
    return instance;
}

// ======= Constructor =======
PALogger::PALogger() 
    : level_(LogLevel::INFO),
      csv_("logs/Log_de_trabajo.csv")
{
    ensureLogDirectory();

    if (!csv_.openForAppend()) {
        std::cerr << "[LOGGER ERROR] No se pudo abrir el archivo de log CSV." << std::endl;
        return;
    }

    csv_.ensureHeader("timestamp,tipo,detalle,usuario,peticion,nodo,codigo");
}

PALogger::~PALogger() {}

// ======= Métodos principales =======

void PALogger::logEvento(LogLevel level, const std::string& detalle, Code codigo, int nodo) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (level < level_) return;

    if (!csv_.openForAppend()) return;
    csv_.writeCsvRow({
        nowISO(),
        "EVENTO_" + levelToString(level),
        detalle,
        "", "",                       // usuario, peticion vacíos
        std::to_string(nodo),
        std::to_string(static_cast<int>(codigo))
    });
}

void PALogger::logPeticion(const std::string& usuario,
                           const std::string& peticion,
                           int nodo,
                           Code codigo)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (!csv_.openForAppend()) return;

    csv_.writeCsvRow({
        nowISO(),
        "PETICION",
        "Petición recibida",
        usuario,
        peticion,
        std::to_string(nodo),
        std::to_string(static_cast<int>(codigo))
    });
}

void PALogger::logLogin(const std::string& usuario, bool ok, int nodo) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!csv_.openForAppend()) return;

    csv_.writeCsvRow({
        nowISO(),
        ok ? "LOGIN_OK" : "LOGIN_FAIL",
        ok ? "Inicio de sesión exitoso" : "Credenciales inválidas",
        usuario,
        "",
        std::to_string(nodo),
        std::to_string(ok ? static_cast<int>(Code::OK) : static_cast<int>(Code::UNAUTHORIZED))
    });
}

// ======= Configuración =======
void PALogger::setLevel(LogLevel level) {
    level_ = level;
}

// ======= Utilitarios =======
std::string PALogger::nowISO() const {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    tm = *std::localtime(&t);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return os.str();
}

std::string PALogger::levelToString(LogLevel l) const {
    switch (l) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
    }
    return "INFO";
}

void PALogger::ensureLogDirectory() {
    std::filesystem::path logDir("logs");
    if (!std::filesystem::exists(logDir)) {
        std::filesystem::create_directory(logDir);
    }
}


std::vector<std::string> PALogger::mostrarPorTipo(const std::string& tipo, const std::string& usuarioFiltro) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> resultados;

    // Intentar abrir el CSV para lectura
    std::ifstream file("logs/Log_de_trabajo.csv");
    if (!file.is_open()) {
        std::cerr << "[LOGGER ERROR] No se pudo abrir el archivo de log para lectura." << std::endl;
        return resultados;
    }

    std::string linea;
    bool primera = true;

    while (std::getline(file, linea)) {
        if (primera) { // Saltar encabezado
            primera = false;
            continue;
        }

        // Partir la línea CSV por comas
        std::vector<std::string> campos;
        std::stringstream ss(linea);
        std::string campo;
        bool dentroComillas = false;
        std::string actual;

        // Parseo básico de CSV con comillas
        while (ss.good()) {
            char c = ss.get();
            if (ss.eof()) break;
            if (c == '"') {
                dentroComillas = !dentroComillas;
            } else if (c == ',' && !dentroComillas) {
                campos.push_back(actual);
                actual.clear();
            } else {
                actual.push_back(c);
            }
        }
        campos.push_back(actual);

        if (campos.size() < 7) continue; // línea mal formada

        const std::string& tipoCampo = campos[1];
        const std::string& usuarioCampo = campos[3];

        // Filtro por tipo y usuario
        if (tipoCampo == tipo && (usuarioFiltro.empty() || usuarioCampo == usuarioFiltro)) {
            resultados.push_back(linea);
        }
    }

    file.close();
    return resultados;
}


