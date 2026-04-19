#include "crow.h"
#include "json.hpp"
#include "parser/mainparser.h"
#include "common/log.h"

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
}

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/execute")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::response res;
        res.code = 200;

        res.set_header("Content-Type", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
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

    std::cout << "Servidor corriendo en http://localhost:5300\n";
    app.port(5300).multithreaded().run();
}
