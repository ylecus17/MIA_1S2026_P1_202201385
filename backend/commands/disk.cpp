#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <ctime>
#include <random>
#include <algorithm>
#include <string>


#include "../structures/disk.h"
#include "../common/log.h"
struct Disk {
    std::string name;   // nombre del disco (ej. "disco1")
    char fit;           // 'F' = First Fit, 'B' = Best Fit, 'W' = Worst Fit
    long sizeBytes;     // tamaño en bytes
    std::string path;   // ruta del archivo físico en el sistema
};
//lista global de discos 
std::vector<Disk> disks;

// Prototipos de funciones auxiliares
bool createPrimary(MBR* mbr, std::fstream& file, int32_t sizeBytes,
                   const std::string& name, const std::string& fit, const std::string& path);

bool createExtended(MBR* mbr, std::fstream& file, int32_t sizeBytes,
                    const std::string& name, const std::string& fit, const std::string& path);

bool createLogical(MBR* mbr, std::fstream& file, int32_t sizeBytes,
                   const std::string& name, const std::string& fit, const std::string& path);

std::pair<int32_t,int> findFreePartitionSlot(MBR* mbr, int32_t size);

using namespace std;

int randomSignature(){
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1,100000);
    return dist(gen);
}

string currentDate(){
    time_t now = time(0);
    tm *ltm = localtime(&now);

    char buffer[20];
    strftime(buffer,20,"%Y-%m-%d %H:%M:%S",ltm);

    return string(buffer);
}

bool MkDisk(int size, std::string unit, std::string path, std::string fit) {
    common::AddInfo("[MKDISK] Iniciando proceso de creacion de disco");

    // Crear directorios
    filesystem::create_directories(filesystem::path(path).parent_path());
    common::AddInfo("[MKDISK] Directorios creados para ruta: " + path);

    // Calcular tamaño en bytes
    int sizeBytes;
    if (unit == "K") {
        sizeBytes = size * 1024;
      
    } else {
        sizeBytes = size * 1024 * 1024;
    }

    // Crear archivo binario y escribir ceros
    ofstream file(path, ios::binary);

    if (!file.is_open()) {
        // este if demuestra que si el archivo no se pudo abrir, lo cual puede ser por varias razones como permisos o ruta incorrecta
        common::AddError("[MKDISK] Error abriendo archivo en " + path);
        return false;
    }

    // Escribir bloques de ceros
    vector<char> zeros(1024, 0);
    for (int i = 0; i < sizeBytes / 1024; i++) {
        file.write(zeros.data(), 1024);
    }

// Crear MBR
// aca mandamos a llamar a la estructura MBR y la llenamos con los datos necesarios para luego escribirla al inicio del disco, esto es importante para luego poder manejar las particiones dentro del disco
// para llamar a una estructura, se crea una variable del tipo de la estructura, en este caso MBR, y luego se llenan sus campos con los datos necesarios, como el tamaño del disco, la fecha de creación, la firma del disco, el tipo de ajuste y las particiones vacías
MBR mbr{};

// mbr{} significa que es una lista de inicialización que inicializa todos los campos de la estructura MBR a sus valores predeterminados (cero para enteros, caracteres nulos para cadenas, etc.), esto es importante para asegurarnos de que no haya datos basura en la estructura antes de llenarla con los datos necesarios
mbr.MbrSize = sizeBytes;

// fecha actual
std::string date = currentDate();
std::memset(mbr.MbrCreationDate, 0, sizeof(mbr.MbrCreationDate));
std::memcpy(mbr.MbrCreationDate, date.c_str(), std::min(date.size(), sizeof(mbr.MbrCreationDate)));

mbr.MbrDiskSignature = randomSignature();
mbr.DskFit = fit.empty() ? 'F' : fit[0];

// inicializar las 4 particiones vacías
for (int i = 0; i < 4; i++) {
    mbr.Partitions[i].PartStatus = '0';
    mbr.Partitions[i].PartType = '0';
    mbr.Partitions[i].PartFit = '0';
    mbr.Partitions[i].PartStart = -1;
    mbr.Partitions[i].PartSize = 0;
    mbr.Partitions[i].PartCorrelative = -1;
    std::memset(mbr.Partitions[i].PartName, 0, sizeof(mbr.Partitions[i].PartName));
    std::memset(mbr.Partitions[i].PartID, 0, sizeof(mbr.Partitions[i].PartID));
}
// solo crea un mbr por comando 
// para llamar a esta lista de 
// escribir MBR al inicio del archivo
file.seekp(0, std::ios::beg);
file.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
// el sizeof(MBR) es importante porque nos dice cuanto espacio ocupa el MBR en el disco, esto es importante para luego poder manejar las particiones dentro del disco, ya que el MBR se encuentra al inicio del disco y contiene la información de las particiones, por lo que es necesario saber cuanto espacio ocupa para luego poder escribir las particiones después del MBR
file.close();
// Verificar si ya existe un disco con el mismo path
auto it = std::find_if(disks.begin(), disks.end(),
    [&](const Disk& d){ return d.path == path; });

if (it != disks.end()) {
    common::AddError("[MKDISK] Ya existe un disco registrado en la ruta: " + path);
    return false; // no lo agregamos de nuevo
}
    Disk newDisk;
    newDisk.name = filesystem::path(path).stem().string(); // nombre sin extensión
    newDisk.fit = mbr.DskFit;
    newDisk.sizeBytes = sizeBytes;
    newDisk.path = path;

    disks.push_back(newDisk);
    std::cout << "\n=== Lista de discos creados ===\n";
    for (const auto& d : disks) {
        std::cout << "Nombre: " << d.name
                  << " | Fit: " << d.fit
                  << " | Tamaño: " << d.sizeBytes
                  << " bytes | Path: " << d.path << "\n";
    }
    std::cout << "===============================\n";
    common::AddSuccess("[MKDISK] Disco creado correctamente en " + path);
    return true;
}
bool RmDisk(const std::string& path) {
    common::AddInfo("[RMDISK] Intentando eliminar disco en " + path);

    try {
        if (std::filesystem::remove(path)) {
            common::AddSuccess("[RMDISK] Disco eliminado correctamente");
            return true;
        } else {
            common::AddError("[RMDISK] No se encontró el archivo en " + path);
            return false;
        }
    } catch (const std::exception& e) {
        common::AddError(std::string("[RMDISK] Error eliminando disco: ") + e.what());
        return false;
    }
} 

