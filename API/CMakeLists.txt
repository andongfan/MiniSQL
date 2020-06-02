add_library(API
    API.cpp
    SQLExecError.cpp
)

target_include_directories(API
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(API
    PUBLIC interpreter record_manager
)