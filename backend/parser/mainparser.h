#ifndef MAINPARSER_H
#define MAINPARSER_H

#include <string>
#include <vector>
#include "../common/log.h"   // tu sistema de logs

// Procesa múltiples líneas
std::vector<common::LogMessage> ParseCommands(const std::string& input);

// Procesa un único comando
void ParseCommand(const std::string& input);

// Tokenizer común
std::vector<std::string> tokenize(const std::string& input);

#endif // MAINPARSER_H
