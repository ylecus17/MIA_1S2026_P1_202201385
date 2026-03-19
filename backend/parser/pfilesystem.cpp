#include "../common/log.h"
#include "../commands/mkfs.h"
#include "../commands/login.h"
#include "../commands/user.h"
#include "../commands/files.h"
#include <regex>
#include <string>
#include <vector>

void parseMkfs(const std::vector<std::string> &tokens)
{
    std::string id = "";
    std::string ftype = "full"; // por defecto

    std::regex re("^-([a-zA-Z]+)=(.+)$");
    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // convertir a minúsculas
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);
            // OJO: no conviertas value si es el ID
            if (param != "id")
            {
                std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            }

            if (param == "id")
            {
                id = value;
            }
            else if (param == "type")
            {
                if (value == "full")
                {
                    ftype = value;
                }
                else
                {
                    common::AddError("Error de sintaxis: -type inválido (" + value + ")");
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

    // Validaciones mínimas
    if (id.empty())
    {
        common::AddError("Error de sintaxis: -id es obligatorio en mkfs");
        return;
    }

    // Llamar al comando
    if (!Mkfs(id, ftype))
    {
        common::AddError("Error en mkfs (id=" + id + ")");
    }
    else
    {
        common::AddSuccess("Partición con id=" + id + " formateada (" + ftype + ")");
    }
}
void parseLogin(const std::vector<std::string> &tokens)
{
    std::string user = "";
    std::string pass = "";
    std::string id = "";

    std::regex re("^-([a-zA-Z]+)=(.+)$");
    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // convertir parámetro a minúsculas
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "user")
            {
                user = value;
            }
            else if (param == "pass")
            {
                pass = value;
            }
            else if (param == "id")
            {
                id = value;
            }
            else
            {
                common::AddError("Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (user.empty() || pass.empty() || id.empty())
    {
        common::AddError("Error de sintaxis: -user, -pass e -id son obligatorios");
        return;
    }

    if (!Login(user, pass, id))
    {
        common::AddError("Error en login (usuario=" + user + ")");
    }
    else
    {
        common::AddSuccess("Usuario " + user + " logueado correctamente");
    }
}

void parseLogout(const std::vector<std::string> &tokens)
{
    if (tokens.size() != 1)
    {
        common::AddError("Error de sintaxis: logout no lleva parámetros");
        return;
    }

    if (!Logout())
    {
        common::AddError("Error en logout");
    }
    else
    {
        common::AddSuccess("Sesión cerrada correctamente");
    }
}
void parseMkgrp(const std::vector<std::string> &tokens)
{
    std::string name;
    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1].str();
            std::string value = match[2].str();
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "name")
            {
                name = value;
            }
            else
            {
                common::AddError("[MKGRP] Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (name.empty())
    {
        common::AddError("[MKGRP] Error de sintaxis: -name es obligatorio en mkgrp");
        return;
    }

    if (!Mkgrp(name))
    {
        common::AddError("Error en mkgrp (name=" + name + ")");
    }
    else
    {
        common::AddSuccess("Grupo " + name + " creado correctamente");
    }
}

// ---------------- CAT ----------------
void parseCat(const std::vector<std::string> &tokens)
{
    std::vector<std::string> paths;
    std::regex re("^-file\\d+=(\"?.+\"?)$");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string value = match[1].str();
            if (!value.empty() && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);
            paths.push_back(value);
        }
        else
        {
            common::AddError("[CAT] Error de sintaxis en parámetro: " + tokens[i]);
            return;
        }
    }

    if (paths.empty())
    {
        common::AddError("[CAT] Error: se requiere al menos un parámetro -fileN");
        return;
    }

    if (!Cat(paths))
    {
        common::AddError("Error en cat");
    }
}

