#include "Usuario.h"

Usuario::Usuario() : ID(0) {}

Usuario::Usuario(int id, const std::string& nom, const std::string& pass, const std::string& priv)
    : ID(id), nombre(nom), clave(pass), privilegio(priv) {}

int Usuario::getID() const { return ID; }
std::string Usuario::getNombre() const { return nombre; }
std::string Usuario::getClave() const { return clave; }
std::string Usuario::getPrivilegio() const { return privilegio; }

bool Usuario::esAdmin() const { return privilegio == "admin"; }
