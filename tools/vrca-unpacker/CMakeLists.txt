set(TARGET_NAME vrca-unpacker)
setup_hifi_project(Core Widgets)
setup_memory_debugger()
link_hifi_libraries(shared networking)

find_package(LZ4 REQUIRED)

target_include_directories(${TARGET_NAME} SYSTEM PRIVATE ${LZ4_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${LZ4_LIBRARY})
target_link_libraries(${TARGET_NAME} lz4)