// ---------------- RMGRP ----------------
void parseRmgrp(const std::vector<std::string> &tokens)
{
    std::string name;
    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1].str();
            std::string value = match[2].str();
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "name")
            {
                name = value;
            }
            else
            {
                common::AddError("[RMGRP] Error de sintaxis: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (name.empty())
    {
        common::AddError("[RMGRP] Error: -name es obligatorio");
        return;
    }

    if (!Rmgrp(name))
    {
        common::AddError("Error en rmgrp (name=" + name + ")");
    }
    else
    {
        common::AddSuccess("Grupo " + name + " eliminado correctamente");
    }
}

// ---------------- MKUSR ----------------
void parseMkusr(const std::vector<std::string> &tokens)
{
    std::string user, pass, grp;
    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1].str();
            std::string value = match[2].str();
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "user")
                user = value;
            else if (param == "pass")
                pass = value;
            else if (param == "grp")
                grp = value;
            else
            {
                common::AddError("[MKUSR] Error: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (user.empty() || pass.empty() || grp.empty())
    {
        common::AddError("[MKUSR] Error: faltan parámetros obligatorios (-user, -pass, -grp)");
        return;
    }

    if (!Mkusr(user, pass, grp))
    {
        common::AddError("Error en mkusr (user=" + user + ")");
    }
    else
    {
        common::AddSuccess("Usuario " + user + " creado correctamente");
    }
}

// ---------------- RMUSR ----------------
void parseRmusr(const std::vector<std::string> &tokens)
{
    std::string user;
    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re))
        {
            std::string param = match[1].str();
            std::string value = match[2].str();
            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "user")
            {
                user = value;
            }
            else
            {
                common::AddError("[RMUSR] Error: parámetro desconocido " + param);
                return;
            }
        }
    }

    if (user.empty())
    {
        common::AddError("[RMUSR] Error: -user es obligatorio");
        return;
    }

    if (!Rmusr(user))
    {
        common::AddError("Error en rmusr (user=" + user + ")");
    }
    else
    {
        common::AddSuccess("Usuario " + user + " eliminado correctamente");
    }
}
void parseChgrp(const std::vector<std::string>& tokens)
{
    std::string user;
    std::string grp;

    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // quitar comillas si las tiene
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "user") {
                user = value;
            } else if (param == "grp") {
                grp = value;
            } else {
                common::AddError("[CHGRP] Error: parámetro desconocido " + param);
                return;
            }
        } else {
            common::AddError("[CHGRP] Error de sintaxis en parámetro: " + tokens[i]);
            return;
        }
    }

    if (user.empty() || grp.empty()) {
        common::AddError("[CHGRP] Error: faltan parámetros obligatorios (-user, -grp)");
        return;
    }

    if (!Chgrp(user, grp)) {
        common::AddError("[CHGRP] Error ejecutando cambio de grupo");
    }
}
void parseMkfile(const std::vector<std::string>& tokens)
{
    std::string path;
    bool rFlag = false;
    int size = 0;
    std::string cont;

    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // quitar comillas si las tiene
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "path") {
                path = value;
            } else if (param == "size") {
                try {
                    size = std::stoi(value);
                    if (size < 0) {
                        common::AddError("[MKFILE] Error: tamaño negativo");
                        return;
                    }
                } catch (...) {
                    common::AddError("[MKFILE] Error: tamaño inválido");
                    return;
                }
            } else if (param == "cont") {
                cont = value;
            } else {
                common::AddError("[MKFILE] Error: parámetro desconocido " + param);
                return;
            }
        } else if (tokens[i] == "-r") {
            rFlag = true;
        } else {
            common::AddError("[MKFILE] Error de sintaxis en parámetro: " + tokens[i]);
            return;
        }
    }

    if (path.empty()) {
        common::AddError("[MKFILE] Error: falta parámetro obligatorio -path");
        return;
    }

    if (!Mkfile(path, rFlag, size, cont)) {
        common::AddError("[MKFILE] Error creando archivo");
    }
}
void parseMkdir(const std::vector<std::string>& tokens)
{
    std::string path;
    bool pFlag = false;

    std::regex re("^-([a-zA-Z]+)=(.+)$");

    for (size_t i = 1; i < tokens.size(); i++) {
        std::smatch match;
        if (std::regex_match(tokens[i], match, re)) {
            std::string param = match[1].str();
            std::string value = match[2].str();

            // quitar comillas si las tiene
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            std::transform(param.begin(), param.end(), param.begin(), ::tolower);

            if (param == "path") {
                path = value;
            } else {
                common::AddError("[MKDIR] Error: parámetro desconocido " + param);
                return;
            }
        } else if (tokens[i] == "-p") {
            pFlag = true;
        } else {
            common::AddError("[MKDIR] Error de sintaxis en parámetro: " + tokens[i]);
            return;
        }
    }

    if (path.empty()) {
        common::AddError("[MKDIR] Error: falta parámetro obligatorio -path");
        return;
    }

    if (!Mkdir(path, pFlag)) {
        common::AddError("[MKDIR] Error creando carpeta");
    }
}

