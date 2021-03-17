cmake_minimum_required(VERSION 3.10)

project(arcanoid CXX)
    include(vendor/conanbuildinfo.cmake)
    conan_basic_setup()

    add_executable(arcanoid src/main.cpp src/glad.cpp)
    target_link_libraries(arcanoid PUBLIC ${CONAN_LIBS})
    target_compile_features(arcanoid PUBLIC cxx_std_20)
    target_include_directories(arcanoid PUBLIC src glm)