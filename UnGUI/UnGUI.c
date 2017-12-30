#include <windows.h>
#include <commctrl.h>
#include "./UnGUI.h"
#include "./ini/src/ini.c"
#include "./CommandLineToArgvA.c"

#define PROG_TITLE			"Universal GUI"
#define PROG_TITLE_SHORT	"UnGUI"
#define PROG_VERSION		"v0.0.1.41"
#define PROG_HELP_TEXT      "Universal GUI v0.0.1.41\n\
(C) AHOHNMYC, 2017\n\
https://GitHub.com/AHOHNMYC\n\n\
-c <file>\t\tload Config from file\n\
-t <0|1>\t\ton Top\n\
-l <abbr|code>\tLanguage\n\
-e <file>\t\tEmbed config in resources"

HWND hwnd, nameEdit, keyEdit, exePathEdit, startBtn, cmdLineEdit;
ini_t* ini_file;
accepted_params cmd = {0};
runtime_options runt_opt = {0};

LPCSTR ini_check_section(ini_t *ini, LPCSTR section) {
	LPSTR current_section = "";
	LPSTR p = ini->data;

	if (*p == '\0') p = next(ini, p);

	while (p < ini->end) {
		if (*p == '[') {
			/* Handle section */
			current_section = p + 1;
			if (!strcmpi(section, current_section)) {
				return p;
			}
		} else {
			next(ini, p);
		}
		p = next(ini, p);
	}
	return NULL;
}

BOOL strcmpi_mul (LPCSTR str, int count, ...) {
	LPSTR str2;

	if (str == NULL)
		return FALSE;

	va_list args;
	va_start(args, count);
	while (count--) {
		str2 = va_arg(args, LPSTR);
		if (str2 == NULL) {
			va_end(args);
			return FALSE;
		}
		if ( !strcmpi(str, str2) ) {
			va_end(args);
			return TRUE;
		}
	}
	va_end(args);	
	return FALSE;
}

BOOL ini_is_true (ini_t *ini, LPCSTR section, LPCSTR key) {
	return strcmpi_mul(ini_get(ini, section, key), 2, "1", "true");
}

void exitProgram(int retCode) {
	ShowWindow(hwnd, SW_HIDE);
	ExitProcess(retCode);
}

/* economy on stack pushing */
HWND cw_ex(DWORD exStyle, LPSTR classw, LPSTR name, DWORD style, int x, int y, int w, int h) {
	return CreateWindowExA(exStyle, classw, name, style, x, y, w, h, hwnd, NULL, NULL, NULL);
}
HWND cw(LPSTR classw, LPSTR name, DWORD style, int x, int y, int w, int h) {
	return cw_ex(0, classw, name, style, x, y, w, h);
}



HWND CreateToolTip(HWND hwndTool, HWND hDlg, PTSTR pszText) {
    // Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
								WS_POPUP | TTS_BALLOON | TTS_NOFADE,
								CW_USEDEFAULT, CW_USEDEFAULT,
								CW_USEDEFAULT, CW_USEDEFAULT,
								hDlg, NULL,
								NULL, NULL);
    
	if (!hwndTool || !hwndTip) {
		return (HWND)NULL;
	}                              
                              
	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = {0};
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);


// Remove delays in displaying
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_INITIAL, 0);
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_RESHOW, 0);
// Hide after 15 seconds. Fuck yeah
	SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 15000);

	
	return hwndTip;
}

// also used in __main
DWORD windStyles = WS_CAPTION|WS_SYSMENU;

typedef struct { int x,y,w,h; } WPOS;
WPOS getCoordsForCenter(int width, int height) {
	int x = width, y = height, dx, dy;
	RECT rectWnd = {0,0,x,y};

	AdjustWindowRect(&rectWnd, windStyles, FALSE);

	x = rectWnd.right  - rectWnd.left;
	y = rectWnd.bottom - rectWnd.top;
	dx = (GetSystemMetrics(SM_CXSCREEN) - rectWnd.right )/2;
	dy = (GetSystemMetrics(SM_CYSCREEN) - rectWnd.bottom)/2;

	WPOS w = {dx,dy,x,y};
	return w;
}


COORD coords = {20, 0};


