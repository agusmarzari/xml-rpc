#ifndef USUARIO_H
#define USUARIO_H

#include <string>

class Usuario {
private:
    int ID;
    std::string nombre;
    std::string clave;
    std::string privilegio; // "admin" o "operario"

public:
    Usuario();
    Usuario(int id, const std::string& nom, const std::string& pass, const std::string& priv);

    int getID() const;
    std::string getNombre() const;
    std::string getClave() const;
    std::string getPrivilegio() const;

    bool esAdmin() const;
};

#endif
