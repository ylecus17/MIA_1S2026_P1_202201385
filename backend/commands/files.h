#ifndef FILES_H
#define FILES_H

#include <string>
#include <vector>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition

bool Cat(const std::vector<std::string>& paths);
bool Mkdir(const std::string& path, bool pFlag);
bool Mkfile(const std::string& path, bool rFlag, int size, const std    ::string& cont);
#endif // FILESYSTEM_HE