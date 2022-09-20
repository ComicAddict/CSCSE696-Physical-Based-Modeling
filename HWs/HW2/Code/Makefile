all: rm compile

compile:
	set OPENGL_LIB_DIR = C:\Libs\opengl32.dll
	set GLFW_LIB_DIR=C:\Libs\glfw3.lib
	clang main.cpp --library-directory=%OPENGL_LIB_DIR% --library-directory=%GLFW_LIB_DIR% -o a.exe -x c++

rm:
	rm a.exe