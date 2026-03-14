#pragma once 
#include <string>
#include <vector>

void parseMkfs(const std::vector<std::string>& tokens);
void parseLogin(const std::vector<std::string>& tokens);
void parseLogout(const std::vector<std::string>& tokens);  
void parseMkgrp(const std::vector<std::string>& tokens);
void parseCat(const std::vector<std::string>& tokens);
void parseMkusr(const std::vector<std::string> &tokens);
void parseRmusr(const std::vector<std::string> &tokens);
void parseRmgrp(const std::vector<std::string> &tokens);



