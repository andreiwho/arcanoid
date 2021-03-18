set CONAN_USER_HOME=%cd%\vendor\Release\
conan install . -if vendor -s build_type=Release -b glfw -b freetype -s compiler.runtime=MT
cmake . -B build