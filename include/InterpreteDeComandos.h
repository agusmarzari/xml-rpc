#ifndef INTERPRETE_DE_COMANDOS_H
#define INTERPRETE_DE_COMANDOS_H

#include <string>
#include <unordered_map>
#include <sstream>
#include <regex>
#include <iostream>

class InterpreteDeComandos {
private:
    std::unordered_map<std::string, std::string> equivalencias;

public:
    InterpreteDeComandos();

    // Traduce la petición del cliente a G-code
    std::string traducir(const std::string& peticion);

private:
    // Método auxiliar para procesar el comando "mover brazo"
    std::string procesarMovimiento(const std::string& peticion);
};

#endif
