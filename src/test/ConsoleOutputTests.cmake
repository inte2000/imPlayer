# execute_process 通过管道捕获输出，验证真实 CLI 的 UTF-8 字节及两条输出流。
execute_process(COMMAND "${PLAYER}" --ld
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
    ENCODING UTF-8 TIMEOUT 30
)
if(NOT result EQUAL 0 OR NOT errors STREQUAL "")
    message(FATAL_ERROR "Decoder listing failed (${result}): ${errors}")
endif()
foreach(text "当前解码器列表:" "名称:" "类型: 内置")
    string(FIND "${output}" "${text}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing UTF-8 stdout text '${text}': ${output}")
    endif()
endforeach()

execute_process(COMMAND "${PLAYER}" --cd=__missing_decoder_for_encoding_test__
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
    ENCODING UTF-8 TIMEOUT 30
)
if(NOT result MATCHES "^-?[0-9]+$" OR result EQUAL 0)
    message(FATAL_ERROR "Missing decoder must fail (${result}): ${errors}")
endif()
string(FIND "${errors}" "未找到插件解码器: __missing_decoder_for_encoding_test__" position)
if(position EQUAL -1)
    message(FATAL_ERROR "Missing UTF-8 stderr text: ${errors}")
endif()

# 在独立的中文路径下扫描空插件目录，覆盖 main.cpp 的扫描完成提示。
get_filename_component(player_dir "${PLAYER}" DIRECTORY)
get_filename_component(player_name "${PLAYER}" NAME)
set(fixture "${CMAKE_CURRENT_BINARY_DIR}/console-output-测试")
file(MAKE_DIRECTORY "${fixture}/plugins")
file(COPY "${PLAYER}" DESTINATION "${fixture}")
if(WIN32)
    # 复用构建目录的依赖 DLL，配置文件只写入 fixture。
    set(ENV{PATH} "${player_dir};$ENV{PATH}")
endif()
execute_process(COMMAND "${fixture}/${player_name}" --rd
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
    ENCODING UTF-8 TIMEOUT 30
)
set(config_written FALSE)
if(EXISTS "${fixture}/config/plugin.config")
    set(config_written TRUE)
endif()
file(REMOVE_RECURSE "${fixture}")
if(NOT result EQUAL 0 OR NOT errors STREQUAL "")
    message(FATAL_ERROR "Plugin scan failed (${result}): ${errors}")
endif()
if(NOT config_written)
    message(FATAL_ERROR "Plugin scan did not save its configuration under the Unicode application path")
endif()
foreach(text "插件扫描完成，已保存 0 个插件到:" "console-output-测试")
    string(FIND "${output}" "${text}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "Missing UTF-8 scan text '${text}': ${output}")
    endif()
endforeach()
