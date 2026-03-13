#include "crow.h"
#include "json.hpp"
#include "parser/mainparser.h"   // tu parser central
#include "common/log.h"   // sistema de logs

#include <iostream>
#include <string>

using json = nlohmann::json;

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/execute")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::response res;
        res.code = 200;

        // Cabeceras CORS
        res.set_header("Content-Type", "application/json");
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "POST");

        // Leer el texto enviado desde el frontend
        std::string texto = req.body;

        // Pasar el texto al parser
        auto logs = ParseCommands(texto);

        // Convertir la salida de logs en JSON
        json response;
        response["output"] = json::array();
        for (const auto& log : logs) {
            response["output"].push_back({
                {"level", log.level},
                {"text", log.text}
            });
        }

        res.write(response.dump());
        return res;
    });

    std::cout << "Servidor corriendo en http://localhost:5300\n";
    app.port(5300).multithreaded().run();
}