#define TYPE_CHECKBOX 	0x01
#define TYPE_EDIT		0x02
#define TYPE_STATIC		0x04
#define TYPE_RADIO		0x08
#define TYPE_BUTTON		0x10
#define TYPE_COMBOBOX	0x20

void initHandles(groupOfParamsEntry* parEn, DWORD groupID) {
	char type[10];
	DWORD style = WS_CHILD|WS_VISIBLE|WS_TABSTOP;
	DWORD exStyle = 0;
	long typeFlag = TYPE_CHECKBOX;

	       if ( !strcmpi(parEn->type, "checkbox") ) {
		typeFlag = TYPE_CHECKBOX;
	} else if ( !strcmpi(parEn->type, "edit") ) {
		typeFlag = TYPE_EDIT;
	} else if ( !strcmpi(parEn->type, "static") ) {
		typeFlag = TYPE_STATIC;
	} else if ( !strcmpi(parEn->type, "radio") ) {
		typeFlag = TYPE_RADIO;
	} else if ( !strcmpi(parEn->type, "button") ) {
		typeFlag = TYPE_BUTTON;
	} else if ( !strcmpi(parEn->type, "combobox") ) {
		typeFlag = TYPE_COMBOBOX;
	}

	strcpy(type, parEn->type);
	POINT wh = {180, 15};

	switch (typeFlag) {
		case TYPE_CHECKBOX:
			strcpy(type, "button");
			style |= BS_AUTOCHECKBOX;
			break;
		case TYPE_RADIO:
			strcpy(type, "button");
			style |= BS_RADIOBUTTON;
			break;
		case TYPE_EDIT:
			style |= WS_BORDER|ES_AUTOHSCROLL;
			exStyle |= WS_EX_ACCEPTFILES;
			wh.y += 2;
			break;
		case TYPE_STATIC:
			style -= WS_TABSTOP;
			break;
		case TYPE_BUTTON:
			break;
		case TYPE_COMBOBOX:
			strcpy(type, "combobox");
			style |= CBS_DROPDOWNLIST;
			break;
	}
	

	if (parEn->key_count > 1) {
		coords.X = 20;
		
		parEn->group_box_hndl = cw("button", parEn->title, WS_CHILD|WS_VISIBLE|BS_GROUPBOX, coords.X-10, coords.Y, 200,
									(typeFlag==TYPE_EDIT||typeFlag==TYPE_COMBOBOX?19:15) * (typeFlag==TYPE_COMBOBOX?1:parEn->key_count) + 30 - (parEn->title[0] == '\0' ? 5:0) )
								;
		
		if (parEn->hint[0] != '\0') {
			#define SQUARE_HELP_BUTTON_SIZE 17
			parEn->group_box_hint_hndls[0] = cw("button", "?", WS_CHILD|WS_VISIBLE|BS_FLAT, 200, coords.Y, SQUARE_HELP_BUTTON_SIZE, SQUARE_HELP_BUTTON_SIZE);
			parEn->group_box_hint_hndls[1] = CreateToolTip(parEn->group_box_hint_hndls[0], hwnd, parEn->hint);
		}
		
		coords.Y += (parEn->title[0] == '\0') ? 15 : 20;
	} else {
//		coords.X  = 10;
		coords.Y += 10;
	}

	DWORD i=0;
	if (typeFlag != TYPE_COMBOBOX) {
		for (; i < parEn->key_count; i++) {
			parEn->handles[i] = CreateWindowExA(exStyle, type,
								(*parEn->values[i] == '\0' && typeFlag != TYPE_EDIT)
									? parEn->keys[i]
									: parEn->values[i],
								style, coords.X, coords.Y, wh.x, wh.y,
								hwnd, (HMENU)groupID, NULL, NULL);

			SendMessage(parEn->handles[i], BM_SETCHECK, parEn->defaults[i], 0);

			if (*parEn->hints[i] != '\0')
				parEn->hint_handles[i] = CreateToolTip(parEn->handles[i], hwnd, parEn->hints[i]);

			if ( typeFlag == TYPE_EDIT )
				coords.Y += 4;
			coords.Y += 15;
		}
	} else {
		parEn->handles[0] = CreateWindowExA(exStyle, type, "", style, coords.X, coords.Y, wh.x, wh.y, hwnd, (HMENU)groupID, NULL, NULL);
		SendMessage(parEn->handles[0], CB_SETCURSEL, 0, 0);

		BOOL defaultChanged = FALSE;
		for (; i < parEn->key_count; i++) {
			SendMessage(parEn->handles[0], CB_ADDSTRING, 0, (LPARAM)(*parEn->values[i]!='\0' ? parEn->values[i] : parEn->keys[i]));
			if (!defaultChanged && parEn->defaults[i]) {
				SendMessage(parEn->handles[0], CB_SETCURSEL, i, 0);
				defaultChanged = TRUE;
			}
		}

		coords.Y += 15;
	}


// huita kakaya-to vyhodit
//		if (typeFlag == TYPE_COMBOBOX) {
//			DlgDirListComboBox(hwnd, "windows", GetDlgCtrlID(parEn->handles[i]), 0, DDL_ARCHIVE);
//		}



	if ( parEn->key_count > 1 || typeFlag!=TYPE_STATIC ) {
		coords.Y += 8;
	}

	MoveWindow(exePathEdit, 10, coords.Y+10,   200, 17, TRUE);
	MoveWindow(cmdLineEdit, 10, coords.Y+29,   200, 17, TRUE);
	MoveWindow(startBtn,    10, coords.Y+48+5, 200, 30, TRUE);
//	MoveWindow(startBtn,    20, coords.Y+48+5, 180, 30, TRUE);

	WPOS w = getCoordsForCenter(220, coords.Y+48+5+30+10);
	SetWindowPos(hwnd, 0, w.x, w.y, w.w, w.h, SWP_NOZORDER);
}

