#ifndef ARCHIVO_H
#define ARCHIVO_H

#include <string>
#include <fstream>
#include <filesystem>
#include <vector>

class Archivo {
private:
    std::string name;       // ruta del archivo
    std::string datetime;   // fecha de última mod
    std::string owner;      // simulado
    std::size_t dimension;  // tamaño
    std::fstream file;      // stream persistente

public:
    explicit Archivo(const std::string& nombre);
    ~Archivo();

    // Abrir en modo arbitrario
    bool open(std::ios::openmode mode = std::ios::in | std::ios::out);
    // Abrir para append (crea si no existe)
    bool openForAppend();
    // Cerrar si está abierto
    void close();

    // Escritura básica
    bool write(const std::string& data);
    bool writeLine(const std::string& line);

    // CSV helpers
    static std::string escapeCsv(const std::string& s);
    bool writeCsvRow(const std::vector<std::string>& cols);
    bool ensureHeader(const std::string& headerLine);

    // Lectura simple
    std::string getLine();

    // Info y estado
    bool exists() const;
    std::string getInfo() const;

    // Getters
    std::string getName() const;
    std::string getOwner() const;
    std::string getDatetime() const;
    std::size_t getDimension() const;
};

#endif


