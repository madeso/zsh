set(src
    zsh/zsh.cc zsh/zsh.h
    zsh/cpp20.cc zsh/cpp20.h
)

add_library(zsh STATIC ${src})
target_link_libraries(zsh
    PRIVATE project_options project_warnings
)
target_include_directories(zsh
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/zsh FILES ${src})
