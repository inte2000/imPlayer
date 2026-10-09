/*
20260428 修改路径拼接错误
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_28.txt

20260428 添加MakeupDecoderPlugPathname函数
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_29.txt
*/
#ifndef _GLOBAL_CONFIG_H_
#define _GLOBAL_CONFIG_H_



std::string GetSpeakerConfigFilePathname();
std::string GetPluginConfigFilePathname();
std::string GetDecoderConfigFilePathname();
std::string MakeupDecoderPlugPathname(const std::string& hostname);


#endif // _GLOBAL_CONFIG_H_