#ifndef REPORTE_H
#define REPORTE_H

#include <string>
#include <vector>
#include "PALogger.h"

class Reporte {
private:
    int cantidadOrdenes;
    std::string OrdenesEjecutadas;
    std::string NombreUsuario;
    bool EstadoConexion;       // true si hay conexión activa con el robot
    std::string EstadoRobot;   // estado leído desde el Controlador

public:
    // --- Constructor ---
    Reporte(const std::string& nombreUsuario);

    // --- Setters ---
    void SetEstadoROBOT(const std::string& estado);
    void SetEstadoConexion(bool estado);

    // --- Generar datos del reporte ---
    void CargarOrdenes_Log();  // busca las órdenes del usuario
    std::string Serializar() const; // convierte el reporte en formato de texto

    // --- Getters ---
    int getCantidadOrdenes() const { return cantidadOrdenes; }
    std::string getOrdenesEjecutadas() const { return OrdenesEjecutadas; }
    std::string getEstadoRobot() const { return EstadoRobot; }
    bool getEstadoConexion() const { return EstadoConexion; }
};

#endif
