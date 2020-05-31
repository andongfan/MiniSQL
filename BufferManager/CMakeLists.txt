cmake_minimum_required(VERSION 3.5)

project(test_bm)

set(SOURCES
    test.cpp
    BufferManager.cpp
    Page.cpp
)

# Add an executable with the above sources
add_executable(test_bm ${SOURCES})

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories(test_bm
    PRIVATE 
        ${PROJECT_SOURCE_DIR}
)
