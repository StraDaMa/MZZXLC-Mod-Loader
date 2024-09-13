#pragma once
#include <string>
#include <boost/unordered_map.hpp>

bool stage1_install();
bool stage1_uninstall();

#include <filesystem>

extern boost::unordered_map<std::filesystem::path, std::string> _assetReplacements;