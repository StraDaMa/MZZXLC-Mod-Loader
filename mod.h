#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "semver/semver.hpp"

struct ModInfo
{
	bool oldMod;
	bool enabled;
	std::string title;
	std::string name;
	std::string description;
	std::vector<std::string> authors;
	semver::version version_requirement;
};

void findMods(const std::filesystem::path& modsDir, std::vector<ModInfo>& mods);

bool checkCompatibility(const semver::version& v);

const semver::version getLoaderVersion();