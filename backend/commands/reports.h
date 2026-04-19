#ifndef REPORTS_H
#define REPORTS_H


#include <fstream>
#include <sstream>
#include <string>
#include "../structures/filesystem.h"   // SuperBloque, Inodo, Bloques
#include "../structures/disk.h" 
#include "mount.h" // MountedPartition
#include "login.h"
bool GenerateReport(const std::string& id,
                    const std::string& outPath,
                    const std::string& name,
                    const std::string& pathFileLS);
bool buildMBRDot(std::ifstream& f, const MBR& mbr, const std::string& diskPath, std::string& dotOut);
bool buildDiskDot(std::ifstream& f, const MBR& mbr, const std::string& diskPath, std::string& dotOut);          
bool buildSBDot(const SuperBloque& sb, std::string& dotOut);
#endif // REPORTS_H