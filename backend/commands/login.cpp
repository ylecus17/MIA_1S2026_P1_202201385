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
 MountedPartition* findMountById(const std::string& id);



    struct Sesion
    {
        std::string User;
        std::string ID;
        bool Status = false;
    };

    static Sesion CurrentSesion;

    // ---------------- LOGIN ----------------
    bool Login(const std::string &user, const std::string &pass, const std::string &id)
    {
        if (CurrentSesion.Status)
        {
            common::AddInfo("[LOGIN] Ya hay una sesión activa: " + CurrentSesion.User);
            return false;
        }

        // Buscar partición montada en RAM
        MountedPartition *mp = findMountById(id);
        if (mp == nullptr)
        {
            common::AddError("[LOGIN] Partición " + id + " no está montada");
            return false;
        }

        std::ifstream file(mp->Path, std::ios::binary);
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
        int index = -1;
        for (int i = 0; i < 4; i++)
        {
            std::string actualID(mbr.Partitions[i].PartID);
            auto pos = std::find(actualID.begin(), actualID.end(), '\0');
            if (pos != actualID.end())
                actualID.erase(pos, actualID.end());

            if (actualID == id && mbr.Partitions[i].PartStatus == '1')
            {
                index = i;
                break;
            }
        }

        if (index == -1)
        {
            common::AddError("[LOGIN] Partición con ID " + id + " no encontrada o no está montada");
            return false;
        }

        // Leer superbloque
        SuperBloque sb;
        file.seekg(mbr.Partitions[index].PartStart, std::ios::beg);
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

        common::AddError("[LOGIN] Usuario o contraseña incorrectos");
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
    Inodo readInode(std::ifstream &file, int offset)
    {
        Inodo inode;
        file.seekg(offset, std::ios::beg);
        file.read(reinterpret_cast<char *>(&inode), sizeof(inode));
        return inode;
    }

    int findFileInode(std::ifstream &file, const Inodo &dirInode, const SuperBloque &sb, const std::string &filename)
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

    std::string readFileContent(std::ifstream &file, const Inodo &inode, const SuperBloque &sb)
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
        return content;
    }
    // Función local a login.cpp para buscar partición montada por ID
    MountedPartition *findMountById(const std::string &id)
    {
        extern std::vector<MountedPartition> mountedPartitions; // asegúrate que esté declarada globalmente
        for (auto &mp : mountedPartitions)
        {
            if (mp.ID == id)
            {
                return &mp;
            }
        }
        return nullptr;
    }


