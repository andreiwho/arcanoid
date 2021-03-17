set CONAN_USER_HOME=%cd%\vendor\%1%\
conan install . -if vendor -s build_type=%1% -b glfw -b glew