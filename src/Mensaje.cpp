#include "Mensaje.h"
#include <sstream>
#include <iostream>

// --- Serializa el mensaje en formato ID|usuario|clave|tipoDato|valor ---
std::string Mensaje::Serializar() const {
    std::ostringstream oss;

    oss << ID << "|" << nombreUsuario << "|" << clave << "|";

    if (std::holds_alternative<int>(datos)) {
        oss << "int|" << std::get<int>(datos);
    } else if (std::holds_alternative<double>(datos)) {
        oss << "double|" << std::get<double>(datos);
    } else {
        oss << "string|" << std::get<std::string>(datos);
    }

    return oss.str();
}

// --- Deserializa a partir del formato anterior ---
bool Mensaje::Deserializar(const std::string& str) {
    std::istringstream iss(str);
    std::string parte;

    if (!std::getline(iss, parte, '|')) return false;
    try {
        ID = std::stoi(parte);
    } catch (...) {
        return false;
    }

    if (!std::getline(iss, nombreUsuario, '|')) return false;
    if (!std::getline(iss, clave, '|')) return false;

    std::string tipo;
    if (!std::getline(iss, tipo, '|')) return false;

    std::string valor;
    if (!std::getline(iss, valor, '|')) return false;

    if (tipo == "int") {
        datos = std::stoi(valor);
    } else if (tipo == "double") {
        datos = std::stod(valor);
    } else {
        datos = valor;
    }

    return true;
}

