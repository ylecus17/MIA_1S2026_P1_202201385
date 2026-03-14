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
        std::string pid(mbr.Partitions[i].PartID);
        pid.erase(std::remove(pid.begin(), pid.end(), '\0'), pid.end());
        if (pid == CurrentSesion.ID) {
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