LPCSTR zero_str = "";
LPCSTR ini_get_def (ini_t *ini, LPCSTR section, LPCSTR key, LPCSTR fallback) {
	LPCSTR tmp = ini_get(ini, section, key);
	return tmp==NULL? fallback : tmp;
}
LPCSTR ini_get_def_zero_str (ini_t *ini, LPCSTR section, LPCSTR key) {
	return ini_get_def(ini, section, key, zero_str);
}

groupOfParamsEntry* initGroup(LPSTR group_section_name) {
	groupOfParamsEntry* grEn;
	grEn = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(groupOfParamsEntry));
	
	strcpy(grEn->       title, ini_get_def_zero_str(ini_file, group_section_name, "title"));
	strcpy(grEn->        hint, ini_get_def_zero_str(ini_file, group_section_name, "hint"));
	strcpy(grEn->        type, ini_get_def         (ini_file, group_section_name, "type", "checkbox"));
	strcpy(grEn->group_prefix, ini_get_def_zero_str(ini_file, group_section_name, "group_prefix"));
	strcpy(grEn->      prefix, ini_get_def         (ini_file, group_section_name, "prefix", " "));

//	LPSTR param_value_key	= "value_1\0\0\0";
//	LPSTR param_key_key		= "key_1\0\0\0";
//	LPSTR param_default_key	= "default_1\0\0\0";
//	LPSTR param_hint_key	= "hint_1\0\0\0";

	char param_value_key[20]	= "value_1";
	char param_key_key[20]		= "key_1";
	char param_default_key[20]	= "default_1";
	char param_hint_key[20]		= "hint_1";
	int i = 1;
	while (	(	ini_get(ini_file, group_section_name, param_key_key) != NULL
				|| (strcmpi_mul(grEn->type, 3, "edit", "static", "combobox") && ini_get(ini_file, group_section_name, param_value_key) != NULL)
			) && i <= 256 ) {
		strcpy(grEn->values[i-1], ini_get_def_zero_str(ini_file, group_section_name, param_value_key));
		strcpy(grEn->  keys[i-1], ini_get_def_zero_str(ini_file, group_section_name, param_key_key));
		strcpy(grEn-> hints[i-1], ini_get_def_zero_str(ini_file, group_section_name, param_hint_key));
		grEn->defaults[i-1] = ini_is_true(ini_file, group_section_name, param_default_key);
		
		i++;
		sprintf(param_value_key,	"value_%d",		i);
		sprintf(param_key_key,		"key_%d",		i);
		sprintf(param_default_key,	"default_%d",	i);
		sprintf(param_hint_key,		"hint_%d",		i);
	};
	grEn->key_count = i-1;

	coords.Y += 5;

