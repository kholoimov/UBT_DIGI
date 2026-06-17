cmake_minimum_required(VERSION 3.16)
project(UBTScintillatorDigi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Geant4 REQUIRED ui_all vis_all)

include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(ubt_scintillator main.cc ${sources} ${headers})
target_link_libraries(ubt_scintillator ${Geant4_LIBRARIES})

file(GLOB macro_files ${PROJECT_SOURCE_DIR}/macros/*.mac)
file(COPY ${macro_files} DESTINATION ${PROJECT_BINARY_DIR}/macros)

file(COPY ${PROJECT_SOURCE_DIR}/README.md DESTINATION ${PROJECT_BINARY_DIR})

install(TARGETS ubt_scintillator DESTINATION bin)
