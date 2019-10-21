set(TARGET_NAME webrtc-server-test)
setup_hifi_project(Core)
setup_memory_debugger()
link_hifi_libraries(shared networking embedded-webserver)

# find_package(Qt5WebSockets REQUIRED)

find_package(
  Qt5 COMPONENTS
  Gui Widgets Multimedia Network Qml Quick Script Svg
  ${PLATFORM_QT_COMPONENTS}
  WebChannel WebSockets
)

if (ANDROID)
else ()
  target_webrtc()
endif ()

target_link_libraries(
  ${TARGET_NAME}
  Qt5::Gui Qt5::Network Qt5::Multimedia Qt5::Widgets
  Qt5::Qml Qt5::Quick Qt5::Script Qt5::Svg
  Qt5::WebSockets
  ${PLATFORM_QT_LIBRARIES}
)
