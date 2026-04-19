# MSVC 编译警告 & 行为

add_compile_options(
    /W3
    /permissive-
    /Zc:__cplusplus
    /EHsc
    /MP
    /std:c++20
)

# 禁用烦人的 CRT 警告
add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    _UNICODE
    UNICODE
)


# Debug

add_compile_options(
    $<$<CONFIG:Debug>:/Od>
    $<$<CONFIG:Debug>:/Zi>
    $<$<CONFIG:Debug>:/MDd>
)

add_link_options(
    $<$<CONFIG:Debug>:/DEBUG>
)


# Release

add_compile_options(
    $<$<CONFIG:Release>:/O2>
    $<$<CONFIG:Release>:/Ob2>
    $<$<CONFIG:Release>:/MD>
    $<$<CONFIG:Release>:/fp:fast>   # DSP 友好
)




