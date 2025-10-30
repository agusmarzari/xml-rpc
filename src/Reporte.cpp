#include "Reporte.h"
#include <sstream>
#include <iostream>

Reporte::Reporte(const std::string& nombreUsuario)
    : cantidadOrdenes(0), 
      NombreUsuario(nombreUsuario),
      EstadoConexion(false), //esto se va a obtener de COntrolador dps
      EstadoRobot("Desconocido") {} //aca va la rta del controlador al comando m114


void Reporte::SetEstadoROBOT(const std::string& estado) {
    EstadoRobot = estado;
}

void Reporte::SetEstadoConexion(bool estado) {
    EstadoConexion = estado;
}

void Reporte::CargarOrdenes_Log() {
    auto& logger = PALogger::getInstance();

    // Recuperamos todas las líneas tipo "PETICION"
    auto peticiones = logger.mostrarPorTipo("PETICION");

    std::ostringstream oss;
    cantidadOrdenes = 0;

    for (const auto& linea : peticiones) {
        // Filtramos solo las del usuario correspondiente
        if (linea.find(NombreUsuario) != std::string::npos) {
            cantidadOrdenes++;
            oss << linea << "\n";
        }
    }

    if (cantidadOrdenes == 0)
        OrdenesEjecutadas = "No se registraron peticiones previas.";
    else
        OrdenesEjecutadas = oss.str();
}


std::string Reporte::Serializar() const {
    std::ostringstream oss;
    oss << "----- REPORTE DE USUARIO -----\n";
    oss << "Usuario: " << NombreUsuario << "\n";
    oss << "Cantidad de órdenes ejecutadas: " << cantidadOrdenes << "\n";
    oss << "Estado del robot: " << EstadoRobot << "\n";
    oss << "Conexión activa: " << (EstadoConexion ? "Sí" : "No") << "\n";
    oss << "--------------------------------\n";
    oss << "Órdenes ejecutadas:\n" << OrdenesEjecutadas;
    return oss.str();
}
