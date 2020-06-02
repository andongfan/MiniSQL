cmake_minimum_required(VERSION 3.5)

project(mini-sql)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(API)
add_subdirectory(BufferManager)
add_subdirectory(CatalogManager)
add_subdirectory(Interpreter)
add_subdirectory(RecordManager)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC API
)