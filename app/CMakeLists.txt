cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

if (${ARCH} STREQUAL arm64)
  set(RUST_TARGET aarch64-unknown-none)
elseif (${ARCH} STREQUAL x86)
  set(RUST_TARGET x86_64-unknown-none)
else ()
  message(FATAL_ERROR "don't know how to compile Rust for ${ARCH}")
endif()

set(WASMTIME_HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/wasmtime-generated)
set(WASMTIME_HEADER ${WASMTIME_HEADER_DIR}/wasmtime.h)
set(WASMTIME_BINDINGS ${WASMTIME_HEADER_DIR}/bindings.rs)
set(WASMTIME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../wasmtime)
set(WASMTIME_TARGET_ROOT ${CMAKE_CURRENT_BINARY_DIR}/wasmtime-target)
set(WASMTIME_OUTPUT ${WASMTIME_TARGET_ROOT}/${RUST_TARGET}/release/libwasmtime_zephyr.a)

target_sources(app PRIVATE
  src/main.c
  src/wasmtime-platform.c
  src/wasmtime_syscalls.c
)

# TODO: is there a better or more automatic way to include a header dir?
zephyr_include_directories(${WASMTIME_HEADER_DIR})

include(ExternalProject)
find_program(CARGO cargo)

message(wut ${ZEPHYR_BASE})
message(wut ${APPLICATION_SOURCE_DIR})

ExternalProject_Add(
  wasmtime_project
  CONFIGURE_COMMAND   ""
  DOWNLOAD_COMMAND    ""
  INSTALL_COMMAND     ""
  # TODO: what RUSTFLAGS should be copied from C to here
  BUILD_COMMAND
    ${CMAKE_COMMAND} -E env
      BINDINGS=${WASMTIME_BINDINGS}
      RUSTFLAGS=-Crelocation-model=static
      ${CARGO} build
        --release
        --target-dir ${WASMTIME_TARGET_ROOT}
        --target ${RUST_TARGET}
        --color always                        # TODO: how to read this from cmake
        --quiet
  BINARY_DIR          ${WASMTIME_DIR}
  BUILD_ALWAYS        YES
  BUILD_BYPRODUCTS    ${WASMTIME_OUTPUT}
)

add_library(wasmtime_lib STATIC IMPORTED GLOBAL)
add_dependencies(wasmtime_lib wasmtime_project)
set_target_properties(wasmtime_lib PROPERTIES IMPORTED_LOCATION ${WASMTIME_OUTPUT})
# TODO: unsure why this didn't work
# set_target_properties(wasmtime_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${WASMTIME_HEADER_DIR})

target_link_libraries(app PUBLIC wasmtime_lib)

# TODO: is this the best way to specify this? How else to put the rust lib in
# its own partition?
set_property(TARGET zephyr_property_target
             APPEND PROPERTY COMPILE_OPTIONS
             -l libwasmtime_zephyr.a z_libc_partition)

# Use the Rust `cbindgen` tool to generate a C header file from Rust code.
find_program(RUST_CBINDGEN cbindgen)
add_custom_command(
  OUTPUT ${WASMTIME_HEADER}
  COMMAND ${RUST_CBINDGEN} --lang C --output ${WASMTIME_HEADER} ${WASMTIME_DIR}
  DEPENDS wasmtime_lib
)
add_custom_target(cbindgen_header DEPENDS ${WASMTIME_HEADER})
add_dependencies(app cbindgen_header)

# Use the Rust `bindgen` tool to generate Rust bindings for our C header file.
set(APP_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../include/app.h)
find_program(RUST_BINDGEN bindgen)
add_custom_command(
  OUTPUT ${WASMTIME_BINDINGS}
  COMMAND ${RUST_BINDGEN}
    ${APP_HEADER}
    --allowlist-item '^app.*'
    --use-core
    -o ${WASMTIME_BINDINGS}
  DEPENDS ${APP_HEADER}
)
ExternalProject_Add_StepDependencies(wasmtime_project build ${WASMTIME_BINDINGS})

# Use a local wasmtime install to recompile the `sample.wat`
set(CWASM_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../wasmtime/src/sample.cwasm)
set(CWASM_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/../wasmtime/src/sample.wat)
add_custom_command(
  OUTPUT ${CWASM_OUTPUT}
  COMMAND
    cargo run
      --manifest-path /home/alex/code/wasmtime/Cargo.toml
      --features all-arch
      -q
      compile
      --target ${RUST_TARGET}
      ${CWASM_INPUT}
      --wasm reference-types=n
      --wasm component-model=n
      --wasm threads=n
      -o ${CWASM_OUTPUT}
  DEPENDS ${CWASM_INPUT}
)
ExternalProject_Add_StepDependencies(wasmtime_project build ${CWASM_OUTPUT})
