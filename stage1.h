#pragma once
#include <string>
#include <filesystem>
#include <boost/unordered/unordered_flat_map.hpp>

namespace stage1 {
	bool install();
	bool uninstall();
	extern boost::unordered::unordered_flat_map<std::filesystem::path, std::string> _assetReplacements;
}