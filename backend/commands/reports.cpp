#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include "../structures/filesystem.h"
#include "../structures/disk.h"
#include "../common/log.h"
#include "mount.h"
#include "reports.h"

// MountedPartition y mountedPartitions deben estar definidos en tu proyecto


// Función auxiliar para comparar case-insensitive
std::string toLower(const std::string &s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });
    return out;
}


bool GenerateReport(const std::string& id,
                    const std::string& outPath,
                    const std::string& name,
                    const std::string& pathFileLS) {
    // 1) Buscar partición montada por ID
    MountedPartition* mp = nullptr;
    for (auto& part : mountedPartitions) {
        if (toLower(part.ID) == toLower(id)) {
            mp = &part;
            break;
        }
    }
    if (!mp) {
        common::AddError("[REP] Error: particion con id " + id + " no esta montada");
        return false;
    }

    // 2) Asegurar carpeta destino
    try {
        std::filesystem::create_directories(std::filesystem::path(outPath).parent_path());
        
    } catch (std::exception& e) {
        common::AddError("[REP] Error creando carpeta destino: " + std::string(e.what()));
        return false;
    }

    // 3) Abrir disco y leer MBR
    std::ifstream f(mp->Path, std::ios::binary);
    if (!f.is_open()) {
        common::AddError("[REP] Error abriendo disco: " + mp->Path);
        return false;
    }

    MBR mbr{};
    f.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    if (!f) {
        common::AddError("[REP] Error leyendo MBR");
        return false;
    }
    common::AddInfo("[REP] MBR leido desde " + mp->Path);

   // 4) Generar contenido DOT según tipo
std::string dot;

if (name == "mbr") {
    if (!buildMBRDot(f, mbr, mp->Path, dot)) {
        common::AddError("[REP] Error generando DOT MBR");
        return false;
    }
} else if (name == "disk") {
    if (!buildDiskDot(f, mbr, mp->Path, dot)) {
        common::AddError("[REP] Error generando DOT DISK");
        return false;
    }
else if (name == "sb") {
    SuperBloque sb{};
    f.seekg(mp->Start, std::ios::beg);
    f.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (!f) {
        common::AddError("[REP] Error leyendo SuperBloque");
        return false;
    }
    common::AddInfo("[REP] SuperBloque leido desde " + mp->Path);

    if (!buildSBDot(sb, dot)) {
        common::AddError("[REP] Error generando DOT SuperBloque");
        return false;
    }
}

}



    // 5) Guardar archivo DOT
    std::string dotPath = std::filesystem::path(outPath).replace_extension(".dot").string();
    std::ofstream dotFile(dotPath);
    if (!dotFile.is_open()) {
        common::AddError("[REP] Error guardando archivo DOT");
        return false;
    }
    dotFile << dot;
    dotFile.close();
    common::AddInfo("[REP] DOT guardado en " + dotPath);

    // 6) Ejecutar Graphviz para PNG
    std::string pngPath = std::filesystem::path(outPath).replace_extension(".png").string();
    std::string cmd = "dot -Tpng " + dotPath + " -o " + pngPath;
    int res = std::system(cmd.c_str());
    if (res != 0) {
        common::AddError("[REP] Error ejecutando Graphviz (dot)");
        return false;
    }

   
    return true;
}
bool buildMBRDot(std::ifstream& f, const MBR& mbr, const std::string& diskPath, std::string& dotOut) {
    std::ostringstream sb;
    sb << "digraph G {\n";
    sb << "node [shape=plaintext fontname=\"Helvetica\"];\n";

    // Tabla MBR
    sb << "mbr [label=<\n";
    sb << "<table border='1' cellborder='1' cellspacing='0' bgcolor='white'>\n";
    sb << "<tr><td colspan='2' bgcolor='#9370DB'><b>REPORTE DE MBR</b></td></tr>\n";
    sb << "<tr><td>mbr_tamano</td><td>" << mbr.MbrSize << "</td></tr>\n";
    sb << "<tr><td>mbr_fecha_creacion</td><td>" << mbr.MbrCreationDate << "</td></tr>\n";
    sb << "<tr><td>mbr_disk_signature</td><td>" << mbr.MbrDiskSignature << "</td></tr>\n";
    sb << "<tr><td>mbr_fit</td><td>" << mbr.DskFit << "</td></tr>\n";

    for (int i = 0; i < 4; i++) {
        Partition p = mbr.Partitions[i];
        if (p.PartSize == 0) continue;

        std::string partName(p.PartName, p.PartName + sizeof(p.PartName));
        partName.erase(std::remove(partName.begin(), partName.end(), '\0'), partName.end());

        sb << "<tr><td colspan='2' bgcolor='#ADD8E6'><b>Partición " << (i+1) << "</b></td></tr>\n";
        sb << "<tr><td>part_status</td><td>" << p.PartStatus << "</td></tr>\n";
        sb << "<tr><td>part_type</td><td>" << p.PartType << "</td></tr>\n";
        sb << "<tr><td>part_fit</td><td>" << p.PartFit << "</td></tr>\n";
        sb << "<tr><td>part_start</td><td>" << p.PartStart << "</td></tr>\n";
        sb << "<tr><td>part_size</td><td>" << p.PartSize << "</td></tr>\n";
        sb << "<tr><td>part_name</td><td>" << partName << "</td></tr>\n";
        sb << "<tr><td>part_correlative</td><td>" << p.PartCorrelative << "</td></tr>\n";
        sb << "<tr><td>part_id</td><td>" << p.PartID << "</td></tr>\n";
    }
    sb << "</table>>];\n";

    // Tabla EBRs
    sb << "ebrs [label=<\n";
    sb << "<table border='1' cellborder='1' cellspacing='0' bgcolor='white'>\n";
    sb << "<tr><td colspan='2' bgcolor='#FFDAB9'><b>REPORTE DE EBRs</b></td></tr>\n";

    for (int i = 0; i < 4; i++) {
        Partition p = mbr.Partitions[i];
        if (p.PartType == 'E' || p.PartType == 'e') {
            int pos = p.PartStart;
            while (true) {
                EBR ebr{};
                f.seekg(pos, std::ios::beg);
                f.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
                if (!f || ebr.PartSize == 0) break;

                std::string ebrName(ebr.PartName, ebr.PartName + sizeof(ebr.PartName));
                ebrName.erase(std::remove(ebrName.begin(), ebrName.end(), '\0'), ebrName.end());

                sb << "<tr><td colspan='2' bgcolor='#FFE4B5'><b>EBR</b></td></tr>\n";
                sb << "<tr><td>part_status</td><td>" << ebr.PartStatus << "</td></tr>\n";
                sb << "<tr><td>part_fit</td><td>" << ebr.PartFit << "</td></tr>\n";
                sb << "<tr><td>part_start</td><td>" << ebr.PartStart << "</td></tr>\n";
                sb << "<tr><td>part_size</td><td>" << ebr.PartSize << "</td></tr>\n";
                sb << "<tr><td>part_name</td><td>" << ebrName << "</td></tr>\n";
                sb << "<tr><td>part_next</td><td>" << ebr.PartNext << "</td></tr>\n";

                if (ebr.PartNext == -1) break;
                pos = ebr.PartNext;
            }
        }
    }
    sb << "</table>>];\n";

    sb << "}\n";
    dotOut = sb.str();
    common::AddInfo("[REP] DOT de MBR+EBR generado correctamente");
    return true;
}