/*      		                    explosive arithmetic
 *			"param_group_1"
 * 				+12 =>   ^
 *		-1 to sync numbers with group order in parsed_config->groups, which starts from 0
 */	initHandles(grEn, atoi(group_section_name+12)-1);
	
	return grEn;
}

parsed_config* config;

char localeSection[10] = "";
void determineLocaleSection() {
	if (localeSection[0] != '\0') return; // we may set locale from cmd via -l switch

	char locAbbr[4], probabSection[10];
	if ( GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SABBREVCTRYNAME, locAbbr, sizeof(locAbbr)) ) {
		sprintf(probabSection, "lang_%s", locAbbr);
		if ( ini_check_section(ini_file, probabSection) ) {
			strcpy(localeSection, probabSection);
			return;
		}
	}

	sprintf(probabSection, "lang_%d", (int)GetSystemDefaultLCID());
	if ( ini_check_section(ini_file, probabSection) ) {
		strcpy(localeSection, probabSection);
		return;
	}

	if ( ini_check_section(ini_file, "lang") ) {
		strcpy(localeSection, "lang");
		return;
	}
}

LPCSTR getLocalizedStr(LPCSTR key, LPCSTR english_fallback) {
	return ini_get_def(ini_file, localeSection, key, english_fallback);
}

void loadLocale() {
	determineLocaleSection();
	CreateToolTip(exePathEdit, hwnd, (PTSTR)getLocalizedStr("tooltip_edit_exePath", "Path to executable file"));
	CreateToolTip(cmdLineEdit, hwnd, (PTSTR)getLocalizedStr("tooltip_edit_cmdLine", "Generated command line"));
//	SetWindowText(startBtn,                 getLocalizedStr("btn_start",            "&Start"));
	char t[30]; sprintf(t, ">>>         %s         <<<", getLocalizedStr("btn_start", "&Start"));
	SetWindowText(startBtn, t);
	CreateToolTip(startBtn,    hwnd, (PTSTR)getLocalizedStr("tooltip_btn_start",    "Press here"));	
}

void updateCmdline() {
	TBYTE curGroup, curKey;
	LPSTR cmdline = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 0x10000); // 65kb for cmdline Ч norm
	LPSTR cmdlineGroup = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 0x1000); // 4kb for every group

	char buf[256];											// Think about size more
	for (curGroup=0; curGroup < config->group_count; curGroup++) {
		cmdlineGroup[0] = '\0';
		
		if ( !strcmpi(config->groups[curGroup]->type, "combobox") )
				sprintf(cmdlineGroup, "%s%s%s",
						cmdlineGroup,
						config->groups[curGroup]->prefix,
						config->groups[curGroup]->keys[ SendMessage(config->groups[curGroup]->handles[0], CB_GETCURSEL, 0, 0) ] );
		else
			for (curKey=0; curKey < config->groups[curGroup]->key_count; curKey++)
				if ( !strcmpi(config->groups[curGroup]->type, "edit") ) {
					GetWindowText(config->groups[curGroup]->handles[curKey], buf, sizeof(buf) );
					if (buf[0] != '\0')
						sprintf(cmdlineGroup, "%s%s%s", cmdlineGroup, config->groups[curGroup]->prefix, buf);
				} else if ( BST_CHECKED == SendMessage(config->groups[curGroup]->handles[curKey], BM_GETCHECK, 0, 0) )
					sprintf(cmdlineGroup, "%s%s%s",
							cmdlineGroup,
							config->groups[curGroup]->prefix,
							config->groups[curGroup]->keys[curKey]);
		
		if (cmdlineGroup[0] != '\0')
			sprintf(cmdline, "%s%s%s", cmdline, config->groups[curGroup]->group_prefix, cmdlineGroup);
	}
	SetWindowText(cmdLineEdit, cmdline+(cmdline[0]==' '?1:0) ); // +1 to filter firsst space

	LocalFree(cmdlineGroup);
	LocalFree(cmdline);
}

