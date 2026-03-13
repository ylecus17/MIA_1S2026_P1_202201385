#ifndef COMMON_LOG_H
#define COMMON_LOG_H

#include <string>
#include <vector>

namespace common {

    struct LogMessage {
        std::string level;  // success, error, info, txt
        std::string text;
    };

    void ClearLogs();
    std::vector<LogMessage> GetLogs();

    void Log(const std::string& level, const std::string& text);

    void AddSuccess(const std::string& text);
    void AddError(const std::string& text);
    void AddInfo(const std::string& text);
    void AddTxt(const std::string& text);

} // namespace common

#endif // COMMON_LOG_H
