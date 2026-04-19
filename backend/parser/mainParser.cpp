#include "mainparser.h"
#include <sstream>
#include <algorithm>
#include <regex>
#include "pdisk.h"
#include "pfilesystem.h"
#include "reports.h"
std::vector<std::string> tokenize(const std::string &input)
{
    std::vector<std::string> tokens;
    std::regex re(R"(-path="[^"]*"|\"[^\"]*\"|\S+)");
    auto begin = std::sregex_iterator(input.begin(), input.end(), re);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it)
    {
        tokens.push_back(it->str());
    }
    return tokens;
}

std::vector<common::LogMessage> ParseCommands(const std::string &input)
{
    common::ClearLogs();

    std::stringstream ss(input);
    std::string line;

    while (std::getline(ss, line))
    {
        // quitar comentarios
        auto idx = line.find("#");
        if (idx != std::string::npos)
        {
            line = line.substr(0, idx);
        }

        // quitar espacios
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty())
            continue;

        ParseCommand(line);
    }

    return common::GetLogs();
}

void ParseCommand(const std::string &input)
{
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto tokens = tokenize(input);

    if (lower.rfind("mkdisk", 0) == 0)
    {
        parseMkdisk(tokens);
    }
    else if (lower.rfind("rmdisk", 0) == 0)
    {
        parseRmdisk(tokens);
    }
    else if (lower.rfind("fdisk", 0) == 0)
    {
        parseFdisk(tokens);
    }
    else if (lower.rfind("mounted", 0) == 0)
    {
        parseMounted(tokens);
    }
    else if (lower.rfind("mount", 0) == 0)
    {
        parseMount(tokens);
    }
    else if (lower.rfind("mkfs", 0) == 0)
    {
        parseMkfs(tokens);
    }
    else if (lower.rfind("login", 0) == 0)
    {
        parseLogin(tokens);
    }
    else if (lower.rfind("logout", 0) == 0)
    {
        parseLogout(tokens);
    }
    else if (lower.rfind("mkgrp", 0) == 0)
    {
        parseMkgrp(tokens);
    }
    else if (lower.rfind("cat", 0) == 0)
    {
        parseCat(tokens);
    }
    else if (lower.rfind("rmgrp", 0) == 0)
    {
        parseRmgrp(tokens);
    }
    else if (lower.rfind("mkusr", 0) == 0)
    {
        parseMkusr(tokens);
    }
    else if (lower.rfind("rmusr", 0) == 0)
    {
        parseRmusr(tokens);
    }
    else if (lower.rfind("chgrp", 0) == 0)
    {
        parseChgrp(tokens);
    }
    else if (lower.rfind("mkdir", 0) == 0)
    {
        parseMkdir(tokens);
    }
    else if (lower.rfind("mkfile", 0) == 0)
    {

        parseMkfile(tokens);
    }
     else if (lower.rfind("rep", 0) == 0) {
        parseRep(tokens);

    }
    else
    {
        common::AddError("Error de sintaxis: comando no reconocido -> " + input);
    }
}
