#pragma once
#include <string>
#include <filesystem>
#include <boost/unordered/unordered_flat_map.hpp>

namespace stage1 {
	bool install();
	bool uninstall();
	inline boost::unordered::unordered_flat_map<std::filesystem::path, std::string> g_assetReplacements;
}