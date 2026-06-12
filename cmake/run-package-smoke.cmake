set(smoke_root "${SIVRA_BINARY_DIR}/package-smoke")
set(install_prefix "${smoke_root}/prefix")
set(smoke_build "${smoke_root}/build")
set(smoke_configure_options)

if(SIVRA_ENABLE_SANITIZERS)
  list(APPEND smoke_configure_options
    "-DCMAKE_CXX_FLAGS=-fno-omit-frame-pointer -fsanitize=address,undefined"
    "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"
  )
endif()

file(REMOVE_RECURSE "${smoke_root}")

execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    --install "${SIVRA_BINARY_DIR}"
    --prefix "${install_prefix}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "SIVRA package smoke install failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -S "${SIVRA_SMOKE_SOURCE_DIR}"
    -B "${smoke_build}"
    -G "${SIVRA_GENERATOR}"
    "-DCMAKE_BUILD_TYPE=${SIVRA_BUILD_TYPE}"
    "-DCMAKE_CXX_COMPILER=${SIVRA_CXX_COMPILER}"
    "-DCMAKE_PREFIX_PATH=${install_prefix}"
    ${smoke_configure_options}
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "SIVRA package smoke configure failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    --build "${smoke_build}"
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "SIVRA package smoke build failed")
endif()

execute_process(
  COMMAND
    "${CMAKE_COMMAND}"
    -E env
    "ASAN_OPTIONS=detect_leaks=0"
    "${smoke_build}/sivra-package-smoke"
  RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "SIVRA package smoke executable failed with ${run_result}")
endif()
