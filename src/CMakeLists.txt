include_directories(SYSTEM ${GLIB2_INCLUDE_DIRS})
include_directories(SYSTEM ${LIBXML2_INCLUDE_DIRS})
include_directories(SYSTEM ${APPINDICATOR_INCLUDE_DIRS})
include_directories(SYSTEM ${GTK3_INCLUDE_DIRS})
include_directories(SYSTEM ${GTKMM_INCLUDE_DIRS})
include_directories(SYSTEM ${GSTREAMER_INCLUDE_DIRS})
include_directories(SYSTEM ${GSTREAMERMM_INCLUDE_DIRS})
include_directories(SYSTEM ${CURL_INCLUDE_DIRS})
include_directories(SYSTEM ${LIBNOTIFY_INCLUDE_DIRS})
include_directories(SYSTEM ${LIBMAGIC_INCLUDE_DIRS})

include_directories(SYSTEM third_party)

add_definitions(-Wno-deprecated-declarations)

set(
    PLAYLIST_DECODERS
    m3u_playlist_decoder.cpp
    pls_playlist_decoder.cpp
    asx_playlist_decoder.cpp
    ram_playlist_decoder.cpp
    xspf_playlist_decoder.cpp
)

set(
    THIRD_PARTY
    third_party/pugixml/pugixml.cpp
)

add_executable(
    radiotray-lite
    radiotray-lite.cpp
    tray.cpp
    player.cpp
    about.cpp
    playlist.cpp
    notification.cpp
    config.cpp
    options.cpp
    event_manager.cpp
    ${PLAYLIST_DECODERS}
    ${THIRD_PARTY}
)
target_link_libraries(
    radiotray-lite
    ${LIBXML2_LIBRARIES}
    ${GSTREAMERMM_LIBRARIES}
    ${GTKMM_LIBRARIES}
    ${CURL_LIBRARIES}
    ${APPINDICATOR_LIBRARIES}
    ${LIBNOTIFY_LIBRARIES}
    ${LIBMAGIC_LIBRARIES}
)
set_target_properties(radiotray-lite PROPERTIES COMPILE_FLAGS "-std=c++11")
target_compile_definitions(radiotray-lite PRIVATE -DELPP_DISABLE_DEFAULT_CRASH_HANDLING)
target_compile_definitions(radiotray-lite PRIVATE -DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_compile_definitions(radiotray-lite PRIVATE -DAPP_VERSION="${APP_VERSION}")

install(
    TARGETS radiotray-lite
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(
        radiotray-lite-cli
        radiotray-lite-cli.cpp
        player.cpp
        playlist.cpp
        config.cpp
        options.cpp
        event_manager.cpp
        ${PLAYLIST_DECODERS}
        ${THIRD_PARTY}
        )
    target_link_libraries(
        radiotray-lite-cli
        ${LIBXML2_LIBRARIES}
        ${GSTREAMERMM_LIBRARIES}
        ${CURL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        ${LIBMAGIC_LIBRARIES}
        )
    set_target_properties(radiotray-lite-cli PROPERTIES COMPILE_FLAGS "-std=c++11")
    target_compile_definitions(radiotray-lite-cli PRIVATE -DELPP_DISABLE_DEFAULT_CRASH_HANDLING)
    target_compile_definitions(radiotray-lite-cli PRIVATE -DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
    target_compile_definitions(radiotray-lite-cli PRIVATE -DAPP_VERSION="${APP_VERSION}")

    install(
        TARGETS radiotray-lite-cli
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
endif()
