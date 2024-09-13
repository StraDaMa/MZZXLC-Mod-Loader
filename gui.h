#pragma once
#include "mod.h"

#include <Windows.h>

#include <vector>
#include <string>
#include <boost/container/set.hpp>

extern bool gui_WinMain(HINSTANCE hInstance, std::vector<ModInfo>* mods, boost::container::set<std::string>* loaderMods);