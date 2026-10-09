/*
20250726 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#pragma once

#include <string>
#include <vector>

bool is_valid_utf8(const std::string& data);
bool is_valid_gbk(const std::string& data);
std::string DecodeUnknownString(const std::string& input);
