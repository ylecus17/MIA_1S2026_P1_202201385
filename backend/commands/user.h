#ifndef USER_H
#define USER_H

#include <fstream>
#include <sstream>
#include <string>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition


bool Mkgrp(const std::string& name);
bool Rmgrp(const std::string& name);
bool Mkusr(const std::string& user, const std::string& pass, const std::string& grp);
bool Rmusr(const std::string& user);
bool Chgrp(const std::string& user, const std::string& grp);
bool appendToFile(std::fstream& file, Inodo& inode, const SuperBloque& sb, const std::string& content, int usersInodeIndex);
bool overwriteFile(std::fstream& file, Inodo& inode, const SuperBloque& sb, const std::string& content, int usersInodeIndex);


#endif