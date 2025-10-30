#ifndef PALOGGER_H
#define PALOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include "Archivo.h"

class PALogger {
public:
    enum class LogLevel { DEBUG = 0, INFO, WARNING, ERROR };
    enum class Code : int { OK = 200, BAD_REQUEST = 400, UNAUTHORIZED = 401, SERVER_ERROR = 500 };

    static PALogger& getInstance();  // Singleton accessor

    // Métodos de log
    void logEvento(LogLevel level, const std::string& detalle, Code codigo = Code::OK, int nodo = -1);
    void logPeticion(const std::string& usuario, const std::string& peticion, int nodo, Code codigo);
    void logLogin(const std::string& usuario, bool ok, int nodo);

    // Configuración
    void setLevel(LogLevel level);

    // Visualización de logs: mostrar registros filtrados
    std::vector<std::string> mostrarPorTipo(const std::string& tipo, const std::string& usuarioFiltro = "");

private:
    PALogger();  // Constructor privado
    ~PALogger();

    PALogger(const PALogger&) = delete;
    PALogger& operator=(const PALogger&) = delete;

    LogLevel level_;
    Archivo csv_;
    std::mutex mtx_;

    // Internos
    std::string nowISO() const;
    std::string levelToString(LogLevel l) const;
    void ensureLogDirectory();
};

#endif