// Returns HLOCAL (pointer to local memory) that contains config data
// Also returns config size in DWORD size variable
HLOCAL getConfig(LPDWORD size) {
	LPCSTR fileName = cmd.config? cmd.config : "config.ini";
	
	OFSTRUCT os;
	HFILE file = OpenFile(fileName, &os, OF_READ);
	
	if (file != HFILE_ERROR) {
		DWORD sizeLo = GetFileSize((HANDLE)file, NULL), sizeHi = 0;
		HLOCAL buf = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeLo+1);
		ReadFile((HANDLE)file, buf, sizeLo, &sizeHi, NULL);

		runt_opt.is_config_in_resources = FALSE;
		runt_opt.config_path = fileName;
		*size = sizeLo;
		return buf;
	}

	HRSRC res = FindResource(NULL, "DEFAULT", "CONFIG");
	if (res != NULL) {
		runt_opt.is_config_in_resources = TRUE;
		*size = SizeofResource(NULL, res);
		return (HLOCAL)LoadResource(NULL, res);
	}

	MessageBox(0, "Config not found.\nEven in resources.\nHonestly, it's strange.\nTry renaming to \"config.ini\".\n\nWe are closing.", PROG_TITLE, MB_ICONWARNING);
	ExitProcess(1);
}

void loadConfig() {
	config = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(parsed_config));
	
	DWORD size;
	HLOCAL file = getConfig(&size);
	ini_file = ini_load(file, size);

	if ( cmd.on_top == 2 || ( cmd.on_top != 1 && ini_is_true(ini_file, "options", "on_top") ) ) {
		runt_opt.is_on_top = TRUE;
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	}
	
	config->do_exit_after_launch = ini_is_true(ini_file, "options", "exit_after_launch");

	char title_result[256];
	LPCSTR title = ini_get_def_zero_str(ini_file, "options", "title");
	if (*title!='\0') sprintf(title_result, "%s - %s", PROG_TITLE_SHORT, title);
	else sprintf(title_result, "%s %s", PROG_TITLE, PROG_VERSION);
	SetWindowText(hwnd, title_result);

	SetWindowText(exePathEdit, ini_get_def_zero_str(ini_file, "options", "command"));
	
	char group_section_name[20] = "param_group_1\0\0\0";
	int i = 1;
	while ( ini_check_section(ini_file, group_section_name) != NULL && i <= 256 ) {
		config->groups[i-1] = initGroup(group_section_name);
		sprintf(group_section_name, "param_group_%d", ++i);
	};
	config->group_count = i-1;

	loadLocale();
//	ini_free(ini_file);
	
	updateCmdline();
}

BOOL UPXcheckAndUnpack(LPCSTR fileName) {
	LPOFSTRUCT os = LocalAlloc(LMEM_ZEROINIT, sizeof(OFSTRUCT));
	HFILE file = OpenFile(fileName, os, OF_READ);
	DWORD size1 = 0;
	LPSTR buf = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 4);
	SetFilePointer((HANDLE)file, 0x1F5, NULL, FILE_BEGIN);
	ReadFile((HANDLE)file, buf, 3, &size1, NULL);
	CloseHandle((HANDLE)file);

	if ( !strcmp(buf, "UPX") ) {
//		MessageBox(0, "File packed with UPX", PROG_TITLE, MB_ICONINFORMATION);
		if ( SearchPath(".", "upx", ".exe", 0, NULL, NULL) ) {
//			MessageBox(0, "UPX found", PROG_TITLE, MB_ICONINFORMATION);

 			STARTUPINFO si = {0};
			si.cb = sizeof(STARTUPINFO);
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;

			PROCESS_INFORMATION pi;

			CHAR params[MAX_PATH+5];
			sprintf(params, " -d %s", fileName);

			CreateProcess("upx.exe", params, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

			while ( WaitForSingleObject(pi.hProcess, 500) == WAIT_TIMEOUT );

			DWORD exitCode;
			GetExitCodeProcess(pi.hProcess, &exitCode);
			if (!exitCode) {
//				MessageBox(0, "Successfully unpacked", PROG_TITLE, MB_ICONINFORMATION);
			} else {
				sprintf(params, "Cannot unpack. Try \"upx -d %s\" manually", fileName);
				MessageBox(0, params, PROG_TITLE, MB_ICONERROR);
				return FALSE;
			}
		} else {
			MessageBox(0, "UPX not found. Goto\nhttps://upx.github.io\nand unpack \"upx.exe\" here", PROG_TITLE, MB_ICONERROR);
			return FALSE;
		}
	} else {
		MessageBox(0, "File NOT packed with UPX", PROG_TITLE, MB_ICONINFORMATION);
	}
	return TRUE;
}

