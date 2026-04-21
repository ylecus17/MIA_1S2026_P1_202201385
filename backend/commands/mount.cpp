#include "mount.h"
#include "../common/log.h"
#include "../structures/disk.h"
#include <fstream>
#include <map>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "disk.h"
const std::string carnetSuffix = "85";

std::vector<MountedPartition> mountedPartitions;
std::map<std::string,std::string> diskLetters;
std::map<std::string,int> partitionCounters;

MountedPartition* getMountById(const std::string& id)
{
    for (auto &mp : mountedPartitions)
    {
        if (mp.ID == id)
        {
            return &mp;
        }
    }
    return nullptr;
}
// ---------------- MOUNT ----------------
bool Mount(const std::string& path, const std::string& name) {
    if (path.empty() || name.empty()) {
        common::AddError("[MOUNT] Error: -path y -name son obligatorios");
        return false;
    }

    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[MOUNT] Error abriendo disco: " + path);
        return false;
    }

    MBR mbr{};
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    if (!file) {
        common::AddError("[MOUNT] Error leyendo MBR en " + path);
        return false;
    }

    int index = -1;
    int count = 1;
    char emptyId[4] = {0};

    for (int i = 0; i < 4; i++) {
        Partition part = mbr.Partitions[i];
        std::string partName(part.PartName);
        
        partName.erase(std::find(partName.begin(), partName.end(), '\0'), partName.end());
       
        if (part.PartSize > 0) {
                std::cout<<"Partición "<<i<<": Name="<<partName<<" Start="<<part.PartStart<<" Size="<<part.PartSize<<" Status="<<(part.PartStatus=='1'?"Montada":"Desmontada")<<"\n";
        }else{
            std::cout<<"Partición "<<i<<": Name="<<partName<<" [VACÍA]\n";
        }
        if (part.PartSize > 0 && partName == name) {
            if (part.PartStatus == '1' || std::memcmp(part.PartID, emptyId, sizeof(emptyId)) != 0) {
                common::AddError("[MOUNT] Partición " + name + " ya está montada (ID=" + std::string(part.PartID) + ")");
                return false;
            }
            index = i;
        }
        if (std::memcmp(part.PartID, emptyId, sizeof(emptyId)) != 0) {
            count++;
        }
    }

    if (index == -1) {
        common::AddError("[MOUNT] Partición " + name + " no encontrada");
        return false;
    }

    // Verificar si ya está en RAM
    for (auto& mp : mountedPartitions) {
        if (mp.Path == path && mp.Name == name) {
            common::AddInfo("[MOUNT] Partición " + name + " ya está montada en RAM con ID " + mp.ID);
            return true;
        }
    }

    // Asignar letra al disco si no tiene
    std::string letter;
    auto it = diskLetters.find(path);
    if (it == diskLetters.end()) {
        letter = std::string(1, 'A' + diskLetters.size());
        diskLetters[path] = letter;
    } else {
        letter = it->second;
    }

    // Generar ID: carnetSuffix + correlativo + letra
    std::string id = carnetSuffix + std::to_string(count) + letter;

    // Actualizar partición en MBR
    std::memset(mbr.Partitions[index].PartID, 0, sizeof(mbr.Partitions[index].PartID));
    std::memcpy(mbr.Partitions[index].PartID, id.c_str(), std::min(id.size(), sizeof(mbr.Partitions[index].PartID)));
    mbr.Partitions[index].PartStatus = '1';
    mbr.Partitions[index].PartCorrelative = count;

    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    if (!file) {
        common::AddError("[MOUNT] Error escribiendo MBR actualizado");
        return false;
    }
    file.close();

// Crear objeto para la lista global
MountedPartition mp{
    id,
    path,
    name,
    count,
    letter,
    mbr.Partitions[index].PartStart
};
mountedPartitions.push_back(mp);

// Crear objeto para la lista interna del disco
ParticionesMontadas nuevaPart;
nuevaPart.name = name;
nuevaPart.id = id;
nuevaPart.fit = mbr.DskFit;
nuevaPart.path = path;
nuevaPart.size = mbr.Partitions[index].PartSize;

// Buscar el disco en listaDiscos y agregar la partición
auto itDisk = std::find_if(disks.begin(), disks.end(),
    [&](Disk& d){ return d.path == path; });

if (itDisk != disks.end()) {
    itDisk->particionesMontadas.push_back(nuevaPart);
    common::AddInfo("[MOUNT] Partición " + name + " registrada en listaDiscos dentro de " + itDisk->name);
} else {
    common::AddError("[MOUNT] No se encontró el disco en listaDiscos para registrar la partición");
}


    common::AddSuccess("[MOUNT] Partición " + name + " montada con ID " + id + " en disco " + path);
    return true;
}

// ---------------- LISTAR ----------------

bool Unmount(const std::string& id) {
    // Buscar la partición montada en la lista global
    auto it = std::find_if(mountedPartitions.begin(), mountedPartitions.end(),
        [&](const MountedPartition& mp){ return mp.ID == id; });

    if (it == mountedPartitions.end()) {
        common::AddError("[UNMOUNT] No se encontró partición montada con ID " + id);
        return false;
    }

    std::string path = it->Path;
    std::string name = it->Name;

    // Abrir el disco
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[UNMOUNT] Error abriendo disco: " + path);
        return false;
    }

    // Leer MBR
    MBR mbr{};
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    if (!file) {
        common::AddError("[UNMOUNT] Error leyendo MBR en " + path);
        return false;
    }

    // Buscar la partición en el MBR y resetear correlativo/estado
    for (int i = 0; i < 4; i++) {
        Partition& part = mbr.Partitions[i];
        std::string pname(part.PartName);
        pname.erase(std::find(pname.begin(), pname.end(), '\0'), pname.end());

        if (pname == name) {
            part.PartStatus = '0';
            part.PartCorrelative = 0;
            std::memset(part.PartID, 0, sizeof(part.PartID));

            file.seekp(0, std::ios::beg);
            file.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
            file.close();

            // Quitar de la lista global
            mountedPartitions.erase(it);

            // Quitar de la lista interna del disco
            auto itDisk = std::find_if(disks.begin(), disks.end(),
                [&](Disk& d){ return d.path == path; });
            if (itDisk != disks.end()) {
                auto& pmList = itDisk->particionesMontadas;
                for (auto it2 = pmList.begin(); it2 != pmList.end(); ++it2) {
    if (it2->id == id) {
        pmList.erase(it2);
        break; // 👈 SOLO BORRA UNO
    }
}
            }

            common::AddSuccess("[UNMOUNT] Partición " + name + " desmontada con ID " + id);
            return true;
        }
    }

    common::AddError("[UNMOUNT] No se encontró partición " + name + " en el MBR de " + path);
    return false;
}

void Mounted() {
    if (mountedPartitions.empty()) {
        common::AddInfo("[MOUNTED] No hay particiones montadas actualmente");
        return;
    }

    common::AddInfo("[MOUNTED] Particiones montadas:");
    for (auto& mp : mountedPartitions) {
        common::AddInfo("  ID=" + mp.ID + " | Disco=" + mp.Path + " | Partición=" + mp.Name + " | Inicio=" + std::to_string(mp.Start));
    }
}
