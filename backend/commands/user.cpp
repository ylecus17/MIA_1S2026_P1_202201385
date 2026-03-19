#include "../common/log.h"
#include "../structures/filesystem.h"
#include "../structures/disk.h"
#include "login.h"
#include "mkfs.h"
#include "mount.h"
#include "disk.h"
#include "files.h"
#include "user.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <filesystem>

// ---------------- MKGRP ----------------
bool Mkgrp(const std::string& name) {
    // 1. Validar sesión
    if (!IsLogged()) {
        common::AddError("[MKGRP] Error: necesita iniciar sesión");
        return false;
    }
    if (CurrentSesion.User != "root") {
        common::AddError("[MKGRP] Error: solo el usuario root puede crear grupos");
        return false;
    }

    // 2. Obtener partición de la sesión
    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[MKGRP] Error: la partición de la sesión no está montada");
        return false;
    }

    // 3. Abrir disco
    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[MKGRP] Error abriendo disco: " + mp->Path);
        return false;
    }

    // 4. Leer MBR y superbloque
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

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
    common::AddError("[MKGRP] Error: partición de sesión no encontrada");
    return false;
}


    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    // 5. Leer contenido de users.txt
    Inodo rootInode = readInode(file, sb.SInodeStart);
    int usersInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (usersInodeIndex == -1) {
        common::AddError("[MKGRP] Error: archivo users.txt no encontrado");
        return false;
    }
    Inodo usersInode = readInode(file, sb.SInodeStart + usersInodeIndex * sizeof(Inodo));
    std::string content = readFileContent(file, usersInode, sb);

    // 6. Verificar si el grupo ya existe
    std::istringstream iss(content);
    std::string line;
    int lastID = 0;
    while (std::getline(iss, line)) {
        std::stringstream ss(line);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);

        if (parts.size() >= 3 && parts[1] == "G") {
            if (parts[2] == name) {
                common::AddError("[MKGRP] Error: el grupo '" + name + "' ya existe");
                return false;
            }
            int id = std::stoi(parts[0]);
            if (id > lastID) lastID = id;
        }
    }

    // 7. Nueva línea
    std::string newLine = std::to_string(lastID + 1) + ",G," + name + "\n";

    // 8. Añadir al archivo users.txt
    if (!appendToFile(file, usersInode, sb, newLine, usersInodeIndex)) {
        common::AddError("[MKGRP] Error escribiendo users.txt");
        return false;
    }

    common::AddSuccess("[MKGRP] Grupo '" + name + "' creado correctamente");
    return true;
}

// ---------------- RMGRP ----------------
bool Rmgrp(const std::string& name) {
    if (!IsLogged()) {
        common::AddError("[RMGRP] Error: necesita iniciar sesión");
        return false;
    }
    if (CurrentSesion.User != "root") {
        common::AddError("[RMGRP] Error: solo el usuario root puede eliminar grupos");
        return false;
    }

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[RMGRP] Error: la partición de la sesión no está montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[RMGRP] Error abriendo disco: " + mp->Path);
        return false;
    }

    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

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
    common::AddError("[RMGRP] Error: partición de sesión no encontrada");
    return false;
}


    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    Inodo rootInode = readInode(file, sb.SInodeStart);
    int usersInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (usersInodeIndex == -1) {
        common::AddError("[RMGRP] Error: archivo users.txt no encontrado");
        return false;
    }
    Inodo usersInode = readInode(file, sb.SInodeStart + usersInodeIndex * sizeof(Inodo));
    std::string content = readFileContent(file, usersInode, sb);

    std::istringstream iss(content);
    std::string line;
    std::vector<std::string> lines;
    bool found = false;
    while (std::getline(iss, line)) {
        std::stringstream ss(line);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);

        if (parts.size() >= 3 && parts[1] == "G" && parts[2] == name) {
            lines.push_back("0,G," + name);
            found = true;
        } else {
            lines.push_back(line);
        }
    }

    if (!found) {
        common::AddError("[RMGRP] Error: grupo '" + name + "' no existe");
        return false;
    }

    std::string newContent;
    for (auto& l : lines) newContent += l + "\n";

    if (!overwriteFile(file, usersInode, sb, newContent, usersInodeIndex)) {
        common::AddError("[RMGRP] Error escribiendo users.txt");
        return false;
    }

    common::AddSuccess("[RMGRP] Grupo '" + name + "' eliminado correctamente");
    return true;
}

