#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <unordered_map>
#include "../common/log.h"
#include "../commands/mkfs.h"
#include "../commands/login.h"
#include "../commands/user.h"
#include "../commands/files.h"
#include "../commands/reports.h"   

// Función para dividir tokens (simulando lo que ya tienes en tu parser general)


// ===================== PARSER REP =====================
void parseRep(const std::vector<std::string>& tokens) {
    std::string id, path, name, pathFileLS;

    std::regex re("^-(\\w+)=([^\"]+|\"[^\"]+\")$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (!std::regex_match(tokens[i], match, re)) {
            common::AddError("[REP] Parámetro inválido: " + tokens[i]);
            return;
        }

        std::string param = match[1].str();
        std::string val = match[2].str();

        // quitar comillas si las hay
        if (!val.empty() && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        // normalizar a minúsculas
        std::transform(param.begin(), param.end(), param.begin(), ::tolower);

        if (param == "id") {
            id = val;
        } else if (param == "path") {
            path = val;
        } else if (param == "name") {
            std::transform(val.begin(), val.end(), val.begin(), ::tolower);
            name = val;
        } else if (param == "path_file_ls") {
            pathFileLS = val;
        } else {
            common::AddError("[REP] Parámetro desconocido: " + param);
            return;
        }
    }

    // Validaciones
    if (id.empty()) {
        common::AddError("[REP] Error: -id es obligatorio");
        return;
    }
    if (path.empty()) {
        common::AddError("[REP] Error: -path es obligatorio");
        return;
    }
    if (name.empty()) {
        common::AddError("[REP] Error: -name es obligatorio");
        return;
    }

    // Reportes válidos
    static std::unordered_map<std::string, bool> valid = {
        {"mbr", true},
        {"disk", true},
        {"inode", true},
        {"block", true},
        {"bm_inode", true},
        {"bm_block", true},
        {"tree", true},
        {"sb", true},
        {"file", true},
        {"ls", true}
    };

    if (!valid[name]) {
        common::AddError("[REP] Error: name inválido (" + name + "). Valores válidos: mbr, disk, inode, block, bm_inode, bm_block, tree, sb, file, ls");
        return;
    }
if (!GenerateReport(id, path, name, pathFileLS)) {
    common::AddError("[REP] Error generando reporte " + name);
} else {
    common::AddSuccess("[REP] Reporte " + name + " generado correctamente");
}


    // El éxito se registra dentro de GenerateReport
}
