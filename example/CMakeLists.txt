source_group("" REGULAR_EXPRESSION "*.cc")
source_group("" REGULAR_EXPRESSION "*.h")

set(src
    computer/computer.cc computer/computer.h
)
add_library(computer STATIC ${src})
target_link_libraries(computer
    PUBLIC zsh
    PRIVATE project_options project_warnings
)
target_include_directories(computer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(sample-prompt prompt.cc)
target_link_libraries(sample-prompt
    PUBLIC computer
    PRIVATE project_options project_warnings
)

add_executable(sample-input input.cc)
target_link_libraries(sample-input
    PUBLIC computer
    PRIVATE project_options project_warnings
)
