#pragma once
#include <string>
#include <vector>

void parseMkdisk(const std::vector<std::string>& tokens);
void parseRmdisk(const std::vector<std::string>& tokens);   
void parseFdisk(const std::vector<std::string>& tokens);
void parseMount(const std::vector<std::string>& tokens);
void parseMounted(const std::vector<std::string>& tokens);