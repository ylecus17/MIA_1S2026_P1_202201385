#include "user.h"
#include "login.h"
#include "mkfs.h"
#include "mount.h"
#include "files.h"
#include "../common/log.h"
#include "../structures/filesystem.h"
#include "../structures/disk.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>


bool Cat(const std::vector<std::string>& paths) {
    // 1. Validar sesión
    if (!IsLogged()) {
        common::AddError("[CAT] Error: necesita iniciar sesión");
        return false;
    }

    // 2. Obtener partición montada
    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[CAT] Error: la partición de la sesión no está montada");
        return false;
    }

    // 3. Abrir disco
    std::fstream file(mp->Path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[CAT] Error abriendo disco: " + mp->Path);
        return false;
    }

    // 4. Leer MBR
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

    // 5. Buscar partición por ID
    Partition* part = nullptr;
for (int i = 0; i < 4; i++) {
    std::string pname(mbr.Partitions[i].PartName);
    pname.erase(std::remove(pname.begin(), pname.end(), '\0'), pname.end());

    common::AddInfo("[DEBUG] Revisando partición: " + pname +
                    " status=" + std::string(1, mbr.Partitions[i].PartStatus) +
                    " mountName=" + mp->Name + " mountID=" + mp->ID);

    if (pname == mp->Name) {
        part = &mbr.Partitions[i];
        break;
    }
}

if (!part) {
    common::AddError("[CAT] Error: partición de sesión no encontrada");
    return false;
}

    // 6. Leer superbloque
    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    // 7. Leer inodo raíz
    Inodo rootInode = readInode(file, sb.SInodeStart);

    // 8. Procesar cada archivo
    for (auto& path : paths) {
        std::string filename = path;
        if (!filename.empty() && filename.front() == '/')
            filename.erase(0, 1);

        int inodeIndex = findFileInode(file, rootInode, sb, filename);
        if (inodeIndex == -1) {
            common::AddError("[CAT] Archivo " + path + " no encontrado");
            continue;
        }

        Inodo inode = readInode(file, sb.SInodeStart + inodeIndex * sizeof(Inodo));

        // Verificar permisos de lectura (simplificado: solo root puede leer todo)
        if (CurrentSesion.User != "root" && inode.IUid != 1) {
            common::AddError("[CAT] Sin permiso de lectura para " + path);
            continue;
        }

        std::string content = readFileContent(file, inode, sb);
        content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());
        common::AddTxt(content + "\n");
    }

    return true;
}




// ===================== UTILIDADES =====================

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/')) {
        if (!item.empty()) parts.push_back(item);
    }
    return parts;
}

std::string getFileName(const std::string& path) {
    auto parts = splitPath(path);
    return parts.empty() ? "" : parts.back();
}

std::string getParentPath(const std::string& path) {
    auto parts = splitPath(path);
    if (parts.size() <= 1) return "/";

    std::string parent;
    for (size_t i = 0; i < parts.size() - 1; i++) {
        parent += "/" + parts[i];
    }
    return parent;
}


// ===================== BUSCAR CARPETA =====================

int findFolderInode(std::fstream& file, const SuperBloque& sb, const std::string& path) {
    if (path == "/" || path.empty()) return 0;

    auto parts = splitPath(path);
    int current = 0;

    for (auto& part : parts) {
        Inodo inode;
        file.seekg(sb.SInodeStart + current * sizeof(Inodo));
        file.read(reinterpret_cast<char*>(&inode), sizeof(Inodo));

        int found = -1;

        for (int i = 0; i < 12; i++) {
            if (inode.IBlock[i] == -1) continue;

            BloqueCarpeta block;
            file.seekg(sb.SBlockStart + inode.IBlock[i] * sizeof(BloqueCarpeta));
            file.read(reinterpret_cast<char*>(&block), sizeof(BloqueCarpeta));

            for (int j = 0; j < 4; j++) {
                std::string name(block.BContent[j].BName);
                name.erase(std::remove(name.begin(), name.end(), '\0'), name.end());

                if (name == part) {
                    found = block.BContent[j].BInodo;
                    break;
                }
            }
        }

        if (found == -1) return -1;
        current = found;
    }

    return current;
}



// ===================== AGREGAR HIJO A CARPETA =====================

bool addEntryToFolder(std::fstream& file, const SuperBloque& sb,
                      int parentIndex, const std::string& name, int inodeIndex) {

    Inodo parent;
    file.seekg(sb.SInodeStart + parentIndex * sizeof(Inodo));
    file.read(reinterpret_cast<char*>(&parent), sizeof(Inodo));

    for (int i = 0; i < 12; i++) {
        if (parent.IBlock[i] == -1) continue;

        BloqueCarpeta block;
        file.seekg(sb.SBlockStart + parent.IBlock[i] * sizeof(BloqueCarpeta));
        file.read(reinterpret_cast<char*>(&block), sizeof(BloqueCarpeta));

        for (int j = 0; j < 4; j++) {
            if (block.BContent[j].BInodo == -1) {
                strcpy(block.BContent[j].BName, name.c_str());
                block.BContent[j].BInodo = inodeIndex;

                file.seekp(sb.SBlockStart + parent.IBlock[i] * sizeof(BloqueCarpeta));
                file.write(reinterpret_cast<char*>(&block), sizeof(BloqueCarpeta));
                return true;
            }
        }
    }

    return false;
}