bool createPrimary(MBR* mbr, std::fstream& file, int32_t sizeBytes,
                   const std::string& name, const std::string& fit, const std::string& path) {
    int count = 0;
    for (auto& p : mbr->Partitions) {
        if (p.PartSize > 0) count++;
    }
    if (count >= 4) {
        common::AddError("[FDISK " + path + "] máximo 4 particiones (primarias+extendida)");
        return false;
    }

    auto [start, idx] = findFreePartitionSlot(mbr, sizeBytes);
    if (idx == -1) {
        common::AddError("[FDISK " + path + "] no hay espacio suficiente");
        return false;
    }

    Partition part{};
    part.PartStatus = '0';
    part.PartType = 'P';
    part.PartFit = fit.empty() ? 'F' : fit[0];
    part.PartStart = start;
    part.PartSize = sizeBytes;
    part.PartCorrelative = -1;
    std::memset(part.PartName, 0, sizeof(part.PartName));
    std::memcpy(part.PartName, name.c_str(), std::min(name.size(), sizeof(part.PartName)));
    std::memset(part.PartID, 0, sizeof(part.PartID));

    mbr->Partitions[idx] = part;

    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<char*>(mbr), sizeof(MBR));
    if (!file) {
        common::AddError("[FDISK " + path + "] error guardando MBR");
        return false;
    }

    return true;
}
bool createExtended(MBR* mbr, std::fstream& file, int32_t sizeBytes,
                    const std::string& name, const std::string& fit, const std::string& path) {
    for (auto& p : mbr->Partitions) {
        if (p.PartType == 'E' && p.PartSize > 0) {
            common::AddError("[FDISK " + path + "] ya existe una partición extendida");
            return false;
        }
    }

    int count = 0;
    for (auto& p : mbr->Partitions) {
        if (p.PartSize > 0) count++;
    }
    if (count >= 4) {
        common::AddError("[FDISK " + path + "] máximo 4 particiones (primarias+extendida)");
        return false;
    }

    auto [start, idx] = findFreePartitionSlot(mbr, sizeBytes);
    if (idx == -1) {
        common::AddError("[FDISK " + path + "] no hay espacio suficiente para extendida");
        return false;
    }

    Partition part{};
    part.PartStatus = '0';
    part.PartType = 'E';
    part.PartFit = fit.empty() ? 'F' : fit[0];
    part.PartStart = start;
    part.PartSize = sizeBytes;
    part.PartCorrelative = -1;
    std::memset(part.PartName, 0, sizeof(part.PartName));
    std::memcpy(part.PartName, name.c_str(), std::min(name.size(), sizeof(part.PartName)));
    std::memset(part.PartID, 0, sizeof(part.PartID));

    mbr->Partitions[idx] = part;

    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<char*>(mbr), sizeof(MBR));
    if (!file) {
        common::AddError("[FDISK " + path + "] error guardando MBR");
        return false;
    }

    // Crear primer EBR vacío al inicio de la extendida
    EBR ebr{};
    ebr.PartStatus = '0';
    ebr.PartFit = 'F';
    ebr.PartStart = start;
    ebr.PartSize = 0;
    ebr.PartNext = -1;
    std::memset(ebr.PartName, 0, sizeof(ebr.PartName));

    file.seekp(part.PartStart, std::ios::beg);
    file.write(reinterpret_cast<char*>(&ebr), sizeof(EBR));
    if (!file) {
        common::AddError("[FDISK " + path + "] error escribiendo EBR inicial");
        return false;
    }

    return true;
}
bool createLogical(MBR* mbr, std::fstream& file, int32_t sizeBytes,
                   const std::string& name, const std::string& fit, const std::string& path) {
    // Buscar partición extendida
    Partition* extended = nullptr;
    for (int i = 0; i < 4; i++) {
        if (mbr->Partitions[i].PartType == 'E' && mbr->Partitions[i].PartSize > 0) {
            extended = &mbr->Partitions[i];
            break;
        }
    }
    if (extended == nullptr) {
        common::AddError("[FDISK " + path + "] no se puede crear lógica porque no existe extendida");
        return false;
    }

    EBR ebr{};
    int32_t pos = extended->PartStart;

    while (true) {
        file.seekg(pos, std::ios::beg);
        file.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
        if (!file) {
            common::AddError("[FDISK " + path + "] error leyendo EBR");
            return false;
        }

        if (ebr.PartSize == 0) {
            break; // espacio vacío
        }

        std::string pname(ebr.PartName);
        pname.erase(std::find(pname.begin(), pname.end(), '\0'), pname.end());
        if (pname == name) {
            common::AddError("[FDISK " + path + "] ya existe partición lógica con nombre " + name);
            return false;
        }

        if (ebr.PartNext != -1) {
            pos = ebr.PartNext;
        } else {
            pos = ebr.PartStart + ebr.PartSize + sizeof(EBR);
            break;
        }
    }

    if (pos + sizeBytes > extended->PartStart + extended->PartSize) {
        common::AddError("[FDISK " + path + "] no hay espacio suficiente en la extendida");
        return false;
    }

    EBR newEBR{};
    newEBR.PartStatus = '0';
    newEBR.PartFit = fit.empty() ? 'F' : fit[0];
    newEBR.PartStart = pos;
    newEBR.PartSize = sizeBytes;
    newEBR.PartNext = -1;
    std::memset(newEBR.PartName, 0, sizeof(newEBR.PartName));
    std::memcpy(newEBR.PartName, name.c_str(), std::min(name.size(), sizeof(newEBR.PartName)));

    file.seekp(pos, std::ios::beg);
    file.write(reinterpret_cast<char*>(&newEBR), sizeof(EBR));
    if (!file) {
        common::AddError("[FDISK " + path + "] error escribiendo nuevo EBR");
        return false;
    }

    return true;
}
std::pair<int32_t,int> findFreePartitionSlot(MBR* mbr, int32_t size) {
    int32_t diskSize = mbr->MbrSize;
    int32_t mbrSize = sizeof(MBR);

    // recolectar particiones activas
    std::vector<Partition> parts;
    for (auto& p : mbr->Partitions) {
        if (p.PartSize > 0) parts.push_back(p);
    }

    // ordenar por inicio
    std::sort(parts.begin(), parts.end(),
              [](const Partition& a, const Partition& b) {
                  return a.PartStart < b.PartStart;
              });

    struct gap { int32_t start; int32_t size; };
    std::vector<gap> gaps;

    if (parts.empty()) {
        gaps.push_back({mbrSize, diskSize - mbrSize});
    } else {
        if (parts[0].PartStart > mbrSize) {
            gaps.push_back({mbrSize, parts[0].PartStart - mbrSize});
        }
        for (size_t i = 0; i < parts.size()-1; i++) {
            int32_t start = parts[i].PartStart + parts[i].PartSize;
            int32_t end = parts[i+1].PartStart;
            if (end > start) {
                gaps.push_back({start, end - start});
            }
        }
        Partition last = parts.back();
        int32_t end = last.PartStart + last.PartSize;
        if (diskSize > end) {
            gaps.push_back({end, diskSize - end});
        }
    }

    gap chosen{};
    bool found = false;
    switch (mbr->DskFit) {
        case 'F': // First Fit
            for (auto& g : gaps) {
                if (g.size >= size) { chosen = g; found = true; break; }
            }
            break;
        case 'B': { // Best Fit
            int32_t min = INT32_MAX;
            for (auto& g : gaps) {
                if (g.size >= size && g.size < min) {
                    chosen = g; min = g.size; found = true;
                }
            }
            break;
        }
        case 'W': { // Worst Fit
            int32_t max = -1;
            for (auto& g : gaps) {
                if (g.size >= size && g.size > max) {
                    chosen = g; max = g.size; found = true;
                }
            }
            break;
        }
    }

    if (!found) return {-1,-1};

    int idx = -1;
    for (int i = 0; i < 4; i++) {
        if (mbr->Partitions[i].PartSize == 0) { idx = i; break; }
    }
    if (idx == -1) return {-1,-1};

    return {chosen.start, idx};
}

