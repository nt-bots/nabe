## Building

### Requirements

* C++17 compatible compiler

Make sure you have [CMake](https://cmake.org/) installed, and [SQLite3](https://www.sqlite.org) & [Python (3)](https://www.python.org/) headers and libraries available.

If you're building on Windows with Visual Studio, note that CMake can also be integrated to the IDE instead of installing it separately.

### Linux

Run the following commands:
```bash
git clone --recurse-submodules this-repository-url
cd inside-this-repo && cd build
cmake ..
cmake --build .
```

### Windows (MSVC/Visual Studio)

#### Compiling the [SQLite3 library](https://www.sqlite.org)

* See the SQLite website for amalgamation releases, or [instructions on building them from source](https://www.sqlite.org/amalgamation.html).
* Place the amalgamation files to the path indicated by CMakeLists.txt.

Example tools for building the statically linked SQLite3 library for MSVC from the amalgamation object files:
* x64 Native Tools Command Prompt for VS 2019
* x64_x86 Cross Tools Command Prompt for VS 2019

Building the library from command prompt:
```
cl /c /EHsc sqlite3.c
lib sqlite3.obj
```

#### Generating the CMake project from Visual Studio
See the [related MS docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio).
