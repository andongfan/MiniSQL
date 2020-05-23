add_library(catalog_manager
    CatalogManager.cpp
    Table.cpp
    Attribute.cpp
)

target_include_directories(catalog_manager
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)