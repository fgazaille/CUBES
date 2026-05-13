# Specify the target system
set(CMAKE_SYSTEM_NAME Windows)

# Specify the cross-compiler (use -posix variant for C++11 thread support)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)

# Where to look for target libraries and headers
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Adjust searching behavior: find programs on host, but libs/headers on target
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
