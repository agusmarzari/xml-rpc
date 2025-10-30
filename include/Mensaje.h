#ifndef MENSAJE_H
#define MENSAJE_H

#include <string>
#include <variant>

using Valor = std::variant<int, double, std::string>;

class Mensaje {
private:
    int ID;                     // Identificador del mensaje
    std::string nombreUsuario;  // Usuario remitente
    std::string clave;          // Clave del usuario
    Valor datos;                // Petición o mensaje (puede ser string, número, etc.)

public:
    // --- Constructores ---
    Mensaje() : ID(0), datos("") {}
    Mensaje(int id, const std::string& usuario, const std::string& clave, const Valor& datos)
        : ID(id), nombreUsuario(usuario), clave(clave), datos(datos) {}

    // --- Setters ---
    void setID(int id) { ID = id; }
    void setUsuario(const std::string& usuario) { nombreUsuario = usuario; }
    void setClave(const std::string& c) { clave = c; }
    void agregarDato(const Valor& d) { datos = d; }

    // --- Getters ---
    int getID() const { return ID; }
    std::string getUsuario() const { return nombreUsuario; }
    std::string getClave() const { return clave; }
    Valor obtenerDato() const { return datos; }

    // --- Serialización / deserialización ---
    std::string Serializar() const;
    bool Deserializar(const std::string& str);
};

#endif


