/*________
 /         \ This is a custom D port of tinyfiledialogs v3.3.9 [Apr 14, 2019]
 |tiny file| http://tinyfiledialogs.sourceforge.net
 | dialogs | Copyright (c) 2014-2019 Guillaume Vareille
 \____  ___/ Copyright (c) 2019 dayllenger
      \|
*/
/**
Native dialog library for Windows, macOS, GTK+, Qt, console & more.
SSH supported via automatic switch to console mode or X11 forwarding.

8 functions:
$(LIST
  * beep
  * notify popup (tray)
  * message & question
  * input & password
  * save file
  * open file(s)
  * select folder
  * color picker
)
Each function is documented with examples.
The dialogs can be forced into console mode.
Supports UTF-8 (except Windows console).

Windows XP to 10:
$(LIST
  * native code & vbs create the graphic dialogs
  * enhanced console mode can use dialog.exe from
    `http://andrear.altervista.org/home/cdialog.php`
  * basic console input
)
Unix (using command line calls):
$(LIST
  * applescript, kdialog, zenity
  * python (2 or 3) + tkinter + python-dbus (optional)
  * whiptail, dialog (opens a console if needed)
  * basic console input
)
The same executable can run across desktops & distributions.

Notes
=====
$(LIST
  * This is $(I not) for Android nor iOS.
  * The code is betterC compatible, originally pure C89.
  * Windows is fully supported from XP to 10 (maybe even older versions).
  * OSX supported from 10.4 to latest (maybe even older versions).
  * On Windows, it links against comdlg32.lib, ole32.lib, and user32.lib.
  * Set TINYFD_NOLIB version if you don't want to include the code creating
    graphic dialogs. Then you won't need to link against those libs.
  * On Unix, it tries command line calls.
    They are already available on most (if not all) desktops.
  * In the absence of those it will use gdialog, gxmessage or whiptail
    with a textinputbox.
  * If nothing is found, it switches to basic console input,
    and opens a console if needed (requires xterm + bash).
  * On OSX, the package dialog can be installed via
    `http://macappstore.org/dialog` or `https://www.macports.org/`
  * On Windows, for enhanced console mode,
    dialog.exe should be copied somewhere on your executable path.
    It can be found at the bottom of the following page:
    `http://andrear.altervista.org/home/cdialog.php`
  * If dialog is missing, it will switch to basic console input.
  * Mutiple selects are not allowed in console mode.
)
$(LIST
  * Avoid using " and ' in titles and messages.
  * String memory is preallocated statically for all the returned values.
  * Use platform-specific path separators: \ on Windows, / on Unix.
  * If you pass only a path instead of path + filename,
    make sure it ends with a separator.
  * File and path names are tested before return, they are valid.
  * You can query the type of dialog that will be used, see [tinyfd_response].
)
Thanks for contributions, bug corrections & thorough testing to:
$(LIST
  * Don Heyse http://ldglite.sf.net for bug corrections & thorough testing!
  * Paul Rouget
)
*/
/*
zlib License

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.  If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
module tinyfiledialogs;

// version = TINYFD_NOLIB;
// version = TINYFD_NOSELECTFOLDERWIN;
version = TINYFD_NOCCSUNICODE;

extern (C) nothrow @nogc:

alias c_str = const(char*);

/// No params, no return value, just beep
void tinyfd_beep()
{
    _beep();
}

/** Params:
        title = C-string or null
        message = C-string or null, can be multiline
        iconType = "info" "warning" "error"

    Returns:
        Return has only meaning for "tinyfd_query".

    Example:
    ---
    tinyfd_notifyPopup("the title", "the message from outer-space", "info");
    ---
*/
int tinyfd_notifyPopup(c_str title, c_str message, c_str iconType)
{
    return _notifyPopup(title, message, iconType);
}

/**	Params:
        title = C-string or null
        message = C-string or null, can be multiline
        dialogType = "ok" "okcancel" "yesno" "yesnocancel"
        iconType = "info" "warning" "error" "question"
        defaultButton = 0 - cancel/no, 1 - ok/yes, 2 - no in yesnocancel

    Returns:
        0 - cancel/no, 1 - ok/yes, 2 - no in yesnocancel.

    Example:
    ---
    const int ret = tinyfd_messageBox("Hello World",
        "graphic dialogs [yes] / console mode [no]?",
        "yesno", "question", 1);
    tinyfd_forceConsole = (ret == 0);
    ---
*/
int tinyfd_messageBox(
    c_str title,
    c_str message,
    c_str dialogType,
    c_str iconType,
    int defaultButton,
)
{
    return _messageBox(title, message, dialogType, iconType, defaultButton);
}

/** Params:
        title = C-string or null
        message = C-string or null; some platforms (Windows, GTK-based) can't display multiline text
        defaultInput = C-string, if null it's a password box

    Returns:
        Entered text, `null` on cancel.

    Example:
    ---
    c_str passwd = tinyfd_inputBox("A password box", "Your password will be revealed", null);
    if (passwd)
        tinyfd_notifyPopup("Your password is:", passwd, "warning");
    ---
*/
c_str tinyfd_inputBox(c_str title, c_str message, c_str defaultInput)
{
    return _inputBox(title, message, defaultInput);
}

/// Single filter, used in open/save file dialogs
struct TFD_Filter
{
    /// Patterns like `["*.jpg", "*.png"]` or MIME types (kdialog only) `["audio/mp3"]`
    c_str[] patterns;
    /// Description like "Image files". If none, the list of patterns becomes the description
    c_str description;
}

/** Params:
        title = C-string or null
        defaultPathAndFile = C-string or null
        filters = list of file type patterns

    Returns:
        Selected file full name, `null` on cancel.

    Example:
    ---
    const TFD_Filter[] filters = [
        { ["*.css" ], "CSS" },
        { ["*.sass", "*.scss" ], "SASS" },
    ];
    c_str filename = tinyfd_saveFileDialog("Save as...", "style.css", filters);
    if (filename)
        tinyfd_messageBox("Chosen file is", filename, "ok", "info", 1);
    ---
*/
c_str tinyfd_saveFileDialog(
    c_str title,
    c_str defaultPathAndFile,
    const TFD_Filter[] filters,
)
{
    return _saveFileDialog(title, defaultPathAndFile, filters);
}

/** Params:
        title = C-string or null
        defaultPathAndFile = C-string or null
        filters = list of file type patterns
        allowMultipleSelects = does not work on console

    Returns:
        Selected file full name, `null` on cancel. In case of multiple files, the separator is |.

    Example:
    ---
    const TFD_Filter[] filters = [
        { ["*.c" ], "C source code" },
        { ["*.cpp", "*.cc", "*.C", "*.cxx", "*.c++"], "C++ source code" }
    ];
    c_str filename = tinyfd_openFileDialog("Open a C/C++ File", null, filters, false);
    if (filename)
        tinyfd_messageBox("Chosen file is", filename, "ok", "info", 1);
    ---
*/
c_str tinyfd_openFileDialog(
    c_str title,
    c_str defaultPathAndFile,
    const TFD_Filter[] filters,
    bool allowMultipleSelects,
)
{
    return _openFileDialog(title, defaultPathAndFile, filters, allowMultipleSelects);
}

/** Params:
        title = C-string or null
        defaultPath = C-string or null

    Returns:
        Selected folder path, `null` on cancel

    Example:
    ---
    c_str name = tinyfd_selectFolderDialog("Let us just select a directory", null);
    if (name)
        tinyfd_messageBox("The selected folder is", name, "ok", "info", 1);
    ---
*/
c_str tinyfd_selectFolderDialog(c_str title, c_str defaultPath)
{
    return _selectFolderDialog(title, defaultPath);
}

/** Params:
        title = C-string or null
        defaultHexRGB = default color, C-string or null
        defaultRGB = used only if the previous parameter is null
        resultRGB = contains the decoded result

    Returns:
        The hexcolor as a string like "#FF0000" or `null` on cancel.

    Example:
    ---
    ubyte[3] rgb;
    c_str hexColor = tinyfd_colorChooser("Choose a nice color", "#FF0077", rgb, rgb);
    if (hexColor)
        tinyfd_messageBox("The selected hexcolor is", hexColor, "ok", "info", 1);
    ---
*/
c_str tinyfd_colorChooser(
    c_str title,
    c_str defaultHexRGB,
    ref const ubyte[3] defaultRGB,
    ref ubyte[3] resultRGB,
)
{
    return _colorChooser(title, defaultHexRGB, defaultRGB, resultRGB);
}

/**** Constants and variables ****/
__gshared:

/// Contains tinyfd current version number
immutable char[8] tinyfd_version = "3.3.9";

/// Info about requirements for a platform
version (Windows)
immutable char[] tinyfd_needs =
` ___________
/           \
| tiny file |
|  dialogs  |
\_____  ____/
      \|
tiny file dialogs on Windows needs:
   a graphic display
or dialog.exe (enhanced console mode)
or a console for basic input`;
else
immutable char[] tinyfd_needs =
` ___________
/           \
| tiny file |
|  dialogs  |
\_____  ____/
      \|
tiny file dialogs on UNIX needs:
   applescript
or kdialog
or zenity (or matedialog or qarma)
or python (2 or 3)
 + tkinter + python-dbus (optional)
or dialog (opens console if needed)
or xterm + bash
   (opens console for basic input)
or existing console for basic input`;

/// On unix, prints the command line calls; default is `false`
bool tinyfd_verbose;
/// On unix, hide errors and warnings from called dialog; default is `true`
bool tinyfd_silent = true;

/** `false` (default) - try to use a graphic solution, if it fails use console mode.
    `true` - force all dialogs into console mode even when an X server is present,
    if the package `whiptail` or `dialog` and a console are present in a system,
    or `dialog.exe` is installed.
    On Windows it only make sense for console applications.
*/
version (Windows)
{
    version (TINYFD_NOLIB)
        bool tinyfd_forceConsole = true;
    else
        bool tinyfd_forceConsole;
}
else
    bool tinyfd_forceConsole;

/** If you pass "tinyfd_query" as `title`, the functions will not display
    the dialogs but will return 0 for console mode, 1 for graphic mode.
    `tinyfd_response` is then filled with the retain solution.
    possible values for `tinyfd_response` are (all lowercase),
    for graphic mode: `windows`
        `applescript kdialog zenity zenity3 matedialog qarma`
        `python2-tkinter python3-tkinter python-dbus perl-dbus`
        `gxmessage gmessage xmessage xdialog gdialog`,
    for console mode: `dialog whiptail basicinput no_solution`

    Example:
    ---
    import core.stdc.string;

    char[1024] buf = '\0';
    c_str mode = tinyfd_inputBox("tinyfd_query", null, null);

    strcpy(buf.ptr, "v");
    strcat(buf.ptr, tinyfd_version.ptr);
    strcat(buf.ptr, "\n");
    if (mode)
        strcat(buf.ptr, "graphic mode: ");
    else
        strcat(buf.ptr, "console mode: ");
    strcat(buf.ptr, tinyfd_response.ptr);
    strcat(buf.ptr, "\n");
    strcat(buf.ptr, tinyfd_needs.ptr + 78);
    tinyfd_messageBox("Hello", buf.ptr, "ok", "info", 0);
    ---
*/
char[1024] tinyfd_response = '\0';

/**************************** IMPLEMENTATION ****************************/

private:
import core.stdc.ctype;
import core.stdc.stdlib;
import core.stdc.string;

version (Windows)
{
    import core.stdc.stdio;
    import core.stdc.wchar_;
    import core.sys.windows.commdlg;
    import core.sys.windows.w32api;
    import core.sys.windows.winbase;
    import core.sys.windows.wincon : GetConsoleMode, SetConsoleMode, GetConsoleWindow, ENABLE_ECHO_INPUT;
    import core.sys.windows.windef;
    import core.sys.windows.winnls;

    static assert(_WIN32_WINNT >= 0x0500);

    enum SLASH = "\\";

    int _getch();
    FILE* _popen(const char* command, const char* mode);
    int _pclose(FILE* stream);

    version (TINYFD_NOLIB)
    {
        bool gWarningDisplayed = true;
    }
    else
    {
        version = TINYFD_LIB;
        import core.sys.windows.com : COINIT_APARTMENTTHREADED;
        import core.sys.windows.wingdi : RGB, GetRValue, GetGValue, GetBValue;
        import core.sys.windows.winuser;

        pragma(lib, "comdlg32.lib");
        pragma(lib, "ole32.lib");
        pragma(lib, "user32.lib");

        FILE* _wfopen(const wchar* filename, const wchar* mode);
        int _wremove(const wchar* path);
        wchar* _wgetenv(const wchar* varname);
        // header versions are not nothrow @nogc
        extern(Windows) HRESULT CoInitializeEx(LPVOID, DWORD);
        extern(Windows) void CoUninitialize();

        bool gWarningDisplayed;
    }

    version (TINYFD_NOSELECTFOLDERWIN) {}
    else
    {
        version = TINYFD_SELECTFOLDERWIN;
        import core.sys.windows.shlobj : BFFM_INITIALIZED, BFFM_SETSELECTIONW, BIF_USENEWUI,
            BROWSEINFOW, LPCITEMIDLIST, LPITEMIDLIST, PBROWSEINFOW;

        extern(Windows) LPITEMIDLIST SHBrowseForFolderW(PBROWSEINFOW);
        extern(Windows) BOOL SHGetPathFromIDListW(LPCITEMIDLIST, LPWSTR);
    }
}
else
{
    import core.stdc.limits;
    import core.sys.posix.dirent;
    import core.sys.posix.signal;
    import core.sys.posix.stdio;
    import core.sys.posix.sys.stat;
    import core.sys.posix.sys.utsname;
    import core.sys.posix.termios;
    import core.sys.posix.unistd;

    enum SLASH = "/";
    bool gWarningDisplayed;
}

enum int MAX_PATH_OR_CMD = 1024; /* _MAX_PATH or MAX_PATH */
enum int MAX_MULTIPLE_FILES = 32;
enum int MAX_PATTERNS = 8;

immutable char[] gTitle = "missing software! (we will try basic console input)";

bool some(const char* str)
{
    return str && str[0] != '\0';
}

bool wsome(const wchar* str)
{
    return str && str[0] != '\0';
}

bool eq(const char* s1, const char* s2)
{
    if (s1 is s2)
        return true;
    if (s1 is null || s2 is null)
        return false;
    return strcmp(s1, s2) == 0;
}

char lastch(const char* str)
{
    if (str)
    {
        size_t len = strlen(str);
        if (len > 0)
            return str[len - 1];
    }
    return '\0';
}

void removeLastNL(char* str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';
    }
}

void response(const char* message)
{
    strcpy(tinyfd_response.ptr, message);
}

char* getPathWithoutFinalSlash(
    char* aoDestination, /* make sure it is allocated, use _MAX_PATH */
    const char* aSource) /* aoDestination and aSource can be the same */
{
    const(char)* lTmp;
    if (aSource)
    {
        lTmp = strrchr(aSource, '/');
        if (!lTmp)
        {
            lTmp = strrchr(aSource, '\\');
        }
        if (lTmp)
        {
            strncpy(aoDestination, aSource, lTmp - aSource);
            aoDestination[lTmp - aSource] = '\0';
        }
        else
        {
            *aoDestination = '\0';
        }
    }
    else
    {
        *aoDestination = '\0';
    }
    return aoDestination;
}

char* getLastName(
    char* aoDestination, /* make sure it is allocated */
    const char* aSource)
{
    /* copy the last name after '/' or '\' */
    const(char)* lTmp;
    if (aSource)
    {
        lTmp = strrchr(aSource, '/');
        if (!lTmp)
        {
            lTmp = strrchr(aSource, '\\');
        }
        if (lTmp)
        {
            strcpy(aoDestination, lTmp + 1);
        }
        else
        {
            strcpy(aoDestination, aSource);
        }
    }
    else
    {
        *aoDestination = '\0';
    }
    return aoDestination;
}

void ensureFinalSlash(char* aioString)
{
    if (some(aioString))
    {
        char* lastcar = aioString + strlen(aioString) - 1;
        if (strncmp(lastcar, SLASH, 1))
        {
            strcat(lastcar, SLASH);
        }
    }
}

uint getValidPatterns(ref const TFD_Filter filter, out bool isMime, out const(char)*[MAX_PATTERNS] output)
{
    uint len;
    foreach (pt; filter.patterns)
    {
        if (some(pt))
        {
            output[len] = pt;
            len++;
            if (len == MAX_PATTERNS)
                break;
        }
    }
    if (len == 0)
        return 0;

    uint mime;
    foreach (pt; output[0 .. len])
    {
        if (strchr(pt, '/'))
            mime++;
        else
            break;
    }
    if (mime > 0)
    {
        // forbid mixing of MIME and usual patterns
        if (mime != len)
        {
            isMime = false;
            output[0 .. len] = null;
            return 0;
        }
        isMime = true;
    }
    return len;
}

void Hex2RGB(const char* aHexRGB, ref ubyte[3] aoResultRGB)
{
    char[8] lColorChannel = '\0';
    if (aHexRGB)
    {
        strcpy(lColorChannel.ptr, aHexRGB);
        aoResultRGB[2] = cast(ubyte)strtoul(lColorChannel.ptr + 5, null, 16);
        lColorChannel[5] = '\0';
        aoResultRGB[1] = cast(ubyte)strtoul(lColorChannel.ptr + 3, null, 16);
        lColorChannel[3] = '\0';
        aoResultRGB[0] = cast(ubyte)strtoul(lColorChannel.ptr + 1, null, 16);
        /* printf("%d %d %d\n", aoResultRGB[0], aoResultRGB[1], aoResultRGB[2]); */
    }
    else
    {
        aoResultRGB[0] = 0;
        aoResultRGB[1] = 0;
        aoResultRGB[2] = 0;
    }
}

void RGB2Hex(const ubyte[3] aRGB, char* aoResultHexRGB)
{
    if (aoResultHexRGB)
    {
        // NOTE: here was compiler ifdef
        sprintf(aoResultHexRGB, "#%02hhx%02hhx%02hhx", aRGB[0], aRGB[1], aRGB[2]);
        /* printf("aoResultHexRGB %s\n", aoResultHexRGB); */
    }
}

void replaceSubStr(const char* aSource,
                   const char* aOldSubStr,
                   const char* aNewSubStr,
                   char* aoDestination)
{
    const(char)* pOccurence;
    const(char)* p;
    const(char)* lNewSubStr = "";
    size_t lOldSubLen = strlen(aOldSubStr);

    if (!aSource)
    {
        *aoDestination = '\0';
        return;
    }
    if (!aOldSubStr)
    {
        strcpy(aoDestination, aSource);
        return;
    }
    if (aNewSubStr)
    {
        lNewSubStr = aNewSubStr;
    }
    p = aSource;
    *aoDestination = '\0';
    while ((pOccurence = strstr(p, aOldSubStr)) !is null)
    {
        strncat(aoDestination, p, pOccurence - p);
        strcat(aoDestination, lNewSubStr);
        p = pOccurence + lOldSubLen;
    }
    strcat(aoDestination, p);
}

