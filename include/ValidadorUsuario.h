#ifndef VALIDADORUSUARIO_H
#define VALIDADORUSUARIO_H

#include <string>
#include <sqlite3.h>
#include "Usuario.h"

class ValidadorUsuario {
private:
    sqlite3* db;

public:
    ValidadorUsuario(const std::string& nombreBD);
    ~ValidadorUsuario();

    bool abrirBase(const std::string& nombreBD);
    bool validarCredenciales(const std::string& nombre, const std::string& clave, Usuario& usuario);
    bool existeUsuario(const std::string& nombre);
};

#endif
