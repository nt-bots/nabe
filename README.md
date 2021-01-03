# NABE

*"Navigate A → B Externally"* — An external pathfinder for accompanying the SRCDS server side NT Bots plugin. WIP.

## License

See the file [LICENSE.md](LICENSE.md) in this repository for details.

## Installing

You can find builds in the [Releases page](https://github.com/nt-bots/nabe/releases), or you can build it yourself with the instructions below.

## Building

### Requirements

* C++17 compatible compiler

Make sure you have [CMake](https://cmake.org/) installed, and [SQLite3](https://www.sqlite.org) & [Python (3.2 or higher, but not 4.0+)](https://www.python.org/) headers and libraries available.

If you're building on Windows with Visual Studio, note that CMake can also be integrated to the IDE instead of installing it separately.

### Linux

Example build commands for Ubuntu:

```bash
# Ubuntu/APT example - Get Python + Python-dev packages
python --version # Example for Python 3.8
sudo apt-get update && sudo apt-get install python3.8 python3.8-dev

# Get the code, and cd inside the cloned repo root
git clone --recurse-submodules this-repository-url
cd this-repo-name

# Make new dir path inside the repo root
mkdir -p include/thirdparty/sqlite-amalgamation
# ... and cd inside it
cd include/thirdparty/sqlite-amalgamation
# Get SQLite code and build the main amalgamated include
git clone https://github.com/sqlite/sqlite .
sh configure
make sqlite3.c
# cd back to the root repo directory
cd ../../..

# From the repo root, cd into the "build" dir
cd build

# And finally run CMake
cmake ..
cmake --build .
```

### Windows (MSVC/Visual Studio)

#### Compiling the [SQLite3 library](https://www.sqlite.org)

* See the SQLite website for amalgamation releases, or [instructions on building them from source](https://www.sqlite.org/amalgamation.html).
* Place the amalgamation files to the path indicated by [CMakeLists.txt](CMakeLists.txt).

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