BOOL resourceReplace(HANDLE exeOpenedToUpdate, LPCSTR resFile, LPCSTR resName) {
	OFSTRUCT os;
	HFILE file = OpenFile(resFile, &os, OF_READ);
	DWORD size = GetFileSize((HANDLE)file, NULL), size1 = 0;
	LPSTR buf = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, size+1);
	ReadFile((HANDLE)file, buf, size, &size1, NULL);

//	return UpdateResource(exeOpenedToUpdate, "CONFIG", resName, 0, buf, size+1);
	return UpdateResource(exeOpenedToUpdate, "CONFIG", resName, 1033, buf, size+1); // 1033 Ч 'coz windres by default makes resources in English. We have to change it to 0
}

void resourceEmbed(LPCSTR fileName, LPCSTR fileResName) {
	DeleteFile("UnGUI.tmp");
	MoveFile(fileName, "UnGUI.tmp");
	CopyFile("UnGUI.tmp", fileName, TRUE);

	if ( !UPXcheckAndUnpack(fileName) )
		goto clean_after;

	HANDLE exeOpenedToUpdate = BeginUpdateResource(fileName, FALSE);
	resourceReplace(exeOpenedToUpdate, fileResName, "DEFAULT");
	EndUpdateResource(exeOpenedToUpdate, FALSE);

	if ( SearchPath(".", "upx", ".exe", 0, NULL, NULL) ) {
		ShellExecute(NULL, NULL, "upx.exe", fileName, NULL, SW_HIDE);
	} else {
		MessageBox(0, "UPX not found. Goto\nhttps://upx.github.io\nand unpack \"upx.exe\" here\nto reduce size", PROG_TITLE, MB_ICONINFORMATION);
	}
	
	MessageBox(0, "Con-fug have beeen successfully\nembedded as default.", PROG_TITLE, MB_ICONINFORMATION);

	CHAR comspec[MAX_PATH];
clean_after:
	ExpandEnvironmentStrings("%ComSpec%", comspec, MAX_PATH);
	ShellExecute(NULL, NULL, comspec, "/c ping 127.0.0.1 -n 2 & del UnGUI.tmp", NULL, SW_HIDE);
	ExitProcess(0);
}

int CALLBACK SetChildFont(HWND child, LPARAM font) {
    SendMessageA(child, WM_SETFONT, font, (LPARAM)TRUE);
    return 1;
}

