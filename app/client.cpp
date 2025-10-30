#include <iostream>
#include <string>
#include <limits>
#include <variant>
#include <cstdlib>
#include <cstdio>
#include <termios.h>
#include <unistd.h>
#include <regex>
#include <fstream>
#include <vector>
#include <iterator>

#include "XmlRpcClient.h"
#include "XmlRpcValue.h"
#include "Mensaje.h"
#include "base64.h"  // misma lib que trae tu proyecto

using namespace std;
using namespace XmlRpc;

// --- Leer password sin eco (POSIX) ---
static string read_password(const string& prompt) {
    cout << prompt;
    termios oldt{}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    string pwd;
    getline(cin, pwd);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << "\n";
    return pwd;
}

static void print_help() {
    cout << R"HELP(
Comandos disponibles:
  on / encender motores             -> M17
  off / apagar motores              -> M18
  grip on / activar gripper         -> M3
  grip off / desactivar gripper     -> M5
  home / homing                     -> G28
  status / rep / reporte            -> M114 (reporte de usuario / estado)
  abs                               -> G90 (modo absoluto)
  rel                               -> G91 (modo relativo)
  move x=<num> y=<num> z=<num>      -> G0 X.. Y.. Z..
  upload <archivo.gcode>            -> sube archivo (base64)
  run <archivo.gcode>               -> ejecuta archivo previamente subido
  salir                             -> terminar

Tips:
  - Los comandos no son sensibles a mayúsculas.
  - move admite una, dos o tres coordenadas (ej: "move x=100 z=50").
)HELP" << endl;
}

// --- Normaliza dato (int/double/string) para Mensaje ---
static Valor build_valor_from(const string& s) {
    // entero puro
    if (!s.empty() && s.find_first_not_of("0123456789") == string::npos) {
        try { return stoi(s); } catch (...) {}
    }
    // real (permite un punto y signo)
    if (!s.empty()) {
        bool ok = true; int dots = 0;
        for (char c : s) {
            if (!(isdigit(c) || c=='-' || c=='.')) { ok = false; break; }
            if (c=='.') dots++;
            if (dots > 1) { ok = false; break; }
        }
        if (ok) {
            try { return stod(s); } catch (...) {}
        }
    }
    return s; // string
}

// --- Helpers para upload ---
static bool read_file_to_string(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::string buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    out.swap(buf);
    return true;
}
static std::string to_base64(const std::string& bin) {
    base64<char> encoder;
    std::vector<char> out;
    int iostatus = 0;
    std::back_insert_iterator<std::vector<char>> it = std::back_inserter(out);
    encoder.put(bin.begin(), bin.end(), it, iostatus, base64<>::crlf());
    return std::string(out.begin(), out.end());
}

// --- Mapea alias del usuario a las frases que entiende el servidor ---
static string normalize_user_command(string in) {
    // bajar a minúsculas
    for (auto &c : in) c = std::tolower(c);

    // ayuda
    if (in == "help" || in == "?") return "help";

    // alias simples
    if (in == "on")  return "encender motores";
    if (in == "off") return "apagar motores";
    if (in == "home") return "homing";
    if (in == "rep" || in == "status") return "reporte";
    if (in == "grip on")  return "activar gripper";
    if (in == "grip off") return "desactivar gripper";
    if (in == "abs" || in == "g90") return "modo absoluto";
    if (in == "rel" || in == "g91") return "modo relativo";

    // move -> construir “mover brazo x=.. y=.. z=..”
    if (in.rfind("move", 0) == 0) {
        std::smatch m;
        std::regex rx(R"(x\s*=\s*([-\d\.]+))");
        std::regex ry(R"(y\s*=\s*([-\d\.]+))");
        std::regex rz(R"(z\s*=\s*([-\d\.]+))");
        string out = "mover brazo";
        if (std::regex_search(in, m, rx)) out += " x=" + m[1].str();
        if (std::regex_search(in, m, ry)) out += " y=" + m[1].str();
        if (std::regex_search(in, m, rz)) out += " z=" + m[1].str();
        return out;
    }

    // dejar tal cual para que lo resuelva el intérprete del server
    return in;
}