// ---------------- appendToFile ----------------
bool appendToFile(std::fstream& file, Inodo& inode, const SuperBloque& sb,
                  const std::string& data, int inodeIndex) {
    BloqueArchivo block;
    int blockSize = sizeof(BloqueArchivo);

    file.seekg(sb.SBlockStart + inode.IBlock[0] * blockSize, std::ios::beg);
    file.read(reinterpret_cast<char*>(&block), sizeof(BloqueArchivo));

    std::string old(block.BContent, sizeof(block.BContent));
    old.erase(std::remove(old.begin(), old.end(), '\0'), old.end());
    std::string newContent = old + data;
    std::memset(block.BContent, 0, sizeof(block.BContent));
    std::memcpy(block.BContent, newContent.c_str(), std::min(newContent.size(), sizeof(block.BContent)));

    file.seekp(sb.SBlockStart + inode.IBlock[0] * blockSize, std::ios::beg);
    file.write(reinterpret_cast<char*>(&block), sizeof(BloqueArchivo));

    inode.ISize = newContent.size();
    file.seekp(sb.SInodeStart + inodeIndex * sizeof(Inodo), std::ios::beg);
    file.write(reinterpret_cast<char*>(&inode), sizeof(Inodo));

    return true;
}
bool overwriteFile(std::fstream& file, Inodo& inode,
                   const SuperBloque& sb, const std::string& data, int inodeIndex) {
    BloqueArchivo block;
    int blockSize = sizeof(BloqueArchivo);

    // escribir nuevo contenido
    std::memset(block.BContent, 0, sizeof(block.BContent));
    std::memcpy(block.BContent, data.c_str(), std::min(data.size(), sizeof(block.BContent)));

    // guardar en disco
    file.seekp(sb.SBlockStart + inode.IBlock[0] * blockSize, std::ios::beg);
    file.write(reinterpret_cast<char*>(&block), sizeof(BloqueArchivo));

    // actualizar inodo
    inode.ISize = data.size();
    file.seekp(sb.SInodeStart + inodeIndex * sizeof(Inodo), std::ios::beg);
    file.write(reinterpret_cast<char*>(&inode), sizeof(Inodo));

    return true;
}

