#include "ValidadorUsuario.h"
#include <iostream>

ValidadorUsuario::ValidadorUsuario(const std::string& nombreBD) : db(nullptr) {
    abrirBase(nombreBD);
}

ValidadorUsuario::~ValidadorUsuario() {
    if (db) sqlite3_close(db);
}

bool ValidadorUsuario::abrirBase(const std::string& nombreBD) {
    if (sqlite3_open(nombreBD.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error al abrir la base de datos: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return false;
    }
    return true;
}

bool ValidadorUsuario::existeUsuario(const std::string& nombre) {
    if (!db) return false;

    std::string sql = "SELECT COUNT(*) FROM usuarios WHERE nombre = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, nombre.c_str(), -1, SQLITE_STATIC);

    bool existe = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        existe = (count > 0);
    }

    sqlite3_finalize(stmt);
    return existe;
}

bool ValidadorUsuario::validarCredenciales(const std::string& nombre, const std::string& clave, Usuario& usuario) {
    if (!db) return false;

    std::string sql = "SELECT id, nombre, clave, privilegio FROM usuarios WHERE nombre = ? AND clave = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, nombre.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, clave.c_str(), -1, SQLITE_STATIC);

    bool valido = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string nombreDB = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string claveDB = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string privilegioDB = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

        usuario = Usuario(id, nombreDB, claveDB, privilegioDB);
        valido = true;
    }

    sqlite3_finalize(stmt);
    return valido;
}