// ===================== CREAR CARPETA =====================

bool createFolder(std::fstream& file, SuperBloque& sb,
                  int parentIndex, const std::string& name) {

    int inodeIndex = sb.SFirstIno;
    int blockIndex = sb.SFirstBlo;

    Inodo inode{};
    inode.IType = '0';
  strcpy(inode.IPerm, "664");

    for (int i = 0; i < 15; i++) inode.IBlock[i] = -1;
    inode.IBlock[0] = blockIndex;

    BloqueCarpeta block{};
    strcpy(block.BContent[0].BName, ".");
    block.BContent[0].BInodo = inodeIndex;

    strcpy(block.BContent[1].BName, "..");
    block.BContent[1].BInodo = parentIndex;

    file.seekp(sb.SInodeStart + inodeIndex * sizeof(Inodo));
    file.write(reinterpret_cast<char*>(&inode), sizeof(Inodo));

    file.seekp(sb.SBlockStart + blockIndex * sizeof(BloqueCarpeta));
    file.write(reinterpret_cast<char*>(&block), sizeof(BloqueCarpeta));

    addEntryToFolder(file, sb, parentIndex, name, inodeIndex);

    sb.SFirstIno++;
    sb.SFirstBlo--;

    return true;
}



// ===================== CREAR ARCHIVO =====================

bool createFile(std::fstream& file, SuperBloque& sb,
                int parentIndex, const std::string& name,
                const std::string& content) {

    int inodeIndex = sb.SFirstIno;
    int blockIndex = sb.SFirstBlo;

    Inodo inode{};
    inode.IType = '1';
    strcpy(inode.IPerm, "664");
    inode.ISize = content.size();

    for (int i = 0; i < 15; i++) inode.IBlock[i] = -1;
    inode.IBlock[0] = blockIndex;

    BloqueArchivo block{};
    memcpy(block.BContent, content.c_str(),
           std::min(content.size(), sizeof(block.BContent)));

    file.seekp(sb.SInodeStart + inodeIndex * sizeof(Inodo));
    file.write(reinterpret_cast<char*>(&inode), sizeof(Inodo));

    file.seekp(sb.SBlockStart + blockIndex * sizeof(BloqueArchivo));
    file.write(reinterpret_cast<char*>(&block), sizeof(BloqueArchivo));

    addEntryToFolder(file, sb, parentIndex, name, inodeIndex);

    sb.SFirstIno++;
    sb.SFirstBlo--;

    return true;
}



// ===================== CREAR PADRES =====================

bool createParentFolders(std::fstream& file, SuperBloque& sb, const std::string& path) {
    auto parts = splitPath(path);
    int current = 0;

    std::string temp = "";

    for (auto& part : parts) {
        temp += "/" + part;

        int found = findFolderInode(file, sb, temp);
        if (found == -1) {
            createFolder(file, sb, current, part);
            found = findFolderInode(file, sb, temp);
        }

        current = found;
    }

    return true;
}



// ===================== MKDIR =====================

bool Mkdir(const std::string& path, bool pFlag) {

    if (!IsLogged()) {
        common::AddError("[MKDIR] Necesita login");
        return false;
    }

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[MKDIR] Partición no montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);

    SuperBloque sb;
    file.seekg(mp->Start);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    std::string parentPath = getParentPath(path);
    int parent = findFolderInode(file, sb, parentPath);

    if (parent == -1) {
    if (!pFlag) {
        common::AddError("[MKDIR] Carpeta padre no existe");
        return false;
    }

    createParentFolders(file, sb, parentPath);

    // 🔥 GUARDAR SB
    file.seekp(mp->Start);
    file.write(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    parent = findFolderInode(file, sb, parentPath);
}

    createFolder(file, sb, parent, getFileName(path));
file.seekp(mp->Start);
file.write(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    common::AddSuccess("[MKDIR] Carpeta creada");
    return true;
}



// ===================== MKFILE =====================

bool Mkfile(const std::string& path, bool rFlag, int size, const std::string& cont) {

    if (!IsLogged()) {
        common::AddError("[MKFILE] Necesita login");
        return false;
    }

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[MKFILE] Partición no montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);

    SuperBloque sb;
    file.seekg(mp->Start);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    std::string parentPath = getParentPath(path);
    int parent = findFolderInode(file, sb, parentPath);

if (parent == -1) {
    if (!rFlag) {
        common::AddError("[MKFILE] Carpeta padre no existe");
        return false;
    }

    createParentFolders(file, sb, parentPath);

    // 🔥 GUARDAR SB DESPUÉS DE CREAR PADRES
    file.seekp(mp->Start);
    file.write(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    parent = findFolderInode(file, sb, parentPath);
}

    std::string content;

    if (!cont.empty()) {
        std::ifstream ext(cont);
        if (!ext.is_open()) {
            common::AddError("[MKFILE] Archivo externo no existe");
            return false;
        }
        std::stringstream buffer;
        buffer << ext.rdbuf();
        content = buffer.str();
    } else if (size > 0) {
        for (int i = 0; i < size; i++) {
            content += char('0' + (i % 10));
        }
    }

    createFile(file, sb, parent, getFileName(path), content);
file.seekp(mp->Start);
file.write(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    common::AddSuccess("[MKFILE] Archivo creado");
    return true;
}