bool filenameValid(const char* aFileNameWithoutPath)
{
    return some(aFileNameWithoutPath) && !strpbrk(aFileNameWithoutPath, "\\/:*?\"<>|");
}

void wipefile(const char* aFilename)
{
    // file must exist beforehand
    FILE* lIn = fopen(aFilename, "w");
    if (lIn)
    {
        fseek(lIn, 0, SEEK_END);
        const size = ftell(lIn);
        rewind(lIn);
        foreach (i; 0 .. size)
        {
            fputc('A', lIn);
        }
        fclose(lIn);
    }
}

/* source and destination can be the same or ovelap*/
const(char)* ensureFilesExist(char* aDestination, const char* aSourcePathsAndNames)
{
    if (!some(aSourcePathsAndNames))
        return null;

    char* lDestination = aDestination;
    const(char)* p;
    const(char)* p2;
    size_t lLen = strlen(aSourcePathsAndNames);

    p = aSourcePathsAndNames;
    while ((p2 = strchr(p, '|')) !is null)
    {
        lLen = p2 - p;
        memmove(lDestination, p, lLen);
        lDestination[lLen] = '\0';
        if (fileExists(lDestination))
        {
            lDestination += lLen;
            *lDestination = '|';
            lDestination++;
        }
        p = p2 + 1;
    }
    if (fileExists(p))
    {
        lLen = strlen(p);
        memmove(lDestination, p, lLen);
        lDestination[lLen] = '\0';
    }
    else
    {
        *(lDestination - 1) = '\0';
    }
    return aDestination;
}

version (Windows)
{
    bool fileExists(const char* aFilePathAndName)
    {
        if (!some(aFilePathAndName))
            return false;

        wchar* lTmpWChar = utf8to16(aFilePathAndName);
        DWORD attribs = GetFileAttributes(lTmpWChar);
        free(lTmpWChar);
        return !(attribs & FILE_ATTRIBUTE_DEVICE) && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
    }
}
else // unix
{
    bool fileExists(const char* aFilePathAndName)
    {
        if (!some(aFilePathAndName))
            return false;

        FILE* lIn = fopen(aFilePathAndName, "r");
        if (!lIn)
            return false;
        fclose(lIn);
        return true;
    }
}

version (Windows) {

bool replaceChr(char* aString, const char aOldChr, const char aNewChr)
{
    if (!aString)
        return false;
    if (aOldChr == aNewChr)
        return false;

    bool ret;
    char* p = aString;
    while ((p = strchr(p, aOldChr)) !is null)
    {
        *p = aNewChr;
        p++;
        ret = true;
    }
    return ret;
}

int sizeUtf16(const char* aUtf8string)
{
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                               aUtf8string, -1, null, 0);
}

wchar* utf8to16(const char* str)
{
    if (!some(str))
        return null;

    int lSize = sizeUtf16(str);
    wchar* ret = cast(wchar*)malloc(lSize * wchar.sizeof);
    lSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                str, -1, ret, lSize);
    if (lSize == 0)
    {
        free(ret);
        return null;
    }
    return ret;
}

bool dirExists(const char* aDirPath)
{
    if (!some(aDirPath))
        return false;
    size_t lDirLen = strlen(aDirPath);
    if (lDirLen == 2 && aDirPath[1] == ':')
        return true;

    wchar* lTmpWChar = utf8to16(aDirPath);
    DWORD attribs = GetFileAttributes(lTmpWChar);
    free(lTmpWChar);
    return (attribs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

version (TINYFD_NOLIB) {

void _beep()
{
    printf("\a");
}

} else { // TINYFD_NOLIB

void _beep()
{
    Beep(440, 300);
}

void wipefileW(const wchar* aFilename)
{
    // file must exist beforehand
    FILE* lIn = _wfopen(aFilename, "w");
    if (lIn)
    {
        fseek(lIn, 0, SEEK_END);
        const size = ftell(lIn);
        rewind(lIn);
        foreach (i; 0 .. size)
        {
            fputc('A', lIn);
        }
        fclose(lIn);
    }
}

wchar* getPathWithoutFinalSlashW(
    wchar* aoDestination, /* make sure it is allocated, use _MAX_PATH */
    const wchar* aSource) /* aoDestination and aSource can be the same */
{
    const(wchar)* lTmp;
    if (aSource)
    {
        lTmp = wcsrchr(aSource, '/');
        if (!lTmp)
        {
            lTmp = wcsrchr(aSource, '\\');
        }
        if (lTmp)
        {
            wcsncpy(aoDestination, aSource, lTmp - aSource);
            aoDestination[lTmp - aSource] = '\0';
        }
        else
        {
            *aoDestination = '\0';
        }
    }
    else
    {
        *aoDestination = '\0';
    }
    return aoDestination;
}

wchar* getLastNameW(
    wchar* aoDestination, /* make sure it is allocated */
    const wchar* aSource)
{
    /* copy the last name after '/' or '\' */
    const(wchar)* lTmp;
    if (aSource)
    {
        lTmp = wcsrchr(aSource, '/');
        if (!lTmp)
        {
            lTmp = wcsrchr(aSource, '\\');
        }
        if (lTmp)
        {
            wcscpy(aoDestination, lTmp + 1);
        }
        else
        {
            wcscpy(aoDestination, aSource);
        }
    }
    else
    {
        *aoDestination = '\0';
    }
    return aoDestination;
}

static if (!is(WC_ERR_INVALID_CHARS))
/* undefined prior to Vista, so not yet in MINGW header file */
enum DWORD WC_ERR_INVALID_CHARS = 0x00000080;

int sizeUtf8(const wchar* aUtf16string)
{
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                               aUtf16string, -1, null, 0, null, null);
}

char* utf16to8(const wchar* str)
{
    if (!wsome(str))
        return null;

    int lSize = sizeUtf8(str);
    char* ret = cast(char*)malloc(lSize);
    lSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                str, -1, ret, lSize, null, null);
    if (lSize == 0)
    {
        free(ret);
        return null;
    }
    return ret;
}

bool replaceWchar(wchar* aString, const wchar aOldChr, const wchar aNewChr)
{
    if (!aString)
        return false;
    if (aOldChr == aNewChr)
        return false;

    bool ret;
    wchar* p = aString;
    while ((p = wcsrchr(p, aOldChr)) !is null)
    {
        *p = aNewChr;
        version (TINYFD_NOCCSUNICODE)
            p++;
        p++;
        ret = true;
    }
    return ret;
}

bool willBeGui()
{
    return (!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent())) &&
           (!getenv("SSH_CLIENT") || getenv("DISPLAY"));
}

extern (Windows) int EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
    wchar[MAX_PATH] lTitleName = '\0';
    GetWindowTextW(hwnd, lTitleName.ptr, MAX_PATH);
    /* wprintf("lTitleName %ls \n", lTitleName.ptr);  */
    if (wcscmp("tinyfiledialogsTopWindow", lTitleName.ptr) == 0)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return 0;
    }
    return 1;
}

void hiddenConsoleW(const wchar* aString, const wchar* aDialogTitle, const int aInFront)
{
    if (!wsome(aString))
        return;

    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    if (!CreateProcessW(null, cast(LPWSTR)aString, null, null, FALSE,
                        CREATE_NEW_CONSOLE, null, null,
                        &StartupInfo, &ProcessInfo))
    {
        return; /* GetLastError(); */
    }

    WaitForInputIdle(ProcessInfo.hProcess, INFINITE);
    if (aInFront)
    {
        while (EnumWindows(&EnumThreadWndProc, 0))
        {
        }
        SetWindowTextW(GetForegroundWindow(), aDialogTitle);
    }
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
}

int messageBoxWinGui(
    const char* aTitle,
    const char* aMessage,
    const char* aDialogType,
    const char* aIconType,
    const int aDefaultButton)
{
    wchar* lTitle = utf8to16(aTitle);
    wchar* lMessage = utf8to16(aMessage);

    UINT aCode;

    if (eq("warning", aIconType))
    {
        aCode = MB_ICONWARNING;
    }
    else if (eq("error", aIconType))
    {
        aCode = MB_ICONERROR;
    }
    else if (eq("question", aIconType))
    {
        aCode = MB_ICONQUESTION;
    }
    else
    {
        aCode = MB_ICONINFORMATION;
    }

    if (eq("okcancel", aDialogType))
    {
        aCode += MB_OKCANCEL;
        if (!aDefaultButton)
        {
            aCode += MB_DEFBUTTON2;
        }
    }
    else if (eq("yesno", aDialogType))
    {
        aCode += MB_YESNO;
        if (!aDefaultButton)
        {
            aCode += MB_DEFBUTTON2;
        }
    }
    else
    {
        aCode += MB_OK;
    }

    aCode += MB_TOPMOST;

    int lBoxReturnValue = MessageBoxW(GetForegroundWindow(), lMessage, lTitle, aCode);
    free(lTitle);
    free(lMessage);

    if (eq("ok", aDialogType) || lBoxReturnValue == IDOK || lBoxReturnValue == IDYES)
        return 1;
    else
        return 0;
}

int notifyWinGui(
    const char* aTitle,
    const char* aMessage,
    const char* aIconType)
{
    wchar* lTitle = utf8to16(aTitle);
    wchar* lMessage = utf8to16(aMessage);
    wchar* lIconType = utf8to16(aIconType);

    size_t lTitleLen = lTitle ? wcslen(lTitle) : 0;
    size_t lMessageLen = lMessage ? wcslen(lMessage) : 0;
    size_t lDialogStringLen = 3 * MAX_PATH_OR_CMD + lTitleLen + lMessageLen;
    wchar* str = cast(wchar*)malloc(lDialogStringLen * wchar.sizeof);

    wcscpy(str, "powershell.exe -command \"" ~
"function Show-BalloonTip {" ~
"[cmdletbinding()] " ~
"param( " ~
"[string]$Title = ' ', " ~
"[string]$Message = ' ', " ~
"[ValidateSet('info', 'warning', 'error')] " ~
"[string]$IconType = 'info');" ~
"[system.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms') | Out-Null ; " ~
"$balloon = New-Object System.Windows.Forms.NotifyIcon ; " ~
"$path = Get-Process -id $pid | Select-Object -ExpandProperty Path ; " ~
"$icon = [System.Drawing.Icon]::ExtractAssociatedIcon($path) ;");

    wcscat(str,
"$balloon.Icon = $icon ; " ~
"$balloon.BalloonTipIcon = $IconType ; " ~
"$balloon.BalloonTipText = $Message ; " ~
"$balloon.BalloonTipTitle = $Title ; " ~
"$balloon.Text = 'lalala' ; " ~
"$balloon.Visible = $true ; " ~
"$balloon.ShowBalloonTip(5000)};" ~
"Show-BalloonTip");

    if (wsome(lTitle))
    {
        wcscat(str, " -Title '");
        wcscat(str, lTitle);
        wcscat(str, "'");
    }
    if (wsome(lMessage))
    {
        wcscat(str, " -Message '");
        wcscat(str, lMessage);
        wcscat(str, "'");
    }
    if (wsome(lIconType))
    {
        wcscat(str, " -IconType '");
        wcscat(str, lIconType);
        wcscat(str, "'");
    }
    wcscat(str, "\"");

    /* wprintf ( "str: %ls\n" , str ) ; */

    hiddenConsoleW(str, lTitle, 0);

    free(str);
    free(lTitle);
    free(lMessage);
    free(lIconType);
    return 1;
}

const(wchar)* _inputBoxW(
    const wchar* aTitle,
    const wchar* aMessage,
    const wchar* aDefaultInput)
{
    static wchar[MAX_PATH_OR_CMD] lBuff = '\0';

    size_t lTitleLen = aTitle ? wcslen(aTitle) : 0;
    size_t lMessageLen = aMessage ? wcslen(aMessage) : 0;
    size_t lDialogStringLen = 3 * MAX_PATH_OR_CMD + lTitleLen + lMessageLen;
    wchar* str = cast(wchar*)malloc(lDialogStringLen * wchar.sizeof);

    wcscpy(str, _wgetenv("USERPROFILE"));
    wcscat(str, "\\AppData\\Local\\Temp\\tinyfd.");
    wcscat(str, aDefaultInput ? "vbs" : "hta");

    FILE* lIn = _wfopen(str, "w");
    if (!lIn)
    {
        free(str);
        return null;
    }

    if (aDefaultInput)
    {
        wcscpy(str, "Dim result:result=InputBox(\"");
        if (wsome(aMessage))
        {
            wcscpy(lBuff.ptr, aMessage);
            replaceWchar(lBuff.ptr, '\n', ' ');
            wcscat(str, lBuff.ptr);
        }
        wcscat(str, "\",\"tinyfiledialogsTopWindow\",\"");
        if (wsome(aDefaultInput))
        {
            wcscpy(lBuff.ptr, aDefaultInput);
            replaceWchar(lBuff.ptr, '\n', ' ');
            wcscat(str, lBuff.ptr);
        }
        wcscat(str, "\"):If IsEmpty(result) then:WScript.Echo 0");
        wcscat(str, ":Else: WScript.Echo \"1\" & result : End If");
    }
    else
    {
        wcscpy(str, `
<html>
<head>
<title>`);

        wcscat(str, "tinyfiledialogsTopWindow");
        wcscat(str, `</title>
<HTA:APPLICATION
ID = 'tinyfdHTA'
APPLICATIONNAME = 'tinyfd_inputBox'
MINIMIZEBUTTON = 'no'
MAXIMIZEBUTTON = 'no'
BORDER = 'dialog'
SCROLL = 'no'
SINGLEINSTANCE = 'yes'
WINDOWSTATE = 'hidden'>

<script language = 'VBScript'>

intWidth = Screen.Width/4
intHeight = Screen.Height/6
ResizeTo intWidth, intHeight
MoveTo((Screen.Width/2)-(intWidth/2)),((Screen.Height/2)-(intHeight/2))
result = 0

Sub Window_onLoad
txt_input.Focus
End Sub
`);

        wcscat(str,
`Sub Window_onUnload
Set objFSO = CreateObject("Scripting.FileSystemObject")
Set oShell = CreateObject("WScript.Shell")
strHomeFolder = oShell.ExpandEnvironmentStrings("%USERPROFILE%")
Set objFile = objFSO.CreateTextFile(strHomeFolder & "\AppData\Local\Temp\tinyfd.txt",True,True)
If result = 1 Then
objFile.Write 1 & txt_input.Value
Else
objFile.Write 0
End If
objFile.Close
End Sub

Sub Run_ProgramOK
result = 1
window.Close
End Sub

Sub Run_ProgramCancel
window.Close
End Sub
`);

        wcscat(str, `Sub Default_Buttons
If Window.Event.KeyCode = 13 Then
btn_OK.Click
ElseIf Window.Event.KeyCode = 27 Then
btn_Cancel.Click
End If
End Sub

</script>
</head>
<body style = 'background-color:#EEEEEE' onkeypress = 'vbs:Default_Buttons' align = 'top'>
<table width = '100%' height = '80%' align = 'center' border = '0'>
<tr border = '0'>
<td align = 'left' valign = 'middle' style='Font-Family:Arial'>
`);

        wcscat(str, aMessage ? aMessage : "");

        wcscat(str, `
</td>
<td align = 'right' valign = 'middle' style = 'margin-top: 0em'>
<table  align = 'right' style = 'margin-right: 0em;'>
<tr align = 'right' style = 'margin-top: 5em;'>
<input type = 'button' value = 'OK' name = 'btn_OK' onClick = 'vbs:Run_ProgramOK' style = 'width: 5em; margin-top: 2em;'><br>
<input type = 'button' value = 'Cancel' name = 'btn_Cancel' onClick = 'vbs:Run_ProgramCancel' style = 'width: 5em;'><br><br>
</tr>
</table>
</td>
</tr>
</table>
`);

        wcscat(str, `<table width = '100%' height = '100%' align = 'center' border = '0'>
<tr>
<td align = 'left' valign = 'top'>
<input type = 'password' id = 'txt_input'
name = 'txt_input' value = '' style = 'float:left;width:100%' ><BR>
</td>
</tr>
</table>
</body>
</html>
`);
    }
    fputws(str, lIn);
    fclose(lIn);

    if (aDefaultInput)
    {
        wcscpy(str, _wgetenv("USERPROFILE"));
        wcscat(str, "\\AppData\\Local\\Temp\\tinyfd.txt");

version (TINYFD_NOCCSUNICODE) {
        FILE* lFile = _wfopen(str, "w");
        fputc(0xFF, lFile);
        fputc(0xFE, lFile);
} else {
        FILE* lFile = _wfopen(str, "wt, ccs=UNICODE");  /*or ccs=UTF-16LE*/
}
        fclose(lFile);

        wcscpy(str, "cmd.exe /c cscript.exe //U //Nologo ");
        wcscat(str, "\"%USERPROFILE%\\AppData\\Local\\Temp\\tinyfd.vbs\" ");
        wcscat(str, ">> \"%USERPROFILE%\\AppData\\Local\\Temp\\tinyfd.txt\"");
    }
    else
    {
        wcscpy(str, "cmd.exe /c mshta.exe \"%USERPROFILE%\\AppData\\Local\\Temp\\tinyfd.hta\"");
    }

    /* wprintf ( "str: %ls\n" , str ) ; */

    hiddenConsoleW(str, aTitle, 1);

    wcscpy(str, _wgetenv("USERPROFILE"));
    wcscat(str, "\\AppData\\Local\\Temp\\tinyfd.txt");
    /* wprintf("str: %ls\n", str); */
    version (TINYFD_NOCCSUNICODE)
        const wchar* mode = "r";
    else
        const wchar* mode = "rt, ccs=UNICODE"; /*or ccs=UTF-16LE*/
    lIn = _wfopen(str, mode);
    if (!lIn)
    {
        _wremove(str);
        free(str);
        return null;
    }

    lBuff = '\0';

version (TINYFD_NOCCSUNICODE) {
    fgets(cast(char*)lBuff.ptr, 2 * MAX_PATH_OR_CMD, lIn);
} else {
    fgetws(lBuff.ptr, MAX_PATH_OR_CMD, lIn);
}
    fclose(lIn);
    wipefileW(str);
    _wremove(str);

    wcscpy(str, _wgetenv("USERPROFILE"));
    wcscat(str, "\\AppData\\Local\\Temp\\tinyfd.");
    wcscat(str, aDefaultInput ? "vbs" : "hta");
    _wremove(str);
    free(str);
    /* wprintf( "lBuff: %ls\n" , lBuff ) ; */
version (TINYFD_NOCCSUNICODE) {
    int lResult = !wcsncmp(lBuff.ptr + 1, "1", 1);
} else {
    int lResult = !wcsncmp(lBuff.ptr, "1", 1);
}

    /* printf( "lResult: %d \n" , lResult ) ; */
    if (!lResult)
        return null;

    /* wprintf( "lBuff+1: %ls\n" , lBuff+1 ) ; */

version (TINYFD_NOCCSUNICODE) {

    if (aDefaultInput)
    {
        lDialogStringLen = wcslen(lBuff.ptr);
        assert(lDialogStringLen >= 2);
        lBuff[lDialogStringLen - 1] = '\0';
        lBuff[lDialogStringLen - 2] = '\0';
    }
    return lBuff.ptr + 2;

} else {

    if (aDefaultInput)
    {
        lDialogStringLen = wcslen(lBuff.ptr);
        assert(lDialogStringLen > 0);
        lBuff[lDialogStringLen - 1] = '\0';
    }
    return lBuff.ptr + 1;
}
}

