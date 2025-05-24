#pragma once
#include "mod.h"

#include <Windows.h>

#include <vector>
#include <string>
#include <boost/container/flat_set.hpp>

namespace gui {
	extern bool WinMain(HINSTANCE hInstance, std::vector<ModInfo>* mods, boost::container::flat_set<std::string>* loaderMods);
}