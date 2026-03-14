#include "mkfs.h"
#include "mount.h"
#include "../common/log.h"
#include "../structures/disk.h"
#include "../structures/filesystem.h"
#include <fstream>
#include <cmath>
#include <ctime>
#include <cstring>
#include <algorithm>

// Buscar partición montada por ID


// ---------------- MKFS ----------------
bool Mkfs(const std::string &id, const std::string &fstype)
{
    MountedPartition *mp = getMountById(id);
    if (!mp)
    {
        common::AddError("[MKFS] Error: partición con ID " + id + " no está montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        common::AddError("[MKFS] Error abriendo disco: " + mp->Path);
        return false;
    }

    // Leer MBR
    MBR mbr{};
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(&mbr), sizeof(MBR));
    if (!file)
    {
        common::AddError("[MKFS] Error leyendo MBR en " + mp->Path);
        return false;
    }

    // Buscar partición por PartID
   // Buscar partición por nombre (el que guardó mount)
Partition *part = nullptr;

for (int i = 0; i < 4; i++)
{
    std::string pname(mbr.Partitions[i].PartName);

    size_t pos = pname.find('\0');
    if (pos != std::string::npos)
    {
        pname.erase(pos);
    }

    if (pname == mp->Name)
    {
        part = &mbr.Partitions[i];
        break;
    }
}

if (!part)
{
    common::AddError("[MKFS] Error: partición " + mp->Name + " no encontrada en el disco");
    return false;
}
    // Calcular estructuras
    int32_t inodeSize = sizeof(Inodo);
    int32_t blockSize = sizeof(BloqueArchivo);
    int32_t superSize = sizeof(SuperBloque);

    double numerador = static_cast<double>(part->PartSize - superSize);
    double denominador = static_cast<double>(1 + 3 + inodeSize + 3 * blockSize);
    int32_t n = static_cast<int32_t>(std::floor(numerador / denominador));

    if (n <= 0)
    {
        common::AddError("[MKFS] Error: partición demasiado pequeña");
        return false;
    }

    common::AddInfo("[MKFS] Formateando " + mp->Name + " como EXT2 con n=" + std::to_string(n));

    // Crear superbloque
    SuperBloque sb = crearSuperbloque(n, inodeSize, blockSize, superSize, *part);

    // Escribir superbloque
    file.seekp(part->PartStart, std::ios::beg);
    file.write(reinterpret_cast<char *>(&sb), sizeof(SuperBloque));

    // Inicializar bitmaps
    inicializarBitmaps(file, sb, n);

    // Crear carpeta raíz y users.txt
    crearRaiz(file, sb);
    crearUsersTxt(file, sb);

    common::AddSuccess("[MKFS] Partición " + mp->Name + " lista con EXT2. Inodos=" +
                       std::to_string(sb.SInodesCount) + ", Bloques=" + std::to_string(sb.SBlocksCount));
    return true;
}

// ---------------- CREAR SUPERBLOQUE ----------------
SuperBloque crearSuperbloque(int32_t n, int32_t inodeSize, int32_t blockSize, int32_t superSize, const Partition &part)
{
    SuperBloque sb{};
    sb.SFileSystemType = 2;
    sb.SInodesCount = n;
    sb.SBlocksCount = 3 * n;
    sb.SFreeInodesCount = n - 2;
    sb.SFreeBlocksCount = 3 * n - 2;
    sb.SMagic = 0xEF53;
    sb.SInodeSize = inodeSize;
    sb.SBlockSize = blockSize;
    sb.SFirstIno = 2;
    sb.SFirstBlo = 2;
    sb.SMntCount = 1;

    std::time_t now = std::time(nullptr);
    std::string date = std::string(std::ctime(&now));
    std::memset(sb.SMtime, 0, sizeof(sb.SMtime));
    std::memcpy(sb.SMtime, date.c_str(), std::min(date.size(), sizeof(sb.SMtime)));
    std::memset(sb.SUmtime, 0, sizeof(sb.SUmtime));
    std::memcpy(sb.SUmtime, "0000-00-00 00:00:00", 19);

    sb.SBitmapInodeStart = part.PartStart + superSize;
    sb.SBitmapBlockStart = sb.SBitmapInodeStart + n;
    sb.SInodeStart = sb.SBitmapBlockStart + 3 * n;
    sb.SBlockStart = sb.SInodeStart + n * inodeSize;
    return sb;
}

// ---------------- INICIALIZAR BITMAPS ----------------
void inicializarBitmaps(std::fstream &file, const SuperBloque &sb, int32_t n)
{
    file.seekp(sb.SBitmapInodeStart, std::ios::beg);
    for (int32_t i = 0; i < n; i++)
    {
        char zero = 0;
        file.write(&zero, 1);
    }
    file.seekp(sb.SBitmapBlockStart, std::ios::beg);
    for (int32_t i = 0; i < 3 * n; i++)
    {
        char zero = 0;
        file.write(&zero, 1);
    }
}