const(char)* inputBoxWinGui(
    char* aoBuff,
    const char* aTitle,
    const char* aMessage,
    const char* aDefaultInput)
{
    wchar* lTitle = utf8to16(aTitle);
    wchar* lMessage = utf8to16(aMessage);
    wchar* lDefaultInput = utf8to16(aDefaultInput);

    const(wchar)* lTmpWChar = _inputBoxW(lTitle, lMessage, lDefaultInput);

    free(lTitle);
    free(lMessage);
    free(lDefaultInput);

    if (!lTmpWChar)
        return null;

    char* lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}

wchar* buildFilterString(const TFD_Filter[] filters)
{
    bool isMime;
    const(char)*[MAX_PATTERNS] patterns;
    char[MAX_PATH_OR_CMD] str = '\0';
    char[MAX_PATH_OR_CMD] patternsStr = '\0';

    foreach (filter; filters)
    {
        const plen = getValidPatterns(filter, isMime, patterns);
        if (plen == 0 || isMime)
            continue;

        patternsStr[0] = '\0';
        foreach (i, pt; patterns[0 .. plen])
        {
            if (i != 0)
                strcat(patternsStr.ptr, ";");
            strcat(patternsStr.ptr, pt);
        }

        strcat(str.ptr, some(filter.description) ? filter.description : patternsStr.ptr);
        strcat(str.ptr, "\n");
        strcat(str.ptr, patternsStr.ptr);
        strcat(str.ptr, "\n");
    }
    strcat(str.ptr, "All Files\n*.*\n");

    wchar* ret = utf8to16(str.ptr);
    if (!ret)
        return null;

    wchar* p = ret;
    while ((p = wcschr(p, '\n')) !is null)
    {
        *p = '\0';
        p++;
    }
    return ret;
}

const(char)* saveFileDialogWinGui(
    char* aoBuff,
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const TFD_Filter[] aFilters)
{
    static wchar[MAX_PATH_OR_CMD] lBuff = '\0';
    wchar[MAX_PATH_OR_CMD] lDirname = '\0';

    wchar* lTitle = utf8to16(aTitle);
    wchar* lDefaultPathAndFile = utf8to16(aDefaultPathAndFile);
    wchar* lFilterStr = buildFilterString(aFilters);

    getPathWithoutFinalSlashW(lDirname.ptr, lDefaultPathAndFile);
    getLastNameW(lBuff.ptr, lDefaultPathAndFile);

    HRESULT lHResult = CoInitializeEx(null, 0);

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = OPENFILENAMEW.sizeof;
    ofn.hwndOwner = GetForegroundWindow();
    ofn.hInstance = null;
    ofn.lpstrFilter = lFilterStr;
    ofn.lpstrCustomFilter = null;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = lBuff.ptr;

    ofn.nMaxFile = MAX_PATH_OR_CMD;
    ofn.lpstrFileTitle = null;
    ofn.nMaxFileTitle = MAX_PATH_OR_CMD / 2;
    ofn.lpstrInitialDir = wsome(lDirname.ptr) ? lDirname.ptr : null;
    ofn.lpstrTitle = wsome(lTitle) ? lTitle : null;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = null;
    ofn.lCustData = 0;
    ofn.lpfnHook = null;
    ofn.lpTemplateName = null;

    wchar* lTmpWChar;
    if (GetSaveFileNameW(&ofn) == 0)
    {
        lTmpWChar = null;
    }
    else
    {
        lTmpWChar = lBuff.ptr;
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }

    free(lTitle);
    free(lDefaultPathAndFile);
    free(lFilterStr);

    if (!lTmpWChar)
        return null;

    char* lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}

const(char)* openFileDialogWinGui(
    char* aoBuff,
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const TFD_Filter[] aFilters,
    const bool aAllowMultipleSelects)
{
    static wchar[MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD] lBuff = '\0';
    wchar[MAX_PATH_OR_CMD] lDirname = '\0';

    wchar* lTitle = utf8to16(aTitle);
    wchar* lDefaultPathAndFile = utf8to16(aDefaultPathAndFile);
    wchar* lFilterStr = buildFilterString(aFilters);

    getPathWithoutFinalSlashW(lDirname.ptr, lDefaultPathAndFile);
    getLastNameW(lBuff.ptr, lDefaultPathAndFile);

    HRESULT lHResult = CoInitializeEx(null, 0);

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = OPENFILENAME.sizeof;
    ofn.hwndOwner = GetForegroundWindow();
    ofn.hInstance = null;
    ofn.lpstrFilter = lFilterStr;
    ofn.lpstrCustomFilter = null;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = lBuff.ptr;
    ofn.nMaxFile = MAX_PATH_OR_CMD;
    ofn.lpstrFileTitle = null;
    ofn.nMaxFileTitle = MAX_PATH_OR_CMD / 2;
    ofn.lpstrInitialDir = wsome(lDirname.ptr) ? lDirname.ptr : null;
    ofn.lpstrTitle = wsome(lTitle) ? lTitle : null;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = null;
    ofn.lCustData = 0;
    ofn.lpfnHook = null;
    ofn.lpTemplateName = null;

    if (aAllowMultipleSelects)
    {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }

    size_t[MAX_MULTIPLE_FILES] lLengths;
    wchar*[MAX_MULTIPLE_FILES] lPointers;
    wchar* lTmpWChar;
    if (GetOpenFileNameW(&ofn) == 0)
    {
        lTmpWChar = null;
    }
    else
    {
        size_t lBuffLen = wcslen(lBuff.ptr);
        lPointers[0] = lBuff.ptr + lBuffLen + 1;
        if (!aAllowMultipleSelects || (lPointers[0][0] == '\0'))
        {
            lTmpWChar = lBuff.ptr;
        }
        else
        {
            int i;
            do
            {
                lLengths[i] = wcslen(lPointers[i]);
                lPointers[i + 1] = lPointers[i] + lLengths[i] + 1;
                i++;
            } while (lPointers[i][0] != '\0');
            i--;
            wchar* p = lBuff.ptr + MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD - 1;
            *p = '\0';
            for (int j = i; j >= 0; j--)
            {
                p -= lLengths[j];
                memmove(p, lPointers[j], lLengths[j] * wchar.sizeof);
                p--;
                *p = '\\';
                p -= lBuffLen;
                memmove(p, lBuff.ptr, lBuffLen * wchar.sizeof);
                p--;
                *p = '|';
            }
            p++;
            lTmpWChar = p;
        }
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }

    free(lTitle);
    free(lDefaultPathAndFile);
    free(lFilterStr);

    if (!lTmpWChar)
        return null;

    char* lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}

version (TINYFD_SELECTFOLDERWIN) {

extern (Windows) int BrowseCallbackProcW(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, cast(LPARAM)pData);
    }
    return 0;
}

const(char)* selectFolderDialogWinGui(
    char* aoBuff,
    const char* aTitle,
    const char* aDefaultPath)
{
    static wchar[MAX_PATH_OR_CMD] lBuff = '\0';

    wchar* lTitle = utf8to16(aTitle);
    wchar* lDefaultPath = utf8to16(aDefaultPath);

    HRESULT lHResult = CoInitializeEx(null, COINIT_APARTMENTTHREADED);

    BROWSEINFOW bInfo;
    bInfo.hwndOwner = GetForegroundWindow();
    bInfo.pidlRoot = null;
    bInfo.pszDisplayName = lBuff.ptr;
    bInfo.lpszTitle = wsome(lTitle) ? lTitle : null;
    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        bInfo.ulFlags = BIF_USENEWUI;
    }
    bInfo.lpfn = &BrowseCallbackProcW;
    bInfo.lParam = cast(LPARAM)lDefaultPath;
    bInfo.iImage = -1;

    LPITEMIDLIST lpItem = SHBrowseForFolderW(&bInfo);
    if (lpItem)
    {
        SHGetPathFromIDListW(lpItem, lBuff.ptr);
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }

    char* lTmpChar = utf16to8(lBuff.ptr);
    if (lTmpChar)
    {
        strcpy(aoBuff, lTmpChar);
        free(lTmpChar);
    }
    free(lTitle);
    free(lDefaultPath);
    return aoBuff;
}

} // TINYFD_SELECTFOLDERWIN

const(char)* colorChooserWinGui(
    const char* aTitle,
    const char* aDefaultHexRGB,
    ref const ubyte[3] aDefaultRGB,
    ref ubyte[3] aoResultRGB)
{
    static char[8] lResultHexRGB = '\0';
    ubyte[3] lDefaultRGB = aDefaultRGB;

    HRESULT lHResult = CoInitializeEx(null, 0);

    if (aDefaultHexRGB)
    {
        Hex2RGB(aDefaultHexRGB, lDefaultRGB);
    }

    CHOOSECOLORW cc;
    COLORREF[16] crCustColors;
    /* we can't use aTitle */
    cc.lStructSize = CHOOSECOLOR.sizeof;
    cc.hwndOwner = GetForegroundWindow();
    cc.hInstance = null;
    cc.rgbResult = RGB(lDefaultRGB[0], lDefaultRGB[1], lDefaultRGB[2]);
    cc.lpCustColors = crCustColors.ptr;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
    cc.lCustData = 0;
    cc.lpfnHook = null;
    cc.lpTemplateName = null;

    const(char)* ret;
    if (ChooseColorW(&cc))
    {
        aoResultRGB[0] = GetRValue(cc.rgbResult);
        aoResultRGB[1] = GetGValue(cc.rgbResult);
        aoResultRGB[2] = GetBValue(cc.rgbResult);
        RGB2Hex(aoResultRGB, lResultHexRGB.ptr);
        ret = lResultHexRGB.ptr;
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }
    return ret;
}

} // TINYFD_NOLIB

int dialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        FILE* lIn = _popen("where dialog.exe", "r");
        if (!lIn)
        {
            ret = 0;
            return 0;
        }

        char[MAX_PATH_OR_CMD] lBuff = '\0';
        const char* lString = "dialog.exe";
        while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
        {
        }
        _pclose(lIn);
        removeLastNL(lBuff.ptr);
        if (strcmp(lBuff.ptr + strlen(lBuff.ptr) - strlen(lString), lString))
            ret = 0;
        else
            ret = 1;
    }
    return ret;
}

int messageBoxWinConsole(
    const char* aTitle,
    const char* aMessage,
    const char* aDialogType,
    const char* aIconType,
    const int aDefaultButton)
{
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char[MAX_PATH_OR_CMD] lDialogFile_buf = '\0';
    char* str = str_buf.ptr;
    char* lDialogFile = lDialogFile_buf.ptr;
    FILE* lIn;
    char[MAX_PATH_OR_CMD] lBuff = '\0';

    strcpy(str, "dialog ");
    if (some(aTitle))
    {
        strcat(str, "--title \"");
        strcat(str, aTitle);
        strcat(str, "\" ");
    }

    if (eq("okcancel", aDialogType) || eq("yesno", aDialogType) || eq("yesnocancel", aDialogType))
    {
        strcat(str, "--backtitle \"");
        strcat(str, "tab: move focus");
        strcat(str, "\" ");
    }

    if (eq("okcancel", aDialogType))
    {
        if (!aDefaultButton)
        {
            strcat(str, "--defaultno ");
        }
        strcat(str,
               "--yes-label \"Ok\" --no-label \"Cancel\" --yesno ");
    }
    else if (eq("yesno", aDialogType))
    {
        if (!aDefaultButton)
        {
            strcat(str, "--defaultno ");
        }
        strcat(str, "--yesno ");
    }
    else if (eq("yesnocancel", aDialogType))
    {
        if (!aDefaultButton)
        {
            strcat(str, "--defaultno ");
        }
        strcat(str, "--menu ");
    }
    else
    {
        strcat(str, "--msgbox ");
    }

    strcat(str, "\"");
    if (some(aMessage))
    {
        replaceSubStr(aMessage, "\n", "\\n", lBuff.ptr);
        strcat(str, lBuff.ptr);
        lBuff[0] = '\0';
    }
    strcat(str, "\" ");

    if (eq("yesnocancel", aDialogType))
    {
        strcat(str, "0 60 0 Yes \"\" No \"\"");
        strcat(str, "2>>");
    }
    else
    {
        strcat(str, "10 60");
        strcat(str, " && echo 1 > ");
    }

    strcpy(lDialogFile, getenv("USERPROFILE"));
    strcat(lDialogFile, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcat(str, lDialogFile);

    /*if (tinyfd_verbose) printf( "str: %s\n" , str ) ;*/
    system(str);

    lIn = fopen(lDialogFile, "r");
    if (!lIn)
    {
        remove(lDialogFile);
        return 0;
    }
    while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
    {
    }
    fclose(lIn);
    remove(lDialogFile);
    removeLastNL(lBuff.ptr);

    /* if (tinyfd_verbose) printf("lBuff: %s\n", lBuff.ptr); */
    if (!some(lBuff.ptr))
    {
        return 0;
    }

    if (eq("yesnocancel", aDialogType))
    {
        if (lBuff[0] == 'Y')
            return 1;
        else
            return 2;
    }

    return 1;
}

const(char)* inputBoxWinConsole(
    char* aoBuff,
    const char* aTitle,
    const char* aMessage,
    const char* aDefaultInput)
{
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char[MAX_PATH_OR_CMD] lDialogFile_buf = '\0';
    char* str = str_buf.ptr;
    char* lDialogFile = lDialogFile_buf.ptr;
    FILE* lIn;
    int lResult;

    strcpy(lDialogFile, getenv("USERPROFILE"));
    strcat(lDialogFile, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcpy(str, "echo|set /p=1 >");
    strcat(str, lDialogFile);
    strcat(str, " & ");

    strcat(str, "dialog ");
    if (some(aTitle))
    {
        strcat(str, "--title \"");
        strcat(str, aTitle);
        strcat(str, "\" ");
    }

    strcat(str, "--backtitle \"");
    strcat(str, "tab: move focus");
    if (!aDefaultInput)
    {
        strcat(str, " (sometimes nothing, no blink nor star, is shown in text field)");
    }

    strcat(str, "\" ");

    if (!aDefaultInput)
    {
        strcat(str, "--insecure --passwordbox");
    }
    else
    {
        strcat(str, "--inputbox");
    }
    strcat(str, " \"");
    if (some(aMessage))
    {
        strcat(str, aMessage);
    }
    strcat(str, "\" 10 60 ");
    if (some(aDefaultInput))
    {
        strcat(str, "\"");
        strcat(str, aDefaultInput);
        strcat(str, "\" ");
    }

    strcat(str, "2>>");
    strcpy(lDialogFile, getenv("USERPROFILE"));
    strcat(lDialogFile, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcat(str, lDialogFile);
    strcat(str, " || echo 0 > ");
    strcat(str, lDialogFile);

    /* printf( "str: %s\n" , str ) ; */
    system(str);

    lIn = fopen(lDialogFile, "r");
    if (!lIn)
    {
        remove(lDialogFile);
        return null;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) !is null)
    {
    }
    fclose(lIn);

    wipefile(lDialogFile);
    remove(lDialogFile);
    removeLastNL(aoBuff);
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */

    /* printf( "aoBuff: %s len: %lu \n" , aoBuff , strlen(aoBuff) ) ; */
    lResult = strncmp(aoBuff, "1", 1) ? 0 : 1;
    /* printf( "lResult: %d \n" , lResult ) ; */
    if (!lResult)
        return null;
    /* printf( "aoBuff+1: %s\n" , aoBuff+1 ) ; */
    return aoBuff + 3;
}

const(char)* saveFileDialogWinConsole(
    char* aoBuff,
    const char* aTitle,
    const char* aDefaultPathAndFile)
{
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char[MAX_PATH_OR_CMD] lPathAndFile_buf = '\0';
    char* str = str_buf.ptr;
    char* lPathAndFile = lPathAndFile_buf.ptr;
    FILE* lIn;

    strcpy(str, "dialog ");
    if (some(aTitle))
    {
        strcat(str, "--title \"");
        strcat(str, aTitle);
        strcat(str, "\" ");
    }

    strcat(str, "--backtitle \"");
    strcat(str,
           "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY");
    strcat(str, "\" ");

    strcat(str, "--fselect \"");
    if (some(aDefaultPathAndFile))
    {
        /* dialog.exe uses unix separators even on windows */
        strcpy(lPathAndFile, aDefaultPathAndFile);
        replaceChr(lPathAndFile, '\\', '/');
    }

    /* dialog.exe needs at least one separator */
    if (!strchr(lPathAndFile, '/'))
    {
        strcat(str, "./");
    }
    strcat(str, lPathAndFile);
    strcat(str, "\" 0 60 2>");
    strcpy(lPathAndFile, getenv("USERPROFILE"));
    strcat(lPathAndFile, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcat(str, lPathAndFile);

    /* printf( "str: %s\n" , str ) ; */
    system(str);

    lIn = fopen(lPathAndFile, "r");
    if (!lIn)
    {
        remove(lPathAndFile);
        return null;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) !is null)
    {
    }
    fclose(lIn);
    remove(lPathAndFile);
    replaceChr(aoBuff, '/', '\\');
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */
    getLastName(str, aoBuff);
    if (!some(str))
        return null;
    return aoBuff;
}

const(char)* openFileDialogWinConsole(
    char* aoBuff,
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const bool aAllowMultipleSelects)
{
    char[MAX_PATH_OR_CMD] lFilterPatterns = '\0';
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char* str = str_buf.ptr;
    FILE* lIn;

    strcpy(str, "dialog ");
    if (some(aTitle))
    {
        strcat(str, "--title \"");
        strcat(str, aTitle);
        strcat(str, "\" ");
    }

    strcat(str, "--backtitle \"");
    strcat(str,
           "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY");
    strcat(str, "\" ");

    strcat(str, "--fselect \"");
    if (some(aDefaultPathAndFile))
    {
        /* dialog.exe uses unix separators even on windows */
        strcpy(lFilterPatterns.ptr, aDefaultPathAndFile);
        replaceChr(lFilterPatterns.ptr, '\\', '/');
    }

    /* dialog.exe needs at least one separator */
    if (!strchr(lFilterPatterns.ptr, '/'))
    {
        strcat(str, "./");
    }
    strcat(str, lFilterPatterns.ptr);
    strcat(str, "\" 0 60 2>");
    strcpy(lFilterPatterns.ptr, getenv("USERPROFILE"));
    strcat(lFilterPatterns.ptr, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcat(str, lFilterPatterns.ptr);

    /* printf( "str: %s\n" , str ) ; */
    system(str);

    lIn = fopen(lFilterPatterns.ptr, "r");
    if (!lIn)
    {
        remove(lFilterPatterns.ptr);
        return null;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) !is null)
    {
    }
    fclose(lIn);
    remove(lFilterPatterns.ptr);
    replaceChr(aoBuff, '/', '\\');
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */
    return aoBuff;
}

const(char)* selectFolderDialogWinConsole(
    char* aoBuff,
    const char* aTitle,
    const char* aDefaultPath)
{
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char[MAX_PATH_OR_CMD] lString_buf = '\0';
    char* str = str_buf.ptr;
    char* lString = lString_buf.ptr;
    FILE* lIn;

    strcpy(str, "dialog ");
    if (some(aTitle))
    {
        strcat(str, "--title \"");
        strcat(str, aTitle);
        strcat(str, "\" ");
    }

    strcat(str, "--backtitle \"");
    strcat(str,
           "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY");
    strcat(str, "\" ");

    strcat(str, "--dselect \"");
    if (some(aDefaultPath))
    {
        /* dialog.exe uses unix separators even on windows */
        strcpy(lString, aDefaultPath);
        ensureFinalSlash(lString);
        replaceChr(lString, '\\', '/');
        strcat(str, lString);
    }
    else
    {
        /* dialog.exe needs at least one separator */
        strcat(str, "./");
    }
    strcat(str, "\" 0 60 2>");
    strcpy(lString, getenv("USERPROFILE"));
    strcat(lString, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcat(str, lString);

    /* printf( "str: %s\n" , str ) ; */
    system(str);

    lIn = fopen(lString, "r");
    if (!lIn)
    {
        remove(lString);
        return null;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) !is null)
    {
    }
    fclose(lIn);
    remove(lString);
    replaceChr(aoBuff, '/', '\\');
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */
    return aoBuff;
}

int _messageBox(
    const char* aTitle,
    const char* aMessage,
    const char* aDialogType,
    const char* aIconType,
    int aDefaultButton)
{
    char lChar;
    const bool lQuery = eq(aTitle, "tinyfd_query");

    version (TINYFD_LIB) {

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return 1;
        }
        return messageBoxWinGui(
            aTitle, aMessage, aDialogType, aIconType, aDefaultButton);
    }
}
    if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return 0;
        }
        return messageBoxWinConsole(
            aTitle, aMessage, aDialogType, aIconType, aDefaultButton);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return 0;
        }
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = true;
            printf("\n\n%s\n", gTitle.ptr);
            printf("%s\n\n", tinyfd_needs.ptr);
        }
        if (some(aTitle))
        {
            printf("\n%s\n\n", aTitle);
        }
        if (eq("yesno", aDialogType))
        {
            do
            {
                if (some(aMessage))
                {
                    printf("%s\n", aMessage);
                }
                printf("y/n: ");
                lChar = cast(char)tolower(_getch());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n');
            return lChar == 'y' ? 1 : 0;
        }
        else if (eq("okcancel", aDialogType))
        {
            do
            {
                if (some(aMessage))
                {
                    printf("%s\n", aMessage);
                }
                printf("[O]kay/[C]ancel: ");
                lChar = cast(char)tolower(_getch());
                printf("\n\n");
            } while (lChar != 'o' && lChar != 'c');
            return lChar == 'o' ? 1 : 0;
        }
        else if (eq("yesnocancel", aDialogType))
        {
            do
            {
                if (some(aMessage))
                {
                    printf("%s\n", aMessage);
                }
                printf("[Y]es/[N]o/[C]ancel: ");
                lChar = cast(char)tolower(_getch());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n' && lChar != 'c');
            return (lChar == 'y') ? 1 : (lChar == 'n') ? 2 : 0;
        }
        else
        {
            if (some(aMessage))
            {
                printf("%s\n\n", aMessage);
            }
            printf("press enter to continue ");
            lChar = cast(char)_getch();
            printf("\n\n");
            return 1;
        }
    }
}

