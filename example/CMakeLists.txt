
add_executable(sample-prompt prompt.cc)
target_link_libraries(sample-prompt
    PUBLIC zsh
    PRIVATE project_options project_warnings
)
source_group("" REGULAR_EXPRESSION "*.cc")
