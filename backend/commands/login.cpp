// commands/login.cpp
#include "../common/log.h"
#include "mount.h"
#include "../structures/filesystem.h"
#include "../structures/disk.h"
#include "login.h"
#include "mkfs.h" // aquí está la declaración de getMountById
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>

Sesion CurrentSesion;

// ---------------- LOGIN ----------------
bool Login(const std::string &user, const std::string &pass, const std::string &id)
{
    if (CurrentSesion.Status)
    {
        common::AddInfo("[LOGIN] Ya hay una sesión activa: " + CurrentSesion.User);
        return false;
    }

    // Buscar partición montada en RAM
    MountedPartition *mp = getMountById(id);
    if (!mp)
    {
        common::AddInfo("[LOGIN] Partición " + id + " no está montada");
        return false;
    }

    std::fstream file(mp->Path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        common::AddError("[LOGIN] Error abriendo disco: " + mp->Path);
        return false;
    }

    // Leer MBR
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(&mbr), sizeof(mbr));

    // Buscar partición por ID
    Partition *part = nullptr;

    for (int i = 0; i < 4; i++)
{
    std::string pname(mbr.Partitions[i].PartName);
    auto pos = std::find(pname.begin(), pname.end(), '\0');
    if (pos != pname.end())
        pname.erase(pos, pname.end());

    // Debug para ver qué hay en el MBR
    common::AddInfo("[DEBUG] Revisando partición: " + pname +
                    " status=" + std::string(1, mbr.Partitions[i].PartStatus) +
                    " mountName=" + mp->Name + " mountID=" + mp->ID);

    // Comparar con el nombre de la partición montada
    if (pname == mp->Name) {
        part = &mbr.Partitions[i];
        break;
    }
}


    if (part == nullptr)
    {
        common::AddError("[LOGIN] Partición " + mp->ID + " no encontrada o no activa");
        return false;
    }

    // Leer superbloque
    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char *>(&sb), sizeof(sb));

    // Leer inodo raíz
    Inodo rootInode = readInode(file, sb.SInodeStart);

    // Buscar inodo de users.txt
    int userInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (userInodeIndex == -1)
    {
        common::AddError("[LOGIN] Archivo users.txt no encontrado");
        return false;
    }

    // Leer inodo de users.txt
    Inodo userInode = readInode(file, sb.SInodeStart + userInodeIndex * sizeof(Inodo));

    // Leer contenido del archivo
    std::string content = readFileContent(file, userInode, sb);

    // Validar usuario y contraseña
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, ','))
        {
            parts.push_back(token);
        }

        if (parts.size() == 5 && parts[1] == "U")
        {
            if (parts[3] == user && parts[4] == pass)
            {
                SetSesion(user, id);
                return true;
            }
        }
    }

    // Si no encontró coincidencia, mostrar qué usuario/contraseña se intentaron
    common::AddError("[LOGIN] Usuario '" + user + "' con contraseña '" + pass + "' incorrectos en partición ID=" + id);
    return false;
}

// ---------------- LOGOUT ----------------
bool Logout()
{
    if (!CurrentSesion.Status)
    {
        common::AddError("[LOGOUT] No hay ninguna sesión activa");
        return false;
    }

    common::AddInfo("[LOGOUT] Cerrando sesión de usuario " + CurrentSesion.User + " en partición " + CurrentSesion.ID);
    ClearSesion();
    return true;
}

// ---------------- SESIÓN ----------------
void SetSesion(const std::string &user, const std::string &id)
{
    CurrentSesion.User = user;
    CurrentSesion.ID = id;
    CurrentSesion.Status = true;
    common::AddInfo("[SESION] Sesión iniciada: Usuario=" + user + ", ID=" + id);
}

void ClearSesion()
{
    CurrentSesion = Sesion{};
}

bool IsLogged()
{
    return CurrentSesion.Status;
}

// ---------------- AUXILIARES ----------------
Inodo readInode(std::fstream &file, int offset)
{
    Inodo inode;
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char *>(&inode), sizeof(inode));
    return inode;
}

int findFileInode(std::fstream &file, const Inodo &dirInode, const SuperBloque &sb, const std::string &filename)
{
    for (int i = 0; i < 15; i++)
    {
        if (dirInode.IBlock[i] == -1)
            continue;
        BloqueCarpeta block;
        int offset = sb.SBlockStart + dirInode.IBlock[i] * sizeof(block);
        file.seekg(offset, std::ios::beg);
        file.read(reinterpret_cast<char *>(&block), sizeof(block));
        for (auto &entry : block.BContent)
        {
            std::string name(entry.BName);
            name.erase(std::remove(name.begin(), name.end(), '\0'), name.end());
            if (name == filename)
            {
                return entry.BInodo;
            }
        }
    }
    return -1;
}

std::string readFileContent(std::fstream &file, const Inodo &inode, const SuperBloque &sb)
{
    std::string content;
    for (int i = 0; i < 15; i++)
    {
        if (inode.IBlock[i] == -1)
            continue;
        BloqueArchivo block;
        int offset = sb.SBlockStart + inode.IBlock[i] * sizeof(block);
        file.seekg(offset, std::ios::beg);
        file.read(reinterpret_cast<char *>(&block), sizeof(block));
        content += std::string(block.BContent, sizeof(block.BContent));
    }
    content.erase(std::remove(content.begin(), content.end(), '\0'), content.end());
    common::AddInfo("[DEBUG] Contenido de users.txt:\n" + content);
    return content;
}
// Función local a login.cpp para buscar partición montada por ID
