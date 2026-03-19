#ifndef LOGIN_H
#define LOGIN_H

#include <string>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition
#include <fstream>

struct Sesion {
    std::string User;
    std::string ID;
    bool Status = false;
};

// Declaración global (extern) → se define en login.cpp
extern Sesion CurrentSesion;

bool Login(const std::string& user, const std::string& pass, const std::string& id);

bool Logout();
void SetSesion(const std::string& user, const std::string& id);
void ClearSesion();
bool IsLogged();
Inodo readInode(std::fstream& file, int offset);
int findFileInode(std::fstream& file, const Inodo& dirInode, const SuperBloque& sb, const std::string& filename);
std::string readFileContent(std::fstream& file, const Inodo& inode, const SuperBloque& sb);

#endif