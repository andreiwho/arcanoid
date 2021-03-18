cmake_minimum_required(VERSION 3.10)

cmake_policy(SET CMP0091 NEW)
project(arcanoid CXX)
    include(vendor/conanbuildinfo.cmake)
    conan_basic_setup()

    file(GLOB_RECURSE sources src/*.cpp)
    file(GLOB_RECURSE headers src/*.h)

    add_executable(arcanoid ${sources} ${headers})
    target_link_libraries(arcanoid PUBLIC ${CONAN_LIBS})
    target_compile_features(arcanoid PUBLIC cxx_std_20)
    target_include_directories(arcanoid PUBLIC src glm)

    set_target_properties(arcanoid
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/res
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")