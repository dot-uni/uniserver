add_library(project_logging STATIC
    src/logging_logger.cc
    src/logging_stream.cc
    src/logging_manager.cc
    src/logging_service.cc
)

add_library(project::logging ALIAS project_logging)

target_include_directories(project_logging PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(project_logging 
    PUBLIC
        project::common
        fmt::fmt
        yaml-cpp::yaml-cpp
        nlohmann_json::nlohmann_json
)