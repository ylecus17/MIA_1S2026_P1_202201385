#ifndef DISK_H
#define DISK_H

#include <string>
#include <vector>

struct ParticionesMontadas {
    std::string name;   // nombre de la partición
    std::string id;     // identificador único
    char fit;           // tipo de ajuste (F, B, W)
    std::string path;   // ruta del disco al que pertenece
    long size;          // tamaño en bytes
};
struct Disk {
    std::string name;
    char fit;
    long sizeBytes;
    std::string path;

    // Lista de particiones montadas
    std::vector<ParticionesMontadas> particionesMontadas;
};


extern std::vector<Disk> disks;
extern std::vector<ParticionesMontadas> particionesMontadas; // lista global de particiones montadas en RAM
bool MkDisk(int size, std::string unit, std::string path, std::string fit);
bool RmDisk(const std::string& path);
bool Fdisk(int size, std::string unit, std::string path,
           std::string name, std::string ptype, std::string fit,
           std::string delMode, int add);
#endif