int _notifyPopup(
    const char* aTitle,
    const char* aMessage,
    const char* aIconType)
{
    const bool lQuery = eq(aTitle, "tinyfd_query");

    version (TINYFD_LIB) {

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return 1;
        }
        return notifyWinGui(aTitle, aMessage, aIconType);
    }
}
    return _messageBox(aTitle, aMessage, "ok", aIconType, 0);
}

const(char*) _inputBox(
    const char* aTitle,
    const char* aMessage,
    const char* aDefaultInput)
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    char* lEOF;
    const bool lQuery = eq(aTitle, "tinyfd_query");

    version (TINYFD_LIB) {

    DWORD mode = 0;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return cast(const(char)*)1;
        }
        lBuff[0] = '\0';
        return inputBoxWinGui(lBuff.ptr, aTitle, aMessage, aDefaultInput);
    }
}
    if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return cast(const(char)*)0;
        }
        lBuff[0] = '\0';
        return inputBoxWinConsole(lBuff.ptr, aTitle, aMessage, aDefaultInput);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return cast(const(char)*)0;
        }
        lBuff[0] = '\0';
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = true;
            printf("\n\n%s\n", gTitle.ptr);
            printf("%s\n\n", tinyfd_needs.ptr);
        }
        if (some(aTitle))
        {
            printf("\n%s\n\n", aTitle);
        }
        if (some(aMessage))
        {
            printf("%s\n", aMessage);
        }
        printf("(ctrl-Z + enter to cancel): ");
        version (TINYFD_LIB)
        {
            if (!aDefaultInput)
            {
                GetConsoleMode(hStdin, &mode);
                SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
            }
        }

        lEOF = fgets(lBuff.ptr, lBuff.sizeof, stdin);
        if (!lEOF)
            return null;

        version (TINYFD_LIB)
        {
            if (!aDefaultInput)
            {
                SetConsoleMode(hStdin, mode);
                printf("\n");
            }
        }
        printf("\n");
        if (strchr(lBuff.ptr, 27))
            return null;
        removeLastNL(lBuff.ptr);
        return lBuff.ptr;
    }
}

const(char*) _saveFileDialog(
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const TFD_Filter[] aFilters)
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char[MAX_PATH_OR_CMD] lString = '\0';
    const(char)* p;
    lBuff[0] = '\0';

    version (TINYFD_LIB) {

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return cast(const(char)*)1;
        }
        p = saveFileDialogWinGui(lBuff.ptr, aTitle, aDefaultPathAndFile, aFilters);
        goto end;
    }
}
    if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return cast(const(char)*)0;
        }
        p = saveFileDialogWinConsole(lBuff.ptr, aTitle, aDefaultPathAndFile);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return cast(const(char)*)0;
        }
        p = _inputBox(aTitle, "Save file", "");
    }

    end:

    if (!some(p))
        return null;
    getPathWithoutFinalSlash(lString.ptr, p);
    if (!dirExists(lString.ptr))
        return null;
    getLastName(lString.ptr, p);
    if (!filenameValid(lString.ptr))
        return null;
    return p;
}

const(char*) _openFileDialog(
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const TFD_Filter[] aFilters,
    const bool aAllowMultipleSelects)
{
    static char[MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    const(char)* p;

    version (TINYFD_LIB) {

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return cast(const(char)*)1;
        }
        p = openFileDialogWinGui(lBuff.ptr, aTitle, aDefaultPathAndFile, aFilters, aAllowMultipleSelects);
        goto end;
    }
}
    if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return cast(const(char)*)0;
        }
        p = openFileDialogWinConsole(lBuff.ptr,
                                     aTitle, aDefaultPathAndFile, aAllowMultipleSelects);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return cast(const(char)*)0;
        }
        p = _inputBox(aTitle, "Open file", "");
    }

    end:

    if (!some(p))
        return null;
    if (aAllowMultipleSelects && strchr(p, '|'))
        p = ensureFilesExist(lBuff.ptr, p);
    else if (!fileExists(p))
        return null;
    return p;
}

const(char*) _selectFolderDialog(const char* aTitle, const char* aDefaultPath)
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    const(char)* p;

    version (TINYFD_LIB) {

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return cast(const(char)*)1;
        }
        version (TINYFD_SELECTFOLDERWIN)
        {
            p = selectFolderDialogWinGui(lBuff.ptr, aTitle, aDefaultPath);
            return dirExists(p) ? p : null;
        }
    }
}
    if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return cast(const(char)*)0;
        }
        p = selectFolderDialogWinConsole(lBuff.ptr, aTitle, aDefaultPath);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return cast(const(char)*)0;
        }
        p = _inputBox(aTitle, "Select folder", "");
    }
    return dirExists(p) ? p : null;
}

const(char*) _colorChooser(
    const char* aTitle,
    const char* aDefaultHexRGB,
    ref const ubyte[3] aDefaultRGB,
    ref ubyte[3] aoResultRGB)
{
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char[8] lDefaultHexRGB = '\0';
    char* lpDefaultHexRGB;
    const(char)* p;

    version (TINYFD_LIB) {

    if (willBeGui())
    {
        if (lQuery)
        {
            response("windows");
            return cast(const(char)*)1;
        }
        return colorChooserWinGui(aTitle, aDefaultHexRGB, aDefaultRGB, aoResultRGB);
    }
}
    if (aDefaultHexRGB)
    {
        lpDefaultHexRGB = cast(char*)aDefaultHexRGB;
    }
    else
    {
        RGB2Hex(aDefaultRGB, lDefaultHexRGB.ptr);
        lpDefaultHexRGB = cast(char*)lDefaultHexRGB;
    }
    p = _inputBox(aTitle, "Enter hex rgb color (i.e. #f5ca20)", lpDefaultHexRGB);
    if (lQuery)
        return p;

    if (!p || strlen(p) != 7 || p[0] != '#')
        return null;

    foreach (i; 1 .. 7)
    {
        if (!isxdigit(p[i]))
            return null;
    }
    Hex2RGB(p, aoResultRGB);
    return p;
}

} else { // unix

char[16] gPython2Name = '\0';
char[16] gPython3Name = '\0';
char[16] gPythonName = '\0';

int isDarwin()
{
    static int ret = -1;
    utsname lUtsname;
    if (ret < 0)
    {
        ret = !uname(&lUtsname) && eq(lUtsname.sysname.ptr, "Darwin");
    }
    return ret;
}

bool dirExists(const char* aDirPath)
{
    if (!some(aDirPath))
        return false;
    DIR* lDir = opendir(aDirPath);
    if (!lDir)
        return false;
    closedir(lDir);
    return true;
}

bool detectPresence(const char* aExecutable)
{
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    char[MAX_PATH_OR_CMD] lTestedString = "which ";

    strcat(lTestedString.ptr, aExecutable);
    strcat(lTestedString.ptr, " 2>/dev/null ");
    FILE* lIn = popen(lTestedString.ptr, "r");
    if (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null && !strchr(lBuff.ptr, ':') && strncmp(lBuff.ptr, "no ", 3))
    {
        // present
        pclose(lIn);
        if (tinyfd_verbose)
            printf("detectPresence %s %d\n", aExecutable, 1);
        return true;
    }
    else
    {
        pclose(lIn);
        if (tinyfd_verbose)
            printf("detectPresence %s %d\n", aExecutable, 0);
        return false;
    }
}

const(char)* getVersion(const char* aExecutable) /*version must be first numeral*/
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    char[MAX_PATH_OR_CMD] lTestedString = '\0';
    FILE* lIn;
    char* lTmp;

    strcpy(lTestedString.ptr, aExecutable);
    strcat(lTestedString.ptr, " --version");

    lIn = popen(lTestedString.ptr, "r");
    lTmp = fgets(lBuff.ptr, lBuff.sizeof, lIn);
    pclose(lIn);

    lTmp += strcspn(lTmp, "0123456789");
    /* printf("lTmp:%s\n", lTmp); */
    return lTmp;
}

const(int)* getMajorMinorPatch(const char* aExecutable)
{
    static int[3] lArray;
    char* lTmp;

    lTmp = cast(char*)getVersion(aExecutable);
    lArray[0] = atoi(strtok(lTmp, " ,.-"));
    /* printf("lArray0 %d\n", lArray[0]); */
    lArray[1] = atoi(strtok(null, " ,.-"));
    /* printf("lArray1 %d\n", lArray[1]); */
    lArray[2] = atoi(strtok(null, " ,.-"));
    /* printf("lArray2 %d\n", lArray[2]); */

    if (!lArray[0] && !lArray[1] && !lArray[2])
        return null;
    return lArray.ptr;
}

bool tryCommand(const char* aCommand)
{
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    FILE* lIn = popen(aCommand, "r");
    const bool present = fgets(lBuff.ptr, lBuff.sizeof, lIn) is null;
    pclose(lIn);
    return present;
}

int isTerminalRunning()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = isatty(1);
        if (tinyfd_verbose)
            printf("isTerminalRunning %d\n", ret);
    }
    return ret;
}

const(char)* dialogNameOnly()
{
    static char[128] ret = "*";
    if (ret[0] == '*')
    {
        if (isDarwin() && strcpy(ret.ptr, "/opt/local/bin/dialog") && detectPresence(ret.ptr))
        {
        }
        else if (strcpy(ret.ptr, "dialog") && detectPresence(ret.ptr))
        {
        }
        else
        {
            strcpy(ret.ptr, "");
        }
    }
    return ret.ptr;
}

bool isDialogVersionBetter09b()
{
    const(char)* lDialogName;
    char* lVersion;
    int lMajor;
    int lMinor;
    int lDate;
    int lResult;
    char* lMinorP;
    char* lLetter;
    char[128] lBuff = '\0';

    /*char[128] lTest = " 0.9b-20031126" ;*/

    lDialogName = dialogNameOnly();
    lVersion = cast(char*)getVersion(lDialogName);
    if (!some(lDialogName) || !lVersion)
        return false;
    /*lVersion = lTest ;*/
    /*printf("lVersion %s\n", lVersion);*/
    strcpy(lBuff.ptr, lVersion);
    lMajor = atoi(strtok(lVersion, " ,.-"));
    /*printf("lMajor %d\n", lMajor);*/
    lMinorP = strtok(null, " ,.-abcdefghijklmnopqrstuvxyz");
    lMinor = atoi(lMinorP);
    /*printf("lMinor %d\n", lMinor );*/
    lDate = atoi(strtok(null, " ,.-"));
    if (lDate < 0)
        lDate = -lDate;
    /*printf("lDate %d\n", lDate);*/
    lLetter = lMinorP + strlen(lMinorP);
    strcpy(lVersion, lBuff.ptr);
    strtok(lLetter, " ,.-");
    /*printf("lLetter %s\n", lLetter);*/
    return lMajor > 0 || (lMinor == 9 && *lLetter == 'b' && lDate >= 20031126);
}

int whiptailPresentOnly()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("whiptail");
    }
    return ret;
}

const(char)* terminalName()
{
    static char[128] ret_buf = "*";
    char[64] shellName_buf = "*";
    char* ret = ret_buf.ptr;
    char* lShellName = shellName_buf.ptr;
    const(int)* lArray;

    if (ret[0] == '*')
    {
        if (detectPresence("bash"))
        {
            strcpy(lShellName, "bash -c "); /*good for basic input*/
        }
        else if (some(dialogNameOnly()) || whiptailPresentOnly())
        {
            strcpy(lShellName, "sh -c "); /*good enough for dialog & whiptail*/
        }
        else
        {
            strcpy(ret, "");
            return null;
        }

        if (isDarwin())
        {
            if (strcpy(ret, "/opt/X11/bin/xterm") && detectPresence(ret))
            {
                strcat(ret, " -fa 'DejaVu Sans Mono' -fs 10 -title tinyfiledialogs -e ");
                strcat(ret, lShellName);
            }
            else
            {
                strcpy(ret, "");
            }
        }
        else if (strcpy(ret, "xterm") /*good (small without parameters)*/
                 && detectPresence(ret))
        {
            strcat(ret, " -fa 'DejaVu Sans Mono' -fs 10 -title tinyfiledialogs -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "terminator") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -x ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "lxterminal") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "konsole") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "kterm") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "tilix") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "xfce4-terminal") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -x ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "mate-terminal") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -x ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "Eterm") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "evilvte") /*good*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "pterm") /*good (only letters)*/
                 && detectPresence(ret))
        {
            strcat(ret, " -e ");
            strcat(ret, lShellName);
        }
        else if (strcpy(ret, "gnome-terminal") && detectPresence(ret))
        {
            lArray = getMajorMinorPatch(ret);
            if (lArray[0] < 3 || lArray[0] == 3 && lArray[1] <= 6)
            {
                strcat(ret, " --disable-factory -x ");
                strcat(ret, lShellName);
            }
            else
            {
                strcpy(ret, "");
            }
        }
        else
        {
            strcpy(ret, "");
        }
        /* bad: koi rxterm guake tilda vala-terminal qterminal
                aterm Terminal terminology sakura lilyterm weston-terminal
                roxterm termit xvt rxvt mrxvt urxvt */
    }
    return some(ret) ? ret : null;
}

const(char)* dialogName()
{
    const(char)* ret;
    ret = dialogNameOnly();
    if (some(ret) && (isTerminalRunning() || terminalName()))
    {
        return ret;
    }
    else
        return null;
}

int whiptailPresent()
{
    int ret;
    ret = whiptailPresentOnly();
    if (ret && (isTerminalRunning() || terminalName()))
    {
        return ret;
    }
    else
        return 0;
}

int graphicMode()
{
    return !(tinyfd_forceConsole && (isTerminalRunning() || terminalName())) && (getenv("DISPLAY") || (isDarwin() && (!getenv("SSH_TTY") || getenv("DISPLAY"))));
}

int pactlPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("pactl");
    }
    return ret;
}

int speakertestPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("speaker-test");
    }
    return ret;
}

int beepexePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("beep.exe");
    }
    return ret;
}

int xmessagePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("xmessage"); /*if not tty,not on osxpath*/
    }
    return ret && graphicMode();
}

int gxmessagePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("gxmessage");
    }
    return ret && graphicMode();
}

int gmessagePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("gmessage");
    }
    return ret && graphicMode();
}

int notifysendPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("notify-send");
    }
    return ret && graphicMode();
}

int perlPresent()
{
    static int ret = -1;
    char[MAX_PATH_OR_CMD] lBuff = 0;
    FILE* lIn;

    if (ret < 0)
    {
        ret = detectPresence("perl");
        if (ret)
        {
            lIn = popen("perl -MNet::DBus -e \"Net::DBus->session->get_service('org.freedesktop.Notifications')\" 2>&1", "r");
            if (fgets(lBuff.ptr, lBuff.sizeof, lIn) is null)
            {
                ret = 2;
            }
            pclose(lIn);
            if (tinyfd_verbose)
                printf("perl-dbus %d\n", ret);
        }
    }
    return graphicMode() ? ret : 0;
}

int afplayPresent()
{
    static int ret = -1;
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    FILE* lIn;

    if (ret < 0)
    {
        ret = detectPresence("afplay");
        if (ret)
        {
            lIn = popen("test -e /System/Library/Sounds/Ping.aiff || echo Ping", "r");
            if (fgets(lBuff.ptr, lBuff.sizeof, lIn) is null)
            {
                ret = 2;
            }
            pclose(lIn);
            if (tinyfd_verbose)
                printf("afplay %d\n", ret);
        }
    }
    return graphicMode() ? ret : 0;
}

int xdialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("Xdialog");
    }
    return ret && graphicMode();
}

int gdialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("gdialog");
    }
    return ret && graphicMode();
}

int osascriptPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        gWarningDisplayed |= !!getenv("SSH_TTY");
        ret = detectPresence("osascript");
    }
    return ret && graphicMode() && !getenv("SSH_TTY");
}

int qarmaPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("qarma");
    }
    return ret && graphicMode();
}

int matedialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("matedialog");
    }
    return ret && graphicMode();
}

int shellementaryPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = 0; /*detectPresence("shellementary"); shellementary is not ready yet */
    }
    return ret && graphicMode();
}

int zenityPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("zenity");
    }
    return ret && graphicMode();
}

int zenity3Present()
{
    static int ret = -1;
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    FILE* lIn;
    int lIntTmp;

    if (ret < 0)
    {
        ret = 0;
        if (zenityPresent())
        {
            lIn = popen("zenity --version", "r");
            if (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
            {
                if (atoi(lBuff.ptr) >= 3)
                {
                    ret = 3;
                    lIntTmp = atoi(strtok(lBuff.ptr, ".") + 2);
                    if (lIntTmp >= 18)
                    {
                        ret = 5;
                    }
                    else if (lIntTmp >= 10)
                    {
                        ret = 4;
                    }
                }
                else if ((atoi(lBuff.ptr) == 2) && (atoi(strtok(lBuff.ptr, ".") + 2) >= 32))
                {
                    ret = 2;
                }
                if (tinyfd_verbose)
                    printf("zenity %d\n", ret);
            }
            pclose(lIn);
        }
    }
    return graphicMode() ? ret : 0;
}

int kdialogPresent()
{
    static int ret = -1;
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    FILE* lIn;
    char* lDesktop;

    if (ret < 0)
    {
        if (zenityPresent())
        {
            lDesktop = getenv("XDG_SESSION_DESKTOP");
            if (!eq(lDesktop, "KDE") && !eq(lDesktop, "lxqt"))
            {
                ret = 0;
                return ret;
            }
        }

        ret = detectPresence("kdialog");
        if (ret && !getenv("SSH_TTY"))
        {
            lIn = popen("kdialog --attach 2>&1", "r");
            if (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
            {
                if (!strstr("Unknown", lBuff.ptr))
                {
                    ret = 2;
                    if (tinyfd_verbose)
                        printf("kdialog-attach %d\n", ret);
                }
            }
            pclose(lIn);

            if (ret == 2)
            {
                ret = 1;
                lIn = popen("kdialog --passivepopup 2>&1", "r");
                if (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
                {
                    if (!strstr("Unknown", lBuff.ptr))
                    {
                        ret = 2;
                        if (tinyfd_verbose)
                            printf("kdialog-popup %d\n", ret);
                    }
                }
                pclose(lIn);
            }
        }
    }
    return graphicMode() ? ret : 0;
}

int osx9orBetter()
{
    static int ret = -1;
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    FILE* lIn;
    int V, v;

    if (ret < 0)
    {
        ret = 0;
        lIn = popen("osascript -e 'set osver to system version of (system info)'", "r");
        if ((fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null) && (2 == sscanf(lBuff.ptr, "%d.%d", &V, &v)))
        {
            V = V * 100 + v;
            if (V >= 1009)
            {
                ret = 1;
            }
        }
        pclose(lIn);
        if (tinyfd_verbose)
            printf("Osx10 = %d, %d = %s\n", ret, V, lBuff.ptr);
    }
    return ret;
}

int python2Present()
{
    static int ret = -1;

    if (ret < 0)
    {
        ret = 0;
        strcpy(gPython2Name.ptr, "python2");
        if (detectPresence(gPython2Name.ptr))
            ret = 1;
        else
        {
            for (int i = 9; i >= 0; i--)
            {
                sprintf(gPython2Name.ptr, "python2.%d", i);
                if (detectPresence(gPython2Name.ptr))
                {
                    ret = 1;
                    break;
                }
            }
            /*if ( ! ret )
            {
                strcpy(gPython2Name , "python" ) ;
                if ( detectPresence(gPython2Name.ptr) ) ret = 1;
            }*/
        }
        if (tinyfd_verbose)
            printf("python2Present %d\n", ret);
        if (tinyfd_verbose)
            printf("gPython2Name %s\n", gPython2Name.ptr);
    }
    return ret;
}

int python3Present()
{
    static int ret = -1;

    if (ret < 0)
    {
        ret = 0;
        strcpy(gPython3Name.ptr, "python3");
        if (detectPresence(gPython3Name.ptr))
            ret = 1;
        else
        {
            for (int i = 9; i >= 0; i--)
            {
                sprintf(gPython3Name.ptr, "python3.%d", i);
                if (detectPresence(gPython3Name.ptr))
                {
                    ret = 1;
                    break;
                }
            }
            /*if ( ! ret )
            {
                strcpy(gPython3Name , "python" ) ;
                if ( detectPresence(gPython3Name.ptr) ) ret = 1;
            }*/
        }
        if (tinyfd_verbose)
            printf("python3Present %d\n", ret);
        if (tinyfd_verbose)
            printf("gPython3Name %s\n", gPython3Name.ptr);
    }
    return ret;
}

int tkinter2Present()
{
    static int ret = -1;
    char[256] lPythonCommand = '\0';
    char[256] lPythonParams =
        "-S -c \"try:\n\timport Tkinter;\nexcept:\n\tprint 0;\"";

    if (ret < 0)
    {
        ret = 0;
        if (python2Present())
        {
            sprintf(lPythonCommand.ptr, "%s %s", gPython2Name.ptr, lPythonParams.ptr);
            ret = tryCommand(lPythonCommand.ptr);
        }
        if (tinyfd_verbose)
            printf("tkinter2Present %d\n", ret);
    }
    return ret && graphicMode() && !(isDarwin() && getenv("SSH_TTY"));
}

int tkinter3Present()
{
    static int ret = -1;
    char[256] lPythonCommand = '\0';
    char[256] lPythonParams =
        "-S -c \"try:\n\timport tkinter;\nexcept:\n\tprint(0);\"";

    if (ret < 0)
    {
        ret = 0;
        if (python3Present())
        {
            sprintf(lPythonCommand.ptr, "%s %s", gPython3Name.ptr, lPythonParams.ptr);
            ret = tryCommand(lPythonCommand.ptr);
        }
        if (tinyfd_verbose)
            printf("tkinter3Present %d\n", ret);
    }
    return ret && graphicMode() && !(isDarwin() && getenv("SSH_TTY"));
}

int pythonDbusPresent()
{
    static int ret = -1;
    char[256] lPythonCommand = '\0';
    char[256] lPythonParams =
`-c "try:
    import dbus
    bus=dbus.SessionBus()
    notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications')
    notify=dbus.Interface(notif,'org.freedesktop.Notifications')
except:
    print(0)"`;

    if (ret < 0)
    {
        ret = 0;
        if (python2Present())
        {
            strcpy(gPythonName.ptr, gPython2Name.ptr);
            sprintf(lPythonCommand.ptr, "%s %s", gPythonName.ptr, lPythonParams.ptr);
            ret = tryCommand(lPythonCommand.ptr);
        }

        if (!ret && python3Present())
        {
            strcpy(gPythonName.ptr, gPython3Name.ptr);
            sprintf(lPythonCommand.ptr, "%s %s", gPythonName.ptr, lPythonParams.ptr);
            ret = tryCommand(lPythonCommand.ptr);
        }

        if (tinyfd_verbose)
            printf("dbusPresent %d\n", ret);
        if (tinyfd_verbose)
            printf("gPythonName %s\n", gPythonName.ptr);
    }
    return ret && graphicMode() && !(isDarwin() && getenv("SSH_TTY"));
}

void sigHandler(int sig)
{
    FILE* lIn = popen("pactl unload-module module-sine", "r");
    if (lIn)
    {
        pclose(lIn);
    }
}

void _beep()
{
    char[256] str_buf = '\0';
    char* str = str_buf.ptr;
    FILE* lIn;

    if (osascriptPresent())
    {
        if (afplayPresent() >= 2)
        {
            strcpy(str, "afplay /System/Library/Sounds/Ping.aiff");
        }
        else
        {
            strcpy(str, "osascript -e 'tell application \"System Events\" to beep'");
        }
    }
    else if (pactlPresent())
    {
        signal(SIGINT, &sigHandler);
        /*strcpy( str , "pactl load-module module-sine frequency=440;sleep .3;pactl unload-module module-sine" ) ;*/
        strcpy(str, "thnum=$(pactl load-module module-sine frequency=440);sleep .3;pactl unload-module $thnum");
    }
    else if (speakertestPresent())
    {
        /*strcpy( str , "timeout -k .3 .3 speaker-test --frequency 440 --test sine > /dev/tty" ) ;*/
        strcpy(str, "( speaker-test -t sine -f 440 > /dev/tty )& pid=$!;sleep .3; kill -9 $pid");
    }
    else if (beepexePresent())
    {
        strcpy(str, "beep.exe 440 300");
    }
    else
    {
        strcpy(str, "printf '\a' > /dev/tty");
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);

    lIn = popen(str, "r");
    if (lIn)
    {
        pclose(lIn);
    }

    if (pactlPresent())
    {
        signal(SIGINT, SIG_DFL);
    }
}

int _messageBox(
    const char* aTitle,
    const char* aMessage,
    const char* aDialogType,
    const char* aIconType,
    int aDefaultButton)
{
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char* str;
    char* lpDialogString;
    FILE* lIn;
    bool lWasGraphicDialog;
    bool lWasXterm;
    int lResult;
    char lChar;
    termios infoOri;
    termios info;
    size_t lTitleLen;
    size_t lMessageLen;

    lBuff[0] = '\0';

    lTitleLen = aTitle ? strlen(aTitle) : 0;
    lMessageLen = aMessage ? strlen(aMessage) : 0;
    if (!aTitle || !lQuery)
    {
        str = cast(char*)malloc(MAX_PATH_OR_CMD + lTitleLen + lMessageLen);
    }

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return 1;
        }

        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'set {vButton} to {button returned} of ( display dialog \"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" ");
        if (some(aTitle))
        {
            strcat(str, "with title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        strcat(str, "with icon ");
        if (eq("error", aIconType))
        {
            strcat(str, "stop ");
        }
        else if (eq("warning", aIconType))
        {
            strcat(str, "caution ");
        }
        else /* question or info */
        {
            strcat(str, "note ");
        }
        if (eq("okcancel", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "default button \"Cancel\" ");
            }
        }
        else if (eq("yesno", aDialogType))
        {
            strcat(str, "buttons {\"No\", \"Yes\"} ");
            if (aDefaultButton)
            {
                strcat(str, "default button \"Yes\" ");
            }
            else
            {
                strcat(str, "default button \"No\" ");
            }
            strcat(str, "cancel button \"No\"");
        }
        else if (eq("yesnocancel", aDialogType))
        {
            strcat(str, "buttons {\"No\", \"Yes\", \"Cancel\"} ");
            switch (aDefaultButton)
            {
            case 1:
                strcat(str, "default button \"Yes\" ");
                break;
            case 2:
                strcat(str, "default button \"No\" ");
                break;
            case 0:
                strcat(str, "default button \"Cancel\" ");
                break;
            default:
                break;
            }
            strcat(str, "cancel button \"Cancel\"");
        }
        else
        {
            strcat(str, "buttons {\"OK\"} ");
            strcat(str, "default button \"OK\" ");
        }
        strcat(str, ")' ");

        strcat(str,
               " -e 'if vButton is \"Yes\" then' -e 'return 1'" ~
               " -e 'else if vButton is \"OK\" then' -e 'return 1'" ~
               " -e 'else if vButton is \"No\" then' -e 'return 2'" ~
               " -e 'else' -e 'return 0' -e 'end if' ");

        strcat(str, "-e 'on error number -128' ");
        strcat(str, "-e '0' ");

        strcat(str, "-e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return 1;
        }

        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }

        strcat(str, " --");
        if (eq("okcancel", aDialogType) || eq("yesno", aDialogType) || eq("yesnocancel", aDialogType))
        {
            if (eq("warning", aIconType) || eq("error", aIconType))
            {
                strcat(str, "warning");
            }
            if (eq("yesnocancel", aDialogType))
            {
                strcat(str, "yesnocancel");
            }
            else
            {
                strcat(str, "yesno");
            }
        }
        else if (eq("error", aIconType))
        {
            strcat(str, "error");
        }
        else if (eq("warning", aIconType))
        {
            strcat(str, "sorry");
        }
        else
        {
            strcat(str, "msgbox");
        }
        strcat(str, " \"");
        if (aMessage)
        {
            strcat(str, aMessage);
        }
        strcat(str, "\"");
        if (eq("okcancel", aDialogType))
        {
            strcat(str,
                   " --yes-label Ok --no-label Cancel");
        }
        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }

        if (eq("yesnocancel", aDialogType))
        {
            strcat(str, "; x=$? ;if [ $x = 0 ] ;then echo 1;elif [ $x = 1 ] ;then echo 2;else echo 0;fi");
        }
        else
        {
            strcat(str, ";if [ $? = 0 ];then echo 1;else echo 0;fi");
        }
    }
    else if (zenityPresent() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        if (zenityPresent())
        {
            if (lQuery)
            {
                response("zenity");
                return 1;
            }
            strcpy(str, "szAnswer=$(zenity");
            if (zenity3Present() >= 4 && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return 1;
            }
            strcpy(str, "szAnswer=$(matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return 1;
            }
            strcpy(str, "szAnswer=$(shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return 1;
            }
            strcpy(str, "szAnswer=$(qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --");

        if (eq("okcancel", aDialogType))
        {
            strcat(str, "question --ok-label=Ok --cancel-label=Cancel");
        }
        else if (eq("yesno", aDialogType))
        {
            strcat(str, "question");
        }
        else if (eq("yesnocancel", aDialogType))
        {
            strcat(str, "list --column \"\" --hide-header \"Yes\" \"No\"");
        }
        else if (eq("error", aIconType))
        {
            strcat(str, "error");
        }
        else if (eq("warning", aIconType))
        {
            strcat(str, "warning");
        }
        else
        {
            strcat(str, "info");
        }
        if (some(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (some(aMessage))
        {
            strcat(str, " --no-wrap --text=\"");
            strcat(str, aMessage);
            strcat(str, "\"");
        }
        if ((zenity3Present() >= 3) || (!zenityPresent() && (shellementaryPresent() || qarmaPresent())))
        {
            strcat(str, " --icon-name=dialog-");
            if (eq("question", aIconType) || eq("error", aIconType) || eq("warning", aIconType))
            {
                strcat(str, aIconType);
            }
            else
            {
                strcat(str, "information");
            }
        }

        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");

        if (eq("yesnocancel", aDialogType))
        {
            strcat(str,
                   ");if [ $? = 1 ];then echo 0;elif [ $szAnswer = \"No\" ];then echo 2;else echo 1;fi");
        }
        else
        {
            strcat(str, ");if [ $? = 0 ];then echo 1;else echo 0;fi");
        }
    }
    else if (!gxmessagePresent() && !gmessagePresent() && !gdialogPresent() && !xdialogPresent() && tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return 1;
        }

        strcpy(str, gPython2Name.ptr);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }

        strcat(str,
               " -S -c \"import Tkinter,tkMessageBox;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set " ~
                   "frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "res=tkMessageBox.");
        if (eq("okcancel", aDialogType))
        {
            strcat(str, "askokcancel(");
            if (aDefaultButton)
            {
                strcat(str, "default=tkMessageBox.OK,");
            }
            else
            {
                strcat(str, "default=tkMessageBox.CANCEL,");
            }
        }
        else if (eq("yesno", aDialogType))
        {
            strcat(str, "askyesno(");
            if (aDefaultButton)
            {
                strcat(str, "default=tkMessageBox.YES,");
            }
            else
            {
                strcat(str, "default=tkMessageBox.NO,");
            }
        }
        else if (eq("yesnocancel", aDialogType))
        {
            strcat(str, "askyesnocancel(");
            switch (aDefaultButton)
            {
            case 1:
                strcat(str, "default=tkMessageBox.YES,");
                break;
            case 2:
                strcat(str, "default=tkMessageBox.NO,");
                break;
            case 0:
                strcat(str, "default=tkMessageBox.CANCEL,");
                break;
            default:
                break;
            }
        }
        else
        {
            strcat(str, "showinfo(");
        }

        strcat(str, "icon='");
        if (eq("question", aIconType) || eq("error", aIconType) || eq("warning", aIconType))
        {
            strcat(str, aIconType);
        }
        else
        {
            strcat(str, "info");
        }

        strcat(str, "',");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aMessage))
        {
            strcat(str, "message='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "'");
        }

        if (eq("yesnocancel", aDialogType))
        {
            strcat(str, `);
if res is None:
    print 0
elif res is False:
    print 2
else:
    print 1
"`);
        }
        else
        {
            strcat(str, `);
if res is False:
    print 0
else:
    print 1
"`);
        }
    }
    else if (!gxmessagePresent() && !gmessagePresent() && !gdialogPresent() && !xdialogPresent() && tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return 1;
        }

        strcpy(str, gPython3Name.ptr);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import messagebox;root=tkinter.Tk();root.withdraw();");

        strcat(str, "res=messagebox.");
        if (eq("okcancel", aDialogType))
        {
            strcat(str, "askokcancel(");
            if (aDefaultButton)
            {
                strcat(str, "default=messagebox.OK,");
            }
            else
            {
                strcat(str, "default=messagebox.CANCEL,");
            }
        }
        else if (eq("yesno", aDialogType))
        {
            strcat(str, "askyesno(");
            if (aDefaultButton)
            {
                strcat(str, "default=messagebox.YES,");
            }
            else
            {
                strcat(str, "default=messagebox.NO,");
            }
        }
        else if (eq("yesnocancel", aDialogType))
        {
            strcat(str, "askyesnocancel(");
            switch (aDefaultButton)
            {
            case 1:
                strcat(str, "default=messagebox.YES,");
                break;
            case 2:
                strcat(str, "default=messagebox.NO,");
                break;
            case 0:
                strcat(str, "default=messagebox.CANCEL,");
                break;
            default:
                break;
            }
        }
        else
        {
            strcat(str, "showinfo(");
        }

        strcat(str, "icon='");
        if (eq("question", aIconType) || eq("error", aIconType) || eq("warning", aIconType))
        {
            strcat(str, aIconType);
        }
        else
        {
            strcat(str, "info");
        }

        strcat(str, "',");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aMessage))
        {
            strcat(str, "message='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "'");
        }

        if (eq("yesnocancel", aDialogType))
        {
            strcat(str, `);
if res is None:
    print(0)
elif res is False:
    print(2)
else:
    print(1)
"`);
        }
        else
        {
            strcat(str, `);
if res is False:
    print(0)
else:
    print(1)
"`);
        }
    }
    else if (gxmessagePresent() || gmessagePresent() || (!gdialogPresent() && !xdialogPresent() && xmessagePresent()))
    {
        if (gxmessagePresent())
        {
            if (lQuery)
            {
                response("gxmessage");
                return 1;
            }
            strcpy(str, "gxmessage");
        }
        else if (gmessagePresent())
        {
            if (lQuery)
            {
                response("gmessage");
                return 1;
            }
            strcpy(str, "gmessage");
        }
        else
        {
            if (lQuery)
            {
                response("xmessage");
                return 1;
            }
            strcpy(str, "xmessage");
        }

        if (eq("okcancel", aDialogType))
        {
            strcat(str, " -buttons Ok:1,Cancel:0");
            switch (aDefaultButton)
            {
            case 1:
                strcat(str, " -default Ok");
                break;
            case 0:
                strcat(str, " -default Cancel");
                break;
            default:
                break;
            }
        }
        else if (eq("yesno", aDialogType))
        {
            strcat(str, " -buttons Yes:1,No:0");
            switch (aDefaultButton)
            {
            case 1:
                strcat(str, " -default Yes");
                break;
            case 0:
                strcat(str, " -default No");
                break;
            default:
                break;
            }
        }
        else if (eq("yesnocancel", aDialogType))
        {
            strcat(str, " -buttons Yes:1,No:2,Cancel:0");
            switch (aDefaultButton)
            {
            case 1:
                strcat(str, " -default Yes");
                break;
            case 2:
                strcat(str, " -default No");
                break;
            case 0:
                strcat(str, " -default Cancel");
                break;
            default:
                break;
            }
        }
        else
        {
            strcat(str, " -buttons Ok:1");
            strcat(str, " -default Ok");
        }

        strcat(str, " -center \"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\"");
        if (some(aTitle))
        {
            strcat(str, " -title  \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        strcat(str, " ; echo $? ");
    }
    else if (xdialogPresent() || gdialogPresent() || dialogName() || whiptailPresent())
    {
        if (gdialogPresent())
        {
            if (lQuery)
            {
                response("gdialog");
                return 1;
            }
            lWasGraphicDialog = true;
            strcpy(str, "(gdialog ");
        }
        else if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return 1;
            }
            lWasGraphicDialog = true;
            strcpy(str, "(Xdialog ");
        }
        else if (dialogName())
        {
            if (lQuery)
            {
                response("dialog");
                return 0;
            }
            if (isTerminalRunning())
            {
                strcpy(str, "(dialog ");
            }
            else
            {
                lWasXterm = true;
                strcpy(str, terminalName());
                strcat(str, "'(");
                strcat(str, dialogName());
                strcat(str, " ");
            }
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("whiptail");
                return 0;
            }
            strcpy(str, "(whiptail ");
        }
        else
        {
            if (lQuery)
            {
                response("whiptail");
                return 0;
            }
            lWasXterm = true;
            strcpy(str, terminalName());
            strcat(str, "'(whiptail ");
        }

        if (some(aTitle))
        {
            strcat(str, "--title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        if (!xdialogPresent() && !gdialogPresent())
        {
            if (eq("okcancel", aDialogType) || eq("yesno", aDialogType) || eq("yesnocancel", aDialogType))
            {
                strcat(str, "--backtitle \"");
                strcat(str, "tab: move focus");
                strcat(str, "\" ");
            }
        }

        if (eq("okcancel", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "--defaultno ");
            }
            strcat(str,
                   "--yes-label \"Ok\" --no-label \"Cancel\" --yesno ");
        }
        else if (eq("yesno", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "--defaultno ");
            }
            strcat(str, "--yesno ");
        }
        else if (eq("yesnocancel", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "--defaultno ");
            }
            strcat(str, "--menu ");
        }
        else
        {
            strcat(str, "--msgbox ");
        }
        strcat(str, "\"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" ");

        if (lWasGraphicDialog)
        {
            if (eq("yesnocancel", aDialogType))
            {
                strcat(str, "0 60 0 Yes \"\" No \"\") 2>/tmp/tinyfd.txt;" ~
                            "if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;" ~
                            "tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");
            }
            else
            {
                strcat(str, "10 60 ) 2>&1;if [ $? = 0 ];then echo 1;else echo 0;fi");
            }
        }
        else
        {
            if (eq("yesnocancel", aDialogType))
            {
                strcat(str, "0 60 0 Yes \"\" No \"\" >/dev/tty ) 2>/tmp/tinyfd.txt;" ~
                            "if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;" ~
                            "tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");

                if (lWasXterm)
                {
                    strcat(str, " >/tmp/tinyfd0.txt';cat /tmp/tinyfd0.txt");
                }
                else
                {
                    strcat(str, "; clear >/dev/tty");
                }
            }
            else
            {
                strcat(str, "10 60 >/dev/tty) 2>&1;if [ $? = 0 ];");
                if (lWasXterm)
                {
                    strcat(str,
                           "then\n\techo 1\nelse\n\techo 0\nfi >/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
                }
                else
                {
                    strcat(str,
                           "then echo 1;else echo 0;fi;clear >/dev/tty");
                }
            }
        }
    }
    else if (!isTerminalRunning() && terminalName())
    {
        if (lQuery)
        {
            response("basicinput");
            return 0;
        }
        strcpy(str, terminalName());
        strcat(str, "'");
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = true;
            strcat(str, "echo \"");
            strcat(str, gTitle.ptr);
            strcat(str, "\";");
            strcat(str, "echo \"");
            strcat(str, tinyfd_needs.ptr);
            strcat(str, "\";echo;echo;");
        }
        if (some(aTitle))
        {
            strcat(str, "echo \"");
            strcat(str, aTitle);
            strcat(str, "\";echo;");
        }
        if (some(aMessage))
        {
            strcat(str, "echo \"");
            strcat(str, aMessage);
            strcat(str, "\"; ");
        }
        if (eq("yesno", aDialogType))
        {
            strcat(str, "echo -n \"y/n: \"; ");
            strcat(str, "stty sane -echo;");
            strcat(str,
                   "answer=$( while ! head -c 1 | grep -i [ny];do true ;done);");
            strcat(str,
                   "if echo \"$answer\" | grep -iq \"^y\";then\n");
            strcat(str, "\techo 1\nelse\n\techo 0\nfi");
        }
        else if (eq("okcancel", aDialogType))
        {
            strcat(str, "echo -n \"[O]kay/[C]ancel: \"; ");
            strcat(str, "stty sane -echo;");
            strcat(str,
                   "answer=$( while ! head -c 1 | grep -i [oc];do true ;done);");
            strcat(str,
                   "if echo \"$answer\" | grep -iq \"^o\";then\n");
            strcat(str, "\techo 1\nelse\n\techo 0\nfi");
        }
        else if (eq("yesnocancel", aDialogType))
        {
            strcat(str, "echo -n \"[Y]es/[N]o/[C]ancel: \"; ");
            strcat(str, "stty sane -echo;");
            strcat(str,
                   "answer=$( while ! head -c 1 | grep -i [nyc];do true ;done);");
            strcat(str,
                   "if echo \"$answer\" | grep -iq \"^y\";then\n\techo 1\n");
            strcat(str, "elif echo \"$answer\" | grep -iq \"^n\";then\n\techo 2\n");
            strcat(str, "else\n\techo 0\nfi");
        }
        else
        {
            strcat(str, "echo -n \"press enter to continue \"; ");
            strcat(str, "stty sane -echo;");
            strcat(str,
                   "answer=$( while ! head -c 1;do true ;done);echo 1");
        }
        strcat(str,
               " >/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
    }
    else if (!isTerminalRunning() && pythonDbusPresent() && eq("ok", aDialogType))
    {
        if (lQuery)
        {
            response("python-dbus");
            return 1;
        }
        strcpy(str, gPythonName.ptr);
        strcat(str, " -c \"import dbus;bus=dbus.SessionBus();");
        strcat(str, "notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');");
        strcat(str, "notify=dbus.Interface(notif,'org.freedesktop.Notifications');");
        strcat(str, "notify.Notify('',0,'");
        if (some(aIconType))
        {
            strcat(str, aIconType);
        }
        strcat(str, "','");
        if (some(aTitle))
        {
            strcat(str, aTitle);
        }
        strcat(str, "','");
        if (some(aMessage))
        {
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
        }
        strcat(str, "','','',5000)\"");
    }
    else if (!isTerminalRunning() && (perlPresent() >= 2) && eq("ok", aDialogType))
    {
        if (lQuery)
        {
            response("perl-dbus");
            return 1;
        }
        sprintf(str, `perl -e "use Net::DBus;
                    my \$sessionBus = Net::DBus->session;
                    my \$notificationsService = \$sessionBus->get_service('org.freedesktop.Notifications');
                    my \$notificationsObject = \$notificationsService->get_object('/org/freedesktop/Notifications',
                        'org.freedesktop.Notifications');
                    my \$notificationId;\$notificationId = \$notificationsObject->Notify(shift, 0, '%s', '%s', '%s', [], {}, -1);"`,
                aIconType ? aIconType : "", aTitle ? aTitle : "", aMessage ? aMessage : "");
    }
    else if (!isTerminalRunning() && notifysendPresent() && eq("ok", aDialogType))
    {

        if (lQuery)
        {
            response("notifysend");
            return 1;
        }
        strcpy(str, "notify-send");
        if (some(aIconType))
        {
            strcat(str, " -i '");
            strcat(str, aIconType);
            strcat(str, "'");
        }
        strcat(str, " \"");
        if (some(aTitle))
        {
            strcat(str, aTitle);
            strcat(str, " | ");
        }
        if (some(aMessage))
        {
            replaceSubStr(aMessage, "\n\t", " |  ", lBuff.ptr);
            replaceSubStr(aMessage, "\n", " | ", lBuff.ptr);
            replaceSubStr(aMessage, "\t", "  ", lBuff.ptr);
            strcat(str, lBuff.ptr);
        }
        strcat(str, "\"");
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return 0;
        }
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = true;
            printf("\n\n%s\n", gTitle.ptr);
            printf("%s\n\n", tinyfd_needs.ptr);
        }
        if (some(aTitle))
        {
            printf("\n%s\n", aTitle);
        }

        tcgetattr(0, &infoOri);
        tcgetattr(0, &info);
        info.c_lflag &= ~ICANON;
        info.c_cc[VMIN] = 1;
        info.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &info);
        if (eq("yesno", aDialogType))
        {
            do
            {
                if (some(aMessage))
                {
                    printf("\n%s\n", aMessage);
                }
                printf("y/n: ");
                fflush(stdout);
                lChar = cast(char)tolower(getchar());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n');
            lResult = lChar == 'y' ? 1 : 0;
        }
        else if (eq("okcancel", aDialogType))
        {
            do
            {
                if (some(aMessage))
                {
                    printf("\n%s\n", aMessage);
                }
                printf("[O]kay/[C]ancel: ");
                fflush(stdout);
                lChar = cast(char)tolower(getchar());
                printf("\n\n");
            } while (lChar != 'o' && lChar != 'c');
            lResult = lChar == 'o' ? 1 : 0;
        }
        else if (eq("yesnocancel", aDialogType))
        {
            do
            {
                if (some(aMessage))
                {
                    printf("\n%s\n", aMessage);
                }
                printf("[Y]es/[N]o/[C]ancel: ");
                fflush(stdout);
                lChar = cast(char)tolower(getchar());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n' && lChar != 'c');
            lResult = (lChar == 'y') ? 1 : (lChar == 'n') ? 2 : 0;
        }
        else
        {
            if (some(aMessage))
            {
                printf("\n%s\n\n", aMessage);
            }
            printf("press enter to continue ");
            fflush(stdout);
            getchar();
            printf("\n\n");
            lResult = 1;
        }
        tcsetattr(0, TCSANOW, &infoOri);
        free(str);
        return lResult;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);

    lIn = popen(str, "r");
    if (!lIn)
    {
        free(str);
        return 0;
    }
    while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
    {
    }

    pclose(lIn);

    /* printf( "lBuff: %s len: %lu \n" , lBuff , strlen(lBuff.ptr) ) ; */
    removeLastNL(lBuff.ptr);
    /* printf( "lBuff1: %s len: %lu \n" , lBuff , strlen(lBuff.ptr) ) ; */

    if (eq("yesnocancel", aDialogType))
    {
        if (lBuff[0] == '1')
        {
            if (eq(lBuff.ptr + 1, "Yes"))
                strcpy(lBuff.ptr, "1");
            else if (eq(lBuff.ptr + 1, "No"))
                strcpy(lBuff.ptr, "2");
        }
    }
    /* printf( "lBuff2: %s len: %lu \n" , lBuff , strlen(lBuff.ptr) ) ; */

    lResult = eq(lBuff.ptr, "2") ? 2 : eq(lBuff.ptr, "1") ? 1 : 0;

    /* printf( "lResult: %d\n" , lResult ) ; */
    free(str);
    return lResult;
}

