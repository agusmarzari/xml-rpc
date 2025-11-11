#include <iostream>
#include <string>
#include <variant>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <iterator>
#include <thread>
#include <chrono>
#include <memory>

#include "XmlRpc.h"
#include "Mensaje.h"
#include "ValidadorUsuario.h"
#include "Usuario.h"
#include "PALogger.h"
#include "InterpreteDeComandos.h"
#include "Reporte.h"
#include "base64.h"
#include "Controlador.h"   // <<<< agregado
#include "Archivo.h"       // <<<< agregado para usar Archivo

using namespace XmlRpc;

// ===== Estado simple del servidor (admin features) =====
static std::atomic<bool> g_remoteAccessEnabled{true};
static std::mutex g_logMutex;

static std::string readLastLogLines(const std::string& path, size_t N) {
    std::lock_guard<std::mutex> lk(g_logMutex);
    std::ifstream f(path);
    if (!f.is_open()) return "No se pudo abrir el log.";
    std::string line;
    std::deque<std::string> dq;
    while (std::getline(f, line)) {
        dq.push_back(line);
        if (dq.size() > N) dq.pop_front();
    }
    std::ostringstream oss;
    for (auto& s : dq) oss << s << "\n";
    return oss.str();
}

// ===== Helpers upload/run =====
static std::string trim(std::string s) {
    auto notspace = [](int ch){ return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
    return s;
}

static bool extractKV(const std::string& src, const std::string& key, std::string& out) {
    auto pos = src.find(key + "=");
    if (pos == std::string::npos) return false;
    pos += key.size() + 1;
    size_t end = src.find(' ', pos);
    out = (end == std::string::npos) ? src.substr(pos) : src.substr(pos, end - pos);
    return true;
}

// Decodifica base64<char> compatible con tu base64.h
static bool from_base64(const std::string& b64, std::string& binOut) {
    base64<char> decoder;
    std::vector<char> out;
    int iostatus = 0;
    auto it = std::back_inserter(out);
    decoder.get(b64.begin(), b64.end(), it, iostatus);
    if (iostatus != 0) return false;
    binOut.assign(out.begin(), out.end());
    return true;
}

static void ensureUploadsDir() {
    std::filesystem::path p("uploads");
    if (!std::filesystem::exists(p)) std::filesystem::create_directories(p);
}

static bool saveFile(const std::string& fname, const std::string& data) {
    ensureUploadsDir();
    std::filesystem::path p = std::filesystem::path("uploads") / fname;
    std::ofstream f(p, std::ios::binary);
    if (!f) return false;
    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    f.close();
    return static_cast<bool>(f);
}

// ===== Método remoto =====
class RecibirMensaje : public XmlRpcServerMethod {
private:
    // <<< integración Controlador >>>
    Controlador controlador_;
    std::atomic<bool> robotConectado_{false};

    // === Estado de grabación de trayectoria (modo “grabación”) ===
    bool grabacionActiva_ = false;                       // <<<< agregado
    std::unique_ptr<Archivo> archivoGrabacion_;          // <<<< agregado
    std::string nombreTrayectoria_;                      // <<<< agregado

public:
    RecibirMensaje(XmlRpcServer* s) : XmlRpcServerMethod("RecibirMensaje", s) {
        // No conectar en el constructor para evitar bloqueos o excepciones
        // al iniciar el servidor (puerto serie puede no estar disponible).
        auto& logger = PALogger::getInstance();
        logger.logEvento(PALogger::LogLevel::INFO,
                         "RecibirMensaje inicializado (conexion serie diferida)",
                         PALogger::Code::OK);
    }

    // Intento seguro de conexión al robot. Llamar desde main() tras bindAndListen
    void conectarRobot() {
        auto& logger = PALogger::getInstance();
        try {
            bool ok = controlador_.conectar();
            robotConectado_.store(ok);
            if (ok) {
                logger.logEvento(PALogger::LogLevel::INFO,
                                 "Conexión serie con robot establecida",
                                 PALogger::Code::OK);
            } else {
                logger.logEvento(PALogger::LogLevel::WARNING,
                                 "No se pudo establecer conexión serie con el robot",
                                 PALogger::Code::SERVER_ERROR);
            }
        } catch (const std::exception& e) {
            robotConectado_.store(false);
            logger.logEvento(PALogger::LogLevel::ERROR,
                             std::string("Excepcion al conectar robot: ") + e.what(),
                             PALogger::Code::SERVER_ERROR);
        }
    }

    void execute(XmlRpcValue& params, XmlRpcValue& result) override {
        auto& logger = PALogger::getInstance();

        // --- Validación de parámetros recibidos ---
        if (params.size() != 1 || params[0].getType() != XmlRpcValue::TypeString) {
            result = "Error: se esperaba un string serializado.";
            logger.logEvento(PALogger::LogLevel::ERROR,
                             "Parametros invalidos en la llamada RPC",
                             PALogger::Code::BAD_REQUEST, -1);
            return;
        }

        // --- Deserializar mensaje ---
        std::string serializado = static_cast<std::string>(params[0]);
        Mensaje msg;
        if (!msg.Deserializar(serializado)) {
            result = "Error al deserializar el mensaje.";
            logger.logEvento(PALogger::LogLevel::ERROR,
                             "Error al deserializar el mensaje",
                             PALogger::Code::BAD_REQUEST, -1);
            return;
        }

        // --- Autenticación ---
        ValidadorUsuario validador("db/usuarios.db");
        Usuario usuario;
        if (!validador.validarCredenciales(msg.getUsuario(), msg.getClave(), usuario)) {
            std::cout << "Login fallido de: " << msg.getUsuario() << std::endl;
            result = "Credenciales inválidas.";
            logger.logLogin(msg.getUsuario(), false, msg.getID());
            return;
        }
        std::cout << "Login OK: " << usuario.getNombre()
                  << " (" << usuario.getPrivilegio() << ")\n";
        logger.logLogin(usuario.getNombre(), true, msg.getID());

        // --- Obtener la petición como string ---
        std::string peticion;
        std::visit([&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::string>) peticion = val;
            else peticion = std::to_string(val);
        }, msg.obtenerDato());

        std::cout << "-------------------------------------------\n";
        std::cout << "Mensaje ID: " << msg.getID() << "\n";
        std::cout << "Usuario:    " << msg.getUsuario() << "\n";
        std::cout << "Peticion:   " << peticion << "\n";
        std::cout << "-------------------------------------------\n";

        // ===== Comandos de administración / ayuda =====

        // Catálogo de comandos (cualquier usuario)
        if (peticion == "comandos" || peticion == "ayuda" || peticion == "help") {
            std::string helpTxt =
                "Comandos usuario:\n"
                "  on | off | grip on | grip off | home | reporte | status\n"
                "  abs | rel | mover x=.. y=.. z=..\n"
                "  upload <archivo.gcode> | run <archivo.gcode>\n"
                "  guardar trayectoria=<archivo.gcode>\n"
                "  fin trayectoria\n"
                "Comandos admin:\n"
                "  admin acceso on | admin acceso off\n"
                "  admin log N  (ultimas N lineas del log)\n"
                "  conectar robot | desconectar robot (solo admin)\n";
            result = helpTxt;
            return;
        }

        // admin acceso on/off
        if (peticion == "admin acceso on" || peticion == "admin acceso off") {
            if (!usuario.esAdmin()) {
                result = "Permiso denegado: requiere admin.";
                logger.logEvento(PALogger::LogLevel::WARNING,
                                 "Intento cambiar acceso remoto sin privilegios",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            bool enable = (peticion == "admin acceso on");
            g_remoteAccessEnabled.store(enable);
            logger.logEvento(PALogger::LogLevel::INFO,
                             std::string("Acceso remoto: ") + (enable ? "ON" : "OFF"),
                             PALogger::Code::OK, msg.getID());
            result = std::string("Acceso remoto: ") + (enable ? "habilitado" : "deshabilitado");
            return;
        }

        // admin log N
        if (peticion.rfind("admin log ", 0) == 0) {
            if (!usuario.esAdmin()) {
                result = "Permiso denegado: requiere admin.";
                logger.logEvento(PALogger::LogLevel::WARNING,
                                 "Intento leer log sin privilegios",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            size_t N = 50; // default
            try { N = std::stoul(peticion.substr(10)); } catch (...) {}
            std::string logTxt = readLastLogLines("logs/Log_de_trabajo.csv", N);
            result = logTxt;
            return;
        }

        // Si el acceso remoto está OFF, permitimos solo 'reporte' y 'comandos'
        if (!g_remoteAccessEnabled.load()
            && peticion != "reporte" && peticion != "comandos"
            && peticion != "help" && peticion != "ayuda")
        {
            result = "Acceso remoto deshabilitado por el administrador.";
            logger.logEvento(PALogger::LogLevel::INFO,
                             "Peticion rechazada por acceso remoto OFF: " + peticion,
                             PALogger::Code::BAD_REQUEST, msg.getID());
            return;
        }

        // ===== NUEVO: Control del robot (conectar / desconectar) =====
        if (peticion == "desconectar robot") {
            if (!usuario.esAdmin()) {
                result = "Permiso denegado: solo el administrador puede desconectar el robot.";
                logger.logEvento(PALogger::LogLevel::WARNING,
                                 "Intento de desconectar robot sin privilegios",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            if (robotConectado_.load()) {
                controlador_.desconectar();
                robotConectado_.store(false);
                logger.logEvento(PALogger::LogLevel::INFO,
                                 "Robot desconectado por administrador",
                                 PALogger::Code::OK, msg.getID());
                result = "Robot desconectado.";
            } else {
                result = "El robot ya estaba desconectado.";
            }
            return;
        }

        if (peticion == "conectar robot") {
            if (!usuario.esAdmin()) {
                result = "Permiso denegado: solo el administrador puede conectar el robot.";
                logger.logEvento(PALogger::LogLevel::WARNING,
                                 "Intento de conectar robot sin privilegios",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            if (!robotConectado_.load()) {
                bool ok = controlador_.conectar();
                robotConectado_.store(ok);
                if (ok) {
                    logger.logEvento(PALogger::LogLevel::INFO,
                                     "Robot conectado por administrador",
                                     PALogger::Code::OK, msg.getID());
                    result = "Robot conectado correctamente.";
                } else {
                    logger.logEvento(PALogger::LogLevel::ERROR,
                                     "Fallo al conectar robot por administrador",
                                     PALogger::Code::SERVER_ERROR, msg.getID());
                    result = "Error al conectar el robot.";
                }
            } else {
                result = "El robot ya estaba conectado.";
            }
            return;
        }

        // ===== NUEVO: Modo GRABACIÓN (con estado, usando Archivo) =====
        // guardar trayectoria=<archivo.gcode>
        if (peticion.rfind("guardar trayectoria=", 0) == 0) {
            if (grabacionActiva_) {
                result = "Ya hay una grabación en curso: " + nombreTrayectoria_;
                return;
            }

            std::string fname;
            if (!extractKV(peticion, "guardar trayectoria", fname)) {
                size_t eq = peticion.find('=');
                if (eq != std::string::npos) fname = trim(peticion.substr(eq + 1));
            }
            if (fname.empty()) {
                result = "Error: use guardar trayectoria=<nombre>.gcode";
                logger.logEvento(PALogger::LogLevel::ERROR, "guardar trayectoria: nombre vacío",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            // normalizar a basename
            if (auto pos = fname.find_last_of("/\\"); pos != std::string::npos) fname = fname.substr(pos + 1);
            // agregar .gcode si no hay extensión
            if (fname.find('.') == std::string::npos) fname += ".gcode";

            ensureUploadsDir();
            std::filesystem::path ruta = std::filesystem::path("uploads") / fname;

            archivoGrabacion_ = std::make_unique<Archivo>(ruta.string());
            if (!archivoGrabacion_->open(std::ios::out | std::ios::trunc)) {
                result = "Error: no se pudo crear el archivo en uploads/.";
                archivoGrabacion_.reset();
                logger.logEvento(PALogger::LogLevel::ERROR, "guardar trayectoria: fallo crear " + fname,
                                 PALogger::Code::SERVER_ERROR, msg.getID());
                return;
            }

            grabacionActiva_ = true;
            nombreTrayectoria_ = fname;

            logger.logPeticion(usuario.getNombre(), "guardar trayectoria=" + fname, msg.getID(), PALogger::Code::OK);
            result = "Grabación iniciada: " + fname + ". Envíe comandos paso a paso y finalice con 'fin trayectoria'.";
            return;
        }

        // fin trayectoria
        if (peticion == "fin trayectoria") {
            if (!grabacionActiva_) {
                result = "No hay grabación activa.";
                return;
            }

            archivoGrabacion_->close();
            grabacionActiva_ = false;

            logger.logPeticion(usuario.getNombre(), "fin trayectoria", msg.getID(), PALogger::Code::OK);
            result = "Grabación finalizada: " + nombreTrayectoria_ + ". Puede ejecutar con 'run " + nombreTrayectoria_ + "'";
            nombreTrayectoria_.clear();
            return;
        }

        // Mientras haya grabación activa: traducir y guardar (no ejecutar)
        if (grabacionActiva_) {
            InterpreteDeComandos interprete;
            std::string gcode = interprete.traducir(peticion);

            if (gcode.rfind("ERR:", 0) == 0) {
                result = "Error al interpretar comando durante grabación.";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Grabación: error al interpretar '" + peticion + "'",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }

            bool ok = archivoGrabacion_->writeLine(gcode);
            if (!ok) {
                result = "Error: no se pudo escribir en archivo de grabación.";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Grabación: error al escribir '" + gcode + "'",
                                 PALogger::Code::SERVER_ERROR, msg.getID());
                return;
            }

            logger.logPeticion(usuario.getNombre(), "grabar " + nombreTrayectoria_ + ": " + gcode, msg.getID(), PALogger::Code::OK);
            result = "Guardado en " + nombreTrayectoria_ + ": " + gcode;
            return;
        }

        // ===== Manejo especial: UPLOAD / RUN =====

        // upload filename=... data=...
        if (peticion.rfind("upload ", 0) == 0) {
            std::string fname, b64;
            if (!extractKV(peticion, "filename", fname) || !extractKV(peticion, "data", b64)) {
                result = "Error: formato de upload invalido. Use: upload filename=<NOMBRE> data=<BASE64>";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Upload con formato invalido",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            auto pos = fname.find_last_of("/\\");
            if (pos != std::string::npos) fname = fname.substr(pos+1);

            std::string bin;
            if (!from_base64(b64, bin)) {
                result = "Error: base64 invalido.";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Upload base64 invalido",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            if (!saveFile(fname, bin)) {
                result = "Error: no se pudo guardar el archivo en 'uploads/'.";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Fallo al guardar archivo: " + fname,
                                 PALogger::Code::SERVER_ERROR, msg.getID());
                return;
            }

            logger.logPeticion(usuario.getNombre(), "upload " + fname, msg.getID(), PALogger::Code::OK);
            result = std::string("Archivo subido: ") + fname;
            return;
        }

        // run filename=...   o   run <archivo>
        if (peticion.rfind("run ", 0) == 0) {
            std::string fname;
            if (!extractKV(peticion, "filename", fname)) {
                if (peticion.size() > 4) fname = trim(peticion.substr(4));
            }
            if (fname.empty()) {
                result = "Error: formato de run invalido. Use: run filename=<NOMBRE> o run <NOMBRE>";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Run con formato invalido",
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }
            auto pos = fname.find_last_of("/\\");
            if (pos != std::string::npos) fname = fname.substr(pos+1);

            std::filesystem::path path = std::filesystem::path("uploads") / fname;
            std::ifstream f(path, std::ios::binary);
            if (!f.is_open()) {
                result = "Error: archivo no encontrado en 'uploads/'. Primero haga upload.";
                logger.logEvento(PALogger::LogLevel::ERROR,
                                 "Run: archivo inexistente: " + path.string(),
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                return;
            }

            // Envío real: leer línea a línea y mandar al controlador si está conectado
            if (!robotConectado_.load()) {
                result = "Error: archivo listo pero robot desconectado. Use 'conectar robot' si es admin.";
                logger.logEvento(PALogger::LogLevel::WARNING,
                                 "Run pedido pero robot desconectado: " + fname,
                                 PALogger::Code::BAD_REQUEST, msg.getID());
                f.close();
                return;
            }

            std::string line; size_t n = 0;
            std::ostringstream respLog;
            while (std::getline(f, line)) {
                // Normalizar/recortar la línea (elimina espacios alrededor)
                line = trim(line);
                if (line.empty()) continue; // ignorar líneas vacías

                ++n;
                // Enviar al controlador y registrar la respuesta
                std::string respuesta = controlador_.enviarComandoGcode(line);
                respLog << "L" << n << ": `" << line << "` -> `" << respuesta << "`\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            f.close();

            logger.logPeticion(usuario.getNombre(), "run " + fname, msg.getID(), PALogger::Code::OK);
            // Resumen: número de líneas y registro con respuestas (puede ser largo)
            result = std::string("Ejecucion completada: ") + fname + " (" + std::to_string(n) + " lineas)\n" + respLog.str();
            return;
        }

        // ===== Flujo normal: interpretar petición / reporte =====
        InterpreteDeComandos interprete;
        std::string comandoGcode = interprete.traducir(peticion);

        if (peticion == "reporte") {
            Reporte reporte(usuario.getNombre());
            reporte.CargarOrdenes_Log();

            std::string estado = "ROBOT DESCONECTADO";
            if (robotConectado_.load()) {
                estado = controlador_.enviarComandoGcode("M114");
            }
            reporte.SetEstadoROBOT(estado);
            reporte.SetEstadoConexion(robotConectado_.load());  // <<<< usa el estado real

            std::string reporteTexto = reporte.Serializar();
            logger.logEvento(PALogger::LogLevel::INFO,
                             "Reporte generado para " + usuario.getNombre(),
                             PALogger::Code::OK, msg.getID());

            Mensaje respuesta;
            respuesta.setID(msg.getID());
            respuesta.setUsuario("Servidor");
            respuesta.setClave("");
            respuesta.agregarDato(reporteTexto);
            result = respuesta.Serializar();

            std::cout << "\n" << reporteTexto << std::endl;
            return;
        }

        if (comandoGcode.rfind("ERR:", 0) == 0) {
            std::cout << "Error en interpretación: " << comandoGcode << std::endl;
            logger.logEvento(PALogger::LogLevel::ERROR,
                             "Error al interpretar comando: " + peticion,
                             PALogger::Code::BAD_REQUEST, msg.getID());
            result = "Error al interpretar la petición.";
            return;
        }

        logger.logPeticion(usuario.getNombre(), peticion, msg.getID(), PALogger::Code::OK);

        // ==== Envío real al controlador, si hay conexión ====
        if (!robotConectado_.load()) {
            result = "Error: robot desconectado. (Use 'conectar robot' si es admin)";
            logger.logEvento(PALogger::LogLevel::WARNING,
                             "Intento de comando con robot desconectado",
                             PALogger::Code::BAD_REQUEST, msg.getID());
            return;
        }

        std::string respuestaArduino = controlador_.enviarComandoGcode(comandoGcode);

        result = "Peticion procesada: " + std::string(comandoGcode) +
                 " | Arduino: " + respuestaArduino;
    }

    std::string help() override {
        return "Valida usuario, maneja admin acceso/log, upload/run, 'reporte' y traduccion a G-code, "
               "con robot serie conectado por defecto, y admin: conectar/desconectar. "
               "Tambien permite: 'guardar trayectoria=<archivo>.gcode' y 'fin trayectoria' para grabar paso a paso.";
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: ./server <puerto>\n";
        return 1;
    }

    int port = std::atoi(argv[1]);
    XmlRpcServer server;

    RecibirMensaje recibir(&server);

    auto& logger = PALogger::getInstance();
    logger.setLevel(PALogger::LogLevel::DEBUG);

    XmlRpc::setVerbosity(1);

    try {
    server.bindAndListen(port);
    std::cout << "Servidor escuchando en puerto " << port << ".\n";
    logger.logEvento(PALogger::LogLevel::INFO,
             "Servidor escuchando en puerto " + std::to_string(port));

    // Intento seguro de conectar al robot (si el hardware/puerto está disponible)
    recibir.conectarRobot();

    server.work(-1.0); // loop principal
    } catch (const std::exception& e) {
        std::cerr << "Error en el servidor: " << e.what() << std::endl;
        logger.logEvento(PALogger::LogLevel::ERROR,
                         std::string("Error fatal: ") + e.what(),
                         PALogger::Code::SERVER_ERROR);
        return 1;
    }

    logger.logEvento(PALogger::LogLevel::INFO, "Servidor finalizado correctamente");
    return 0;
}


