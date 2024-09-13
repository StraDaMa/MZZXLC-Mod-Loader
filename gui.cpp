#include "gui.h"
#include <cstdio>
#include <filesystem>
#include <boost/container/set.hpp>

#include "shellapi.h"
#include "CommCtrl.h"

namespace fs = std::filesystem;


enum ControlIDs {
	PlayButtonID = 0x100,
	RefreshButtonID,
	OpenModDirButtonID,
	ModListID
};


static bool sPlayClicked = false;

static HWND sWindowHwnd = NULL;
static HWND sListViewHwnd = NULL;

static HFONT sWindowFont = nullptr;
static HFONT sPlayButtonFont = nullptr;

static std::vector<ModInfo>* sMods;
static boost::container::set<std::string>* sLoaderMods;

static HWND hwndTitleField = NULL;
static HWND hwndAuthorField = NULL;
static HWND hwndVersionWarning = NULL;
static HWND hwndDescriptionField = NULL;

static void ConvertToWString(const std::string& str, std::wstring& wsTmp) {
	int numChar = MultiByteToWideChar(
		CP_UTF8,
		0,
		str.data(),
		-1,
		NULL,
		0
	);
	wsTmp.resize(numChar);
	MultiByteToWideChar(
		CP_UTF8,
		0,
		str.data(),
		-1,
		wsTmp.data(),
		numChar
	);
}

