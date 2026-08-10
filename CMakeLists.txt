cmake_minimum_required(VERSION 3.28)

project(Dualcast VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUALCAST_BUILD_TESTS "Build Dualcast unit tests" ON)
set(DUALCAST_OBS_DIR "" CACHE PATH "OBS Studio build/install prefix containing libobs and obs-frontend-api")
set(DUALCAST_GOOGLE_CLIENT_ID "" CACHE STRING "Google OAuth client id for the YouTube installed-app flow")

if(DUALCAST_OBS_DIR)
  list(PREPEND CMAKE_PREFIX_PATH "${DUALCAST_OBS_DIR}")
endif()

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

add_library(Dualcast MODULE
  src/plugin-main.cpp
  src/core/bandwidth-monitor.cpp
  src/core/encoder-manager.cpp
  src/core/output-manager.cpp
  src/config/configuration.cpp
  src/auth/oauth-manager.cpp
  src/auth/secure-storage.cpp
  src/providers/youtube-provider.cpp
  src/providers/tiktok-provider.cpp
  src/ui/account-dialog.cpp
  src/ui/dualcast-dock.cpp
  src/ui/layout-editor.cpp
  src/ui/output-settings.cpp
  src/ui/stream-health-widget.cpp
)

target_include_directories(Dualcast PRIVATE src)
target_compile_definitions(Dualcast PRIVATE
  DUALCAST_VERSION="${PROJECT_VERSION}"
  DUALCAST_GOOGLE_CLIENT_ID="${DUALCAST_GOOGLE_CLIENT_ID}"
)
target_compile_features(Dualcast PRIVATE cxx_std_20)
set_target_properties(Dualcast PROPERTIES AUTOMOC ON AUTOUIC ON)

target_link_libraries(Dualcast PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
  Qt6::Network
)

if(WIN32)
  target_link_libraries(Dualcast PRIVATE Crypt32)
endif()

if(MSVC)
  target_compile_options(Dualcast PRIVATE /W4 /permissive-)
else()
  target_compile_options(Dualcast PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS Dualcast LIBRARY DESTINATION obs-plugins/64bit RUNTIME DESTINATION obs-plugins/64bit)
install(DIRECTORY resources/ DESTINATION data/obs-plugins/Dualcast)

if(DUALCAST_BUILD_TESTS)
  enable_testing()
  add_executable(dualcast-core-tests
    tests/bandwidth-monitor-test.cpp
    src/core/bandwidth-monitor.cpp
  )
  target_include_directories(dualcast-core-tests PRIVATE src)
  target_link_libraries(dualcast-core-tests PRIVATE Qt6::Core)
  add_test(NAME dualcast-core-tests COMMAND dualcast-core-tests)
endif()
