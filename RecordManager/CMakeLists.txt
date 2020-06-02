add_library(record_manager
    RecordManager.cpp
)

target_include_directories(record_manager
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(record_manager
    PUBLIC API interpreter buffer_manager
)