static void RefreshListBoxItems() {
	std::vector<ModInfo>& mods = *sMods;
	boost::container::set<std::string>& loaderMods = *sLoaderMods;

	const fs::path modsDir = "mods";
	findMods(modsDir, mods);

	ListView_DeleteAllItems(sListViewHwnd);
	LVITEM item = { 0 };
	item.mask = LVIF_TEXT | LVIF_STATE;
	std::wstring wsTmp;
	
	for (size_t i = 0; i < mods.size(); i++)
	{
		item.iItem = i;
		const std::string& modName = mods[i].name;
		ConvertToWString(modName, wsTmp);
		bool modChecked = loaderMods.count(modName) != 0;
		mods[i].enabled = modChecked;
		item.pszText = (LPWSTR)wsTmp.c_str();
		ListView_InsertItem(sListViewHwnd, &item);
		ListView_SetCheckState(sListViewHwnd, i, modChecked);
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_CREATE:
	{
		// Create Fonts
		NONCLIENTMETRICS metrics{ 0 };
		metrics.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
		LOGFONTW mLogFont = metrics.lfMessageFont;
		sWindowFont = CreateFontIndirect(&mLogFont);
		mLogFont.lfHeight = 28;
		mLogFont.lfWeight = 28;
		sPlayButtonFont = CreateFontIndirect(&mLogFont);
		RECT clRect;
		GetClientRect(hWnd, &clRect);
		//Create ListBox
		sListViewHwnd = CreateWindow(
			WC_LISTVIEW,
			L"",
			WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
			0, 25,
			230, clRect.bottom - 30 - 25,
			hWnd,
			(HMENU)ControlIDs::ModListID,
			NULL,
			NULL);
		ListView_SetExtendedListViewStyle(sListViewHwnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
		LVCOLUMN col = { 0 };
		col.mask = LVCF_WIDTH;
		col.cx = 230;
		ListView_InsertColumn(sListViewHwnd, 0, &col);
		RefreshListBoxItems();
		// Create Play Button
		HWND hwndButton = CreateWindow(
			L"BUTTON",                                              // Predefined class; Unicode assumed
			L"Play",                                                // Button text
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles
			0, clRect.bottom - 30,                                  // x,y position
			230, 30,                                                // Button width,height
			hWnd,                                                   // Parent window
			(HMENU)ControlIDs::PlayButtonID,                        // No menu.
			NULL,
			NULL);
		SendMessage(hwndButton, WM_SETFONT, (WPARAM)sPlayButtonFont, TRUE);
		// Create Refresh Button
		HWND hwndRefreshButton = CreateWindow(
			L"BUTTON",                                              // Predefined class; Unicode assumed
			L"Refresh",                                             // Button text
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,                     // Styles
			0, 0,                                                   // x,y position
			115, 25,                                                // Button width,height
			hWnd,                                                   // Parent window
			(HMENU)ControlIDs::RefreshButtonID,                     // No menu.
			NULL,
			NULL);
		SendMessage(hwndRefreshButton, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		// Create Open Mod Dir Button
		HWND hwndOpenDirButton = CreateWindow(
			L"BUTTON",                                              // Predefined class; Unicode assumed
			L"Open Folder",                                         // Button text
			WS_TABSTOP | WS_VISIBLE | WS_CHILD,                     // Styles
			115, 0,                                                 // x,y position
			115, 25,                                                // Button width,height
			hWnd,                                                   // Parent window
			(HMENU)ControlIDs::OpenModDirButtonID,                  // No menu.
			NULL,
			NULL);
		SendMessage(hwndOpenDirButton, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		//Create title label
		HWND hwndTitleLabel = CreateWindow(
			L"STATIC",                         // Predefined class; Unicode assumed
			L"Title:",                         // Button text
			WS_VISIBLE | WS_CHILD | SS_RIGHT,  // Styles
			350, 10,                           // x,y position
			80, 20,                            // Button width,height
			hWnd,                              // Parent window
			NULL,                              // No menu.
			NULL,
			NULL);
		//Create authros label
		HWND hwndAuthorsLabel = CreateWindow(
			L"STATIC",                         // Predefined class; Unicode assumed
			L"Authors:",                       // Button text
			WS_VISIBLE | WS_CHILD | SS_RIGHT,  // Styles
			350, 50,                           // x,y position
			80, 20,                            // Button width,height
			hWnd,                              // Parent window
			NULL,                              // No menu.
			NULL,
			NULL);
		//Create description label
		HWND hwndDescriptionLabel = CreateWindow(
			L"STATIC",                        // Predefined class; Unicode assumed
			L"Description:",                  // Button text
			WS_VISIBLE | WS_CHILD | SS_LEFT,  // Styles
			230, 200,                         // x,y position
			80, 20,                           // Button width,height
			hWnd,                             // Parent window
			NULL,                             // No menu.
			NULL,
			NULL);
		SendMessage(hwndTitleLabel, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		SendMessage(hwndAuthorsLabel, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		SendMessage(hwndDescriptionLabel, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		//Create title field
		hwndTitleField = CreateWindow(
			L"STATIC",                        // Predefined class; Unicode assumed
			L"",                              // Button text
			WS_VISIBLE | WS_CHILD | SS_LEFT,  // Styles
			430 + 4, 10,                      // x,y position
			clRect.right - 430 + 4, 40,       // Button width,height
			hWnd,                             // Parent window
			NULL,                             // No menu.
			NULL,
			NULL);
		//Create authors field
		hwndAuthorField = CreateWindow(
			L"STATIC",                        // Predefined class; Unicode assumed
			L"",                              // Button text
			WS_VISIBLE | WS_CHILD | SS_LEFT,  // Styles
			430 + 4, 50,                      // x,y position
			clRect.right - 430 + 4, 40,       // Button width,height
			hWnd,                             // Parent window
			NULL,                             // No menu.
			NULL,
			NULL);
		//Create version warning field
		hwndVersionWarning = CreateWindow(
			L"STATIC",                        // Predefined class; Unicode assumed
			L"",                              // Button text
			WS_VISIBLE | WS_CHILD | SS_CENTER,// Styles
			230, 90,                          // x,y position
			clRect.right - (230) + 4, 40,     // Button width,height
			hWnd,                             // Parent window
			NULL,                             // No menu.
			NULL,
			NULL);
		//Create description field
		hwndDescriptionField = CreateWindow(
			L"EDIT",                                                                                    // Predefined class; Unicode assumed
			L"",                                                                                        // Button text
			WS_VISIBLE | WS_BORDER | WS_CHILD | ES_READONLY | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,  // Styles
			230, 220,                                                                                   // x,y position
			clRect.right - 230, clRect.bottom - 220,                                                    // Button width,height
			hWnd,                                                                                       // Parent window
			NULL,                                                                                       // No menu.
			NULL,
			NULL);
		SendMessage(hwndTitleField, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		SendMessage(hwndAuthorField, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		SendMessage(hwndDescriptionField, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
		SendMessage(hwndVersionWarning, WM_SETFONT, (WPARAM)sWindowFont, TRUE);
	}
	break;
	case WM_NOTIFY:
	{
		LPNMHDR lpnmhdr = (LPNMHDR)lParam;
		// Check for listbox events
		if ((lpnmhdr->idFrom == ControlIDs::ModListID) && (lpnmhdr->code == LVN_ITEMCHANGED)) {
			NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
			int selectedIndex = pnmv->iItem;
			if (selectedIndex != -1) {
				// Get the struct for the selected mod
				ModInfo& mod = (*sMods)[selectedIndex];
				// Check if this event is because the selected item has changed
				if (pnmv->uNewState & (LVIS_FOCUSED | LVIS_SELECTED)) {
					// The control requires a wstring but the struct has strings so this janky conversion is needed at the moment
					std::wstring wsTmp;
					ConvertToWString(mod.title, wsTmp);
					SendMessage(hwndTitleField, WM_SETTEXT, NULL, (LPARAM)wsTmp.c_str());
					{
						std::string authors;
						authors.reserve(128);
						for (size_t i = 0; i < mod.authors.size(); i++)
						{
							if (i != 0) {
								authors += ", ";
							}
							authors += mod.authors[i];
						}
						ConvertToWString(authors, wsTmp);
						SendMessage(hwndAuthorField, WM_SETTEXT, NULL, (LPARAM)wsTmp.c_str());
					}
					ConvertToWString(mod.description, wsTmp);
					SendMessage(hwndDescriptionField, WM_SETTEXT, NULL, (LPARAM)wsTmp.c_str());
					wsTmp.clear();
					if(!checkCompatibility(mod.version_requirement)) {
						const std::string loaderVersionStr = getLoaderVersion().to_string();
						const std::string modRequirementStr = mod.version_requirement.to_string();
						int outStringlength = snprintf(nullptr, 0,
							"Loader Version (%s) does not match mod requirement's (%s)",
							loaderVersionStr.c_str(),
							modRequirementStr.c_str());
						if (outStringlength > 0) {
							std::string versionWarning(outStringlength, (char)0x00);
							snprintf(versionWarning.data(), outStringlength+1,
								"Loader Version (%s) does not match mod requirement's (%s)",
								loaderVersionStr.c_str(),
								modRequirementStr.c_str());
							ConvertToWString(versionWarning, wsTmp);
						}
					}
					SendMessage(hwndVersionWarning, WM_SETTEXT, NULL, (LPARAM)wsTmp.c_str());
				}
				// Check if this event is because the checkbox state was changed
				int imageState = pnmv->uNewState & LVIS_STATEIMAGEMASK;
				if (imageState) {
					// Theres 2 states for the checkbox, unchecked (1<<12) or checked (2<<12)
					mod.enabled = imageState == (2 << 12);
				}
			}
			else {
				SendMessage(hwndTitleField, WM_SETTEXT, NULL, (LPARAM)L"");
				SendMessage(hwndAuthorField, WM_SETTEXT, NULL, (LPARAM)L"");
				SendMessage(hwndDescriptionField, WM_SETTEXT, NULL, (LPARAM)L"");
				SendMessage(hwndVersionWarning, WM_SETTEXT, NULL, (LPARAM)L"");
			}
		}
		if (lpnmhdr->code == NM_CUSTOMDRAW) {
			LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
			switch (lplvcd->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:
			{
				return CDRF_NOTIFYITEMDRAW;
			}
			break;
			case CDDS_ITEMPREPAINT:
			{
				std::vector<ModInfo>& mods = *sMods;
				const ModInfo& mod = mods[(int)lplvcd->nmcd.dwItemSpec];
				if (!checkCompatibility(mod.version_requirement)) {
					lplvcd->clrText = RGB(0, 0, 0);
					lplvcd->clrTextBk = RGB(255, 0, 0);
				}
				return CDRF_NEWFONT;
			}
			break;
			}
		}
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case ControlIDs::PlayButtonID:
			sPlayClicked = true;
			DestroyWindow(hWnd);
		break;
		case ControlIDs::RefreshButtonID:
		{
			RefreshListBoxItems();
		}
		break;
		case ControlIDs::OpenModDirButtonID:
		{
			ShellExecuteW(
				hWnd,
				L"open",
				L"mods",
				NULL,
				NULL,
				SW_SHOW
			);
		}
		break;
		}
	}
	break;
	case WM_CTLCOLORSTATIC:
	{
		HWND controlHWND = (HWND)lParam;
		HDC hdc = (HDC)(wParam);
		if (controlHWND == hwndVersionWarning) {
			SetTextColor(hdc, RGB(255, 0, 0));
			SetBkMode(hdc, TRANSPARENT);
			//SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
			return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
		}
		else if (controlHWND == hwndDescriptionField) {
			return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
	break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

bool gui_WinMain(HINSTANCE hInstance, std::vector<ModInfo>* mods, boost::container::set<std::string>* loaderMods) {
	sMods = mods;
	sLoaderMods = loaderMods;

	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);;
	wcex.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"MODLOADER_WINDOW";
	wcex.hIconSm = NULL;
	RegisterClassExW(&wcex);

	INITCOMMONCONTROLSEX icex = {0};
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);
	{
		const std::string loaderVersionStr = getLoaderVersion().to_string();
		std::string titleBarStr = "MZZXLC Mod Loader v";
		titleBarStr += loaderVersionStr;
		std::wstring wsTemp;
		ConvertToWString(titleBarStr, wsTemp);
		sWindowHwnd = CreateWindowW(
			L"MODLOADER_WINDOW",
			wsTemp.c_str(),
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_DLGFRAME | WS_MINIMIZEBOX,
			CW_USEDEFAULT, 0,
			800, 600,
			nullptr,
			nullptr,
			hInstance,
			nullptr);
	}

	ShowWindow(sWindowHwnd, SW_SHOWNA | SW_SHOWMINIMIZED);
	UpdateWindow(sWindowHwnd);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(sWindowFont);
	DeleteObject(sPlayButtonFont);
	return sPlayClicked;
}