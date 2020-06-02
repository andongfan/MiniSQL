add_library(buffer_manager
    BufferManager.cpp
    Page.cpp
)

target_include_directories(buffer_manager
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)