## Building

### Requirements

* C++17 compatible compiler

Make sure you have [CMake](https://cmake.org/), [SQLite3](https://www.sqlite.org), and [Python (3)](https://www.python.org/) libraries available.

If you're building on Windows with Visual Studio, note that CMake can also be integrated to the IDE instead of installing it separately.

### Linux

Then, run the following commands in the repo path.
```
git clone --recurse-submodules this-repository-url
cd inside-this-repo && cd build
cmake ..
cmake --build .
```

### Windows (MSVC/Visual Studio)

#### Compiling the [SQLite3 library](https://www.sqlite.org)

Example tools for building the statically linked SQLite3 library for MSVC:
* x64 Native Tools Command Prompt for VS 2019
* x64_x86 Cross Tools Command Prompt for VS 2019

Building the library from command prompt:
```
cl /c /EHsc sqlite3.c
lib sqlite3.obj
```

#### Generating the CMake project from Visual Studio
See the [related MS docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio).
