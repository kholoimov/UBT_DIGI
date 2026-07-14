cmake_minimum_required(VERSION 3.16)
project(UBTScintillatorDigi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(Geant4 REQUIRED ui_all vis_all)
find_package(ROOT REQUIRED COMPONENTS Core Hist Graf Gpad)

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

add_executable(ubt_timing_model_validation tests/validate_timing_model.cc)
target_include_directories(ubt_timing_model_validation PRIVATE
                           ${PROJECT_SOURCE_DIR}/include
                           ${ROOT_INCLUDE_DIRS})
target_link_libraries(ubt_timing_model_validation ${ROOT_LIBRARIES})

add_test(
  NAME timing_model_validation
  COMMAND ubt_timing_model_validation
          --samples 200000
          --output-root-file timing_model_validation.root
          --output-plot-file timing_model_validation.pdf)