bool buildDiskDot(std::ifstream& f, const MBR& mbr, const std::string& diskPath, std::string& dotOut) {
    std::ostringstream sb;
    sb << "digraph Disk {\n";
    sb << "node [shape=plaintext fontname=\"Helvetica\"];\n";

    sb << "disk [label=<\n";
    sb << "<table border='1' cellborder='1' cellspacing='0'>\n";
    sb << "<tr><td colspan='20'><b>" << std::filesystem::path(diskPath).filename().string() << "</b></td></tr>\n";
    sb << "<tr>";

    double total = static_cast<double>(mbr.MbrSize);
    sb << "<td>MBR</td>";

    // Ordenar particiones por PartStart
    std::vector<Partition> partitions;
    for (auto& p : mbr.Partitions) {
        if (p.PartSize > 0) partitions.push_back(p);
    }
    std::sort(partitions.begin(), partitions.end(),
              [](const Partition& a, const Partition& b){ return a.PartStart < b.PartStart; });

    long pos = sizeof(MBR);
    for (auto& p : partitions) {
        long pStart = p.PartStart;
        long pSize  = p.PartSize;

        // Espacio libre antes de la partición
        if (pStart > pos) {
            double freeSize = static_cast<double>(pStart - pos);
            sb << "<td>Libre<br/>" << std::fixed << std::setprecision(2)
               << (freeSize/total*100) << "%</td>";
            pos = pStart;
        }

        std::string name(p.PartName, p.PartName + sizeof(p.PartName));
        name.erase(std::remove(name.begin(), name.end(), '\0'), name.end());
        double percent = (static_cast<double>(pSize)/total)*100;

        if (p.PartType == 'E' || p.PartType == 'e') {
            // Subtabla para extendida
            sb << "<td><table border='1' cellborder='1' cellspacing='0'>";
            sb << "<tr><td colspan='10'><b>Extendida (" << name << ")</b></td></tr><tr>";

            long ebrPos = pStart;
            long ebrUsed = 0;
            while (true) {
                EBR ebr{};
                f.seekg(ebrPos, std::ios::beg);
                f.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
                if (!f || ebr.PartSize == 0) break;

                std::string ebrName(ebr.PartName, ebr.PartName + sizeof(ebr.PartName));
                ebrName.erase(std::remove(ebrName.begin(), ebrName.end(), '\0'), ebrName.end());
                double ebrPercent = (static_cast<double>(ebr.PartSize)/total)*100;

                sb << "<td>EBR</td><td>Lógica " << ebrName << "<br/>"
                   << std::fixed << std::setprecision(2) << ebrPercent << "%</td>";

                ebrUsed += ebr.PartSize;
                if (ebr.PartNext == -1) break;
                ebrPos = ebr.PartNext;
            }

            long freeInside = pSize - ebrUsed;
            if (freeInside > 0) {
                sb << "<td>Libre<br/>" << std::fixed << std::setprecision(2)
                   << (static_cast<double>(freeInside)/total*100) << "%</td>";
            }

            sb << "</tr></table></td>";
        } else {
            sb << "<td>Primaria " << name << "<br/>"
               << std::fixed << std::setprecision(2) << percent << "%</td>";
        }

        pos = pStart + pSize;
    }

    // Espacio libre al final
    if (pos < mbr.MbrSize) {
        double freeSize = static_cast<double>(mbr.MbrSize - pos);
        sb << "<td>Libre<br/>" << std::fixed << std::setprecision(2)
           << (freeSize/total*100) << "%</td>";
    }

    sb << "</tr></table>>];\n";
    sb << "}\n";

    dotOut = sb.str();
    common::AddInfo("[REP] DOT de DISK generado correctamente");
    return true;
}
bool buildSBDot(const SuperBloque& sb, std::string& dotOut) {
    std::ostringstream ss;
    ss << "digraph G {\n";
    ss << "node [shape=plaintext fontname=\"Helvetica\"];\n";

    ss << "sb [label=<\n";
    ss << "<table border='1' cellborder='1' cellspacing='0' bgcolor='white'>\n";
    ss << "<tr><td colspan='2' bgcolor='#20B2AA'><b>REPORTE DE SUPERBLOQUE</b></td></tr>\n";

    ss << "<tr><td>SFileSystemType</td><td>" << sb.SFileSystemType << "</td></tr>\n";
    ss << "<tr><td>SInodesCount</td><td>" << sb.SInodesCount << "</td></tr>\n";
    ss << "<tr><td>SBlocksCount</td><td>" << sb.SBlocksCount << "</td></tr>\n";
    ss << "<tr><td>SFreeBlocksCount</td><td>" << sb.SFreeBlocksCount << "</td></tr>\n";
    ss << "<tr><td>SFreeInodesCount</td><td>" << sb.SFreeInodesCount << "</td></tr>\n";
    ss << "<tr><td>SMtime</td><td>" << sb.SMtime << "</td></tr>\n";
    ss << "<tr><td>SUmtime</td><td>" << sb.SUmtime << "</td></tr>\n";
    ss << "<tr><td>SMntCount</td><td>" << sb.SMntCount << "</td></tr>\n";
    ss << "<tr><td>SMagic</td><td>" << sb.SMagic << "</td></tr>\n";
    ss << "<tr><td>SInodeSize</td><td>" << sb.SInodeSize << "</td></tr>\n";
    ss << "<tr><td>SBlockSize</td><td>" << sb.SBlockSize << "</td></tr>\n";
    ss << "<tr><td>SFirstIno</td><td>" << sb.SFirstIno << "</td></tr>\n";
    ss << "<tr><td>SFirstBlo</td><td>" << sb.SFirstBlo << "</td></tr>\n";
    ss << "<tr><td>SBitmapInodeStart</td><td>" << sb.SBitmapInodeStart << "</td></tr>\n";
    ss << "<tr><td>SBitmapBlockStart</td><td>" << sb.SBitmapBlockStart << "</td></tr>\n";
    ss << "<tr><td>SInodeStart</td><td>" << sb.SInodeStart << "</td></tr>\n";
    ss << "<tr><td>SBlockStart</td><td>" << sb.SBlockStart << "</td></tr>\n";

    ss << "</table>>];\n";
    ss << "}\n";

    dotOut = ss.str();
    common::AddInfo("[REP] DOT de SuperBloque generado correctamente");
    return true;
}