int _notifyPopup(
    const char* aTitle,
    const char* aMessage,
    const char* aIconType)
{
    char[MAX_PATH_OR_CMD] lBuff = '\0';
    char* str;
    char* lpDialogString;
    FILE* lIn;
    size_t lTitleLen;
    size_t lMessageLen;
    bool lQuery;

    if (getenv("SSH_TTY"))
    {
        return _messageBox(aTitle, aMessage, "ok", aIconType, 0);
    }

    lQuery = eq(aTitle, "tinyfd_query");
    lTitleLen = aTitle ? strlen(aTitle) : 0;
    lMessageLen = aMessage ? strlen(aMessage) : 0;
    if (!aTitle || !lQuery)
    {
        str = cast(char*)malloc(MAX_PATH_OR_CMD + lTitleLen + lMessageLen);
    }

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return 1;
        }

        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'display notification \"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, " \" ");
        if (some(aTitle))
        {
            strcat(str, "with title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        strcat(str, "' -e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return 1;
        }
        strcpy(str, "kdialog");

        if (some(aIconType))
        {
            strcat(str, " --icon '");
            strcat(str, aIconType);
            strcat(str, "'");
        }
        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }

        strcat(str, " --passivepopup");
        strcat(str, " \"");
        if (aMessage)
        {
            strcat(str, aMessage);
        }
        strcat(str, " \" 5");
    }
    else if ((zenity3Present() >= 5) || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        /* zenity 2.32 & 3.14 has the notification but with a bug: it doesnt return from it */
        /* zenity 3.8 show the notification as an alert ok cancel box */
        if (zenity3Present() >= 5)
        {
            if (lQuery)
            {
                response("zenity");
                return 1;
            }
            strcpy(str, "zenity");
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return 1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return 1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return 1;
            }
            strcpy(str, "qarma");
        }

        strcat(str, " --notification");

        if (some(aIconType))
        {
            strcat(str, " --window-icon '");
            strcat(str, aIconType);
            strcat(str, "'");
        }

        strcat(str, " --text \"");
        if (some(aTitle))
        {
            strcat(str, aTitle);
            strcat(str, "\n");
        }
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, " \"");
    }
    else if (perlPresent() >= 2)
    {
        if (lQuery)
        {
            response("perl-dbus");
            return 1;
        }
        sprintf(str, `perl -e "use Net::DBus;
                    my \$sessionBus = Net::DBus->session;
                    my \$notificationsService = \$sessionBus->get_service('org.freedesktop.Notifications');
                    my \$notificationsObject = \$notificationsService->get_object('/org/freedesktop/Notifications',
                        'org.freedesktop.Notifications');
                    my \$notificationId;\$notificationId = \$notificationsObject->Notify(shift, 0, '%s', '%s', '%s', [], {}, -1);"`,
                aIconType ? aIconType : "", aTitle ? aTitle : "", aMessage ? aMessage : "");
    }
    else if (pythonDbusPresent())
    {
        if (lQuery)
        {
            response("python-dbus");
            return 1;
        }
        strcpy(str, gPythonName.ptr);
        strcat(str, " -c \"import dbus;bus=dbus.SessionBus();");
        strcat(str, "notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');");
        strcat(str, "notify=dbus.Interface(notif,'org.freedesktop.Notifications');");
        strcat(str, "notify.Notify('',0,'");
        if (some(aIconType))
        {
            strcat(str, aIconType);
        }
        strcat(str, "','");
        if (some(aTitle))
        {
            strcat(str, aTitle);
        }
        strcat(str, "','");
        if (some(aMessage))
        {
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
        }
        strcat(str, "','','',5000)\"");
    }
    else if (notifysendPresent())
    {
        if (lQuery)
        {
            response("notifysend");
            return 1;
        }
        strcpy(str, "notify-send");
        if (some(aIconType))
        {
            strcat(str, " -i '");
            strcat(str, aIconType);
            strcat(str, "'");
        }
        strcat(str, " \"");
        if (some(aTitle))
        {
            strcat(str, aTitle);
            strcat(str, " | ");
        }
        if (some(aMessage))
        {
            replaceSubStr(aMessage, "\n\t", " |  ", lBuff.ptr);
            replaceSubStr(aMessage, "\n", " | ", lBuff.ptr);
            replaceSubStr(aMessage, "\t", "  ", lBuff.ptr);
            strcat(str, lBuff.ptr);
        }
        strcat(str, "\"");
    }
    else
    {
        return _messageBox(aTitle, aMessage, "ok", aIconType, 0);
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);

    lIn = popen(str, "r");
    if (!lIn)
    {
        free(str);
        return 0;
    }

    pclose(lIn);
    free(str);
    return 1;
}

