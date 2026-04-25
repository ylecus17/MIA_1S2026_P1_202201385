#include "../common/log.h"
#include "mount.h"
#include "../structures/filesystem.h"
#include "../structures/disk.h"
#include "login.h"
#include "mkfs.h" 
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>

// ---------------- VARIABLE GLOBAL DE SESIÓN ----------------
Sesion CurrentSesion;
// ---------------- AUXILIARES ----------------

// Leer un inodo desde disco
Inodo readInode(std::fstream &file, int offset)
{
    Inodo inode;
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char *>(&inode), sizeof(inode));
    return inode;
}

// Buscar el inodo de un archivo dentro de un directorio
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

// Leer contenido de archivo (users.txt) usando ISize
std::string readFileContent(std::fstream &file, const Inodo &inode, const SuperBloque &sb)
{
    std::string content;
    int remaining = inode.ISize;

    for (int i = 0; i < 15 && remaining > 0; i++)
    {
        if (inode.IBlock[i] == -1)
            continue;

        BloqueArchivo block{};
        int offset = sb.SBlockStart + inode.IBlock[i] * sizeof(block);

        file.seekg(offset, std::ios::beg);
        file.read(reinterpret_cast<char*>(&block), sizeof(BloqueArchivo));

        int toCopy = std::min(remaining, (int)sizeof(block.BContent));
        content.append(block.BContent, toCopy);

        remaining -= toCopy;
    }

    return content;
}

// 
// ---------------- LOGIN ----------------
// Recibe usuario, contraseña e ID de partición
bool Login(const std::string &user, const std::string &pass, const std::string &id, std::string &message)
{
    // 1. Verificar si ya hay sesión activa
    if (CurrentSesion.Status)
    {
        message = "Ya hay una sesión activa: " + CurrentSesion.User;
        common::AddInfo("[LOGIN] " + message);
        return false;
    }

    // 2. Buscar partición montada en RAM
    MountedPartition *mp = getMountById(id);
    if (!mp)
    {
        message = "Partición " + id + " no está montada";
        common::AddError("[LOGIN] " + message);
        return false;
    }

    // 3. Abrir archivo del disco
    std::fstream file(mp->Path, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        message = "Error abriendo disco";
        common::AddError("[LOGIN] " + mp->Path);
        return false;
    }

    // 4. Leer MBR
    MBR mbr;
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(&mbr), sizeof(mbr));
    // 5. Buscar la partición dentro del MBR
    Partition *part = nullptr;
    for (int i = 0; i < 4; i++)
    {
        std::string pname(mbr.Partitions[i].PartName);
        pname.erase(std::find(pname.begin(), pname.end(), '\0'), pname.end());

        common::AddInfo("[DEBUG] Revisando partición: " + pname);

        if (pname == mp->Name)
        {
            part = &mbr.Partitions[i];
            break;
        }
    }

    if (part == nullptr)
    {
        message = "Partición no encontrada en disco";
        common::AddError("[LOGIN] " + message);
        return false;
    }

    // 6. Leer SuperBloque
    SuperBloque sb;
    file.seekg(part->PartStart, std::ios::beg);
    file.read(reinterpret_cast<char *>(&sb), sizeof(sb));
    // 7. Leer inodo raíz
    Inodo rootInode = readInode(file, sb.SInodeStart);

    // 8. Buscar archivo users.txt en la raíz
    int userInodeIndex = findFileInode(file, rootInode, sb, "users.txt");
    if (userInodeIndex == -1)
    {
        message = "Archivo users.txt no encontrado";
        common::AddError("[LOGIN] " + message);
        return false;
    }

    // 9. Leer inodo de users.txt
    Inodo userInode = readInode(file, sb.SInodeStart + userInodeIndex * sizeof(Inodo));

    // 10. Leer contenido del archivo usando ISize para evitar basura
    std::string content = readFileContent(file, userInode, sb);

    common::AddInfo("[DEBUG] users.txt:\n" + content);
    // 11. Validar usuario y contraseña
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

        // formato esperado: id, tipo, grupo, user, pass
        if (parts.size() == 5 && parts[1] == "U")
        {
            if (parts[3] == user && parts[4] == pass)
            {
                SetSesion(user, id);
                message = "Login correcto";
                return true;
            }
        }
    }

    // 12. Si no se encontró el usuario
    message = "Usuario o contraseña incorrectos";
    common::AddError("[LOGIN] " + message);
    return false;
}
// ---------------- LOGOUT ----------------
bool Logout(std::string &message)
{
    if (!CurrentSesion.Status)
    {
        message = "No hay sesión activa";
        common::AddError("[LOGOUT] " + message);
        return false;
    }

    message = "Sesión cerrada: " + CurrentSesion.User;
    common::AddInfo("[LOGOUT] " + message);

    ClearSesion();
    return true;
}

// ---------------- SESIÓN ----------------
void SetSesion(const std::string &user, const std::string &id)
{
    CurrentSesion.User = user;
    CurrentSesion.ID = id;
    CurrentSesion.Status = true;

    common::AddInfo("[SESION] Usuario=" + user + " ID=" + id);
}

void ClearSesion()
{
    CurrentSesion = Sesion{};
}

bool IsLogged()
{
    return CurrentSesion.Status;
}
