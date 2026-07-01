编码风格符合Google C++ Style Guide，参考地址: https://google.github.io/styleguide/cppguide.html。
禁止使用类似#include "../models/xxx.h"的相对路径引入头文件，必须用类似#include "core/models/xxx.h"的绝对路径。
C++使用17、20、23等最新版本的语法。
源码需要支持跨平台编译，必须支持MacOS、Windows、Linux三种操作系统。
源码需要支持MinGW和MSVC两种编译器工具集。
文件名统一用小写自如，例如：functiondata.cpp。
头文件上方要加上详细的文件说明，包括作者、日期、版本、作用等。doxygen风格的注释。
所有头文件中的函数都需要添加中文注释，包括函数参数、返回值、异常等。使用doxygen风格的注释。
类的成员变量需要加上注释说明，例如int a; ///< xxx作用。
源代码的日志使用中文。终端打印中文不能乱码。
CMakeLists文件中，禁止硬编码查找文件。必须使用file(GLOB_RECURSE xxx "src/*.cpp")等命令查找文件。
执行命令时，有任何模糊的地方，需要列举问题让用户确认。