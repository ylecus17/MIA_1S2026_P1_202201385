#include "../common/log.h"
#include "../commands/mkfs.h"
#include "../commands/login.h"
#include <regex>
#include <string>
#include <vector>

void parseMkfs(const std::vector<std::string>& tokens) {
    std::string id = "";
    std::string ftype = "full"; // por defecto

    std::regex re("^-([a-zA-Z]+)=(.+)$");
    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // convertir a minúsculas
std::transform(param.begin(), param.end(), param.begin(), ::tolower);
// OJO: no conviertas value si es el ID
if (param != "id") {
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
}


            if (param == "id") {
                id = value;
            } else if (param == "type") {
                if (value == "full") {
                    ftype = value;
                } else {
                    common::AddError("Error de sintaxis: -type inválido (" + value + ")");
                    return;
                }
            } else {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    // Validaciones mínimas
    if (id.empty()) {
        common::AddError("Error de sintaxis: -id es obligatorio en mkfs");
        return;
    }

    // Llamar al comando
    if (!Mkfs(id, ftype)) {
        common::AddError("Error en mkfs (id=" + id + ")");
    } else {
        common::AddSuccess("Partición con id=" + id + " formateada (" + ftype + ")");
    }
}
void parseLogin(const std::vector<std::string>& tokens) {
    std::string user = "";
    std::string pass = "";
    std::string id   = "";

    std::regex re("^-([a-zA-Z]+)=(.+)$");
    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // convertir parámetro a minúsculas
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "user") {
                user = value;
            } else if (param == "pass") {
                pass = value;
            } else if (param == "id") {
                id = value;
            } else {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (user.empty() || pass.empty() || id.empty()) {
        common::AddError("Error de sintaxis: -user, -pass e -id son obligatorios");
        return;
    }

    if (!Login(user, pass, id)) {
        common::AddError("Error en login (usuario=" + user + ")");
    } else {
        common::AddSuccess("Usuario " + user + " logueado correctamente");
    }
}

void parseLogout(const std::vector<std::string>& tokens) {
    if (tokens.size() != 1) {
        common::AddError("Error de sintaxis: logout no lleva parámetros");
        return;
    }

    if (!Logout()) {
        common::AddError("Error en logout");
    } else {
        common::AddSuccess("Sesión cerrada correctamente");
    }
}