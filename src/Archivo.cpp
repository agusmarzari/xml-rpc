#include "Archivo.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

Archivo::Archivo(const std::string& nombre) : name(nombre), dimension(0) {
    if (std::filesystem::exists(nombre)) {
        dimension = std::filesystem::file_size(nombre);
        owner = "GRUPO_F";
        auto ftime = std::filesystem::last_write_time(nombre);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
        );
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&cftime), "%Y-%m-%d %H:%M:%S");
        datetime = ss.str();
    } else {
        owner = "GRUPO_F";
        datetime = "";
        dimension = 0;
    }
}

Archivo::~Archivo() {
    if (file.is_open()) file.close();
}

bool Archivo::open(std::ios::openmode mode) {
    file.open(name, mode);
    return file.is_open();
}

bool Archivo::openForAppend() {
    if (file.is_open()) return true; // dejamos el stream persistente
    file.open(name, std::ios::out | std::ios::app);
    return file.is_open();
}

void Archivo::close() {
    if (file.is_open()) file.close();
}

bool Archivo::write(const std::string& data) {
    if (!file.is_open() && !openForAppend()) return false;
    file << data;
    file.flush();
    return static_cast<bool>(file);
}

bool Archivo::writeLine(const std::string& line) {
    if (!file.is_open() && !openForAppend()) return false;
    file << line << '\n';
    file.flush();
    return static_cast<bool>(file);
}

bool Archivo::exists() const {
    return std::filesystem::exists(name);
}

std::string Archivo::getLine() {
    std::string out;
    if (file.is_open() && std::getline(file, out)) return out;
    return "";
}

std::string Archivo::getInfo() const {
    std::string info = "Archivo: " + name + "\n";
    info += "Propietario: " + owner + "\n";
    info += "Fecha mod: " + datetime + "\n";
    info += "Dimension: " + std::to_string(dimension) + " bytes\n";
    return info;
}

std::string Archivo::getName() const { return name; }
std::string Archivo::getOwner() const { return owner; }
std::string Archivo::getDatetime() const { return datetime; }
std::size_t Archivo::getDimension() const { return dimension; }

// ---------- CSV helpers ----------
std::string Archivo::escapeCsv(const std::string& s) {
    bool needQuotes = s.find_first_of(",\"\n\r") != std::string::npos;
    if (!needQuotes) return s;
    std::string out; out.reserve(s.size()+2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"') out.push_back('"'); // duplicar comillas internas
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

bool Archivo::writeCsvRow(const std::vector<std::string>& cols) {
    if (!file.is_open() && !openForAppend()) return false;
    for (std::size_t i = 0; i < cols.size(); ++i) {
        if (i) file << ',';
        file << escapeCsv(cols[i]);
    }
    file << '\n';
    file.flush();                    //  fuerza la escritura en disco
    return static_cast<bool>(file);
}

bool Archivo::ensureHeader(const std::string& headerLine) {
    // si el archivo no existe o está vacío -> escribir header
    if (!std::filesystem::exists(name) || std::filesystem::file_size(name) == 0) {
        if (!openForAppend()) return false;
        return writeLine(headerLine);
    }
    return true;
}


