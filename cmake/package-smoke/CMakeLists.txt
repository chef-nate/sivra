cmake_minimum_required(VERSION 3.25)

project(
  sivra-package-smoke
  LANGUAGES CXX
)

find_package(sivra CONFIG REQUIRED)

add_executable(sivra-package-smoke main.cpp)

target_compile_features(sivra-package-smoke
  PRIVATE
    cxx_std_23
)

target_link_libraries(sivra-package-smoke
  PRIVATE
    sivra::ir
)
