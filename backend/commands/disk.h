#ifndef DISK_H
#define DISK_H

#include <string>

bool MkDisk(int size, std::string unit, std::string path, std::string fit);
bool RmDisk(const std::string& path);
bool Fdisk(int size, std::string unit, std::string path,
           std::string name, std::string ptype, std::string fit);
#endif