bool Fdisk(int size, std::string unit, std::string path,
           std::string name, std::string ptype, std::string fit) {
    // Convertir size a bytes
    int32_t sizeBytes = 0;
    std::string u = unit;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    if (u == "B") {
        sizeBytes = static_cast<int32_t>(size);
    } else if (u == "K") {
        sizeBytes = static_cast<int32_t>(size * 1024);
    } else if (u == "M") {
        sizeBytes = static_cast<int32_t>(size * 1024 * 1024);
    }

    // Abrir el disco
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[FDISK " + path + "] Error abriendo disco");
        return false;
    }

    // Leer el MBR
    MBR mbr;
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    if (!file) {
        common::AddError("[FDISK " + path + "] Error leyendo MBR");
        return false;
    }
    common::AddInfo("[FDISK " + path + "] MBR leído correctamente");

    // Validar nombre único
for (auto& p : mbr.Partitions) {
    if (p.PartSize > 0) {
        std::string pname(p.PartName);
        pname.erase(std::find(pname.begin(), pname.end(), '\0'), pname.end());

        if (pname == name) {
            common::AddError("[FDISK " + path + "] Error: ya existe partición con nombre " + name);
            return false;
        }
    }
}

    // Crear partición según tipo
    bool ok = false;
    std::string pt = ptype;
    std::transform(pt.begin(), pt.end(), pt.begin(), ::toupper);
    if (pt == "P") {
        ok = createPrimary(&mbr, file, sizeBytes, name, fit, path);
    } else if (pt == "E") {
        ok = createExtended(&mbr, file, sizeBytes, name, fit, path);
    } else if (pt == "L") {
        ok = createLogical(&mbr, file, sizeBytes, name, fit, path);
    }

    if (!ok) {
        common::AddError("[FDISK " + path + "] Error creando partición " + name);
        return false;
    }

    common::AddSuccess("[FDISK " + path + "] Partición " + name + " creada con éxito");
    return true;
}
