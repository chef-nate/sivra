add_executable(sivra main.cpp)

target_link_libraries(sivra PRIVATE sivra::canonicalizer CLI11::CLI11)
