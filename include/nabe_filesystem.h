#pragma once

#ifdef _WIN32
#include <filesystem>
namespace fs = std::filesystem;
#else
#if(0)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#endif
