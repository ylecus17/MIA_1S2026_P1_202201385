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
// los vectores en c++ se forman  vector<tipo> nombreVector;
// los vectores se mandan a llamar con el nombre del vector seguido de un punto y la función que se quiera usar, por ejemplo: mountedPartitions.push_back(mp); para agregar un elemento al vector
// los vectores al estar en un extern se llaman igual pero sin el extern, por ejemplo: std::vector<MountedPartition> mountedPartitions; para definir el vector en un archivo .cpp
// Funciones públicas del módulo mount
bool Mount(const std::string& path, const std::string& name);
void Mounted();
bool Unmount(const std::string& id);
MountedPartition* getMountById(const std::string& id);

#endif // MOUNT_H
