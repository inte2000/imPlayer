#pragma once

#include <string>
#include <vector>

bool is_valid_utf8(const std::string& data);
bool is_valid_gbk(const std::string& data);
std::string DecodeUnknownString(const std::string& input);
