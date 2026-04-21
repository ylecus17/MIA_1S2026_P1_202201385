#include <regex>
#include <vector>
#include <string>
#include <iostream>
#include "pdisk.h"
#include "../common/log.h"

#include "../commands/disk.h"
#include "../commands/mount.h"

using namespace std;

void parseMkdisk(const std::vector<std::string>& tokens) {
       

    int size = -1;
    std::string unit = "K";
    std::string path = "";
    std::string fit = "FF";

    regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        
        smatch match;
        if (regex_match(tokens[i], match, re)) {
            std::string param = match[1];
            std::string value = match[2];

            value.erase(remove(value.begin(), value.end(), '\"'), value.end());
            transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "size") {
                size = stoi(value);
                
            } else if (param == "unit") {
                transform(value.begin(), value.end(), value.begin(), ::toupper);
                if (value != "K" && value != "M") {
                    common::AddError("-unit debe ser K o M");
                    return;
                }
                unit = value;
               
            } else if (param == "path") {
                path = value;
                
            } else if (param == "fit") {
                transform(value.begin(), value.end(), value.begin(), ::toupper);
                if (value != "BF" && value != "FF" && value != "WF") {
                    common::AddError("-fit debe ser BF FF WF");
                    return;
                }
                fit = value;
                
            }
        }
    }

    if (size <= 0) {
        common::AddError("-size debe ser mayor a 0");
        return;
    }

    if (path == "") {
        common::AddError("-path obligatorio");
        return;
    }

    if (!MkDisk(size, unit, path, fit)) {
        common::AddError("Error creando disco");
    } else {
        common::AddSuccess("Disco creado correctamente");
    }
}
void parseRmdisk(const std::vector<std::string>& tokens) {

    std::string path = "";

    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1];
            std::string value = match[2];
            value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "path") {
                path = value;

            } else {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (path.empty()) {
        common::AddError("Error de sintaxis: -path es obligatorio");
        return;
    }

    if (!RmDisk(path)) {
        common::AddError("Error eliminando disco: " + path);
    } else {
        common::AddSuccess("Disco eliminado: " + path);
    }
}

//funciona de momento de aqui para arriba 

void parseFdisk(const std::vector<std::string> &tokens)
{
    int size = -1;
    std::string unit = "K";
    std::string path = "";
    std::string name = "";
    std::string ptype = "P";
    std::string fit = "WF";
    std::string delMode = ""; // delete puede ser "fast" o "full"
    int add = 0;              // cantidad de espacio a agregar
    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1];
            std::string value = match[2];
            value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "size")
            {
                size = std::stoi(value);
                common::AddInfo("Parámetro size aceptado: " + value);
            }
            else if (param == "unit")
            {
                std::transform(value.begin(), value.end(), value.begin(), ::toupper);
                if (value != "B" && value != "K" && value != "M")
                {
                    common::AddError("Error de sintaxis: -unit inválido (" + value + ")");
                    return;
                }
                unit = value;
                common::AddInfo("Parámetro unit aceptado: " + unit);
            }
            else if (param == "path")
            {
                path = value;
                common::AddInfo("Parámetro path aceptado: " + path);
            }
            else if (param == "name")
            {
                name = value;
                common::AddInfo("Parámetro name aceptado: " + name);
            }
            else if (param == "type")
            {
                std::transform(value.begin(), value.end(), value.begin(), ::toupper);
                if (value != "P" && value != "E" && value != "L")
                {
                    common::AddError("Error de sintaxis: -type inválido (" + value + ")");
                    return;
                }
                ptype = value;
                common::AddInfo("Parámetro type aceptado: " + ptype);
            }
            else if (param == "fit")
            {
                std::transform(value.begin(), value.end(), value.begin(), ::toupper);
                if (value != "BF" && value != "FF" && value != "WF")
                {
                    common::AddError("Error de sintaxis: -fit inválido (" + value + ")");
                    return;
                }
                fit = value;
                common::AddInfo("Parámetro fit aceptado: " + fit);
            }
            else if (param == "delete")
            {
                std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                if (value != "fast" && value != "full")
                {
                    common::AddError("Error de sintaxis: -delete inválido (" + value + ")");
                    return;
                }
                delMode = value;
                common::AddInfo("Parámetro delete aceptado: " + delMode);
            }
            else if (param == "add")
            {
                try
                {
                    add = std::stoi(value);
                    common::AddInfo("Parámetro add aceptado: " + value);
                }
                catch (...)
                {
                    common::AddError("Error de sintaxis: -add inválido (" + value + ")");
                    return;
                }
            }
            else
            {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

   // path y name siempre son obligatorios
if (path.empty() || name.empty()) {
    common::AddError("Error de sintaxis: parámetros obligatorios faltantes (path y name)");
    return;
}

// Si no es delete ni add, entonces sí requiere size
if (delMode.empty() && add == 0) {
    if (size <= 0) {
        common::AddError("Error de sintaxis: falta parámetro size para crear partición");
        return;
    }
}

    if (!Fdisk(size, unit, path, name, ptype, fit, delMode, add))
    {
        common::AddError("Error en fdisk");
    }
    else
    {
        common::AddSuccess("Partición procesada: " + name + " en " + path +
                           (delMode.empty() && add == 0
                                ? " (" + std::to_string(size) + unit + ", type=" + ptype + ", fit=" + fit + ")"
                                : (delMode.empty() ? ", add=" + std::to_string(add) + unit : ", delete=" + delMode)));
    }
}

// ---------------- MOUNT ----------------
void parseMount(const std::vector<std::string>& tokens) {
    std::string path = "";
    std::string name = "";

    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1];
            std::string value = match[2];
            value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "path") {
                path = value;
                common::AddInfo("Parámetro path aceptado: " + path);
            } else if (param == "name") {
                name = value;
                common::AddInfo("Parámetro name aceptado: " + name);
            } else {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (path.empty() || name.empty()) {
        common::AddError("Error de sintaxis: faltan parámetros obligatorios en mount");
        return;
    }

    if (!Mount(path, name)) {
        common::AddError("Error montando partición");
    } else {
        common::AddSuccess("Partición montada: " + name + " en " + path);
    }
    
}

// ---------------- MOUNTED ----------------
void parseMounted(const std::vector<std::string>& tokens) {
    common::AddInfo("Mostrando particiones montadas");
    Mounted();
}
void parseUnmount(const std::vector<std::string>& tokens) {
    std::string id = "";

    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1];
            std::string value = match[2];
            value.erase(std::remove(value.begin(), value.end(), '\"'), value.end());
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "id") {
                id = value;
                common::AddInfo("Parámetro id aceptado: " + id);
            } else {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (id.empty()) {
        common::AddError("Error de sintaxis: -id es obligatorio para unmount");
        return;
    }

    if (!Unmount(id)) {
        common::AddError("Error desmontando partición con ID: " + id);
    } else {
        common::AddSuccess("Partición desmontada con ID: " + id);
    }
}