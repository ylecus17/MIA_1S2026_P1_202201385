#ifndef LOGIN_H
#define LOGIN_H

#include <string>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition

bool Login(const std::string& user, const std::string& pass, const std::string& id);
MountedPartition* findMountById(const std::string& id);
bool Logout();
void SetSesion(const std::string& user, const std::string& id);
void ClearSesion();
bool IsLogged();
Inodo readInode(std::ifstream& file, int offset);
int findFileInode(std::ifstream& file, const Inodo& dirInode, const SuperBloque& sb, const std::string& filename);
std::string readFileContent(std::ifstream& file, const Inodo& inode, const SuperBloque& sb);
std:: string getdiskpath(const std::string& id);
#endif