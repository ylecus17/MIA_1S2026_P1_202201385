#ifndef MOUNT_H
#define MOUNT_H

#include <string>
#include <vector>
#include <cstdint>

// Estructura para particiones montadas en RAM
struct MountedPartition {
    std::string ID;       // ID generado (carnet + correlativo + letra)
    std::string Path;     // Ruta del disco
    std::string Name;     // Nombre de la partición
    int Number;           // Correlativo de montaje
    std::string Letter;   // Letra asignada al disco
    int32_t Start;        // Offset en el disco
};
extern std::vector<MountedPartition> mountedPartitions;
// Funciones públicas del módulo mount
bool Mount(const std::string& path, const std::string& name);
void Mounted();

#endif // MOUNT_H
