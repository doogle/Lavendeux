#ifdef _WIN32
#define NTDDI_VERSION NTDDI_WIN2K
#define _WIN32_IE 0x0500
#define _WIN32_WINNT 0x0500

    #include <stdio.h>
    #include <windows.h>
    #include <winable.h>
    #include <shellapi.h>
    #include <conio.h>
    #include <Shlobj.h>
    #include <Winuser.h>
    #include <tchar.h>

    #include "language.h"
    #include "lavendeux.h"
    #include "interface_win32.h"
    #include "interface.h"
    #include "settings.h"
    #include "cmdflags.h"

    /* Event globals */
    HWND hWnd;
    HWND hDlg, hEdit;
    NOTIFYICONDATA nid;

    /* Stored callbacks */
    parseCallback _parse_callback;
    exitCallback _exit_callback;

    /* Menu equations */
    wchar_t *stored_entries[MAX_EQUATIONS];

    char prefered_path[MAX_PATH+1];

    /** 
     * Prepare and draw the interface 
     * @param exit_callback Method to be called in event of a quit
     * @param parse_callback Method to be called in event of the hotkeys being pressed
     */
    void init_interface(exitCallback exit_callback, parseCallback parse_callback) {
        int i;
        HICON hIcon;
        HINSTANCE hInstance;
        WNDCLASSEX hClass;

        /* Callback methods */
        _parse_callback = parse_callback;
        _exit_callback = exit_callback;

        /* Init equation stores */
        for (i=0; i<MAX_EQUATIONS; ++i)
            stored_entries[i] = NULL;

        /* No default path yet */
        prefered_path[0] = '\0';

        /* Get module instance */
        hInstance = GetModuleHandle(NULL);
        if (!hInstance)
            error_msg(L"Error while starting", L"Cannot get handle to module", 1);

        /* Load icon */
        hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_ID)); 
        if (!hIcon) {
            exit(EXIT_FAILURE);
        }

        /* Register class */
        hClass.cbSize         = sizeof(hClass);
        hClass.style          = CS_HREDRAW | CS_VREDRAW;
        hClass.lpfnWndProc    = wnd_callback;
        hClass.cbClsExtra     = 0;
        hClass.cbWndExtra     = 0;
        hClass.hInstance      = hInstance;
        hClass.hIcon          = hIcon;
        hClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        hClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        hClass.lpszMenuName   = 0;
        hClass.lpszClassName  = WINDOW_CALLBACK;
        hClass.hIconSm        = hIcon;
        RegisterClassEx(&hClass);

        /* Create the window */
        hWnd = CreateWindowEx(0, WINDOW_CALLBACK, APPLICATION_NAME, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
        if (GetLastError() != 0)
            error_msg(L"Error while starting", L"Cannot get handle to window", 1);

        if (get_setting(SETTING_HOTMOD) == 0 || get_setting(SETTING_HOTKEY) == 0) {
            set_setting(SETTING_HOTMOD, MOD_CONTROL);
            set_setting(SETTING_HOTKEY, VK_SPACE);
        }

        /* Register hotkey */
        if (!RegisterHotKey(hWnd, HOTKEY_ID, get_setting(SETTING_HOTMOD), get_setting(SETTING_HOTKEY)))
            error_msg(L"Error while starting", L"Cannot register hotkey. Is Lavendeux already running?", 1);

        /* Start window */
        ShowWindow(hWnd, 0);
        UpdateWindow(hWnd);

        /* Populate notification data */
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = 100;
        nid.uFlags = NIF_INFO | NIF_ICON | NIF_TIP | NIF_MESSAGE;
        nid.uCallbackMessage = RegisterWindowMessage(WINDOW_CALLBACK);
        nid.hIcon = hIcon;
        nid.dwInfoFlags = NIIF_INFO;
        strcpy(nid.szTip, APPLICATION_NAME);
        if (get_setting(SETTING_SILENTSTART) == SETTING_SILENTSTART_OFF) {
            nid.uFlags |= NIF_INFO;
            strcpy(nid.szInfoTitle, RUNNING_TITLE);
            strcpy(nid.szInfo, RUNNING_MSG);
        }

        /* Add notification icon to tray */
        Shell_NotifyIcon(NIM_ADD, &nid);
        DestroyIcon(hIcon);
    }

    /**
     * Prepare to register a new hothey
     */
    void key_registrar( void ) {
        HINSTANCE hInstance;
        WNDCLASSEX wcex;
        MSG msg;
        int modifiers = 0;
        char text[256];
        char buffer[256];
        unsigned char lpKeyState[256];
        hInstance = GetModuleHandle(NULL);

        /* Register dialog class */
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = key_callback;
        wcex.cbClsExtra     = 0;
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_ID));
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName   = NULL;
        wcex.lpszClassName  = HOTKEY_CALLBACK;
        wcex.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_ID));
        if (!RegisterClassEx(&wcex))
            error_msg(L"Error loading window", L"Can't register class", 1);

        /* Create window */
        hDlg = CreateWindow(
            HOTKEY_CALLBACK,
            _T("Hotkey selection"),
            WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            500, 100,
            NULL,
            NULL,
            hInstance,
            NULL
        );
        if (!hDlg)
            error_msg(L"Error loading window", L"Can't create window", 1);

        /* Add a text element */
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "static", "ST_U", 
            WS_CHILD | WS_VISIBLE | SS_CENTER, 
            0, 0, 500, 100, hDlg, NULL, hInstance, NULL);
        if (!hEdit)
            error_msg(L"Error loading window", L"Can't create label", 1);

        /* Start the dialog */
        if (!hotkey_name(get_setting(SETTING_HOTMOD), get_setting(SETTING_HOTKEY), buffer))
            error_msg(L"Error loading window", L"Invalid hotkey! Try running with the -n flag to restore defaults!", 1);
        sprintf(text, "\nCurrent hotkey is %s\nPlease enter the new hotkey", buffer);
        SetWindowText(hEdit, _T(text));
        ShowWindow(hDlg, SW_SHOW);
        UpdateWindow(hDlg);

        /* Message loop */
        while (GetMessage(&msg, NULL, 0, 0)) {
            GetKeyboardState(lpKeyState);


            /* Get modifiers */
            if (lpKeyState[VK_SHIFT]>1) {
                modifiers |= MOD_SHIFT;
            } 
            if (lpKeyState[VK_CONTROL]>1) {
                modifiers |= MOD_CONTROL;
            } 
            if (lpKeyState[VK_MENU]>1) {
                modifiers |= MOD_ALT;
            }

            /* Don't do this if we don't need to */
            if (modifiers != 0) {
                for (int i=0x01; i<0xFF; i++) {
                    /* Skip useless cases */
                    if (i == VK_SHIFT || i == VK_CONTROL || i == VK_MENU || 
                        i == VK_LSHIFT || i == VK_LCONTROL || i == VK_LMENU || 
                        i == VK_RSHIFT || i == VK_RCONTROL || i == VK_RMENU
                    ) continue;

                    /* If pressed */
                    if (lpKeyState[i]>1) {
                        if (!hotkey_name(modifiers, i, buffer))
                            continue;
                        sprintf(text, "\nHotkey set to %s", buffer);


                        /* Unregister old hotkey */
                        UnregisterHotKey(hWnd, HOTKEY_ID);

                        /* New hotkey */
                        if (!RegisterHotKey(hWnd, HOTKEY_ID, modifiers, i)) {
                            if (!RegisterHotKey(hWnd, HOTKEY_ID, MOD_CONTROL, VK_SPACE))
                                error_msg(L"Runtime Error", L"Cannot re-register default hotkey.", 1);

                            sprintf(text, "\nError registering new hotkey: %s", buffer);
                        }

                        /* Save */
                        set_setting(SETTING_HOTMOD, modifiers);
                        set_setting(SETTING_HOTKEY, i);

                        /* Update window */
                        SetWindowText(hEdit, _T(text));
                        break;
                    }
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } 
    }

    int hotkey_name(int modifiers, int vk, char text[]) {
        char mod_name[256];
        char key_name[256];

        mod_name[0] = '\0';

        /* Get modifier names */
        if (modifiers&MOD_SHIFT)
            strcpy(mod_name, "SHIFT");
        if (modifiers&MOD_CONTROL) {
            if (strlen(mod_name)>0)
                strcat(mod_name, "+CTRL");
            else
                strcpy(mod_name, "CTRL");
        }
        if (modifiers&MOD_ALT) {
            if (strlen(mod_name)>0)
                strcat(mod_name, "+ALT");
            else
                strcpy(mod_name, "ALT");
        }

        /* Key name, skip weirdo keys we don't know */
        GetKeyNameText(MapVirtualKey(vk, 0) << 16, key_name, 256);
        if(strlen(key_name) == 0) return 0;

        /* Get final string */
        sprintf(text, "%s+%s", mod_name, key_name);
        return 1;
    }

    /**
     * Enable debug console
     */
    void debug_enable( void ) {
        HWND console;
        HICON hIcon;

        /* Load icon */
        hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICON_ID)); 
        if (!hIcon) {
            exit(EXIT_FAILURE);
        }

        AllocConsole();
        console = GetConsoleWindow();
        SetConsoleTitle(DEBUG_TITLE);
        SendMessage(console, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
        SendMessage(console, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
        DestroyIcon(hIcon);

        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        printf("Debug mode enabled.\n");
    }
    
    /** 
     * Print help message to stdout
     */
    void print_help( void ) {
        PostMessage(hWnd, WM_COMMAND, CMD_ABOUT, 0);
    }

    /** 
     * Check window messages, process events
     */
    void update_interface( void ) {
        MSG msg;
        
        if (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    /** 
     * Key window event callbacks
     */
    LRESULT CALLBACK key_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_DESTROY:
                DestroyWindow(hWnd);
                UnregisterClass(HOTKEY_CALLBACK, GetModuleHandle(NULL));
            break;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    /** 
     * Windows event callbacks
     */
    LRESULT CALLBACK wnd_callback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        static UINT wndMsg = 0;
        static INPUT control_c[] = {
            {INPUT_KEYBOARD, .ki={VK_CONTROL,0,KEYEVENTF_KEYUP,0,0}},
            {INPUT_KEYBOARD, .ki={VK_MENU,0,KEYEVENTF_KEYUP,0,0}},
            {INPUT_KEYBOARD, .ki={VK_SHIFT,0,KEYEVENTF_KEYUP,0,0}},
            {INPUT_KEYBOARD, .ki={VK_CONTROL,0,0,0,0}},
            {INPUT_KEYBOARD, .ki={0x43,0,0,0,0}},
            {INPUT_KEYBOARD, .ki={VK_CONTROL,0,KEYEVENTF_KEYUP,0,0}},
            {INPUT_KEYBOARD, .ki={0x43,0,KEYEVENTF_KEYUP,0,0}},
        };
        static INPUT control_v[] = {
            {INPUT_KEYBOARD, .ki={VK_CONTROL,0,KEYEVENTF_KEYUP,0,0}},
            {INPUT_KEYBOARD, .ki={VK_CONTROL,0,0,0,0}},
            {INPUT_KEYBOARD, .ki={0x56,0,0,0,0}},
            {INPUT_KEYBOARD, .ki={VK_CONTROL,0,KEYEVENTF_KEYUP,0,0}},
            {INPUT_KEYBOARD, .ki={0x56,0,KEYEVENTF_KEYUP,0,0}},
        };
        int i;
        char* help;

        /* Register for callbacks, but only once */
        if (wndMsg == 0)
            wndMsg = RegisterWindowMessage(WINDOW_CALLBACK);

        /* What type of event is it */
        switch (message) {

            /* New thing to process */
            case WM_HOTKEY:
                if (get_setting(SETTING_AUTOCOPY)==SETTING_AUTOCOPY_ON) {
                    /* Control C */
                    SendInput(sizeof(control_c)/sizeof(INPUT), control_c, sizeof(INPUT));
                    Sleep(50);
                }

                _parse_callback(from_clipboard());

                if (get_setting(SETTING_AUTOCOPY)==SETTING_AUTOCOPY_ON) {
                    /* Control V */
                    SendInput(sizeof(control_v)/sizeof(INPUT), control_v, sizeof(INPUT));
                }
            break;

            /* Time to exit */
            case WM_DESTROY:
                Shell_NotifyIcon(NIM_DELETE, &nid);

                /* Delete stored entries */
                for (i=0; i<MAX_EQUATIONS; i++)
                    if (stored_entries[i] != NULL)
                        free(stored_entries[i]);

                PostQuitMessage(0);
                _exit_callback();
            break;

            case WM_HELP:
                ShellExecuteA(
                    hWnd, 
                    "open", 
                    HELP_URL, 
                    NULL, 
                    NULL, 
                    SW_SHOWDEFAULT
                );
            break;

            /* Some other command type */
            case WM_COMMAND:

                /* Which type of custom command */
                switch (LOWORD(wParam)) {

                    /* Exit through menu */
                    case CMD_EXIT:
                        Shell_NotifyIcon(NIM_DELETE, &nid);

                    /* Delete stored entries */
                    for (i=0; i<MAX_EQUATIONS; i++)
                        if (stored_entries[i] != NULL)
                            free(stored_entries[i]);

                        PostQuitMessage(0);
                        _exit_callback();
                    break;
                    
                    /* Show about box */
                    case CMD_ABOUT:
                        help = cmdflag_help(HELP_MSG);
                        SetForegroundWindow(hWnd);
                        MessageBox(hWnd, 
                            help, HELP_TITLE,
                            MB_OK | MB_HELP | MB_DEFBUTTON1);
                        free(help);
                    break;

                    /* Change to degrees mode */
                    case CMD_ANGLE_DEG:
                        set_setting(SETTING_ANGLE, SETTING_ANGLE_DEG);
                    break;

                    /* Change to radians mode */
                    case CMD_ANGLE_RAD:
                        set_setting(SETTING_ANGLE, SETTING_ANGLE_RAD);
                    break;

                    /* Turn silent errors on and off mode */
                    case CMD_TOGGLE_SILENT_MODE:
                        set_setting(SETTING_SILENT, (get_setting(SETTING_SILENT)==SETTING_SILENT_ON) ? SETTING_SILENT_OFF : SETTING_SILENT_ON);
                    break;

                    /* Turn autocopy on and off */
                    case CMD_TOGGLE_AUTOCOPY:
                        set_setting(SETTING_AUTOCOPY, (get_setting(SETTING_AUTOCOPY)==SETTING_AUTOCOPY_ON) ? SETTING_AUTOCOPY_OFF : SETTING_AUTOCOPY_ON);
                    break;

                    /* Turn silent start on and off */
                    case CMD_TOGGLE_SILENTSTART:
                        set_setting(SETTING_SILENTSTART, (get_setting(SETTING_SILENTSTART)==SETTING_SILENTSTART_ON) ? SETTING_SILENTSTART_OFF : SETTING_SILENTSTART_ON);
                    break;

                    /* English language */
                    case CMD_LANG_EN:
                        set_setting(SETTING_LANG, LANG_EN);
                        language_set_current(LANG_EN);
                    break;

                    /* French language */
                    case CMD_LANG_FR:
                        set_setting(SETTING_LANG, LANG_FR);
                        language_set_current(LANG_FR);
                    break;

                    case CMD_REGKEY:
                        key_registrar();
                    break;

                    /* One of the equations was clicked. Put in it the clipboard */
                    default:
                        if (LOWORD(wParam) < CMD_CPX) break;
                        to_clipboard(stored_entries[LOWORD(wParam) - CMD_CPX]);
                }
            break;

            /* Show the menu */
            default:
                if (message == wndMsg && lParam == WM_RBUTTONUP)
                    show_menu(hWnd);
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    /* Show the menu interface */
    void show_menu(HWND hWnd)
    {
        wchar_t buffer[MAX_HISTORY_LEN + wcslen(HISTORY_SUFFIX) + 1];
        int has_equation = 0;
        int i;
        POINT p;
        HMENU hMenu;
        HMENU hAnglesMenu;
        HMENU hLangMenu;

        /* Get position of cursor */
        GetCursorPos(&p);

        /* Create the empty menud */
        hMenu = CreatePopupMenu();
        hAnglesMenu = CreatePopupMenu();
        hLangMenu = CreatePopupMenu();

        /* Equations */
        for (i=0; i<MAX_EQUATIONS; ++i)
            if (stored_entries[i] != NULL) {
                wcsncpy(buffer, stored_entries[i], MAX_HISTORY_LEN);
                buffer[MAX_HISTORY_LEN] = L'\0';
                if (wcslen(stored_entries[i]) > MAX_HISTORY_LEN)
                    wcscat(buffer, HISTORY_SUFFIX);
                AppendMenuW(hMenu, MF_STRING, CMD_CPX+i, buffer);
                has_equation = 1;
            }
        if (!has_equation) {
            AppendMenuW(hMenu, MF_STRING, 0, language_str(LANG_STR_NO_EQUATIONS));
        }

        /* Settings */
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hLangMenu, (get_setting(SETTING_LANG)==LANG_EN)?MF_CHECKED:MF_UNCHECKED, CMD_LANG_EN, L"English");
        AppendMenuW(hLangMenu, (get_setting(SETTING_LANG)==LANG_FR)?MF_CHECKED:MF_UNCHECKED, CMD_LANG_FR, L"Français");
        AppendMenuW(hMenu, MF_POPUP, (UINT) hLangMenu, language_str(LANG_STR_LANGUAGE));

        AppendMenuW(hMenu, (get_setting(SETTING_SILENT)==SETTING_SILENT_ON)?MF_CHECKED:MF_UNCHECKED, CMD_TOGGLE_SILENT_MODE, language_str(LANG_STR_SILENT_ERRS));
        AppendMenuW(hMenu, (get_setting(SETTING_AUTOCOPY)==SETTING_AUTOCOPY_ON)?MF_CHECKED:MF_UNCHECKED, CMD_TOGGLE_AUTOCOPY, language_str(LANG_STR_ENABLEAUTOCOPY));
        AppendMenuW(hMenu, (get_setting(SETTING_SILENTSTART)==SETTING_SILENTSTART_ON)?MF_CHECKED:MF_UNCHECKED, CMD_TOGGLE_SILENTSTART, language_str(LANG_STR_ENABLESILENTSTART));

        AppendMenuW(hAnglesMenu, (get_setting(SETTING_ANGLE)==SETTING_ANGLE_DEG)?MF_CHECKED:MF_UNCHECKED, CMD_ANGLE_DEG, language_str(LANG_STR_DEGREES));
        AppendMenuW(hAnglesMenu, (get_setting(SETTING_ANGLE)==SETTING_ANGLE_RAD)?MF_CHECKED:MF_UNCHECKED, CMD_ANGLE_RAD, language_str(LANG_STR_RADIANS));
        AppendMenuW(hMenu, MF_POPUP, (UINT) hAnglesMenu, language_str(LANG_STR_ANGLE_UNITS));
        

        /* System menu */
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, CMD_REGKEY, language_str(LANG_STR_HOTKEY));
        AppendMenuW(hMenu, MF_STRING, CMD_ABOUT, language_str(LANG_STR_ABOUT));
        AppendMenuW(hMenu, MF_STRING, CMD_EXIT, language_str(LANG_STR_EXIT));

        SetForegroundWindow(hWnd);
        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, p.x, p.y, 0, hWnd, NULL);

    }

    /** 
     * Add an entry to the menu history
     * @param entry The string to add
     */
    void add_history(const wchar_t *entry) {
        int i;

        /* Free memory for last entry */
        if (stored_entries[MAX_EQUATIONS-1] != NULL)
            free(stored_entries[MAX_EQUATIONS-1]);

        /* Shift all current entries by one */
        for (i=MAX_EQUATIONS-2; i>=0; --i)
            stored_entries[i+1] = stored_entries[i];

        /* Allocate new entry */
        stored_entries[0] = (wchar_t*) malloc(sizeof(wchar_t)*(wcslen(entry)+1));
        if (stored_entries[0] == NULL)
            error_msg(language_str(LANG_STR_RUNTIME_ERR), language_str(LANG_STR_ERR_ALLOCATION), 1);
        wcscpy( stored_entries[0], entry);
    }

    /** 
     * Copy string to system clipboard
     * @param entry The string to add
     */
    void to_clipboard(const wchar_t *entry) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t)*(wcslen(entry)+1));
        memcpy(GlobalLock(hMem), entry, sizeof(wchar_t)*(wcslen(entry)+1));

        GlobalUnlock(hMem);
        if (OpenClipboard(0)) {
            EmptyClipboard();
            SetClipboardData(CF_UNICODETEXT, hMem);
            CloseClipboard();
        }

        printf("Saved a string to clipboard: %S\n", entry);
    }

    /** 
     * Get string from system clipboard
     * @return const char* The string fetched
     */
    const wchar_t* from_clipboard( void ) {
        HGLOBAL hMem;
        const wchar_t* clipText = NULL;

        if (OpenClipboard(0)) {
            hMem = GetClipboardData(CF_UNICODETEXT);
            if (hMem != NULL) {
                clipText = GlobalLock(hMem);
                GlobalUnlock(hMem);
            }

            CloseClipboard();
        }

        printf("Read a string from clipboard: %S\n", clipText);
        return clipText;
    }

    /**
     * Get the path to a valid configuration file
     * Search in current directory, then relevant home dir
     */
    char* config_path( void ) {
        char *path, *self;
        FILE* test;

        /* Prepare to hold path */
        path = (char*) malloc(sizeof(char)*(MAX_PATH+1));
        if (path == NULL)
            error_msg(language_str(LANG_STR_RUNTIME_ERR), language_str(LANG_STR_ERR_ALLOCATION), 1);

        /* Preferences first */
        if (strlen(prefered_path) != 0) {
            strcpy(path, prefered_path);
            return path;
        }

        /* Test current directory */
        test = fopen(CONFIG_FILENAME, "r");
        if (test != NULL) {
            fclose(test);

            path[0] = '\0';
            strcat(path, CONFIG_FILENAME);

            printf("Found configuration path: %s\n", path);
            return path;
        }

        /* Test executable directory */
        self = self_path();
        strcat(self, "\\");
        strcat(self, CONFIG_FILENAME);
        test = fopen(self, "r");
        if (test != NULL) {
            fclose(test);

            printf("Found configuration path: %s\n", self);
            return self;
        }
        free(self);

        /* Get home dir path */
        if (SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, path) == S_OK) {
            strcat(path, "\\");
            strcat(path, CONFIG_FILENAME);
            printf("Found configuration path: %s\n", path);
            return path;
        }

        return NULL;
    }

    char* self_path( void ) {
        int i;
        char *path;

        /* Prepare to hold path */
        path = (char*) malloc(sizeof(char)*(MAX_PATH+1));
        if (path == NULL)
            error_msg(language_str(LANG_STR_RUNTIME_ERR), language_str(LANG_STR_ERR_ALLOCATION), 1);

        GetModuleFileName(NULL, path, MAX_PATH+1);
        for (i=strlen(path)-1; i>=0; i--)
            if (path[i] == '\\') break;
            else path[i] = '\0';
        return path;
    }

    void config_set(const char* path) {
        strcpy(prefered_path, path);
    }

    /**
     * Get the path to a valid configuration file
     * @param title The title of the error message
     * @param msg The error message
     * @param fatal if non 0, exit
     */
    void error_msg(const wchar_t* title, const wchar_t* msg, char fatal) {
        if (get_setting(SETTING_SILENT) == SETTING_SILENT_OFF || fatal) {
            SetForegroundWindow(hWnd);
            MessageBoxW(hWnd, 
                msg,
                title,
            MB_OK | MB_ICONWARNING | MB_DEFBUTTON1);

            printf("Displaying an error message: %S\n", msg);
        }

        if (fatal)
            exit(EXIT_FAILURE);
    }

#endif
