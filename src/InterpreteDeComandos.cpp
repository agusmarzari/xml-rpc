#include "InterpreteDeComandos.h"

InterpreteDeComandos::InterpreteDeComandos() {
  equivalencias = {
    {"encender motores", "M17"},
    {"apagar motores", "M18"},
    {"activar gripper", "M3"},
    {"desactivar gripper", "M5"},
    {"reporte", "M114"},
    {"homing", "G28"},
    {"modo absoluto", "G90"},   // NUEVO
    {"modo relativo",  "G91"},  // NUEVO
    {"mover brazo", "G0"}
  };

}

std::string InterpreteDeComandos::traducir(const std::string& peticion) {
    std::string p = peticion;
    // Pasamos a minúsculas para evitar errores de capitalización
    for (auto& c : p) c = std::tolower(c);
    // al comienzo de traducir(...)
    if (p == "abs" || p == "g90") return "G90";
    if (p == "rel" || p == "g91") return "G91";


    // Buscar coincidencia directa
    for (const auto& [clave, gcode] : equivalencias) {
        if (p.find(clave) != std::string::npos) {
            if (clave == "mover brazo")
                return procesarMovimiento(p);
            else
                return gcode;
        }
    }

    return "ERR: comando desconocido";
}

std::string InterpreteDeComandos::procesarMovimiento(const std::string& peticion) {
    // Busca coordenadas con expresiones regulares
    std::regex re(R"(x\s*=\s*([-\d\.]+)|y\s*=\s*([-\d\.]+)|z\s*=\s*([-\d\.]+))",
                  std::regex_constants::icase);
    std::smatch match;
    std::stringstream gcode;
    gcode << "G0";

    auto begin = std::sregex_iterator(peticion.begin(), peticion.end(), re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch m = *it;
        if (m[1].matched) gcode << " X" << m[1].str();
        else if (m[2].matched) gcode << " Y" << m[2].str();
        else if (m[3].matched) gcode << " Z" << m[3].str();
    }

    // Si no hay coordenadas, devolver error
    if (begin == end)
        return "ERR: faltan coordenadas en el comando mover brazo";

    return gcode.str();
}