// ---------------- CREAR RAÍZ ----------------
// ---------------- CREAR RAÍZ ----------------
void crearRaiz(std::fstream &file, const SuperBloque &sb)
{
    Inodo rootInode{};
    rootInode.IUid = 0;
    rootInode.IGid = 0;
    rootInode.ISize = 0;
    rootInode.IType = '0'; // carpeta
    rootInode.IPerm[0] = '6';
    rootInode.IPerm[1] = '6';
    rootInode.IPerm[2] = '4';

    std::time_t now = std::time(nullptr);
    std::string date = std::string(std::ctime(&now));
    std::memcpy(rootInode.ICtime, date.c_str(), std::min(date.size(), sizeof(rootInode.ICtime)));

    for (int i = 0; i < 15; i++)
        rootInode.IBlock[i] = -1;
    rootInode.IBlock[0] = 0; // apunta al bloque 0 (carpeta raíz)

    BloqueCarpeta rootBlock{};
    std::memset(&rootBlock, 0, sizeof(rootBlock));
    std::memcpy(rootBlock.BContent[0].BName, ".", 1);
    rootBlock.BContent[0].BInodo = 0;
    std::memcpy(rootBlock.BContent[1].BName, "..", 2);
    rootBlock.BContent[1].BInodo = 0;
    // IMPORTANTE: aquí ya dejamos preparada la entrada para users.txt
    std::memcpy(rootBlock.BContent[2].BName, "users.txt", 9);
    rootBlock.BContent[2].BInodo = 1; // apunta al inodo 1

    // Actualizar bitmaps: inodo 0 y bloque 0 ocupados
    file.seekp(sb.SBitmapInodeStart, std::ios::beg);
    char one = 1;
    file.write(&one, 1);
    file.seekp(sb.SBitmapBlockStart, std::ios::beg);
    file.write(&one, 1);

    // Escribir inodo raíz y bloque de carpeta raíz
    file.seekp(sb.SInodeStart, std::ios::beg);
    file.write(reinterpret_cast<char *>(&rootInode), sizeof(Inodo));
    file.seekp(sb.SBlockStart, std::ios::beg);
    file.write(reinterpret_cast<char *>(&rootBlock), sizeof(BloqueCarpeta));
}

// ---------------- CREAR USERS.TXT ----------------
void crearUsersTxt(std::fstream& file, const SuperBloque& sb) {
    std::string content = "1,G,root\n1,U,root,root,123\n";

    // Inodo para users.txt (inodo 1)
    Inodo usersInode{};
    usersInode.IUid = 1;
    usersInode.IGid = 1;
    usersInode.ISize = static_cast<int32_t>(content.size());
    usersInode.IType = '1'; // archivo
    usersInode.IPerm[0] = '6'; usersInode.IPerm[1] = '6'; usersInode.IPerm[2] = '4';

    std::time_t now = std::time(nullptr);
    std::string date = std::string(std::ctime(&now));
    std::memset(usersInode.ICtime, 0, sizeof(usersInode.ICtime));
    std::memcpy(usersInode.ICtime, date.c_str(), std::min(date.size(), sizeof(usersInode.ICtime)));

    for (int i = 0; i < 15; i++) usersInode.IBlock[i] = -1;
    usersInode.IBlock[0] = 1; // apunta al bloque 1

    // Bloque con contenido del archivo
    BloqueArchivo usersBlock{};
    std::memset(usersBlock.BContent, 0, sizeof(usersBlock.BContent));
    std::memcpy(usersBlock.BContent, content.c_str(), std::min(content.size(), sizeof(usersBlock.BContent)));

    // Actualizar bitmaps: inodo 1 y bloque 1 ocupados
    file.seekp(sb.SBitmapInodeStart + 1, std::ios::beg);
    char one = 1;
    file.write(&one, 1);

    file.seekp(sb.SBitmapBlockStart + 1, std::ios::beg);
    file.write(&one, 1);

    // Escribir inodo users.txt en posición 1
    file.seekp(sb.SInodeStart + 1 * sizeof(Inodo), std::ios::beg);
    file.write(reinterpret_cast<char*>(&usersInode), sizeof(Inodo));

    // Escribir bloque de datos en posición 1
    file.seekp(sb.SBlockStart + 1 * sizeof(BloqueArchivo), std::ios::beg);
    file.write(reinterpret_cast<char*>(&usersBlock), sizeof(BloqueArchivo));
}
