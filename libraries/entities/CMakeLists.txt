set(TARGET_NAME entities)
setup_hifi_library(Network Script)
link_hifi_libraries(avatars shared audio octree model fbx networking animation)

target_bullet()

include_hifi_library_headers(render)
add_dependency_external_projects(leopoly)
find_package(LeoPoly REQUIRED)
target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${LEOPOLY_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${POLYVOX_LIBRARIES} ${LEOPOLY_LIBRARIES} ${LEOPOLY_LIBCURL})