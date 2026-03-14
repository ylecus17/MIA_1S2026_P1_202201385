#ifndef FILES_H
#define FILES_H

#include <string>
#include <vector>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition

bool Cat(const std::vector<std::string>& paths);

#endif // FILESYSTEM_HE