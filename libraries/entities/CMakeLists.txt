set(TARGET_NAME entities)
setup_hifi_library(Network Script)
link_hifi_libraries(avatars shared audio octree model fbx networking animation)

target_bullet()

include_hifi_library_headers(render)

include_directories(${3RDPARTY_INCLUDE_DIR})
target_link_libraries(entities ${3RDPARTY_INCLUDE_DIR}/SculptAppDLL.lib)