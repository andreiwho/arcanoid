set CONAN_USER_HOME=%cd%\vendor\Debug\
conan install . -if vendor -s build_type=Debug -b glfw -b freetype -b openal -s compiler.runtime=MTd
cmake . -B build