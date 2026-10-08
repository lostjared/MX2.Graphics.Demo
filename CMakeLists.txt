cmake_minimum_required(VERSION 3.20)
project(MX2GraphicsDemo LANGUAGES CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Build with Emscripten: emcmake cmake -S . -B build/web")
endif()

set(MX2_ROOT "$ENV{HOME}/emscripten-libs/mx2" CACHE PATH
    "Emscripten libmx2 installation prefix")
find_path(MX2_INCLUDE_DIR NAMES mx.hpp HINTS "${MX2_ROOT}/include"
    NO_CMAKE_FIND_ROOT_PATH REQUIRED)
find_library(MX2_LIBRARY NAMES mx HINTS "${MX2_ROOT}/lib"
    NO_CMAKE_FIND_ROOT_PATH REQUIRED)
find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp
    HINTS "${MX2_ROOT}/include" /usr/include /usr/local/include
    NO_CMAKE_FIND_ROOT_PATH REQUIRED)
find_package(Perl REQUIRED)

# Isolate GLM from host system headers when it is installed under /usr/include.
file(COPY "${GLM_INCLUDE_DIR}/glm" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/include")

add_executable(MX_app graphics.cpp)
set_target_properties(MX_app PROPERTIES
    SUFFIX ".html"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_include_directories(MX_app PRIVATE
    "${MX2_INCLUDE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include/glm")
target_link_libraries(MX_app PRIVATE "${MX2_LIBRARY}" embind)

set(EMSCRIPTEN_PORT_OPTIONS
    "-sUSE_SDL=2"
    "-sUSE_SDL_MIXER=2"
    "-sUSE_SDL_TTF=2"
    "-sUSE_SDL_IMAGE=2"
    "-sSDL2_IMAGE_FORMATS=[\"png\",\"jpg\"]"
    "-sUSE_LIBJPEG=1"
    "-sUSE_LIBPNG=1"
    "-sUSE_ZLIB=1"
    "-sNO_DISABLE_EXCEPTION_CATCHING"
)
target_compile_options(MX_app PRIVATE ${EMSCRIPTEN_PORT_OPTIONS})
target_link_options(MX_app PRIVATE
    ${EMSCRIPTEN_PORT_OPTIONS}
    "SHELL:--preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/data@/data\""
    "-sALLOW_MEMORY_GROWTH=1"
    "-sASSERTIONS=1"
    "-sENVIRONMENT=web"
    "-sUSE_WEBGL2=1"
    "-sFULL_ES3=1"
    "-sEXPORTED_RUNTIME_METHODS=[\"HEAPU8\",\"FS\"]"
    "-sEXPORTED_FUNCTIONS=[\"_malloc\",\"_free\",\"_main\"]"
    "-sOFFSCREEN_FRAMEBUFFER=1"
    "-sGL_PREINITIALIZED_CONTEXT=1"
    "-sINCOMING_MODULE_JS_API=[\"ENVIRONMENT\",\"arguments\",\"canvas\",\"dynamicLibraries\",\"elementPointerLock\",\"instantiateWasm\",\"locateFile\",\"monitorRunDependencies\",\"noExitRuntime\",\"noInitialRun\",\"onAbort\",\"onExit\",\"onRuntimeInitialized\",\"postRun\",\"preInit\",\"preRun\",\"print\",\"printErr\",\"setStatus\",\"statusMessage\",\"stderr\",\"stdin\",\"stdout\",\"thisProgram\",\"wasm\",\"websocket\",\"wasmBinary\"]"
    "-sASYNCIFY=1"
    "-sASYNCIFY_STACK_SIZE=65536"
)

file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/shaders/*.glsl")
set(SHADER_CACHE_STAMP "${CMAKE_CURRENT_BINARY_DIR}/shader-cache.stamp")
add_custom_command(
    OUTPUT "${SHADER_CACHE_STAMP}"
    COMMAND "${PERL_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/cache_webgl_shaders.pl"
    COMMAND "${CMAKE_COMMAND}" -E touch "${SHADER_CACHE_STAMP}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cache_webgl_shaders.pl"
        "${CMAKE_CURRENT_SOURCE_DIR}/data/shaders/index.txt" ${SHADER_SOURCES}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating WebGL shader cache"
    VERBATIM
)
add_custom_target(shader_cache DEPENDS "${SHADER_CACHE_STAMP}")
add_dependencies(MX_app shader_cache)

file(GLOB_RECURSE DATA_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
set_property(TARGET MX_app APPEND PROPERTY LINK_DEPENDS
    ${DATA_FILES} "${SHADER_CACHE_STAMP}")

foreach(WEB_FILE IN ITEMS index.html server.py)
    configure_file("${WEB_FILE}" "${CMAKE_CURRENT_BINARY_DIR}/${WEB_FILE}" COPYONLY)
endforeach()