// ---------------- MKUSR ----------------
bool Mkusr(const std::string& user, const std::string& pass, const std::string& grp) {
    if (!IsLogged()) {
        common::AddError("[MKUSR] Error: necesita iniciar sesión");
        return false;
    }
    if (CurrentSesion.User != "root") {
        common::AddError("[MKUSR] Error: solo el usuario root puede crear usuarios");
        return false;
    }

    if (user.empty() || pass.empty() || grp.empty()) {
        common::AddError("[MKUSR] Error: parámetros obligatorios faltantes (user, pass, grp)");
        return false;
    }
    if (user.size() > 10 || pass.size() > 10 || grp.size() > 10) {
        common::AddError("[MKUSR] Error: parámetros exceden 10 caracteres");
        return false;
    }

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[MKUSR] Error: la partición de la sesión no está montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[MKUSR] Error abriendo disco: " + mp->Path);
        return false;
    }

    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

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
    common::AddError("[MKUSR] Error: partición de sesión no encontrada");
    return false;
}

    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    Inodo rootInode = readInode(file, sb.SInodeStart);
    int usersInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (usersInodeIndex == -1) {
        common::AddError("[MKUSR] Error: archivo users.txt no encontrado");
        return false;
    }
    Inodo usersInode = readInode(file, sb.SInodeStart + usersInodeIndex * sizeof(Inodo));
    std::string content = readFileContent(file, usersInode, sb);
    content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());

    std::istringstream iss(content);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(iss, line)) lines.push_back(line);

    // verificar grupo
    bool grpExists = false;
    for (auto& l : lines) {
        std::stringstream ss(l);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);
        if (parts.size() >= 3 && parts[1] == "G" && parts[0] != "0" && parts[2] == grp) {
            grpExists = true;
            break;
        }
    }
    if (!grpExists) {
        common::AddError("[MKUSR] Error: el grupo '" + grp + "' no existe o fue eliminado");
        return false;
    }

    // verificar usuario
    for (auto& l : lines) {
        std::stringstream ss(l);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);
        if (parts.size() == 5 && parts[1] == "U" && parts[0] != "0" && parts[3] == user) {
            common::AddError("[MKUSR] Error: el usuario '" + user + "' ya existe");
            return false;
        }
    }

    // calcular nuevo ID
    int lastID = 0;
    for (auto& l : lines) {
        std::stringstream ss(l);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);
        if (parts.size() >= 2 && parts[1] == "U") {
            int id = std::stoi(parts[0]);
            if (id > lastID) lastID = id;
        }
    }

    std::string newLine = std::to_string(lastID + 1) + ",U," + grp + "," + user + "," + pass + "\n";
    if (!appendToFile(file, usersInode, sb, newLine, usersInodeIndex)) {
        common::AddError("[MKUSR] Error escribiendo users.txt");
        return false;
    }

    common::AddSuccess("[MKUSR] Usuario '" + user + "' creado correctamente en grupo '" + grp + "'");
    return true;
}
bool Rmusr(const std::string& user) {
    // 1. Validar sesión
    if (!IsLogged()) {
        common::AddError("[RMUSR] Error: necesita iniciar sesión");
        return false;
    }
    if (CurrentSesion.User != "root") {
        common::AddError("[RMUSR] Error: solo el usuario root puede eliminar usuarios");
        return false;
    }

    // 2. Obtener partición montada
    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[RMUSR] Error: la partición de la sesión no está montada");
        return false;
    }

    // 3. Abrir disco
    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[RMUSR] Error abriendo disco: " + mp->Path);
        return false;
    }

    // 4. Leer MBR
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

    // 5. Buscar partición
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
    common::AddError("[RMUSR] Error: partición de sesión no encontrada");
    return false;
}

    // 6. Leer superbloque
    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    // 7. Localizar users.txt
    Inodo rootInode = readInode(file, sb.SInodeStart);
    int usersInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (usersInodeIndex == -1) {
        common::AddError("[RMUSR] Error: archivo users.txt no encontrado");
        return false;
    }
    Inodo usersInode = readInode(file, sb.SInodeStart + usersInodeIndex * sizeof(Inodo));
    std::string content = readFileContent(file, usersInode, sb);
    content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());

    // 8. Buscar usuario
    std::istringstream iss(content);
    std::string line;
    std::vector<std::string> lines;
    bool found = false;

    while (std::getline(iss, line)) {
        std::stringstream ss(line);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);

        if (parts.size() == 5 && parts[1] == "U") {
            if (parts[3] == user) {
                // marcar como eliminado (ID=0)
                line = "0,U," + parts[2] + "," + parts[3] + "," + parts[4];
                found = true;
            }
        }
        lines.push_back(line);
    }

    if (!found) {
        common::AddError("[RMUSR] Error: usuario '" + user + "' no existe");
        return false;
    }

    // 9. Reescribir contenido
    std::string newContent;
    for (auto& l : lines) newContent += l + "\n";

    if (!overwriteFile(file, usersInode, sb, newContent, usersInodeIndex)) {
        common::AddError("[RMUSR] Error escribiendo users.txt");
        return false;
    }

    common::AddSuccess("[RMUSR] Usuario '" + user + "' eliminado correctamente");
    return true;
}
bool Chgrp(const std::string& user, const std::string& newGrp)
{
    if (!CurrentSesion.Status) {
        common::AddError("[CHGRP] Error: necesita iniciar sesión");
        return false;
    }
    if (CurrentSesion.User != "root") {
        common::AddError("[CHGRP] Error: solo el usuario root puede cambiar grupos");
        return false;
    }

    MountedPartition* mp = getMountById(CurrentSesion.ID);
    if (!mp) {
        common::AddError("[CHGRP] Error: la partición de la sesión no está montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        common::AddError("[CHGRP] Error abriendo disco: " + mp->Path);
        return false;
    }

    // Leer MBR
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

    // Buscar partición por nombre
    Partition* part = nullptr;
    for (int i = 0; i < 4; i++) {
        std::string pname(mbr.Partitions[i].PartName);
        pname.erase(std::remove(pname.begin(), pname.end(), '\0'), pname.end());
        if (pname == mp->Name) {
            part = &mbr.Partitions[i];
            break;
        }
    }
    if (!part) {
        common::AddError("[CHGRP] Error: partición de sesión no encontrada");
        return false;
    }

    // Leer superbloque
    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    // Leer users.txt
    Inodo rootInode = readInode(file, sb.SInodeStart);
    int usersInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (usersInodeIndex == -1) {
        common::AddError("[CHGRP] Error: archivo users.txt no encontrado");
        return false;
    }
    Inodo usersInode = readInode(file, sb.SInodeStart + usersInodeIndex * sizeof(Inodo));
    std::string content = readFileContent(file, usersInode, sb);
    content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());

    // Separar líneas
    std::istringstream iss(content);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    // Validar que el grupo exista
    bool groupExists = false;
    for (auto& l : lines) {
        std::stringstream ss(l);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);

        if (parts.size() >= 3 && parts[1] == "G" && parts[0] != "0") {
            if (parts[2] == newGrp) {
                groupExists = true;
                break;
            }
        }
    }
    if (!groupExists) {
        common::AddError("[CHGRP] Error: grupo '" + newGrp + "' no existe o está eliminado");
        return false;
    }

    // Buscar usuario y cambiar grupo
    bool found = false;
    for (size_t i = 0; i < lines.size(); i++) {
        std::stringstream ss(lines[i]);
        std::vector<std::string> parts;
        std::string token;
        while (std::getline(ss, token, ',')) parts.push_back(token);

        if (parts.size() == 5 && parts[1] == "U" && parts[0] != "0") {
            if (parts[3] == user) {
                // Reemplazar línea con nuevo grupo
                lines[i] = parts[0] + ",U," + newGrp + "," + parts[3] + "," + parts[4];
                found = true;
                break;
            }
        }
    }
    if (!found) {
        common::AddError("[CHGRP] Error: usuario '" + user + "' no existe o fue eliminado");
        return false;
    }

    // Reescribir archivo
    std::string newContent;
    for (auto& l : lines) {
        newContent += l + "\n";
    }
    if (!overwriteFile(file, usersInode, sb, newContent, usersInodeIndex)) {
        common::AddError("[CHGRP] Error escribiendo users.txt");
        return false;
    }

    common::AddSuccess("[CHGRP] Usuario '" + user + "' cambiado al grupo '" + newGrp + "'");
    return true;
}