const(char*) _inputBox(
    const char* aTitle,
    const char* aMessage,
    const char* aDefaultInput)
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char* str;
    char* lpDialogString;
    FILE* lIn;
    int lResult;
    bool lWasGdialog;
    bool lWasGraphicDialog;
    bool lWasXterm;
    bool lWasBasicXterm;
    termios oldt;
    termios newt;
    char* lEOF;
    size_t lTitleLen;
    size_t lMessageLen;

    lBuff[0] = '\0';

    lTitleLen = aTitle ? strlen(aTitle) : 0;
    lMessageLen = aMessage ? strlen(aMessage) : 0;
    if (!aTitle || !lQuery)
    {
        str = cast(char*)malloc(MAX_PATH_OR_CMD + lTitleLen + lMessageLen);
    }

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return cast(const(char)*)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'display dialog \"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" ");
        strcat(str, "default answer \"");
        if (some(aDefaultInput))
        {
            strcat(str, aDefaultInput);
        }
        strcat(str, "\" ");
        if (!aDefaultInput)
        {
            strcat(str, "hidden answer true ");
        }
        if (some(aTitle))
        {
            strcat(str, "with title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        strcat(str, "with icon note' ");
        strcat(str, "-e '\"1\" & text returned of result' ");
        strcat(str, "-e 'on error number -128' ");
        strcat(str, "-e '0' ");
        strcat(str, "-e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return cast(const(char)*)1;
        }
        strcpy(str, "szAnswer=$(kdialog");

        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }

        if (!aDefaultInput)
        {
            strcat(str, " --password ");
        }
        else
        {
            strcat(str, " --inputbox ");
        }
        strcat(str, "\"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" \"");
        if (some(aDefaultInput))
        {
            strcat(str, aDefaultInput);
        }
        strcat(str, "\"");
        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        strcat(str, ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
    }
    else if (zenityPresent() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        if (zenityPresent())
        {
            if (lQuery)
            {
                response("zenity");
                return cast(const(char)*)1;
            }
            strcpy(str, "szAnswer=$(zenity");
            if (zenity3Present() >= 4 && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return cast(const(char)*)1;
            }
            strcpy(str, "szAnswer=$(matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return cast(const(char)*)1;
            }
            strcpy(str, "szAnswer=$(shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return cast(const(char)*)1;
            }
            strcpy(str, "szAnswer=$(qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --entry");

        if (some(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (some(aMessage))
        {
            strcat(str, " --text=\"");
            strcat(str, aMessage);
            strcat(str, "\"");
        }
        if (some(aDefaultInput))
        {
            strcat(str, " --entry-text=\"");
            strcat(str, aDefaultInput);
            strcat(str, "\"");
        }
        else
        {
            strcat(str, " --hide-text");
        }
        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
        strcat(str, ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
    }
    else if (gxmessagePresent() || gmessagePresent())
    {
        if (gxmessagePresent())
        {
            if (lQuery)
            {
                response("gxmessage");
                return cast(const(char)*)1;
            }
            strcpy(str, "szAnswer=$(gxmessage -buttons Ok:1,Cancel:0 -center \"");
        }
        else
        {
            if (lQuery)
            {
                response("gmessage");
                return cast(const(char)*)1;
            }
            strcpy(str, "szAnswer=$(gmessage -buttons Ok:1,Cancel:0 -center \"");
        }

        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\"");
        if (some(aTitle))
        {
            strcat(str, " -title  \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        strcat(str, " -entrytext \"");
        if (some(aDefaultInput))
        {
            strcat(str, aDefaultInput);
        }
        strcat(str, "\"");
        strcat(str, ");echo $?$szAnswer");
    }
    else if (!gdialogPresent() && !xdialogPresent() && tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython2Name.ptr);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }

        strcat(str,
               " -S -c \"import Tkinter,tkSimpleDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set " ~
                   "frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "res=tkSimpleDialog.askstring(");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aMessage))
        {
            strcat(str, "prompt='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "',");
        }
        if (aDefaultInput)
        {
            if (some(aDefaultInput))
            {
                strcat(str, "initialvalue='");
                strcat(str, aDefaultInput);
                strcat(str, "',");
            }
        }
        else
        {
            strcat(str, "show='*'");
        }
        strcat(str, ");\nif res is None :\n\tprint 0");
        strcat(str, "\nelse :\n\tprint '1'+res\n\"");
    }
    else if (!gdialogPresent() && !xdialogPresent() && tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython3Name.ptr);
        strcat(str,
               " -S -c \"import tkinter; from tkinter import simpledialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "res=simpledialog.askstring(");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aMessage))
        {
            strcat(str, "prompt='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "',");
        }
        if (aDefaultInput)
        {
            if (some(aDefaultInput))
            {
                strcat(str, "initialvalue='");
                strcat(str, aDefaultInput);
                strcat(str, "',");
            }
        }
        else
        {
            strcat(str, "show='*'");
        }
        strcat(str, ");\nif res is None :\n\tprint(0)");
        strcat(str, "\nelse :\n\tprint('1'+res)\n\"");
    }
    else if (gdialogPresent() || xdialogPresent() || dialogName() || whiptailPresent())
    {
        if (gdialogPresent())
        {
            if (lQuery)
            {
                response("gdialog");
                return cast(const(char)*)1;
            }
            lWasGraphicDialog = true;
            lWasGdialog = true;
            strcpy(str, "(gdialog ");
        }
        else if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return cast(const(char)*)1;
            }
            lWasGraphicDialog = true;
            strcpy(str, "(Xdialog ");
        }
        else if (dialogName())
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            if (isTerminalRunning())
            {
                strcpy(str, "(dialog ");
            }
            else
            {
                lWasXterm = true;
                strcpy(str, terminalName());
                strcat(str, "'(");
                strcat(str, dialogName());
                strcat(str, " ");
            }
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("whiptail");
                return cast(const(char)*)0;
            }
            strcpy(str, "(whiptail ");
        }
        else
        {
            if (lQuery)
            {
                response("whiptail");
                return cast(const(char)*)0;
            }
            lWasXterm = true;
            strcpy(str, terminalName());
            strcat(str, "'(whiptail ");
        }

        if (some(aTitle))
        {
            strcat(str, "--title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        if (!xdialogPresent() && !gdialogPresent())
        {
            strcat(str, "--backtitle \"");
            strcat(str, "tab: move focus");
            if (!aDefaultInput && !lWasGdialog)
            {
                strcat(str, " (sometimes nothing, no blink nor star, is shown in text field)");
            }
            strcat(str, "\" ");
        }

        if (aDefaultInput || lWasGdialog)
        {
            strcat(str, "--inputbox");
        }
        else
        {
            if (!lWasGraphicDialog && dialogName() && isDialogVersionBetter09b())
            {
                strcat(str, "--insecure ");
            }
            strcat(str, "--passwordbox");
        }
        strcat(str, " \"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" 10 60 ");
        if (some(aDefaultInput))
        {
            strcat(str, "\"");
            strcat(str, aDefaultInput);
            strcat(str, "\" ");
        }
        if (lWasGraphicDialog)
        {
            strcat(str, ") 2>/tmp/tinyfd.txt;" ~
                        "if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;" ~
                        "tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");
        }
        else
        {
            strcat(str, ">/dev/tty ) 2>/tmp/tinyfd.txt;" ~
                        "if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;" ~
                        "tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");

            if (lWasXterm)
            {
                strcat(str, " >/tmp/tinyfd0.txt';cat /tmp/tinyfd0.txt");
            }
            else
            {
                strcat(str, "; clear >/dev/tty");
            }
        }
    }
    else if (!isTerminalRunning() && terminalName())
    {
        if (lQuery)
        {
            response("basicinput");
            return cast(const(char)*)0;
        }
        lWasBasicXterm = true;
        strcpy(str, terminalName());
        strcat(str, "'");
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = true;
            _messageBox(gTitle.ptr, tinyfd_needs.ptr, "ok", "warning", 0);
        }
        if (some(aTitle) && !tinyfd_forceConsole)
        {
            strcat(str, "echo \"");
            strcat(str, aTitle);
            strcat(str, "\";echo;");
        }

        strcat(str, "echo \"");
        if (some(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\";read ");
        if (!aDefaultInput)
        {
            strcat(str, "-s ");
        }
        strcat(str, "-p \"");
        strcat(str, "(esc+enter to cancel): \" ANSWER ");
        strcat(str, ";echo 1$ANSWER >/tmp/tinyfd.txt';");
        strcat(str, "cat -v /tmp/tinyfd.txt");
    }
    else if (!gWarningDisplayed && !isTerminalRunning() && !terminalName())
    {
        gWarningDisplayed = true;
        _messageBox(gTitle.ptr, tinyfd_needs.ptr, "ok", "warning", 0);
        if (lQuery)
        {
            response("no_solution");
            return cast(const(char)*)0;
        }
        return null;
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return cast(const(char)*)0;
        }
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = true;
            _messageBox(gTitle.ptr, tinyfd_needs.ptr, "ok", "warning", 0);
        }
        if (some(aTitle))
        {
            printf("\n%s\n", aTitle);
        }
        if (some(aMessage))
        {
            printf("\n%s\n", aMessage);
        }
        printf("(esc+enter to cancel): ");
        fflush(stdout);
        if (!aDefaultInput)
        {
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        }

        lEOF = fgets(lBuff.ptr, MAX_PATH_OR_CMD, stdin);
        /* printf("lbuff<%c><%d>\n",lBuff[0],lBuff[0]); */
        if (!lEOF || (lBuff[0] == '\0'))
        {
            free(str);
            return null;
        }

        if (lBuff[0] == '\n')
        {
            lEOF = fgets(lBuff.ptr, MAX_PATH_OR_CMD, stdin);
            /* printf("lbuff<%c><%d>\n",lBuff[0],lBuff[0]); */
            if (!lEOF || (lBuff[0] == '\0'))
            {
                free(str);
                return null;
            }
        }

        if (!aDefaultInput)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            printf("\n");
        }
        printf("\n");
        if (strchr(lBuff.ptr, 27))
        {
            free(str);
            return null;
        }
        removeLastNL(lBuff.ptr);
        free(str);
        return lBuff.ptr;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    lIn = popen(str, "r");
    if (!lIn)
    {
        if (fileExists("/tmp/tinyfd.txt"))
        {
            wipefile("/tmp/tinyfd.txt");
            remove("/tmp/tinyfd.txt");
        }
        if (fileExists("/tmp/tinyfd0.txt"))
        {
            wipefile("/tmp/tinyfd0.txt");
            remove("/tmp/tinyfd0.txt");
        }
        free(str);
        return null;
    }
    while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
    {
    }

    pclose(lIn);

    if (fileExists("/tmp/tinyfd.txt"))
    {
        wipefile("/tmp/tinyfd.txt");
        remove("/tmp/tinyfd.txt");
    }
    if (fileExists("/tmp/tinyfd0.txt"))
    {
        wipefile("/tmp/tinyfd0.txt");
        remove("/tmp/tinyfd0.txt");
    }

    /* printf( "len Buff: %lu\n" , strlen(lBuff.ptr) ) ; */
    /* printf( "lBuff0: %s\n" , lBuff ) ; */
    removeLastNL(lBuff.ptr);
    /* printf( "lBuff1: %s len: %lu \n" , lBuff , strlen(lBuff.ptr) ) ; */
    if (lWasBasicXterm)
    {
        if (strstr(lBuff.ptr, "^[")) /* esc was pressed */
        {
            free(str);
            return null;
        }
    }

    lResult = strncmp(lBuff.ptr, "1", 1) ? 0 : 1;
    /* printf( "lResult: %d \n" , lResult ) ; */
    if (!lResult)
    {
        free(str);
        return null;
    }
    /* printf( "lBuff+1: %s\n" , lBuff+1 ) ; */
    free(str);

    return lBuff.ptr + 1;
}

void osascriptAppendFilters(const TFD_Filter[] filters, char* output)
{
    assert(output);

    if (!filters.length)
        return;

    bool isMime;
    const(char)*[MAX_PATTERNS] patterns;

    foreach (filter; filters)
    {
        const plen = getValidPatterns(filter, isMime, patterns);
        if (plen == 0 || isMime)
            continue;

        strcat(output, "of type {\"");
        foreach (i, pt; patterns[0 .. plen])
        {
            if (strstr(pt, "*."))
                pt += 2;
            if (*pt == '\0')
                continue;

            if (i != 0)
                strcat(output, "\",\"");
            strcat(output, pt);
        }
        strcat(output, "\"} ");
        break; // TODO: can multiple?
    }
}

void kdialogAppendFilters(const TFD_Filter[] filters, char* output)
{
    assert(output);

    if (!filters.length)
        return;

    bool isMime;
    const(char)*[MAX_PATTERNS] patterns;
    char[MAX_PATH_OR_CMD] patternsStr = void;
    int j;

    // "Description(p1 p2) | p1(p1) | mime1 mime2"
    strcat(output, " \"");
    foreach (filter; filters)
    {
        const plen = getValidPatterns(filter, isMime, patterns);
        if (plen == 0)
            continue;

        patternsStr[0] = '\0';
        foreach (i, pt; patterns[0 .. plen])
        {
            if (i != 0)
                strcat(patternsStr.ptr, " ");
            strcat(patternsStr.ptr, pt);
        }

        if (j != 0)
            strcat(output, " | ");
        j++;

        if (!isMime)
        {
            strcat(output, some(filter.description) ? filter.description : patternsStr.ptr);
            strcat(output, "(");
        }
        strcat(output, patternsStr.ptr);
        if (!isMime)
        {
            strcat(output, ")");
        }
    }
    strcat(output, "\"");
    // no need to add "All files"
}

void zenityAppendFilters(const TFD_Filter[] filters, char* output)
{
    assert(output);

    if (!filters.length)
        return;

    bool isMime;
    const(char)*[MAX_PATTERNS] patterns;

    foreach (filter; filters)
    {
        const plen = getValidPatterns(filter, isMime, patterns);
        if (plen == 0 || isMime)
            continue;

        strcat(output, " --file-filter='");
        if (some(filter.description))
        {
            strcat(output, filter.description);
            strcat(output, " | ");
        }
        foreach (pt; patterns[0 .. plen])
        {
            strcat(output, pt);
            strcat(output, " ");
        }
        strcat(output, "'");
    }
    strcat(output, " --file-filter='All files | *'");
}

void tkinterAppendFilters(const TFD_Filter[] filters, char* output)
{
    assert(output);

    if (!filters.length)
        return;

    bool isMime;
    const(char)*[MAX_PATTERNS] patterns;
    char placeholder = 'a';

    // filetypes=[('Description', ['p1','p2',]), ('a', 'p3'), ('All files', '*')]
    strcat(output, "filetypes=[");
    foreach (filter; filters)
    {
        const plen = getValidPatterns(filter, isMime, patterns);
        if (plen == 0 || isMime)
            continue;
        if (plen == 1 && lastch(patterns[0]) == '*')
            continue; // poor osx behaviour

        strcat(output, "('");
        if (some(filter.description))
        {
            strcat(output, filter.description);
        }
        else
        {
            const len = strlen(output);
            output[len] = placeholder;
            output[len + 1] = '\0';
            placeholder++;
        }
        strcat(output, "', ");
        if (plen > 1)
        {
            strcat(output, "[");
            foreach (pt; patterns[0 .. plen])
            {
                strcat(output, "'");
                strcat(output, pt);
                strcat(output, "',");
            }
            strcat(output, "]");
        }
        else
        {
            strcat(output, "'");
            strcat(output, patterns[0]);
            strcat(output, "'");
        }
        strcat(output, "), ");
    }
    strcat(output, "('All files', '*')]");
}

const(char*) _saveFileDialog(
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const TFD_Filter[] aFilters)
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char[MAX_PATH_OR_CMD] str_buf1 = '\0';
    char[MAX_PATH_OR_CMD] str_buf2 = '\0';
    char* str = str_buf1.ptr;
    char* lString = str_buf2.ptr;
    bool lWasGraphicDialog;
    bool lWasXterm;
    const(char)* p;
    FILE* lIn;
    lBuff[0] = '\0';

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return cast(const(char)*)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"Finder\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'POSIX path of ( choose file name ");
        if (some(aTitle))
        {
            strcat(str, "with prompt \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
        if (some(lString))
        {
            strcat(str, "default location \"");
            strcat(str, lString);
            strcat(str, "\" ");
        }
        getLastName(lString, aDefaultPathAndFile);
        if (some(lString))
        {
            strcat(str, "default name \"");
            strcat(str, lString);
            strcat(str, "\" ");
        }
        strcat(str, ")' ");
        strcat(str, "-e 'on error number -128' ");
        strcat(str, "-e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return cast(const(char)*)1;
        }

        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        strcat(str, " --getsavefilename ");

        if (some(aDefaultPathAndFile))
        {
            if (aDefaultPathAndFile[0] != '/')
            {
                strcat(str, "$PWD/");
            }
            strcat(str, "\"");
            strcat(str, aDefaultPathAndFile);
            strcat(str, "\"");
        }
        else
        {
            strcat(str, "$PWD/");
        }

        kdialogAppendFilters(aFilters, str);

        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
    }
    else if (zenityPresent() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        if (zenityPresent())
        {
            if (lQuery)
            {
                response("zenity");
                return cast(const(char)*)1;
            }
            strcpy(str, "zenity");
            if (zenity3Present() >= 4 && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return cast(const(char)*)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return cast(const(char)*)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return cast(const(char)*)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --file-selection --save --confirm-overwrite");

        if (some(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (some(aDefaultPathAndFile))
        {
            strcat(str, " --filename=\"");
            strcat(str, aDefaultPathAndFile);
            strcat(str, "\"");
        }

        zenityAppendFilters(aFilters, str);

        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
    }
    else if (!xdialogPresent() && tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython2Name.ptr);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }
        strcat(str,
               " -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set " ~
                   "frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "print tkFileDialog.asksaveasfilename(");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }

        tkinterAppendFilters(aFilters, str);

        strcat(str, ")\"");
    }
    else if (!xdialogPresent() && tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython3Name.ptr);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "print( filedialog.asksaveasfilename(");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }

        tkinterAppendFilters(aFilters, str);

        strcat(str, "))\"");
    }
    else if (xdialogPresent() || dialogName())
    {
        if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return cast(const(char)*)1;
            }
            lWasGraphicDialog = true;
            strcpy(str, "(Xdialog ");
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            strcpy(str, "(dialog ");
        }
        else
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            lWasXterm = true;
            strcpy(str, terminalName());
            strcat(str, "'(");
            strcat(str, dialogName());
            strcat(str, " ");
        }

        if (some(aTitle))
        {
            strcat(str, "--title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        if (!xdialogPresent() && !gdialogPresent())
        {
            strcat(str, "--backtitle \"");
            strcat(str,
                   "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY");
            strcat(str, "\" ");
        }

        strcat(str, "--fselect \"");
        if (some(aDefaultPathAndFile))
        {
            if (!strchr(aDefaultPathAndFile, '/'))
            {
                strcat(str, "./");
            }
            strcat(str, aDefaultPathAndFile);
        }
        else if (!isTerminalRunning() && !lWasGraphicDialog)
        {
            strcat(str, getenv("HOME"));
            strcat(str, "/");
        }
        else
        {
            strcat(str, "./");
        }

        if (lWasGraphicDialog)
        {
            strcat(str, "\" 0 60 ) 2>&1 ");
        }
        else
        {
            strcat(str, "\" 0 60  >/dev/tty) ");
            if (lWasXterm)
            {
                strcat(str,
                       "2>/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
            }
            else
            {
                strcat(str, "2>&1 ; clear >/dev/tty");
            }
        }
    }
    else
    {
        if (lQuery)
        {
            return _inputBox(aTitle, null, null);
        }
        p = _inputBox(aTitle, "Save file", "");
        getPathWithoutFinalSlash(lString, p);
        if (!dirExists(lString))
            return null;
        getLastName(lString, p);
        if (!some(lString))
            return null;
        return p;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    lIn = popen(str, "r");
    if (!lIn)
        return null;
    while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
    {
    }
    pclose(lIn);
    removeLastNL(lBuff.ptr);
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    if (!some(lBuff.ptr))
        return null;
    getPathWithoutFinalSlash(lString, lBuff.ptr);
    if (!dirExists(lString))
        return null;
    getLastName(lString, lBuff.ptr);
    if (!filenameValid(lString))
        return null;
    return lBuff.ptr;
}

const(char*) _openFileDialog(
    const char* aTitle,
    const char* aDefaultPathAndFile,
    const TFD_Filter[] aFilters,
    const bool aAllowMultipleSelects)
{
    static char[MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char[MAX_PATH_OR_CMD] str_buf1 = '\0';
    char[MAX_PATH_OR_CMD] str_buf2 = '\0';
    char* str = str_buf1.ptr;
    char* lString = str_buf2.ptr;
    FILE* lIn;
    char* p;
    const(char)* p2;
    bool lWasKdialog;
    bool lWasGraphicDialog;
    bool lWasXterm;
    lBuff[0] = '\0';

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return cast(const(char)*)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e '");
        if (!aAllowMultipleSelects)
        {

            strcat(str, "POSIX path of ( ");
        }
        else
        {
            strcat(str, "set mylist to ");
        }
        strcat(str, "choose file ");
        if (some(aTitle))
        {
            strcat(str, "with prompt \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
        if (some(lString))
        {
            strcat(str, "default location \"");
            strcat(str, lString);
            strcat(str, "\" ");
        }

        osascriptAppendFilters(aFilters, str);

        if (aAllowMultipleSelects)
        {
            strcat(str, "multiple selections allowed true ' ");
            strcat(str,
                   "-e 'set mystring to POSIX path of item 1 of mylist' ");
            strcat(str,
                   "-e 'repeat with  i from 2 to the count of mylist' ");
            strcat(str, "-e 'set mystring to mystring & \"|\"' ");
            strcat(str,
                   "-e 'set mystring to mystring & POSIX path of item i of mylist' ");
            strcat(str, "-e 'end repeat' ");
            strcat(str, "-e 'mystring' ");
        }
        else
        {
            strcat(str, ")' ");
        }
        strcat(str, "-e 'on error number -128' ");
        strcat(str, "-e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return cast(const(char)*)1;
        }
        lWasKdialog = true;

        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        strcat(str, " --getopenfilename ");

        if (some(aDefaultPathAndFile))
        {
            if (aDefaultPathAndFile[0] != '/')
            {
                strcat(str, "$PWD/");
            }
            strcat(str, "\"");
            strcat(str, aDefaultPathAndFile);
            strcat(str, "\"");
        }
        else
        {
            strcat(str, "$PWD/");
        }

        kdialogAppendFilters(aFilters, str);

        if (aAllowMultipleSelects)
        {
            strcat(str, " --multiple --separate-output");
        }
        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
    }
    else if (zenityPresent() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        if (zenityPresent())
        {
            if (lQuery)
            {
                response("zenity");
                return cast(const(char)*)1;
            }
            strcpy(str, "zenity");
            if (zenity3Present() >= 4 && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return cast(const(char)*)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return cast(const(char)*)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return cast(const(char)*)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --file-selection");

        if (aAllowMultipleSelects)
        {
            strcat(str, " --multiple");
        }
        if (some(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (some(aDefaultPathAndFile))
        {
            strcat(str, " --filename=\"");
            strcat(str, aDefaultPathAndFile);
            strcat(str, "\"");
        }

        zenityAppendFilters(aFilters, str);

        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
    }
    else if (tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython2Name.ptr);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }
        strcat(str,
               " -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set " ~
                   "frontmost of process \\\"Python\\\" to true' ''');");
        }
        strcat(str, "lFiles=tkFileDialog.askopenfilename(");
        if (aAllowMultipleSelects)
        {
            strcat(str, "multiple=1,");
        }
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }

        tkinterAppendFilters(aFilters, str);

        strcat(str, `);
if not isinstance(lFiles, tuple):
    print lFiles
else:
    lFilesString=''
    for lFile in lFiles:
        lFilesString += str(lFile) + '|'
    print lFilesString[:-1]
"`);
    }
    else if (tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython3Name.ptr);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "lFiles=filedialog.askopenfilename(");
        if (aAllowMultipleSelects)
        {
            strcat(str, "multiple=1,");
        }
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (some(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }

        tkinterAppendFilters(aFilters, str);

        strcat(str, `);
if not isinstance(lFiles, tuple):
    print(lFiles)
else:
    lFilesString = ''
    for lFile in lFiles:
        lFilesString += str(lFile) + '|'
    print(lFilesString[:-1])
"`);
    }
    else if (xdialogPresent() || dialogName())
    {
        if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return cast(const(char)*)1;
            }
            lWasGraphicDialog = true;
            strcpy(str, "(Xdialog ");
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            strcpy(str, "(dialog ");
        }
        else
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            lWasXterm = true;
            strcpy(str, terminalName());
            strcat(str, "'(");
            strcat(str, dialogName());
            strcat(str, " ");
        }

        if (some(aTitle))
        {
            strcat(str, "--title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        if (!xdialogPresent() && !gdialogPresent())
        {
            strcat(str, "--backtitle \"");
            strcat(str,
                   "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY");
            strcat(str, "\" ");
        }

        strcat(str, "--fselect \"");
        if (some(aDefaultPathAndFile))
        {
            if (!strchr(aDefaultPathAndFile, '/'))
            {
                strcat(str, "./");
            }
            strcat(str, aDefaultPathAndFile);
        }
        else if (!isTerminalRunning() && !lWasGraphicDialog)
        {
            strcat(str, getenv("HOME"));
            strcat(str, "/");
        }
        else
        {
            strcat(str, "./");
        }

        if (lWasGraphicDialog)
        {
            strcat(str, "\" 0 60 ) 2>&1 ");
        }
        else
        {
            strcat(str, "\" 0 60  >/dev/tty) ");
            if (lWasXterm)
            {
                strcat(str,
                       "2>/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
            }
            else
            {
                strcat(str, "2>&1 ; clear >/dev/tty");
            }
        }
    }
    else
    {
        if (lQuery)
        {
            return _inputBox(aTitle, null, null);
        }
        p2 = _inputBox(aTitle, "Open file", "");
        if (!fileExists(p2))
            return null;
        return p2;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    lIn = popen(str, "r");
    if (!lIn)
        return null;

    lBuff[0] = '\0';
    p = lBuff.ptr;
    while (fgets(p, lBuff.sizeof, lIn) !is null)
    {
        p += strlen(p);
    }
    pclose(lIn);
    removeLastNL(lBuff.ptr);
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    if (lWasKdialog && aAllowMultipleSelects)
    {
        p = lBuff.ptr;
        while (p)
        {
            p = strchr(p, '\n');
            *p = '|';
        }
    }
    /* printf( "lBuff2: %s\n" , lBuff ) ; */
    if (!some(lBuff.ptr))
        return null;

    if (aAllowMultipleSelects && strchr(lBuff.ptr, '|'))
    {
        p2 = ensureFilesExist(lBuff.ptr, lBuff.ptr);
    }
    else if (fileExists(lBuff.ptr))
    {
        p2 = lBuff.ptr;
    }
    else
        return null;
    /* printf( "lBuff3: %s\n" , p2 ) ; */

    return p2;
}

const(char*) _selectFolderDialog(const char* aTitle, const char* aDefaultPath)
{
    static char[MAX_PATH_OR_CMD] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char* str = str_buf.ptr;
    FILE* lIn;
    const(char)* p;
    bool lWasGraphicDialog;
    bool lWasXterm;
    lBuff[0] = '\0';

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return cast(const(char)*)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'POSIX path of ( choose folder ");
        if (some(aTitle))
        {
            strcat(str, "with prompt \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        if (some(aDefaultPath))
        {
            strcat(str, "default location \"");
            strcat(str, aDefaultPath);
            strcat(str, "\" ");
        }
        strcat(str, ")' ");
        strcat(str, "-e 'on error number -128' ");
        strcat(str, "-e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return cast(const(char)*)1;
        }
        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        strcat(str, " --getexistingdirectory ");

        if (some(aDefaultPath))
        {
            if (aDefaultPath[0] != '/')
            {
                strcat(str, "$PWD/");
            }
            strcat(str, "\"");
            strcat(str, aDefaultPath);
            strcat(str, "\"");
        }
        else
        {
            strcat(str, "$PWD/");
        }

        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
    }
    else if (zenityPresent() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        if (zenityPresent())
        {
            if (lQuery)
            {
                response("zenity");
                return cast(const(char)*)1;
            }
            strcpy(str, "zenity");
            if (zenity3Present() >= 4 && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return cast(const(char)*)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return cast(const(char)*)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return cast(const(char)*)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --file-selection --directory");

        if (some(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (some(aDefaultPath))
        {
            strcat(str, " --filename=\"");
            strcat(str, aDefaultPath);
            strcat(str, "\"");
        }
        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
    }
    else if (!xdialogPresent() && tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython2Name.ptr);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }
        strcat(str,
               " -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set " ~
                   "frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "print tkFileDialog.askdirectory(");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aDefaultPath))
        {
            strcat(str, "initialdir='");
            strcat(str, aDefaultPath);
            strcat(str, "'");
        }
        strcat(str, ")\"");
    }
    else if (!xdialogPresent() && tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython3Name.ptr);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "print( filedialog.askdirectory(");
        if (some(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (some(aDefaultPath))
        {
            strcat(str, "initialdir='");
            strcat(str, aDefaultPath);
            strcat(str, "'");
        }
        strcat(str, ") )\"");
    }
    else if (xdialogPresent() || dialogName())
    {
        if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return cast(const(char)*)1;
            }
            lWasGraphicDialog = true;
            strcpy(str, "(Xdialog ");
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            strcpy(str, "(dialog ");
        }
        else
        {
            if (lQuery)
            {
                response("dialog");
                return cast(const(char)*)0;
            }
            lWasXterm = true;
            strcpy(str, terminalName());
            strcat(str, "'(");
            strcat(str, dialogName());
            strcat(str, " ");
        }

        if (some(aTitle))
        {
            strcat(str, "--title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        if (!xdialogPresent() && !gdialogPresent())
        {
            strcat(str, "--backtitle \"");
            strcat(str,
                   "tab: focus | /: populate | spacebar: fill text field | ok: TEXT FIELD ONLY");
            strcat(str, "\" ");
        }

        strcat(str, "--dselect \"");
        if (some(aDefaultPath))
        {
            strcat(str, aDefaultPath);
            ensureFinalSlash(str);
        }
        else if (!isTerminalRunning() && !lWasGraphicDialog)
        {
            strcat(str, getenv("HOME"));
            strcat(str, "/");
        }
        else
        {
            strcat(str, "./");
        }

        if (lWasGraphicDialog)
        {
            strcat(str, "\" 0 60 ) 2>&1 ");
        }
        else
        {
            strcat(str, "\" 0 60  >/dev/tty) ");
            if (lWasXterm)
            {
                strcat(str,
                       "2>/tmp/tinyfd.txt';cat /tmp/tinyfd.txt;rm /tmp/tinyfd.txt");
            }
            else
            {
                strcat(str, "2>&1 ; clear >/dev/tty");
            }
        }
    }
    else
    {
        if (lQuery)
        {
            return _inputBox(aTitle, null, null);
        }
        p = _inputBox(aTitle, "Select folder", "");
        if (!dirExists(p))
            return null;
        return p;
    }
    if (tinyfd_verbose)
        printf("str: %s\n", str);
    lIn = popen(str, "r");
    if (!lIn)
        return null;

    while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
    {
    }
    pclose(lIn);
    removeLastNL(lBuff.ptr);
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    if (!dirExists(lBuff.ptr))
        return null;
    return lBuff.ptr;
}

const(char*) _colorChooser(
    const char* aTitle,
    const char* aDefaultHexRGB,
    ref const ubyte[3] aDefaultRGB,
    ref ubyte[3] aoResultRGB)
{
    static char[128] lBuff = '\0';
    const bool lQuery = eq(aTitle, "tinyfd_query");
    char[128] tmp_buf = '\0';
    char[MAX_PATH_OR_CMD] str_buf = '\0';
    char* lTmp = tmp_buf.ptr;
    char* str = str_buf.ptr;
    char[8] lDefaultHexRGB = '\0';
    char* lpDefaultHexRGB;
    ubyte[3] lDefaultRGB;
    const(char)* p;
    FILE* lIn;
    bool lWasZenity3;
    bool lWasOsascript;
    bool lWasXdialog;
    lBuff[0] = '\0';

    if (aDefaultHexRGB)
    {
        Hex2RGB(aDefaultHexRGB, lDefaultRGB);
        lpDefaultHexRGB = cast(char*)aDefaultHexRGB;
    }
    else
    {
        lDefaultRGB[0] = aDefaultRGB[0];
        lDefaultRGB[1] = aDefaultRGB[1];
        lDefaultRGB[2] = aDefaultRGB[2];
        RGB2Hex(aDefaultRGB, lDefaultHexRGB.ptr);
        lpDefaultHexRGB = lDefaultHexRGB.ptr;
    }

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return cast(const(char)*)1;
        }
        lWasOsascript = true;
        strcpy(str, "osascript");

        if (!osx9orBetter())
        {
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
            strcat(str, " -e 'try' -e 'set mycolor to choose color default color {");
        }
        else
        {
            strcat(str,
                   " -e 'try' -e 'tell app (path to frontmost application as Unicode text) " ~
                   "to set mycolor to choose color default color {");
        }

        sprintf(lTmp, "%d", 256 * lDefaultRGB[0]);
        strcat(str, lTmp);
        strcat(str, ",");
        sprintf(lTmp, "%d", 256 * lDefaultRGB[1]);
        strcat(str, lTmp);
        strcat(str, ",");
        sprintf(lTmp, "%d", 256 * lDefaultRGB[2]);
        strcat(str, lTmp);
        strcat(str, "}' ");
        strcat(str,
               "-e 'set mystring to ((item 1 of mycolor) div 256 as integer) as string' ");
        strcat(str,
               "-e 'repeat with i from 2 to the count of mycolor' ");
        strcat(str,
               "-e 'set mystring to mystring & \" \" & ((item i of mycolor) div 256 as integer) as string' ");
        strcat(str, "-e 'end repeat' ");
        strcat(str, "-e 'mystring' ");
        strcat(str, "-e 'on error number -128' ");
        strcat(str, "-e 'end try'");
        if (!osx9orBetter())
            strcat(str, " -e 'end tell'");
    }
    else if (kdialogPresent())
    {
        if (lQuery)
        {
            response("kdialog");
            return cast(const(char)*)1;
        }
        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        sprintf(str + strlen(str), " --getcolor --default '%s'", lpDefaultHexRGB);

        if (some(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
    }
    else if (zenity3Present() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        lWasZenity3 = true;
        if (zenity3Present())
        {
            if (lQuery)
            {
                response("zenity3");
                return cast(const(char)*)1;
            }
            strcpy(str, "zenity");
            if (zenity3Present() >= 4 && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return cast(const(char)*)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return cast(const(char)*)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return cast(const(char)*)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --color-selection --show-palette");
        sprintf(str + strlen(str), " --color=%s", lpDefaultHexRGB);

        if (some(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
    }
    else if (xdialogPresent())
    {
        if (lQuery)
        {
            response("xdialog");
            return cast(const(char)*)1;
        }
        lWasXdialog = true;
        strcpy(str, "Xdialog --colorsel \"");
        if (some(aTitle))
        {
            strcat(str, aTitle);
        }
        strcat(str, "\" 0 60 ");
        sprintf(lTmp, "%hhu %hhu %hhu", lDefaultRGB[0], lDefaultRGB[1], lDefaultRGB[2]);
        strcat(str, lTmp);
        strcat(str, " 2>&1");
    }
    else if (tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython2Name.ptr);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }

        strcat(str,
               " -S -c \"import Tkinter,tkColorChooser;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''osascript -e 'tell app \\\"Finder\\\" to set " ~
                   "frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "res=tkColorChooser.askcolor(color='");
        strcat(str, lpDefaultHexRGB);
        strcat(str, "'");

        if (some(aTitle))
        {
            strcat(str, ",title='");
            strcat(str, aTitle);
            strcat(str, "'");
        }
        strcat(str, `);
if res[1] is not None:
    print res[1]
"`);
    }
    else if (tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return cast(const(char)*)1;
        }
        strcpy(str, gPython3Name.ptr);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import colorchooser;root=tkinter.Tk();root.withdraw();");
        strcat(str, "res=colorchooser.askcolor(color='");
        strcat(str, lpDefaultHexRGB);
        strcat(str, "'");

        if (some(aTitle))
        {
            strcat(str, ",title='");
            strcat(str, aTitle);
            strcat(str, "'");
        }
        strcat(str, `);
if res[1] is not None:
    print(res[1])
"`);
    }
    else
    {
        if (lQuery)
        {
            return _inputBox(aTitle, null, null);
        }
        p = _inputBox(aTitle, "Enter hex rgb color (i.e. #f5ca20)", lpDefaultHexRGB);
        if (!p || strlen(p) != 7 || p[0] != '#')
            return null;

        foreach (i; 1 .. 7)
        {
            if (!isxdigit(p[i]))
                return null;
        }
        Hex2RGB(p, aoResultRGB);
        return p;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    lIn = popen(str, "r");
    if (!lIn)
        return null;

    while (fgets(lBuff.ptr, lBuff.sizeof, lIn) !is null)
    {
    }
    pclose(lIn);
    if (!some(lBuff.ptr))
        return null;
    /* printf( "len Buff: %lu\n" , strlen(lBuff.ptr) ) ; */
    /* printf( "lBuff0: %s\n" , lBuff ) ; */
    removeLastNL(lBuff.ptr);

    if (lWasZenity3)
    {
        if (lBuff[0] == '#')
        {
            if (strlen(lBuff.ptr) > 7)
            {
                lBuff[3] = lBuff[5];
                lBuff[4] = lBuff[6];
                lBuff[5] = lBuff[9];
                lBuff[6] = lBuff[10];
                lBuff[7] = '\0';
            }
            Hex2RGB(lBuff.ptr, aoResultRGB);
        }
        else if (lBuff[3] == '(')
        {
            sscanf(lBuff.ptr, "rgb(%hhu,%hhu,%hhu",
                   &aoResultRGB[0], &aoResultRGB[1], &aoResultRGB[2]);
            RGB2Hex(aoResultRGB, lBuff.ptr);
        }
        else if (lBuff[4] == '(')
        {
            sscanf(lBuff.ptr, "rgba(%hhu,%hhu,%hhu",
                   &aoResultRGB[0], &aoResultRGB[1], &aoResultRGB[2]);
            RGB2Hex(aoResultRGB, lBuff.ptr);
        }
    }
    else if (lWasOsascript || lWasXdialog)
    {
        /* printf( "lBuff: %s\n" , lBuff ) ; */
        sscanf(lBuff.ptr, "%hhu %hhu %hhu",
               &aoResultRGB[0], &aoResultRGB[1], &aoResultRGB[2]);
        RGB2Hex(aoResultRGB, lBuff.ptr);
    }
    else
    {
        Hex2RGB(lBuff.ptr, aoResultRGB);
    }
    /* printf("%d %d %d\n", aoResultRGB[0],aoResultRGB[1],aoResultRGB[2]); */
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    return lBuff.ptr;
}

} // windows-unix
