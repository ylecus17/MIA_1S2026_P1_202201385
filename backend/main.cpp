#include "crow.h"
#include "json.hpp"
#include "parser/mainparser.h"
#include "common/log.h"

#include "commands/disk.h"
#include "commands/login.h"
#include <iostream>
#include <string>

using json = nlohmann::json;

// Función para sanear cadenas (elimina bytes no válidos en UTF-8)
std::string sanitize(const std::string& input) {
    std::string out;
    for (unsigned char c : input) {
        if (c < 128) { // solo ASCII seguro
            out.push_back(c);
        }
        // aquí podrías agregar lógica para mapear otros caracteres válidos en UTF-8
    }
    return out;
}json DisksToJson() {
    json j = json::array();
    for (const auto& d : disks) {
        json diskJson;
        diskJson["name"] = d.name;
        diskJson["fit"] = std::string(1, d.fit);
        diskJson["sizeBytes"] = d.sizeBytes;
        diskJson["path"] = d.path;

        // Particiones montadas
        diskJson["particionesMontadas"] = json::array();
        for (const auto& p : d.particionesMontadas) {
            diskJson["particionesMontadas"].push_back({
                {"name", p.name},
                {"id", p.id},
                {"fit", std::string(1, p.fit)},
                {"path", p.path},
                {"size", p.size}
            });
        }

        j.push_back(diskJson);
    }
    return j;
}

int main() {
    crow::SimpleApp app;

// Login con texto plano
CROW_ROUTE(app, "/login").methods("POST"_method)
([](const crow::request& req){
    std::cout << "[DEBUG] ENTRÓ A LOGIN (POST TEXTO PLANO)" << std::endl;
    std::cout << "[DEBUG] BODY: " << req.body << std::endl;

    // Parsear formato user=...;pass=...;id=...
    std::map<std::string,std::string> params;
    std::stringstream ss(req.body);
    std::string item;
    while (std::getline(ss, item, ';')) {
        auto pos = item.find('=');
        if (pos != std::string::npos) {
            params[item.substr(0,pos)] = item.substr(pos+1);
        }
    }

    std::string user = params["user"];
    std::string pass = params["pass"];
    std::string id   = params["id"];

    std::string message;
    bool ok = Login(user, pass, id, message);

    nlohmann::json r;
    if (ok) {
        r["success"] = true;
        r["user"] = user;
        r["token"] = "token_" + user + "_" + id;
        r["message"] = message;
    } else {
        r["success"] = false;
        r["error"] = message;
    }

    crow::response res(200, r.dump());
    res.set_header("Content-Type", "application/json");
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
    res.set_header("Access-Control-Allow-Methods", "POST");
    return res;
});


 // Nuevo endpoint para listar discos
    CROW_ROUTE(app, "/disks")
    .methods("GET"_method)
    ([]() {
        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*"); // igual cámbialo a tu dominio S3

        json response;
        response["disks"] = DisksToJson(); // tu función que convierte la lista en JSON

        res.write(response.dump());
        return res;
    });
    CROW_ROUTE(app, "/execute")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::response res;
        res.code = 200;

        res.set_header("Content-Type", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Origin", "*"); // luego cámbialo a tu dominio S3
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "POST");

        std::string texto = req.body;

        auto logs = ParseCommands(texto);

        json response;
        response["output"] = json::array();
        for (const auto& log : logs) {
            response["output"].push_back({
                {"level", sanitize(log.level)},
                {"text", sanitize(log.text)}
            });
        }

        res.write(response.dump());
        return res;
    });

    
   

    std::cout << "Servidor corriendo en http://0.0.0.0:5300\n";
    app.port(5300).multithreaded().bindaddr("0.0.0.0").run();
}