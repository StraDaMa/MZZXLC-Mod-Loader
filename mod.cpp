#include "mod.h"
#include "inih/cpp/INIReader.h"
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

constexpr semver::version modloader_version{ 0, 1, 0};

namespace fs = std::filesystem;

void findMods(const fs::path& modsDir, std::vector<ModInfo>& mods) {
	mods.clear();
	if (fs::is_directory(modsDir)) {
		for (const auto& entry : fs::directory_iterator(modsDir)) {
			if (entry.is_directory()) {
				const fs::path modPath = entry.path();
				fs::path iniPath = modPath;
				iniPath /= "modinfo.ini";
				ModInfo mod;
				mod.name = modPath.filename().u8string();
				mod.version_requirement = { 0, 0, 0 };
				if (fs::exists(iniPath)) {
					//initialize by ini
					INIReader reader(iniPath.u8string());
					if (reader.ParseError() <  0) {
						mod.oldMod = true;
						mod.title = mod.name;
						mod.description = mod.name;
					}
					else {
						mod.oldMod = true;
						mod.title = reader.Get("", "name", mod.name);
						mod.description = reader.Get("", "description", mod.name);
						mod.authors.push_back(reader.Get("", "author", "None"));
					}
				}
				else {
					fs::path tomlPath = modPath;
					tomlPath /= "info.toml";
					if (fs::exists(tomlPath)) {
						//initialize by toml
						mod.oldMod = false;
						toml::parse_result result = toml::parse_file(tomlPath.u8string());
						if (result) {
							toml::table& tbl = result;
							mod.title = tbl["title"].value_or(mod.name);
							mod.description = tbl["description"].value_or(mod.name);
							std::optional<semver::version> semver_result = semver::from_string_noexcept(tbl["requires_loader_version"].value_or("0.0.0"));
							if (semver_result) {
								mod.version_requirement = semver_result.value();
							}
							
							toml::array* authors = tbl["authors"].as_array();
							if (authors) {
								for (const auto& auth : *authors) {
									auto authStr = auth.as_string();
									mod.authors.emplace_back(**authStr);
								}
							}
						}
						else {
							mod.oldMod = true;
							mod.title = mod.name;
							mod.description = mod.name;
						}
					}
					else {
						mod.oldMod = true;
						mod.title = mod.name;
						mod.description = mod.name;
					}
				}
				mods.emplace_back(std::move(mod));
			}
		}
	}
}

bool checkCompatibility(const semver::version& v) {
	return modloader_version >= v;
}

const semver::version getLoaderVersion() {
	return modloader_version;
}