void startExe() {
	char exe[256];
	#define PARAM_SIZE 0x10000
	LPSTR params = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, PARAM_SIZE); // 65kb for cmdline Ч norm
	GetWindowText(exePathEdit, exe, sizeof(exe) );
	GetWindowText(cmdLineEdit, params, PARAM_SIZE);

	LPSTR message =	LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, 0x100);
	DWORD result = ShellExecute(NULL, NULL, exe, params, NULL, SW_SHOWDEFAULT);
	switch (result) {
		case ERROR_FILE_NOT_FOUND:
			sprintf(message, getLocalizedStr("error_no_file", "File \"%s\" not found"), exe);
			break;
		case ERROR_PATH_NOT_FOUND:
			sprintf(message, getLocalizedStr("error_no_path", "Path not found"));
			break;
		default:
			if (result <= 32)
				sprintf(message, getLocalizedStr("error_unknown", "Unknown error 0x%08X"), result);
	}
	if (message[0] != '\0')
		MessageBox(hwnd, message, getLocalizedStr("error", "Error"), MB_ICONERROR|(runt_opt.is_on_top?MB_TOPMOST:0) );
	else
		if (config->do_exit_after_launch)
			exitProgram(0);
	
	LocalFree(message);
	LocalFree(params);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {
		case WM_COMMAND: {
			if ( GetWindowLong((HWND)lParam, GWL_STYLE) & BS_RADIOBUTTON ) {
				int grpID = GetWindowLong((HWND)lParam, GWL_ID);
				int i = config->groups[grpID]->key_count;
				while (i--)
					SendMessage(config->groups[grpID]->handles[i], BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessage((HWND)lParam, BM_SETCHECK, BST_CHECKED, 0);
			}
			
			if (lParam == (LPARAM)startBtn) {
				startExe();
				break;
			}
			// Change text, but not on main edit window, to not bit userentered params
			// And not after click on main "Start" button
			if ( ( HIWORD(wParam) == EN_UPDATE && !( (HWND)lParam == exePathEdit)  && !( (HWND)lParam == cmdLineEdit) )
			     || (HIWORD(wParam) == BN_CLICKED && !( (HWND)lParam == startBtn) )
				 || (HIWORD(wParam) == CBN_SELCHANGE) ) updateCmdline();
			break;
		}
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT: {
			ExitProcess(0);
		}
		case WM_ACTIVATE: {
			SetFocus(startBtn);
			break;
		}
		case WM_SHOWWINDOW: {
			EnumChildWindows(hwnd, (WNDENUMPROC)SetChildFont, (LPARAM)GetStockObject(DEFAULT_GUI_FONT));
			break;
		}
		default:
			return DefWindowProcA(hwnd, Message, wParam, lParam);
	}
	return 0;
}

void showHelpWindow() {
	MessageBox(0, PROG_HELP_TEXT, PROG_TITLE, MB_ICONINFORMATION);
	ExitProcess(0);
}

void analyzeCmdParams () {
	int argc;
	LPCSTR* argv = CommandLineToArgvA(GetCommandLine(), &argc);
	
	char ch;
	int curArgv = 0;
	cmd.optional_count = 0;
	
	while (++curArgv < argc)
		if ( argv[curArgv][0] == '-' && (ch = argv[curArgv][1]) ) {
			if (argv[curArgv][2] != '\0') {
				showHelpWindow();
				continue;
			}
			switch (ch) {
				case 'c': {
					cmd.config = argv[++curArgv];
					break;
				}
				case 't': {
					cmd.on_top = 1+atoi(argv[++curArgv]);
					break;
				}
				case 'l': {
//					cmd.lang = argv[++curArgv];
					sprintf(localeSection, "lang_%s", argv[++curArgv]);
					break;
				}
				case 'a': {
					cmd.autoclose = atoi(argv[++curArgv]);
					break;
				}
				case 'e': {
					resourceEmbed(argv[0], argv[++curArgv]);
					break;
				}
				default: {
					showHelpWindow();
				}
			}
		} else {
			if ( strcmpi_mul(argv[curArgv], 3, "/?", "/h", "/help") ) {
				showHelpWindow();
				continue;
			}

//			char t[30]; sprintf(t, "%s Ч нинужно. Ќахер ты это сюда припЄр?\n”пирай обратно!", argv[curArgv]);
//			MessageBox(0, t, "", MB_ICONEXCLAMATION);
			cmd.optionals[cmd.optional_count] = argv[curArgv];
			cmd.optional_count++;
		}
}

void __main() {
//	InitCommonControls();
	
	WNDCLASSEXA wc = {0};
	MSG msg;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "WindowClass";
	RegisterClassExA(&wc);
	
	hwnd		= cw("WindowClass", "", windStyles, 0,0,0,0);
	exePathEdit	= cw_ex(WS_EX_ACCEPTFILES, "edit",   "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL, 0,0,0,0);
	cmdLineEdit	= cw_ex(WS_EX_ACCEPTFILES, "edit",   "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL, 0,0,0,0);
	startBtn	= cw("button", "", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON/*|BS_DEFCOMMANDLINK*/, 0,0,0,0);

	analyzeCmdParams();
	loadConfig();

	ShowWindow(hwnd, SW_SHOW);

	while(GetMessageA(&msg, NULL, 0, 0) > 0) {
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
			exitProgram(0);
		}
		
	    if(IsDialogMessageA(hwnd, &msg)) continue;
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}
