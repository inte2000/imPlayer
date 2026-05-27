# imPlayer
imPlayer music player

## Description

这是一个使用 AI 生成代码的方式创作一个音乐播放器程序的测试项目

## Build

使用 cmake 正常编译，如果使用 vscode，可以使用 "cmake-config-debug (MSVC C++20)" 配置项目，用 "cmake-build-debug (MSVC C++20)" 任务编译，这些任务都定义在 .vscode/tasks.json 文件中。

关于第三方库，有两种方式，一种是用 vcpkg 配置，然后让你的 Code Agent 修改 cmake 文件，引入这些库。另一种是将编译好的库放在 thirdparty 目录下，然后让 Code Agent 生成类似 FindLibXXX.cmake，然后在项目 cmake 文件中引入这个 配置。目前的例子代码中两种方式都有，你可以根据自己系统配置和安装的情况自行决定如何引入第三方库。

编译完成所有解码器插件后，执行以下命令自动生成插件配置：
```
implayer.exe -rd
```

可通过以下命令查看当前配置的解码器：
```
implayer.exe -ld
```

## 第三方库

### cmdline
### libSndFile
### mpeg123
### ffmpeg
### CLI11
### zlib
### libvgm
### ftxui
### libsox
### libcdio
### Catch2 
### libiconv
### ICU
### nlohmann_json
### libogg
### libFLAC
### libgme
### wavpack