// --- Parseo de Mensaje serializado (para reporte) ---
static bool parse_mensaje_serializado(const std::string& s, std::string& tipo, std::string& valor) {
    // Formato: ID|usuario|clave|tipo|valor
    size_t p1 = s.find('|'); if (p1==string::npos) return false;
    size_t p2 = s.find('|', p1+1); if (p2==string::npos) return false;
    size_t p3 = s.find('|', p2+1); if (p3==string::npos) return false;
    size_t p4 = s.find('|', p3+1); if (p4==string::npos) return false;
    tipo = s.substr(p3+1, p4-(p3+1));
    valor = s.substr(p4+1);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Uso: ./client <ip_servidor> <puerto>\n";
        return 1;
    }
    const string ip   = argv[1];
    const int    port = atoi(argv[2]);

    XmlRpcClient client(ip.c_str(), port);

    cout << "=== Cliente TPI ===\n";
    cout << "Servidor: " << ip << ":" << port << "\n";
    cout << "Usuario: ";
    string usuario; getline(cin, usuario);
    string clave = read_password("Clave: ");

    cout << "\nEscribí 'help' para ver comandos.\n";

    long nextID = 1;
    while (true) {
        cout << "\n> ";
        string entrada;
        if (!getline(cin, entrada)) break;
        if (entrada == "salir") break;
        if (entrada == "help" || entrada == "?") { print_help(); continue; }
        if (entrada.empty()) continue;

        // --- Comandos especiales que se manejan acá: upload / run
        {
            // upload <archivo>
            if (entrada.rfind("upload ", 0) == 0) {
                std::string path = entrada.substr(7);
                if (path.empty()) { cout << "Uso: upload <archivo.gcode>\n"; continue; }

                std::string raw;
                if (!read_file_to_string(path, raw)) {
                    cout << "No pude leer el archivo: " << path << "\n";
                    continue;
                }
                // basename
                auto pos = path.find_last_of("/\\");
                std::string fname = (pos == std::string::npos) ? path : path.substr(pos+1);

                std::string b64 = to_base64(raw);
                std::string peticion = "upload filename=" + fname + " data=" + b64;

                Valor dato = peticion;  // string largo
                Mensaje msg(nextID++, usuario, clave, dato);
                const std::string serializado = msg.Serializar();

                XmlRpcValue args, result;
                args[0] = serializado;

                bool ok = client.execute("RecibirMensaje", args, result);
                if (!ok) { cerr << "[RPC] Error al subir.\n"; continue; }
                try {
                    cout << "Servidor: " << static_cast<std::string>(result) << "\n";
                } catch (...) {
                    cout << "Servidor devolvió un tipo inesperado.\n";
                }
                continue;
            }

            // run <archivo>
            if (entrada.rfind("run ", 0) == 0) {
                std::string fname = entrada.substr(4);
                if (fname.empty()) { cout << "Uso: run <archivo.gcode>\n"; continue; }

                std::string peticion = "run filename=" + fname;

                Valor dato = peticion;
                Mensaje msg(nextID++, usuario, clave, dato);
                const std::string serializado = msg.Serializar();

                XmlRpcValue args, result;
                args[0] = serializado;

                bool ok = client.execute("RecibirMensaje", args, result);
                if (!ok) { cerr << "[RPC] Error al ejecutar.\n"; continue; }
                try {
                    cout << "Servidor: " << static_cast<std::string>(result) << "\n";
                } catch (...) {
                    cout << "Servidor devolvió un tipo inesperado.\n";
                }
                continue;
            }
        }

        // --- Resto de comandos: mapear alias a frases del intérprete
        string peticion = normalize_user_command(entrada);
        if (peticion == "help") { print_help(); continue; }

        Valor dato = build_valor_from(peticion);
        Mensaje msg(nextID++, usuario, clave, dato);
        const string serializado = msg.Serializar();

        XmlRpcValue args, result;
        args[0] = serializado;

        bool ok = client.execute("RecibirMensaje", args, result);
        if (!ok) {
            cerr << "[RPC] Error al ejecutar 'RecibirMensaje'.\n";
            continue;
        }
        if (client.isFault()) {
            cerr << "[RPC] Fault del servidor: " << string(static_cast<string>(result)) << "\n";
            continue;
        }

        // Si pedimos "reporte", el server puede devolver un Mensaje serializado
        if (peticion == "reporte") {
            try {
                string payload = static_cast<string>(result);
                string tipo, valor;
                if (parse_mensaje_serializado(payload, tipo, valor) && tipo == "string") {
                    cout << "\n==== REPORTE ====\n" << valor << "\n=================\n";
                } else {
                    cout << "[WARN] No se pudo deserializar el reporte.\n";
                    cout << "Contenido recibido: " << payload << "\n";
                }
            } catch (...) {
                cout << "Servidor devolvió un tipo inesperado para 'reporte'.\n";
            }
        } else {
            // Respuestas comunes: string
            try {
                cout << "Servidor: " << static_cast<string>(result) << "\n";
            } catch (...) {
                cout << "Servidor devolvió un tipo inesperado.\n";
            }
        }
    }

    cout << "Cerrando cliente. ¡Chau!\n";
    return 0;
}


