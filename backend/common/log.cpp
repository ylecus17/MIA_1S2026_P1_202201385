#include "log.h"
#include <iostream>

namespace common {
    static std::vector<LogMessage> ConsoleLog;

    void ClearLogs() {
        ConsoleLog.clear();
    }

    std::vector<LogMessage> GetLogs() {
        return ConsoleLog;
    }

    void Log(const std::string& level, const std::string& text) {
        ConsoleLog.push_back({level, text});
        std::cout << text << std::endl; // opcional: imprime en consola
    }

    void AddSuccess(const std::string& text) {
        Log("success", text);
    }

    void AddError(const std::string& text) {
        Log("error", text);
    }

    void AddInfo(const std::string& text) {
        Log("info", text);
    }

    void AddTxt(const std::string& text) {
        Log("txt", text);
    }
}
