#ifndef MKFS_H
#define MKFS_H

#include <string>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition

bool Mkfs(const std::string& id, const std::string& fstype);
MountedPartition* getMountById(const std::string& id);
SuperBloque crearSuperbloque(int32_t n, int32_t inodeSize, int32_t blockSize, int32_t superSize, const Partition& part);
void inicializarBitmaps(std::fstream& file, const SuperBloque& sb, int32_t n);
void crearRaiz(std::fstream& file, const SuperBloque& sb);
void crearUsersTxt(std::fstream& file, const SuperBloque& sb);
#endif // MKFS_H
