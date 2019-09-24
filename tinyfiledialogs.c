/*_________
 /         \ tinyfiledialogs.c v3.3.9 [Apr 14, 2019] zlib licence
 |tiny file| Unique code file created [November 9, 2014]
 | dialogs | Copyright (c) 2014 - 2018 Guillaume Vareille http://ysengrin.com
 \____  ___/ http://tinyfiledialogs.sourceforge.net
      \|     git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd

- License -

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

#ifndef __sun
#define _POSIX_C_SOURCE 2 /* to accept POSIX 2 in old ANSI C standards */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "tinyfiledialogs.h"
/* #define TINYFD_NOLIB */

#ifdef _WIN32
#ifdef __BORLANDC__
#define _getch getch
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#ifndef TINYFD_NOLIB
#include <windows.h>
/*#define TINYFD_NOSELECTFOLDERWIN*/
#ifndef TINYFD_NOSELECTFOLDERWIN
#include <shlobj.h>
#endif /*TINYFD_NOSELECTFOLDERWIN*/
#endif
#include <conio.h>
#include <commdlg.h>
#define TINYFD_NOCCSUNICODE
#define SLASH "\\"
#else
#include <limits.h>
#include <unistd.h>
#include <dirent.h> /* on old systems try <sys/dir.h> instead */
#include <termios.h>
#include <sys/utsname.h>
#include <signal.h> /* on old systems try <sys/signal.h> instead */
#define SLASH "/"
#endif /* _WIN32 */

#define MAX_PATH_OR_CMD 1024 /* _MAX_PATH or MAX_PATH */
#define MAX_MULTIPLE_FILES 32

char const tinyfd_version[8] = "3.3.9";

int tinyfd_verbose = 0;
int tinyfd_silent = 1;

#if defined(TINYFD_NOLIB) && defined(_WIN32)
int tinyfd_forceConsole = 1;
#else
int tinyfd_forceConsole = 0;
#endif

char tinyfd_response[1024];

#if defined(TINYFD_NOLIB) && defined(_WIN32)
static int gWarningDisplayed = 1;
#else
static int gWarningDisplayed = 0;
#endif

static char const gTitle[] = "missing software! (we will try basic console input)";

#ifdef _WIN32
char const tinyfd_needs[] = "\
 ___________\n\
/           \\ \n\
| tiny file |\n\
|  dialogs  |\n\
\\_____  ____/\n\
      \\|\
\ntiny file dialogs on Windows needs:\
\n   a graphic display\
\nor dialog.exe (enhanced console mode)\
\nor a console for basic input";
#else
char const tinyfd_needs[] = "\
 ___________\n\
/           \\ \n\
| tiny file |\n\
|  dialogs  |\n\
\\_____  ____/\n\
      \\|\
\ntiny file dialogs on UNIX needs:\
\n   applescript\
\nor kdialog\
\nor zenity (or matedialog or qarma)\
\nor python (2 or 3)\
\n + tkinter + python-dbus (optional)\
\nor dialog (opens console if needed)\
\nor xterm + bash\
\n   (opens console for basic input)\
\nor existing console for basic input";
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4996) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
#pragma warning(disable : 4100) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
#pragma warning(disable : 4706) /* allows usage of strncpy, strcpy, strcat, sprintf, fopen */
#endif

static void response(char const *const message)
{
    strcpy(tinyfd_response, message);
}

static char *getPathWithoutFinalSlash(
    char *const aoDestination, /* make sure it is allocated, use _MAX_PATH */
    char const *const aSource) /* aoDestination and aSource can be the same */
{
    char const *lTmp;
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

static char *getLastName(
    char *const aoDestination, /* make sure it is allocated */
    char const *const aSource)
{
    /* copy the last name after '/' or '\' */
    char const *lTmp;
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

static void ensureFinalSlash(char *const aioString)
{
    if (aioString && strlen(aioString))
    {
        char *lastcar = aioString + strlen(aioString) - 1;
        if (strncmp(lastcar, SLASH, 1))
        {
            strcat(lastcar, SLASH);
        }
    }
}

static void Hex2RGB(char const aHexRGB[8],
                    unsigned char aoResultRGB[3])
{
    char lColorChannel[8];
    if (aoResultRGB)
    {
        if (aHexRGB)
        {
            strcpy(lColorChannel, aHexRGB);
            aoResultRGB[2] = (unsigned char)strtoul(lColorChannel + 5, NULL, 16);
            lColorChannel[5] = '\0';
            aoResultRGB[1] = (unsigned char)strtoul(lColorChannel + 3, NULL, 16);
            lColorChannel[3] = '\0';
            aoResultRGB[0] = (unsigned char)strtoul(lColorChannel + 1, NULL, 16);
            /* printf("%d %d %d\n", aoResultRGB[0], aoResultRGB[1], aoResultRGB[2]); */
        }
        else
        {
            aoResultRGB[0] = 0;
            aoResultRGB[1] = 0;
            aoResultRGB[2] = 0;
        }
    }
}

static void RGB2Hex(unsigned char const aRGB[3],
                    char aoResultHexRGB[8])
{
    if (aoResultHexRGB)
    {
        if (aRGB)
        {
#if defined(__GNUC__) && defined(_WIN32)
            sprintf(aoResultHexRGB, "#%02hx%02hx%02hx",
#else
            sprintf(aoResultHexRGB, "#%02hhx%02hhx%02hhx",
#endif
                    aRGB[0], aRGB[1], aRGB[2]);
            /* printf("aoResultHexRGB %s\n", aoResultHexRGB); */
        }
        else
        {
            aoResultHexRGB[0] = 0;
            aoResultHexRGB[1] = 0;
            aoResultHexRGB[2] = 0;
        }
    }
}

static void replaceSubStr(char const *const aSource,
                          char const *const aOldSubStr,
                          char const *const aNewSubStr,
                          char *const aoDestination)
{
    char const *pOccurence;
    char const *p;
    char const *lNewSubStr = "";
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
    while ((pOccurence = strstr(p, aOldSubStr)) != NULL)
    {
        strncat(aoDestination, p, pOccurence - p);
        strcat(aoDestination, lNewSubStr);
        p = pOccurence + lOldSubLen;
    }
    strcat(aoDestination, p);
}

static int filenameValid(char const *const aFileNameWithoutPath)
{
    if (!aFileNameWithoutPath || !strlen(aFileNameWithoutPath) || strpbrk(aFileNameWithoutPath, "\\/:*?\"<>|"))
    {
        return 0;
    }
    return 1;
}

#ifndef _WIN32

static int fileExists(char const *const aFilePathAndName)
{
    FILE *lIn;
    if (!aFilePathAndName || !strlen(aFilePathAndName))
    {
        return 0;
    }
    lIn = fopen(aFilePathAndName, "r");
    if (!lIn)
    {
        return 0;
    }
    fclose(lIn);
    return 1;
}

#elif defined(TINYFD_NOLIB)

static int fileExists(char const *const aFilePathAndName)
{
    FILE *lIn;
    if (!aFilePathAndName || !strlen(aFilePathAndName))
    {
        return 0;
    }

    if (1) // was tinyfd_winUtf8
        return 1; /* we cannot test */

    lIn = fopen(aFilePathAndName, "r");
    if (!lIn)
    {
        return 0;
    }
    fclose(lIn);
    return 1;
}

#endif

static void wipefile(char const *const aFilename)
{
    int i;
    struct stat st;
    FILE *lIn;

    if (stat(aFilename, &st) == 0)
    {
        if ((lIn = fopen(aFilename, "w")))
        {
            for (i = 0; i < st.st_size; i++)
            {
                fputc('A', lIn);
            }
        }
        fclose(lIn);
    }
}

#ifdef _WIN32

static int replaceChr(char *const aString,
                      char const aOldChr,
                      char const aNewChr)
{
    char *p;
    int lRes = 0;

    if (!aString)
    {
        return 0;
    }

    if (aOldChr == aNewChr)
    {
        return 0;
    }

    p = aString;
    while ((p = strchr(p, aOldChr)))
    {
        *p = aNewChr;
        p++;
        lRes = 1;
    }
    return lRes;
}

#ifdef TINYFD_NOLIB

static int dirExists(char const *const aDirPath)
{
    struct stat lInfo;

    if (!aDirPath || !strlen(aDirPath))
        return 0;
    if (stat(aDirPath, &lInfo) != 0)
        return 0;
    else if (1) // was tinyfd_winUtf8
        return 1; /* we cannot test */
    else if (lInfo.st_mode & S_IFDIR)
        return 1;
    else
        return 0;
}

void tinyfd_beep()
{
    printf("\a");
}

#else /* ndef TINYFD_NOLIB */

void tinyfd_beep()
{
    Beep(440, 300);
}

static void wipefileW(wchar_t const *const aFilename)
{
    int i;
    struct _stat st;
    FILE *lIn;

    if (_wstat(aFilename, &st) == 0)
    {
        if ((lIn = _wfopen(aFilename, L"w")))
        {
            for (i = 0; i < st.st_size; i++)
            {
                fputc('A', lIn);
            }
        }
        fclose(lIn);
    }
}

static wchar_t *getPathWithoutFinalSlashW(
    wchar_t *const aoDestination, /* make sure it is allocated, use _MAX_PATH */
    wchar_t const *const aSource) /* aoDestination and aSource can be the same */
{
    wchar_t const *lTmp;
    if (aSource)
    {
        lTmp = wcsrchr(aSource, L'/');
        if (!lTmp)
        {
            lTmp = wcsrchr(aSource, L'\\');
        }
        if (lTmp)
        {
            wcsncpy(aoDestination, aSource, lTmp - aSource);
            aoDestination[lTmp - aSource] = L'\0';
        }
        else
        {
            *aoDestination = L'\0';
        }
    }
    else
    {
        *aoDestination = L'\0';
    }
    return aoDestination;
}

static wchar_t *getLastNameW(
    wchar_t *const aoDestination, /* make sure it is allocated */
    wchar_t const *const aSource)
{
    /* copy the last name after '/' or '\' */
    wchar_t const *lTmp;
    if (aSource)
    {
        lTmp = wcsrchr(aSource, L'/');
        if (!lTmp)
        {
            lTmp = wcsrchr(aSource, L'\\');
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
        *aoDestination = L'\0';
    }
    return aoDestination;
}

static void Hex2RGBW(wchar_t const aHexRGB[8],
                     unsigned char aoResultRGB[3])
{
    wchar_t lColorChannel[8];
    if (aoResultRGB)
    {
        if (aHexRGB)
        {
            wcscpy(lColorChannel, aHexRGB);
            aoResultRGB[2] = (unsigned char)wcstoul(lColorChannel + 5, NULL, 16);
            lColorChannel[5] = '\0';
            aoResultRGB[1] = (unsigned char)wcstoul(lColorChannel + 3, NULL, 16);
            lColorChannel[3] = '\0';
            aoResultRGB[0] = (unsigned char)wcstoul(lColorChannel + 1, NULL, 16);
            /* printf("%d %d %d\n", aoResultRGB[0], aoResultRGB[1], aoResultRGB[2]); */
        }
        else
        {
            aoResultRGB[0] = 0;
            aoResultRGB[1] = 0;
            aoResultRGB[2] = 0;
        }
    }
}

static void RGB2HexW(
    unsigned char const aRGB[3],
    wchar_t aoResultHexRGB[8])
{
    if (aoResultHexRGB)
    {
        if (aRGB)
        {
            /* wprintf(L"aoResultHexRGB %s\n", aoResultHexRGB); */
            swprintf(aoResultHexRGB,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
                     8,
#endif
                     L"#%02hhx%02hhx%02hhx", aRGB[0], aRGB[1], aRGB[2]);
        }
        else
        {
            aoResultHexRGB[0] = 0;
            aoResultHexRGB[1] = 0;
            aoResultHexRGB[2] = 0;
        }
    }
}

#if !defined(WC_ERR_INVALID_CHARS)
/* undefined prior to Vista, so not yet in MINGW header file */
#define WC_ERR_INVALID_CHARS 0x00000080
#endif

static int sizeUtf16(char const *const aUtf8string)
{
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                               aUtf8string, -1, NULL, 0);
}

static int sizeUtf8(wchar_t const *const aUtf16string)
{
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                               aUtf16string, -1, NULL, 0, NULL, NULL);
}

static int sizeMbcs(wchar_t const *const aMbcsString)
{
    int lRes = WideCharToMultiByte(CP_ACP, 0,
                                   aMbcsString, -1, NULL, 0, NULL, NULL);
    /* DWORD licic = GetLastError(); */
    return lRes;
}

static wchar_t *utf8to16(char const *const aUtf8string)
{
    wchar_t *lUtf16string;
    int lSize = sizeUtf16(aUtf8string);
    lUtf16string = (wchar_t *)malloc(lSize * sizeof(wchar_t));
    lSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                aUtf8string, -1, lUtf16string, lSize);
    if (lSize == 0)
    {
        free(lUtf16string);
        return NULL;
    }
    return lUtf16string;
}

static wchar_t *mbcsTo16(char const *const aMbcsString)
{
    wchar_t *lMbcsString;
    int lSize = sizeUtf16(aMbcsString);
    lMbcsString = (wchar_t *)malloc(lSize * sizeof(wchar_t));
    lSize = MultiByteToWideChar(CP_ACP, 0,
                                aMbcsString, -1, lMbcsString, lSize);
    if (lSize == 0)
    {
        free(lMbcsString);
        return NULL;
    }
    return lMbcsString;
}

static char *utf16to8(wchar_t const *const aUtf16string)
{
    char *lUtf8string;
    int lSize = sizeUtf8(aUtf16string);
    lUtf8string = (char *)malloc(lSize);
    lSize = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                aUtf16string, -1, lUtf8string, lSize, NULL, NULL);
    if (lSize == 0)
    {
        free(lUtf8string);
        return NULL;
    }
    return lUtf8string;
}

static char *utf16toMbcs(wchar_t const *const aUtf16string)
{
    char *lMbcsString;
    int lSize = sizeMbcs(aUtf16string);
    lMbcsString = (char *)malloc(lSize);
    lSize = WideCharToMultiByte(CP_ACP, 0,
                                aUtf16string, -1, lMbcsString, lSize, NULL, NULL);
    if (lSize == 0)
    {
        free(lMbcsString);
        return NULL;
    }
    return lMbcsString;
}

static int dirExists(char const *const aDirPath)
{
    struct _stat lInfo;
    wchar_t *lTmpWChar;
    int lStatRet;
    int lDirLen;

    if (!aDirPath)
        return 0;
    lDirLen = strlen(aDirPath);
    if (!lDirLen)
        return 1;
    if ((lDirLen == 2) && (aDirPath[1] == ':'))
        return 1;

    lTmpWChar = utf8to16(aDirPath);
    lStatRet = _wstat(lTmpWChar, &lInfo);
    free(lTmpWChar);
    if (lStatRet != 0)
        return 0;
    else if (lInfo.st_mode & S_IFDIR)
        return 1;
    else
        return 0;
}

static int fileExists(char const *const aFilePathAndName)
{
    struct _stat lInfo;
    wchar_t *lTmpWChar;
    int lStatRet;
    FILE *lIn;

    if (!aFilePathAndName || !strlen(aFilePathAndName))
    {
        return 0;
    }

    lTmpWChar = utf8to16(aFilePathAndName);
    lStatRet = _wstat(lTmpWChar, &lInfo);
    free(lTmpWChar);
    if (lStatRet != 0)
        return 0;
    else if (lInfo.st_mode & _S_IFREG)
        return 1;
    else
        return 0;
}

static int replaceWchar(wchar_t *const aString,
                        wchar_t const aOldChr,
                        wchar_t const aNewChr)
{
    wchar_t *p;
    int lRes = 0;

    if (!aString)
    {
        return 0;
    }

    if (aOldChr == aNewChr)
    {
        return 0;
    }

    p = aString;
    while ((p = wcsrchr(p, aOldChr)))
    {
        *p = aNewChr;
#ifdef TINYFD_NOCCSUNICODE
        p++;
#endif
        p++;
        lRes = 1;
    }
    return lRes;
}

#endif /* TINYFD_NOLIB */
#endif /* _WIN32 */

/* source and destination can be the same or ovelap*/
static char const *ensureFilesExist(char *const aDestination,
                                    char const *const aSourcePathsAndNames)
{
    char *lDestination = aDestination;
    char const *p;
    char const *p2;
    size_t lLen;

    if (!aSourcePathsAndNames)
    {
        return NULL;
    }
    lLen = strlen(aSourcePathsAndNames);
    if (!lLen)
    {
        return NULL;
    }

    p = aSourcePathsAndNames;
    while ((p2 = strchr(p, '|')) != NULL)
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

#ifdef _WIN32
#ifndef TINYFD_NOLIB

static int __stdcall EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
    wchar_t lTitleName[MAX_PATH];
    GetWindowTextW(hwnd, lTitleName, MAX_PATH);
    /* wprintf(L"lTitleName %ls \n", lTitleName);  */
    if (wcscmp(L"tinyfiledialogsTopWindow", lTitleName) == 0)
    {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return 0;
    }
    return 1;
}

static void hiddenConsoleW(wchar_t const *const aString, wchar_t const *const aDialogTitle, int const aInFront)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    if (!aString || !wcslen(aString))
        return;

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOW);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    if (!CreateProcessW(NULL, (LPWSTR)aString, NULL, NULL, FALSE,
                        CREATE_NEW_CONSOLE, NULL, NULL,
                        &StartupInfo, &ProcessInfo))
    {
        return; /* GetLastError(); */
    }

    WaitForInputIdle(ProcessInfo.hProcess, INFINITE);
    if (aInFront)
    {
        while (EnumWindows(EnumThreadWndProc, (LPARAM)NULL))
        {
        }
        SetWindowTextW(GetForegroundWindow(), aDialogTitle);
    }
    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
}

static int tinyfd_messageBoxW(
    wchar_t const *const aTitle,      /* NULL or "" */
    wchar_t const *const aMessage,    /* NULL or ""  may contain \n and \t */
    wchar_t const *const aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
    wchar_t const *const aIconType,   /* "info" "warning" "error" "question" */
    int const aDefaultButton)         /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
    int lBoxReturnValue;
    UINT aCode;

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return 1;
    }

    if (aIconType && !wcscmp(L"warning", aIconType))
    {
        aCode = MB_ICONWARNING;
    }
    else if (aIconType && !wcscmp(L"error", aIconType))
    {
        aCode = MB_ICONERROR;
    }
    else if (aIconType && !wcscmp(L"question", aIconType))
    {
        aCode = MB_ICONQUESTION;
    }
    else
    {
        aCode = MB_ICONINFORMATION;
    }

    if (aDialogType && !wcscmp(L"okcancel", aDialogType))
    {
        aCode += MB_OKCANCEL;
        if (!aDefaultButton)
        {
            aCode += MB_DEFBUTTON2;
        }
    }
    else if (aDialogType && !wcscmp(L"yesno", aDialogType))
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

    lBoxReturnValue = MessageBoxW(GetForegroundWindow(), aMessage, aTitle, aCode);
    if (((aDialogType && wcscmp(L"okcancel", aDialogType) && wcscmp(L"yesno", aDialogType))) || (lBoxReturnValue == IDOK) || (lBoxReturnValue == IDYES))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int messageBoxWinGui8(
    char const *const aTitle,      /* NULL or "" */
    char const *const aMessage,    /* NULL or ""  may contain \n and \t */
    char const *const aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const *const aIconType,   /* "info" "warning" "error" "question" */
    int const aDefaultButton)      /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
    int lIntRetVal;
    wchar_t *lTitle;
    wchar_t *lMessage;
    wchar_t *lDialogType;
    wchar_t *lIconType;

    lTitle = utf8to16(aTitle);
    lMessage = utf8to16(aMessage);
    lDialogType = utf8to16(aDialogType);
    lIconType = utf8to16(aIconType);

    lIntRetVal = tinyfd_messageBoxW(lTitle, lMessage,
                                    lDialogType, lIconType, aDefaultButton);

    free(lTitle);
    free(lMessage);
    free(lDialogType);
    free(lIconType);

    return lIntRetVal;
}

/* return has only meaning for tinyfd_query */
static int tinyfd_notifyPopupW(
    wchar_t const *const aTitle,    /* NULL or L"" */
    wchar_t const *const aMessage,  /* NULL or L"" may contain \n \t */
    wchar_t const *const aIconType) /* L"info" L"warning" L"error" */
{
    wchar_t *str;
    size_t lTitleLen;
    size_t lMessageLen;
    size_t lDialogStringLen;

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return 1;
    }

    lTitleLen = aTitle ? wcslen(aTitle) : 0;
    lMessageLen = aMessage ? wcslen(aMessage) : 0;
    lDialogStringLen = 3 * MAX_PATH_OR_CMD + lTitleLen + lMessageLen;
    str = (wchar_t *)malloc(2 * lDialogStringLen);

    wcscpy(str, L"powershell.exe -command \"\
function Show-BalloonTip {\
[cmdletbinding()] \
param( \
[string]$Title = ' ', \
[string]$Message = ' ', \
[ValidateSet('info', 'warning', 'error')] \
[string]$IconType = 'info');\
[system.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms') | Out-Null ; \
$balloon = New-Object System.Windows.Forms.NotifyIcon ; \
$path = Get-Process -id $pid | Select-Object -ExpandProperty Path ; \
$icon = [System.Drawing.Icon]::ExtractAssociatedIcon($path) ;");

    wcscat(str, L"\
$balloon.Icon = $icon ; \
$balloon.BalloonTipIcon = $IconType ; \
$balloon.BalloonTipText = $Message ; \
$balloon.BalloonTipTitle = $Title ; \
$balloon.Text = 'lalala' ; \
$balloon.Visible = $true ; \
$balloon.ShowBalloonTip(5000)};\
Show-BalloonTip");

    if (aTitle && wcslen(aTitle))
    {
        wcscat(str, L" -Title '");
        wcscat(str, aTitle);
        wcscat(str, L"'");
    }
    if (aMessage && wcslen(aMessage))
    {
        wcscat(str, L" -Message '");
        wcscat(str, aMessage);
        wcscat(str, L"'");
    }
    if (aMessage && wcslen(aIconType))
    {
        wcscat(str, L" -IconType '");
        wcscat(str, aIconType);
        wcscat(str, L"'");
    }
    wcscat(str, L"\"");

    /* wprintf ( L"str: %ls\n" , str ) ; */

    hiddenConsoleW(str, aTitle, 0);
    free(str);
    return 1;
}

static int notifyWinGui(
    char const *const aTitle,   /* NULL or "" */
    char const *const aMessage, /* NULL or "" may NOT contain \n nor \t */
    char const *const aIconType)
{
    wchar_t *lTitle;
    wchar_t *lMessage;
    wchar_t *lIconType;

    lTitle = utf8to16(aTitle);
    lMessage = utf8to16(aMessage);
    lIconType = utf8to16(aIconType);

    tinyfd_notifyPopupW(lTitle, lMessage, lIconType);

    free(lTitle);
    free(lMessage);
    free(lIconType);
    return 1;
}

static wchar_t const *tinyfd_inputBoxW(
    wchar_t const *const aTitle,        /* NULL or L"" */
    wchar_t const *const aMessage,      /* NULL or L"" may NOT contain \n nor \t */
    wchar_t const *const aDefaultInput) /* L"" , if NULL it's a passwordBox */
{
    static wchar_t lBuff[MAX_PATH_OR_CMD];
    wchar_t *str;
    FILE *lIn;
    FILE *lFile;
    int lResult;
    size_t lTitleLen;
    size_t lMessageLen;
    size_t lDialogStringLen;

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return (wchar_t const *)1;
    }

    lTitleLen = aTitle ? wcslen(aTitle) : 0;
    lMessageLen = aMessage ? wcslen(aMessage) : 0;
    lDialogStringLen = 3 * MAX_PATH_OR_CMD + lTitleLen + lMessageLen;
    str = (wchar_t *)malloc(2 * lDialogStringLen);

    if (aDefaultInput)
    {
        swprintf(str,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
                 lDialogStringLen,
#endif
                 L"%ls\\AppData\\Local\\Temp\\tinyfd.vbs", _wgetenv(L"USERPROFILE"));
    }
    else
    {
        swprintf(str,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
                 lDialogStringLen,
#endif
                 L"%ls\\AppData\\Local\\Temp\\tinyfd.hta", _wgetenv(L"USERPROFILE"));
    }
    lIn = _wfopen(str, L"w");
    if (!lIn)
    {
        free(str);
        return NULL;
    }

    if (aDefaultInput)
    {
        wcscpy(str, L"Dim result:result=InputBox(\"");
        if (aMessage && wcslen(aMessage))
        {
            wcscpy(lBuff, aMessage);
            replaceWchar(lBuff, L'\n', L' ');
            wcscat(str, lBuff);
        }
        wcscat(str, L"\",\"tinyfiledialogsTopWindow\",\"");
        if (aDefaultInput && wcslen(aDefaultInput))
        {
            wcscpy(lBuff, aDefaultInput);
            replaceWchar(lBuff, L'\n', L' ');
            wcscat(str, lBuff);
        }
        wcscat(str, L"\"):If IsEmpty(result) then:WScript.Echo 0");
        wcscat(str, L":Else: WScript.Echo \"1\" & result : End If");
    }
    else
    {
        wcscpy(str, L"\n\
<html>\n\
<head>\n\
<title>");

        wcscat(str, L"tinyfiledialogsTopWindow");
        wcscat(str, L"</title>\n\
<HTA:APPLICATION\n\
ID = 'tinyfdHTA'\n\
APPLICATIONNAME = 'tinyfd_inputBox'\n\
MINIMIZEBUTTON = 'no'\n\
MAXIMIZEBUTTON = 'no'\n\
BORDER = 'dialog'\n\
SCROLL = 'no'\n\
SINGLEINSTANCE = 'yes'\n\
WINDOWSTATE = 'hidden'>\n\
\n\
<script language = 'VBScript'>\n\
\n\
intWidth = Screen.Width/4\n\
intHeight = Screen.Height/6\n\
ResizeTo intWidth, intHeight\n\
MoveTo((Screen.Width/2)-(intWidth/2)),((Screen.Height/2)-(intHeight/2))\n\
result = 0\n\
\n\
Sub Window_onLoad\n\
txt_input.Focus\n\
End Sub\n\
\n");

        wcscat(str, L"\
Sub Window_onUnload\n\
Set objFSO = CreateObject(\"Scripting.FileSystemObject\")\n\
Set oShell = CreateObject(\"WScript.Shell\")\n\
strHomeFolder = oShell.ExpandEnvironmentStrings(\"%USERPROFILE%\")\n\
Set objFile = objFSO.CreateTextFile(strHomeFolder & \"\\AppData\\Local\\Temp\\tinyfd.txt\",True,True)\n\
If result = 1 Then\n\
objFile.Write 1 & txt_input.Value\n\
Else\n\
objFile.Write 0\n\
End If\n\
objFile.Close\n\
End Sub\n\
\n\
Sub Run_ProgramOK\n\
result = 1\n\
window.Close\n\
End Sub\n\
\n\
Sub Run_ProgramCancel\n\
window.Close\n\
End Sub\n\
\n");

        wcscat(str, L"Sub Default_Buttons\n\
If Window.Event.KeyCode = 13 Then\n\
btn_OK.Click\n\
ElseIf Window.Event.KeyCode = 27 Then\n\
btn_Cancel.Click\n\
End If\n\
End Sub\n\
\n\
</script>\n\
</head>\n\
<body style = 'background-color:#EEEEEE' onkeypress = 'vbs:Default_Buttons' align = 'top'>\n\
<table width = '100%' height = '80%' align = 'center' border = '0'>\n\
<tr border = '0'>\n\
<td align = 'left' valign = 'middle' style='Font-Family:Arial'>\n");

        wcscat(str, aMessage ? aMessage : L"");

        wcscat(str, L"\n\
</td>\n\
<td align = 'right' valign = 'middle' style = 'margin-top: 0em'>\n\
<table  align = 'right' style = 'margin-right: 0em;'>\n\
<tr align = 'right' style = 'margin-top: 5em;'>\n\
<input type = 'button' value = 'OK' name = 'btn_OK' onClick = 'vbs:Run_ProgramOK' style = 'width: 5em; margin-top: 2em;'><br>\n\
<input type = 'button' value = 'Cancel' name = 'btn_Cancel' onClick = 'vbs:Run_ProgramCancel' style = 'width: 5em;'><br><br>\n\
</tr>\n\
</table>\n\
</td>\n\
</tr>\n\
</table>\n");

        wcscat(str, L"<table width = '100%' height = '100%' align = 'center' border = '0'>\n\
<tr>\n\
<td align = 'left' valign = 'top'>\n\
<input type = 'password' id = 'txt_input'\n\
name = 'txt_input' value = '' style = 'float:left;width:100%' ><BR>\n\
</td>\n\
</tr>\n\
</table>\n\
</body>\n\
</html>\n\
");
    }
    fputws(str, lIn);
    fclose(lIn);

    if (aDefaultInput)
    {
        swprintf(str,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
                 lDialogStringLen,
#endif
                 L"%ls\\AppData\\Local\\Temp\\tinyfd.txt", _wgetenv(L"USERPROFILE"));

#ifdef TINYFD_NOCCSUNICODE
        lFile = _wfopen(str, L"w");
        fputc(0xFF, lFile);
        fputc(0xFE, lFile);
#else
        lFile = _wfopen(str, L"wt, ccs=UNICODE");  /*or ccs=UTF-16LE*/
#endif
        fclose(lFile);

        wcscpy(str, L"cmd.exe /c cscript.exe //U //Nologo ");
        wcscat(str, L"\"%USERPROFILE%\\AppData\\Local\\Temp\\tinyfd.vbs\" ");
        wcscat(str, L">> \"%USERPROFILE%\\AppData\\Local\\Temp\\tinyfd.txt\"");
    }
    else
    {
        wcscpy(str,
               L"cmd.exe /c mshta.exe \"%USERPROFILE%\\AppData\\Local\\Temp\\tinyfd.hta\"");
    }

    /* wprintf ( "str: %ls\n" , str ) ; */

    hiddenConsoleW(str, aTitle, 1);

    swprintf(str,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
             lDialogStringLen,
#endif
             L"%ls\\AppData\\Local\\Temp\\tinyfd.txt", _wgetenv(L"USERPROFILE"));
    /* wprintf(L"str: %ls\n", str); */
#ifdef TINYFD_NOCCSUNICODE
    if (!(lIn = _wfopen(str, L"r")))
#else
    if (!(lIn = _wfopen(str, L"rt, ccs=UNICODE"))) /*or ccs=UTF-16LE*/
#endif
    {
        _wremove(str);
        free(str);
        return NULL;
    }

    memset(lBuff, 0, MAX_PATH_OR_CMD);

#ifdef TINYFD_NOCCSUNICODE
    fgets((char *)lBuff, 2 * MAX_PATH_OR_CMD, lIn);
#else
    fgetws(lBuff, MAX_PATH_OR_CMD, lIn);
#endif
    fclose(lIn);
    wipefileW(str);
    _wremove(str);

    if (aDefaultInput)
    {
        swprintf(str,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
                 lDialogStringLen,
#endif
                 L"%ls\\AppData\\Local\\Temp\\tinyfd.vbs",
                 _wgetenv(L"USERPROFILE"));
    }
    else
    {
        swprintf(str,
#if !defined(__BORLANDC__) && !defined(__TINYC__) && (!defined(__GNUC__) || (__GNUC__) >= 5)
                 lDialogStringLen,
#endif
                 L"%ls\\AppData\\Local\\Temp\\tinyfd.hta",
                 _wgetenv(L"USERPROFILE"));
    }
    _wremove(str);
    free(str);
    /* wprintf( L"lBuff: %ls\n" , lBuff ) ; */
#ifdef TINYFD_NOCCSUNICODE
    lResult = !wcsncmp(lBuff + 1, L"1", 1);
#else
    lResult = !wcsncmp(lBuff, L"1", 1);
#endif

    /* printf( "lResult: %d \n" , lResult ) ; */
    if (!lResult)
    {
        return NULL;
    }

    /* wprintf( "lBuff+1: %ls\n" , lBuff+1 ) ; */

#ifdef TINYFD_NOCCSUNICODE
    if (aDefaultInput)
    {
        lDialogStringLen = wcslen(lBuff);
        lBuff[lDialogStringLen - 1] = L'\0';
        lBuff[lDialogStringLen - 2] = L'\0';
    }
    return lBuff + 2;
#else
    if (aDefaultInput)
        lBuff[wcslen(lBuff) - 1] = L'\0';
    return lBuff + 1;
#endif
}

static char const *inputBoxWinGui(
    char *const aoBuff,
    char const *const aTitle,        /* NULL or "" */
    char const *const aMessage,      /* NULL or "" may NOT contain \n nor \t */
    char const *const aDefaultInput) /* "" , if NULL it's a passwordBox */
{
    wchar_t *lTitle;
    wchar_t *lMessage;
    wchar_t *lDefaultInput;
    wchar_t const *lTmpWChar;
    char *lTmpChar;

    lTitle = utf8to16(aTitle);
    lMessage = utf8to16(aMessage);
    lDefaultInput = utf8to16(aDefaultInput);

    lTmpWChar = tinyfd_inputBoxW(lTitle, lMessage, lDefaultInput);

    free(lTitle);
    free(lMessage);
    free(lDefaultInput);

    if (!lTmpWChar)
    {
        return NULL;
    }

    lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}

static wchar_t const *tinyfd_saveFileDialogW(
    wchar_t const *const aTitle,                   /* NULL or "" */
    wchar_t const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,                /* 0 */
    wchar_t const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    wchar_t const *const aSingleFilterDescription) /* NULL or "image files" */
{
    static wchar_t lBuff[MAX_PATH_OR_CMD];
    wchar_t lDirname[MAX_PATH_OR_CMD];
    wchar_t str[MAX_PATH_OR_CMD];
    wchar_t lFilterPatterns[MAX_PATH_OR_CMD] = L"";
    wchar_t *p;
    wchar_t *lRetval;
    int i;
    HRESULT lHResult;
    OPENFILENAMEW ofn = {0};

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return (wchar_t const *)1;
    }

    lHResult = CoInitializeEx(NULL, 0);

    getPathWithoutFinalSlashW(lDirname, aDefaultPathAndFile);
    getLastNameW(lBuff, aDefaultPathAndFile);

    if (aNumOfFilterPatterns > 0)
    {
        if (aSingleFilterDescription && wcslen(aSingleFilterDescription))
        {
            wcscpy(lFilterPatterns, aSingleFilterDescription);
            wcscat(lFilterPatterns, L"\n");
        }
        wcscat(lFilterPatterns, aFilterPatterns[0]);
        for (i = 1; i < aNumOfFilterPatterns; i++)
        {
            wcscat(lFilterPatterns, L";");
            wcscat(lFilterPatterns, aFilterPatterns[i]);
        }
        wcscat(lFilterPatterns, L"\n");
        if (!(aSingleFilterDescription && wcslen(aSingleFilterDescription)))
        {
            wcscpy(str, lFilterPatterns);
            wcscat(lFilterPatterns, str);
        }
        wcscat(lFilterPatterns, L"All Files\n*.*\n");
        p = lFilterPatterns;
        while ((p = wcschr(p, L'\n')) != NULL)
        {
            *p = L'\0';
            p++;
        }
    }

    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = GetForegroundWindow();
    ofn.hInstance = 0;
    ofn.lpstrFilter = lFilterPatterns && wcslen(lFilterPatterns) ? lFilterPatterns : NULL;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = lBuff;

    ofn.nMaxFile = MAX_PATH_OR_CMD;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = MAX_PATH_OR_CMD / 2;
    ofn.lpstrInitialDir = lDirname && wcslen(lDirname) ? lDirname : NULL;
    ofn.lpstrTitle = aTitle && wcslen(aTitle) ? aTitle : NULL;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0L;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (GetSaveFileNameW(&ofn) == 0)
    {
        lRetval = NULL;
    }
    else
    {
        lRetval = lBuff;
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }
    return lRetval;
}

static char const *saveFileDialogWinGui8(
    char *const aoBuff,
    char const *const aTitle,                   /* NULL or "" */
    char const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,             /* 0 */
    char const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    char const *const aSingleFilterDescription) /* NULL or "image files" */
{
    wchar_t *lTitle;
    wchar_t *lDefaultPathAndFile;
    wchar_t *lSingleFilterDescription;
    wchar_t **lFilterPatterns;
    wchar_t const *lTmpWChar;
    char *lTmpChar;
    int i;

    lFilterPatterns = (wchar_t **)malloc(aNumOfFilterPatterns * sizeof(wchar_t *));
    for (i = 0; i < aNumOfFilterPatterns; i++)
    {
        lFilterPatterns[i] = utf8to16(aFilterPatterns[i]);
    }

    lTitle = utf8to16(aTitle);
    lDefaultPathAndFile = utf8to16(aDefaultPathAndFile);
    lSingleFilterDescription = utf8to16(aSingleFilterDescription);

    lTmpWChar = tinyfd_saveFileDialogW(
        lTitle,
        lDefaultPathAndFile,
        aNumOfFilterPatterns,
        (wchar_t const **)/*stupid cast for gcc*/
        lFilterPatterns,
        lSingleFilterDescription);

    free(lTitle);
    free(lDefaultPathAndFile);
    free(lSingleFilterDescription);
    for (i = 0; i < aNumOfFilterPatterns; i++)
    {
        free(lFilterPatterns[i]);
    }
    free(lFilterPatterns);

    if (!lTmpWChar)
    {
        return NULL;
    }

    lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}

static wchar_t const *tinyfd_openFileDialogW(
    wchar_t const *const aTitle,                   /* NULL or "" */
    wchar_t const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,                /* 0 */
    wchar_t const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    wchar_t const *const aSingleFilterDescription, /* NULL or "image files" */
    int const aAllowMultipleSelects)               /* 0 or 1 */
{
    static wchar_t lBuff[MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD];

    size_t lLengths[MAX_MULTIPLE_FILES];
    wchar_t lDirname[MAX_PATH_OR_CMD];
    wchar_t lFilterPatterns[MAX_PATH_OR_CMD] = L"";
    wchar_t str[MAX_PATH_OR_CMD];
    wchar_t *lPointers[MAX_MULTIPLE_FILES];
    wchar_t *lRetval, *p;
    int i, j;
    size_t lBuffLen;
    HRESULT lHResult;
    OPENFILENAMEW ofn = {0};

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return (wchar_t const *)1;
    }

    lHResult = CoInitializeEx(NULL, 0);

    getPathWithoutFinalSlashW(lDirname, aDefaultPathAndFile);
    getLastNameW(lBuff, aDefaultPathAndFile);

    if (aNumOfFilterPatterns > 0)
    {
        if (aSingleFilterDescription && wcslen(aSingleFilterDescription))
        {
            wcscpy(lFilterPatterns, aSingleFilterDescription);
            wcscat(lFilterPatterns, L"\n");
        }
        wcscat(lFilterPatterns, aFilterPatterns[0]);
        for (i = 1; i < aNumOfFilterPatterns; i++)
        {
            wcscat(lFilterPatterns, L";");
            wcscat(lFilterPatterns, aFilterPatterns[i]);
        }
        wcscat(lFilterPatterns, L"\n");
        if (!(aSingleFilterDescription && wcslen(aSingleFilterDescription)))
        {
            wcscpy(str, lFilterPatterns);
            wcscat(lFilterPatterns, str);
        }
        wcscat(lFilterPatterns, L"All Files\n*.*\n");
        p = lFilterPatterns;
        while ((p = wcschr(p, L'\n')) != NULL)
        {
            *p = L'\0';
            p++;
        }
    }

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = GetForegroundWindow();
    ofn.hInstance = 0;
    ofn.lpstrFilter = lFilterPatterns && wcslen(lFilterPatterns) ? lFilterPatterns : NULL;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = lBuff;
    ofn.nMaxFile = MAX_PATH_OR_CMD;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = MAX_PATH_OR_CMD / 2;
    ofn.lpstrInitialDir = lDirname && wcslen(lDirname) ? lDirname : NULL;
    ofn.lpstrTitle = aTitle && wcslen(aTitle) ? aTitle : NULL;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0L;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (aAllowMultipleSelects)
    {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }

    if (GetOpenFileNameW(&ofn) == 0)
    {
        lRetval = NULL;
    }
    else
    {
        lBuffLen = wcslen(lBuff);
        lPointers[0] = lBuff + lBuffLen + 1;
        if (!aAllowMultipleSelects || (lPointers[0][0] == L'\0'))
        {
            lRetval = lBuff;
        }
        else
        {
            i = 0;
            do
            {
                lLengths[i] = wcslen(lPointers[i]);
                lPointers[i + 1] = lPointers[i] + lLengths[i] + 1;
                i++;
            } while (lPointers[i][0] != L'\0');
            i--;
            p = lBuff + MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD - 1;
            *p = L'\0';
            for (j = i; j >= 0; j--)
            {
                p -= lLengths[j];
                memmove(p, lPointers[j], lLengths[j] * sizeof(wchar_t));
                p--;
                *p = L'\\';
                p -= lBuffLen;
                memmove(p, lBuff, lBuffLen * sizeof(wchar_t));
                p--;
                *p = L'|';
            }
            p++;
            lRetval = p;
        }
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }
    return lRetval;
}

static char const *openFileDialogWinGui8(
    char *const aoBuff,
    char const *const aTitle,                   /*  NULL or "" */
    char const *const aDefaultPathAndFile,      /*  NULL or "" */
    int const aNumOfFilterPatterns,             /* 0 */
    char const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    char const *const aSingleFilterDescription, /* NULL or "image files" */
    int const aAllowMultipleSelects)            /* 0 or 1 */
{
    wchar_t *lTitle;
    wchar_t *lDefaultPathAndFile;
    wchar_t *lSingleFilterDescription;
    wchar_t **lFilterPatterns;
    wchar_t const *lTmpWChar;
    char *lTmpChar;
    int i;

    lFilterPatterns = (wchar_t **)malloc(aNumOfFilterPatterns * sizeof(wchar_t *));
    for (i = 0; i < aNumOfFilterPatterns; i++)
    {
        lFilterPatterns[i] = utf8to16(aFilterPatterns[i]);
    }

    lTitle = utf8to16(aTitle);
    lDefaultPathAndFile = utf8to16(aDefaultPathAndFile);
    lSingleFilterDescription = utf8to16(aSingleFilterDescription);

    lTmpWChar = tinyfd_openFileDialogW(
        lTitle,
        lDefaultPathAndFile,
        aNumOfFilterPatterns,
        (wchar_t const **)/*stupid cast for gcc*/
        lFilterPatterns,
        lSingleFilterDescription,
        aAllowMultipleSelects);

    free(lTitle);
    free(lDefaultPathAndFile);
    free(lSingleFilterDescription);
    for (i = 0; i < aNumOfFilterPatterns; i++)
    {
        free(lFilterPatterns[i]);
    }
    free(lFilterPatterns);

    if (!lTmpWChar)
    {
        return NULL;
    }

    lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}

#ifndef TINYFD_NOSELECTFOLDERWIN
static int __stdcall BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
    }
    return 0;
}

static int __stdcall BrowseCallbackProcW(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pData);
    }
    return 0;
}

static wchar_t const *tinyfd_selectFolderDialogW(
    wchar_t const *const aTitle,       /* NULL or "" */
    wchar_t const *const aDefaultPath) /* NULL or "" */
{
    static wchar_t lBuff[MAX_PATH_OR_CMD];

    BROWSEINFOW bInfo;
    LPITEMIDLIST lpItem;
    HRESULT lHResult;

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return (wchar_t const *)1;
    }

    lHResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    bInfo.hwndOwner = GetForegroundWindow();
    bInfo.pidlRoot = NULL;
    bInfo.pszDisplayName = lBuff;
    bInfo.lpszTitle = aTitle && wcslen(aTitle) ? aTitle : NULL;
    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        bInfo.ulFlags = BIF_USENEWUI;
    }
    bInfo.lpfn = BrowseCallbackProcW;
    bInfo.lParam = (LPARAM)aDefaultPath;
    bInfo.iImage = -1;

    lpItem = SHBrowseForFolderW(&bInfo);
    if (lpItem)
    {
        SHGetPathFromIDListW(lpItem, lBuff);
    }

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }
    return lBuff;
}

static char const *selectFolderDialogWinGui8(
    char *const aoBuff,
    char const *const aTitle,       /*  NULL or "" */
    char const *const aDefaultPath) /* NULL or "" */
{
    wchar_t *lTitle;
    wchar_t *lDefaultPath;
    wchar_t const *lTmpWChar;
    char *lTmpChar;

    lTitle = utf8to16(aTitle);
    lDefaultPath = utf8to16(aDefaultPath);

    lTmpWChar = tinyfd_selectFolderDialogW(
        lTitle,
        lDefaultPath);

    free(lTitle);
    free(lDefaultPath);
    if (!lTmpWChar)
    {
        return NULL;
    }

    lTmpChar = utf16to8(lTmpWChar);
    strcpy(aoBuff, lTmpChar);
    free(lTmpChar);

    return aoBuff;
}
#endif /*TINYFD_NOSELECTFOLDERWIN*/

static wchar_t const *tinyfd_colorChooserW(
    wchar_t const *const aTitle,         /* NULL or "" */
    wchar_t const *const aDefaultHexRGB, /* NULL or "#FF0000"*/
    unsigned char const aDefaultRGB[3],  /* { 0 , 255 , 255 } */
    unsigned char aoResultRGB[3])        /* { 0 , 0 , 0 } */
{
    static wchar_t lResultHexRGB[8];
    CHOOSECOLORW cc;
    COLORREF crCustColors[16];
    unsigned char lDefaultRGB[3];
    int lRet;

    HRESULT lHResult;

    if (aTitle && !wcscmp(aTitle, L"tinyfd_query"))
    {
        response("windows_wchar");
        return (wchar_t const *)1;
    }

    lHResult = CoInitializeEx(NULL, 0);

    if (aDefaultHexRGB)
    {
        Hex2RGBW(aDefaultHexRGB, lDefaultRGB);
    }
    else
    {
        lDefaultRGB[0] = aDefaultRGB[0];
        lDefaultRGB[1] = aDefaultRGB[1];
        lDefaultRGB[2] = aDefaultRGB[2];
    }

    /* we can't use aTitle */
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = GetForegroundWindow();
    cc.hInstance = NULL;
    cc.rgbResult = RGB(lDefaultRGB[0], lDefaultRGB[1], lDefaultRGB[2]);
    cc.lpCustColors = crCustColors;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR;
    cc.lCustData = 0;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;

    lRet = ChooseColorW(&cc);

    if (!lRet)
    {
        return NULL;
    }

    aoResultRGB[0] = GetRValue(cc.rgbResult);
    aoResultRGB[1] = GetGValue(cc.rgbResult);
    aoResultRGB[2] = GetBValue(cc.rgbResult);

    RGB2HexW(aoResultRGB, lResultHexRGB);

    if (lHResult == S_OK || lHResult == S_FALSE)
    {
        CoUninitialize();
    }

    return lResultHexRGB;
}

static char const *colorChooserWinGui8(
    char const *const aTitle,           /* NULL or "" */
    char const *const aDefaultHexRGB,   /* NULL or "#FF0000"*/
    unsigned char const aDefaultRGB[3], /* { 0 , 255 , 255 } */
    unsigned char aoResultRGB[3])       /* { 0 , 0 , 0 } */
{
    static char lResultHexRGB[8];

    wchar_t *lTitle;
    wchar_t *lDefaultHexRGB;
    wchar_t const *lTmpWChar;
    char *lTmpChar;

    lTitle = utf8to16(aTitle);
    lDefaultHexRGB = utf8to16(aDefaultHexRGB);

    lTmpWChar = tinyfd_colorChooserW(
        lTitle,
        lDefaultHexRGB,
        aDefaultRGB,
        aoResultRGB);

    free(lTitle);
    free(lDefaultHexRGB);
    if (!lTmpWChar)
    {
        return NULL;
    }

    lTmpChar = utf16to8(lTmpWChar);
    strcpy(lResultHexRGB, lTmpChar);
    free(lTmpChar);

    return lResultHexRGB;
}
#endif /* TINYFD_NOLIB */

static int dialogPresent()
{
    static int lDialogPresent = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;
    char const *lString = "dialog.exe";
    if (lDialogPresent < 0)
    {
        if (!(lIn = _popen("where dialog.exe", "r")))
        {
            lDialogPresent = 0;
            return 0;
        }
        while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
        {
        }
        _pclose(lIn);
        if (lBuff[strlen(lBuff) - 1] == '\n')
        {
            lBuff[strlen(lBuff) - 1] = '\0';
        }
        if (strcmp(lBuff + strlen(lBuff) - strlen(lString), lString))
        {
            lDialogPresent = 0;
        }
        else
        {
            lDialogPresent = 1;
        }
    }
    return lDialogPresent;
}

static int messageBoxWinConsole(
    char const *const aTitle,      /* NULL or "" */
    char const *const aMessage,    /* NULL or ""  may contain \n and \t */
    char const *const aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const *const aIconType,   /* "info" "warning" "error" "question" */
    int const aDefaultButton)      /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
    char str[MAX_PATH_OR_CMD];
    char lDialogFile[MAX_PATH_OR_CMD];
    FILE *lIn;
    char lBuff[MAX_PATH_OR_CMD] = "";

    strcpy(str, "dialog ");
    if (aTitle && strlen(aTitle))
    {
        strcat(str, "--title \"");
        strcat(str, aTitle);
        strcat(str, "\" ");
    }

    if (aDialogType && (!strcmp("okcancel", aDialogType) || !strcmp("yesno", aDialogType) || !strcmp("yesnocancel", aDialogType)))
    {
        strcat(str, "--backtitle \"");
        strcat(str, "tab: move focus");
        strcat(str, "\" ");
    }

    if (aDialogType && !strcmp("okcancel", aDialogType))
    {
        if (!aDefaultButton)
        {
            strcat(str, "--defaultno ");
        }
        strcat(str,
               "--yes-label \"Ok\" --no-label \"Cancel\" --yesno ");
    }
    else if (aDialogType && !strcmp("yesno", aDialogType))
    {
        if (!aDefaultButton)
        {
            strcat(str, "--defaultno ");
        }
        strcat(str, "--yesno ");
    }
    else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
    if (aMessage && strlen(aMessage))
    {
        replaceSubStr(aMessage, "\n", "\\n", lBuff);
        strcat(str, lBuff);
        lBuff[0] = '\0';
    }
    strcat(str, "\" ");

    if (aDialogType && !strcmp("yesnocancel", aDialogType))
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

    if (!(lIn = fopen(lDialogFile, "r")))
    {
        remove(lDialogFile);
        return 0;
    }
    while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
    {
    }
    fclose(lIn);
    remove(lDialogFile);
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }

    /* if (tinyfd_verbose) printf("lBuff: %s\n", lBuff); */
    if (!strlen(lBuff))
    {
        return 0;
    }

    if (aDialogType && !strcmp("yesnocancel", aDialogType))
    {
        if (lBuff[0] == 'Y')
            return 1;
        else
            return 2;
    }

    return 1;
}

static char const *inputBoxWinConsole(
    char *const aoBuff,
    char const *const aTitle,        /* NULL or "" */
    char const *const aMessage,      /* NULL or "" may NOT contain \n nor \t */
    char const *const aDefaultInput) /* "" , if NULL it's a passwordBox */
{
    char str[MAX_PATH_OR_CMD];
    char lDialogFile[MAX_PATH_OR_CMD];
    FILE *lIn;
    int lResult;

    strcpy(lDialogFile, getenv("USERPROFILE"));
    strcat(lDialogFile, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcpy(str, "echo|set /p=1 >");
    strcat(str, lDialogFile);
    strcat(str, " & ");

    strcat(str, "dialog ");
    if (aTitle && strlen(aTitle))
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
    if (aMessage && strlen(aMessage))
    {
        strcat(str, aMessage);
    }
    strcat(str, "\" 10 60 ");
    if (aDefaultInput && strlen(aDefaultInput))
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

    if (!(lIn = fopen(lDialogFile, "r")))
    {
        remove(lDialogFile);
        return 0;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
    {
    }
    fclose(lIn);

    wipefile(lDialogFile);
    remove(lDialogFile);
    if (aoBuff[strlen(aoBuff) - 1] == '\n')
    {
        aoBuff[strlen(aoBuff) - 1] = '\0';
    }
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */

    /* printf( "aoBuff: %s len: %lu \n" , aoBuff , strlen(aoBuff) ) ; */
    lResult = strncmp(aoBuff, "1", 1) ? 0 : 1;
    /* printf( "lResult: %d \n" , lResult ) ; */
    if (!lResult)
    {
        return NULL;
    }
    /* printf( "aoBuff+1: %s\n" , aoBuff+1 ) ; */
    return aoBuff + 3;
}

static char const *saveFileDialogWinConsole(
    char *const aoBuff,
    char const *const aTitle,              /* NULL or "" */
    char const *const aDefaultPathAndFile) /* NULL or "" */
{
    char str[MAX_PATH_OR_CMD];
    char lPathAndFile[MAX_PATH_OR_CMD] = "";
    FILE *lIn;

    strcpy(str, "dialog ");
    if (aTitle && strlen(aTitle))
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
    if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
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

    if (!(lIn = fopen(lPathAndFile, "r")))
    {
        remove(lPathAndFile);
        return NULL;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
    {
    }
    fclose(lIn);
    remove(lPathAndFile);
    replaceChr(aoBuff, '/', '\\');
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */
    getLastName(str, aoBuff);
    if (!strlen(str))
    {
        return NULL;
    }
    return aoBuff;
}

static char const *openFileDialogWinConsole(
    char *const aoBuff,
    char const *const aTitle,              /*  NULL or "" */
    char const *const aDefaultPathAndFile, /*  NULL or "" */
    int const aAllowMultipleSelects)       /* 0 or 1 */
{
    char lFilterPatterns[MAX_PATH_OR_CMD] = "";
    char str[MAX_PATH_OR_CMD];
    FILE *lIn;

    strcpy(str, "dialog ");
    if (aTitle && strlen(aTitle))
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
    if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
    {
        /* dialog.exe uses unix separators even on windows */
        strcpy(lFilterPatterns, aDefaultPathAndFile);
        replaceChr(lFilterPatterns, '\\', '/');
    }

    /* dialog.exe needs at least one separator */
    if (!strchr(lFilterPatterns, '/'))
    {
        strcat(str, "./");
    }
    strcat(str, lFilterPatterns);
    strcat(str, "\" 0 60 2>");
    strcpy(lFilterPatterns, getenv("USERPROFILE"));
    strcat(lFilterPatterns, "\\AppData\\Local\\Temp\\tinyfd.txt");
    strcat(str, lFilterPatterns);

    /* printf( "str: %s\n" , str ) ; */
    system(str);

    if (!(lIn = fopen(lFilterPatterns, "r")))
    {
        remove(lFilterPatterns);
        return NULL;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
    {
    }
    fclose(lIn);
    remove(lFilterPatterns);
    replaceChr(aoBuff, '/', '\\');
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */
    return aoBuff;
}

static char const *selectFolderDialogWinConsole(
    char *const aoBuff,
    char const *const aTitle,       /*  NULL or "" */
    char const *const aDefaultPath) /* NULL or "" */
{
    char str[MAX_PATH_OR_CMD];
    char lString[MAX_PATH_OR_CMD];
    FILE *lIn;

    strcpy(str, "dialog ");
    if (aTitle && strlen(aTitle))
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
    if (aDefaultPath && strlen(aDefaultPath))
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

    if (!(lIn = fopen(lString, "r")))
    {
        remove(lString);
        return NULL;
    }
    while (fgets(aoBuff, MAX_PATH_OR_CMD, lIn) != NULL)
    {
    }
    fclose(lIn);
    remove(lString);
    replaceChr(aoBuff, '/', '\\');
    /* printf( "aoBuff: %s\n" , aoBuff ) ; */
    return aoBuff;
}

int tinyfd_messageBox(
    char const *const aTitle,      /* NULL or "" */
    char const *const aMessage,    /* NULL or ""  may contain \n and \t */
    char const *const aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const *const aIconType,   /* "info" "warning" "error" "question" */
    int const aDefaultButton)      /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
    char lChar;
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");

#ifndef TINYFD_NOLIB
    if ((!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent())) && (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return 1;
        }
        return messageBoxWinGui8(
            aTitle, aMessage, aDialogType, aIconType, aDefaultButton);
    }
    else
#endif /* TINYFD_NOLIB */
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
            gWarningDisplayed = 1;
            printf("\n\n%s\n", gTitle);
            printf("%s\n\n", tinyfd_needs);
        }
        if (aTitle && strlen(aTitle))
        {
            printf("\n%s\n\n", aTitle);
        }
        if (aDialogType && !strcmp("yesno", aDialogType))
        {
            do
            {
                if (aMessage && strlen(aMessage))
                {
                    printf("%s\n", aMessage);
                }
                printf("y/n: ");
                lChar = (char)tolower(_getch());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n');
            return lChar == 'y' ? 1 : 0;
        }
        else if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            do
            {
                if (aMessage && strlen(aMessage))
                {
                    printf("%s\n", aMessage);
                }
                printf("[O]kay/[C]ancel: ");
                lChar = (char)tolower(_getch());
                printf("\n\n");
            } while (lChar != 'o' && lChar != 'c');
            return lChar == 'o' ? 1 : 0;
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
            do
            {
                if (aMessage && strlen(aMessage))
                {
                    printf("%s\n", aMessage);
                }
                printf("[Y]es/[N]o/[C]ancel: ");
                lChar = (char)tolower(_getch());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n' && lChar != 'c');
            return (lChar == 'y') ? 1 : (lChar == 'n') ? 2 : 0;
        }
        else
        {
            if (aMessage && strlen(aMessage))
            {
                printf("%s\n\n", aMessage);
            }
            printf("press enter to continue ");
            lChar = (char)_getch();
            printf("\n\n");
            return 1;
        }
    }
}

/* return has only meaning for tinyfd_query */
int tinyfd_notifyPopup(
    char const *const aTitle,    /* NULL or "" */
    char const *const aMessage,  /* NULL or "" may contain \n \t */
    char const *const aIconType) /* "info" "warning" "error" */
{
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");

#ifndef TINYFD_NOLIB
    if ((!tinyfd_forceConsole || !(
                                     GetConsoleWindow() ||
                                     dialogPresent())) &&
        (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return 1;
        }
        return notifyWinGui(aTitle, aMessage, aIconType);
    }
    else
#endif /* TINYFD_NOLIB */
    {
        return tinyfd_messageBox(aTitle, aMessage, "ok", aIconType, 0);
    }
}

/* returns NULL on cancel */
char const *tinyfd_inputBox(
    char const *const aTitle,        /* NULL or "" */
    char const *const aMessage,      /* NULL or "" may NOT contain \n nor \t */
    char const *const aDefaultInput) /* "" , if NULL it's a passwordBox */
{
    static char lBuff[MAX_PATH_OR_CMD];
    char *lEOF;
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");

#ifndef TINYFD_NOLIB
    DWORD mode = 0;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    if ((!tinyfd_forceConsole || !(
                                     GetConsoleWindow() ||
                                     dialogPresent())) &&
        (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return (char const *)1;
        }
        lBuff[0] = '\0';
        return inputBoxWinGui(lBuff, aTitle, aMessage, aDefaultInput);
    }
    else
#endif /* TINYFD_NOLIB */
        if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return (char const *)0;
        }
        lBuff[0] = '\0';
        return inputBoxWinConsole(lBuff, aTitle, aMessage, aDefaultInput);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return (char const *)0;
        }
        lBuff[0] = '\0';
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = 1;
            printf("\n\n%s\n", gTitle);
            printf("%s\n\n", tinyfd_needs);
        }
        if (aTitle && strlen(aTitle))
        {
            printf("\n%s\n\n", aTitle);
        }
        if (aMessage && strlen(aMessage))
        {
            printf("%s\n", aMessage);
        }
        printf("(ctrl-Z + enter to cancel): ");
#ifndef TINYFD_NOLIB
        if (!aDefaultInput)
        {
            GetConsoleMode(hStdin, &mode);
            SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
        }
#endif /* TINYFD_NOLIB */
        lEOF = fgets(lBuff, MAX_PATH_OR_CMD, stdin);
        if (!lEOF)
        {
            return NULL;
        }
#ifndef TINYFD_NOLIB
        if (!aDefaultInput)
        {
            SetConsoleMode(hStdin, mode);
            printf("\n");
        }
#endif /* TINYFD_NOLIB */
        printf("\n");
        if (strchr(lBuff, 27))
        {
            return NULL;
        }
        if (lBuff[strlen(lBuff) - 1] == '\n')
        {
            lBuff[strlen(lBuff) - 1] = '\0';
        }
        return lBuff;
    }
}

char const *tinyfd_saveFileDialog(
    char const *const aTitle,                   /* NULL or "" */
    char const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,             /* 0 */
    char const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    char const *const aSingleFilterDescription) /* NULL or "image files" */
{
    static char lBuff[MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char lString[MAX_PATH_OR_CMD];
    char const *p;
    lBuff[0] = '\0';
#ifndef TINYFD_NOLIB
    if ((!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent())) && (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return (char const *)1;
        }
        p = saveFileDialogWinGui8(lBuff,
                                    aTitle, aDefaultPathAndFile, aNumOfFilterPatterns, aFilterPatterns, aSingleFilterDescription);
    }
    else
#endif /* TINYFD_NOLIB */
        if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return (char const *)0;
        }
        p = saveFileDialogWinConsole(lBuff, aTitle, aDefaultPathAndFile);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return (char const *)0;
        }
        p = tinyfd_inputBox(aTitle, "Save file", "");
    }

    if (!p || !strlen(p))
    {
        return NULL;
    }
    getPathWithoutFinalSlash(lString, p);
    if (strlen(lString) && !dirExists(lString))
    {
        return NULL;
    }
    getLastName(lString, p);
    if (!filenameValid(lString))
    {
        return NULL;
    }
    return p;
}

/* in case of multiple files, the separator is | */
char const *tinyfd_openFileDialog(
    char const *const aTitle,                   /* NULL or "" */
    char const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,             /* 0 */
    char const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    char const *const aSingleFilterDescription, /* NULL or "image files" */
    int const aAllowMultipleSelects)            /* 0 or 1 */
{
    static char lBuff[MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char const *p;
#ifndef TINYFD_NOLIB
    if ((!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent())) && (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return (char const *)1;
        }
        p = openFileDialogWinGui8(lBuff,
                                  aTitle, aDefaultPathAndFile, aNumOfFilterPatterns,
                                  aFilterPatterns, aSingleFilterDescription, aAllowMultipleSelects);
    }
    else
#endif /* TINYFD_NOLIB */
        if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return (char const *)0;
        }
        p = openFileDialogWinConsole(lBuff,
                                     aTitle, aDefaultPathAndFile, aAllowMultipleSelects);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return (char const *)0;
        }
        p = tinyfd_inputBox(aTitle, "Open file", "");
    }

    if (!p || !strlen(p))
    {
        return NULL;
    }
    if (aAllowMultipleSelects && strchr(p, '|'))
    {
        p = ensureFilesExist(lBuff, p);
    }
    else if (!fileExists(p))
    {
        return NULL;
    }
    /* printf( "lBuff3: %s\n" , p ) ; */
    return p;
}

char const *tinyfd_selectFolderDialog(
    char const *const aTitle,       /* NULL or "" */
    char const *const aDefaultPath) /* NULL or "" */
{
    static char lBuff[MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char const *p;
#ifndef TINYFD_NOLIB
    if ((!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent())) && (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return (char const *)1;
        }
#ifndef TINYFD_NOSELECTFOLDERWIN
        p = selectFolderDialogWinGui8(lBuff, aTitle, aDefaultPath);
#endif
    }
    else
#endif /* TINYFD_NOLIB */
        if (dialogPresent())
    {
        if (lQuery)
        {
            response("dialog");
            return (char const *)0;
        }
        p = selectFolderDialogWinConsole(lBuff, aTitle, aDefaultPath);
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return (char const *)0;
        }
        p = tinyfd_inputBox(aTitle, "Select folder", "");
    }

    if (!p || !strlen(p) || !dirExists(p))
    {
        return NULL;
    }
    return p;
}

/* returns the hexcolor as a string "#FF0000" */
/* aoResultRGB also contains the result */
/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
/* aDefaultRGB and aoResultRGB can be the same array */
char const *tinyfd_colorChooser(
    char const *const aTitle,           /* NULL or "" */
    char const *const aDefaultHexRGB,   /* NULL or "#FF0000"*/
    unsigned char const aDefaultRGB[3], /* { 0 , 255 , 255 } */
    unsigned char aoResultRGB[3])       /* { 0 , 0 , 0 } */
{
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char lDefaultHexRGB[8];
    char *lpDefaultHexRGB;
    int i;
    char const *p;

#ifndef TINYFD_NOLIB
    if ((!tinyfd_forceConsole || !(GetConsoleWindow() || dialogPresent())) && (!getenv("SSH_CLIENT") || getenv("DISPLAY")))
    {
        if (lQuery)
        {
            response("windows");
            return (char const *)1;
        }
        return colorChooserWinGui8(
            aTitle, aDefaultHexRGB, aDefaultRGB, aoResultRGB);
    }
    else
#endif /* TINYFD_NOLIB */
        if (aDefaultHexRGB)
    {
        lpDefaultHexRGB = (char *)aDefaultHexRGB;
    }
    else
    {
        RGB2Hex(aDefaultRGB, lDefaultHexRGB);
        lpDefaultHexRGB = (char *)lDefaultHexRGB;
    }
    p = tinyfd_inputBox(aTitle,
                        "Enter hex rgb color (i.e. #f5ca20)", lpDefaultHexRGB);
    if (lQuery)
        return p;

    if (!p || (strlen(p) != 7) || (p[0] != '#'))
    {
        return NULL;
    }
    for (i = 1; i < 7; i++)
    {
        if (!isxdigit(p[i]))
        {
            return NULL;
        }
    }
    Hex2RGB(p, aoResultRGB);
    return p;
}

#else  /* unix */

static char gPython2Name[16];
static char gPython3Name[16];
static char gPythonName[16];

static int isDarwin()
{
    static int ret = -1;
    struct utsname lUtsname;
    if (ret < 0)
    {
        ret = !uname(&lUtsname) && !strcmp(lUtsname.sysname, "Darwin");
    }
    return ret;
}

static int dirExists(char const *const aDirPath)
{
    DIR *lDir;
    if (!aDirPath || !strlen(aDirPath))
        return 0;
    lDir = opendir(aDirPath);
    if (!lDir)
    {
        return 0;
    }
    closedir(lDir);
    return 1;
}

static int detectPresence(char const *const aExecutable)
{
    char lBuff[MAX_PATH_OR_CMD];
    char lTestedString[MAX_PATH_OR_CMD] = "which ";
    FILE *lIn;

    strcat(lTestedString, aExecutable);
    strcat(lTestedString, " 2>/dev/null ");
    lIn = popen(lTestedString, "r");
    if ((fgets(lBuff, sizeof(lBuff), lIn) != NULL) && (!strchr(lBuff, ':')) && (strncmp(lBuff, "no ", 3)))
    { /* present */
        pclose(lIn);
        if (tinyfd_verbose)
            printf("detectPresence %s %d\n", aExecutable, 1);
        return 1;
    }
    else
    {
        pclose(lIn);
        if (tinyfd_verbose)
            printf("detectPresence %s %d\n", aExecutable, 0);
        return 0;
    }
}

static char const *getVersion(char const *const aExecutable) /*version must be first numeral*/
{
    static char lBuff[MAX_PATH_OR_CMD];
    char lTestedString[MAX_PATH_OR_CMD];
    FILE *lIn;
    char *lTmp;

    strcpy(lTestedString, aExecutable);
    strcat(lTestedString, " --version");

    lIn = popen(lTestedString, "r");
    lTmp = fgets(lBuff, sizeof(lBuff), lIn);
    pclose(lIn);

    lTmp += strcspn(lTmp, "0123456789");
    /* printf("lTmp:%s\n", lTmp); */
    return lTmp;
}

static int *const getMajorMinorPatch(char const *const aExecutable)
{
    static int lArray[3];
    char *lTmp;

    lTmp = (char *)getVersion(aExecutable);
    lArray[0] = atoi(strtok(lTmp, " ,.-"));
    /* printf("lArray0 %d\n", lArray[0]); */
    lArray[1] = atoi(strtok(0, " ,.-"));
    /* printf("lArray1 %d\n", lArray[1]); */
    lArray[2] = atoi(strtok(0, " ,.-"));
    /* printf("lArray2 %d\n", lArray[2]); */

    if (!lArray[0] && !lArray[1] && !lArray[2])
        return NULL;
    return lArray;
}

static int tryCommand(char const *const aCommand)
{
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;

    lIn = popen(aCommand, "r");
    if (fgets(lBuff, sizeof(lBuff), lIn) == NULL)
    { /* present */
        pclose(lIn);
        return 1;
    }
    else
    {
        pclose(lIn);
        return 0;
    }
}

static int isTerminalRunning()
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

static char const *dialogNameOnly()
{
    static char ret[128] = "*";
    if (ret[0] == '*')
    {
        if (isDarwin() && strcpy(ret, "/opt/local/bin/dialog") && detectPresence(ret))
        {
        }
        else if (strcpy(ret, "dialog") && detectPresence(ret))
        {
        }
        else
        {
            strcpy(ret, "");
        }
    }
    return ret;
}

int isDialogVersionBetter09b()
{
    char const *lDialogName;
    char *lVersion;
    int lMajor;
    int lMinor;
    int lDate;
    int lResult;
    char *lMinorP;
    char *lLetter;
    char lBuff[128];

    /*char lTest[128] = " 0.9b-20031126" ;*/

    lDialogName = dialogNameOnly();
    if (!strlen(lDialogName) || !(lVersion = (char *)getVersion(lDialogName)))
        return 0;
    /*lVersion = lTest ;*/
    /*printf("lVersion %s\n", lVersion);*/
    strcpy(lBuff, lVersion);
    lMajor = atoi(strtok(lVersion, " ,.-"));
    /*printf("lMajor %d\n", lMajor);*/
    lMinorP = strtok(0, " ,.-abcdefghijklmnopqrstuvxyz");
    lMinor = atoi(lMinorP);
    /*printf("lMinor %d\n", lMinor );*/
    lDate = atoi(strtok(0, " ,.-"));
    if (lDate < 0)
        lDate = -lDate;
    /*printf("lDate %d\n", lDate);*/
    lLetter = lMinorP + strlen(lMinorP);
    strcpy(lVersion, lBuff);
    strtok(lLetter, " ,.-");
    /*printf("lLetter %s\n", lLetter);*/
    lResult = (lMajor > 0) || ((lMinor == 9) && (*lLetter == 'b') && (lDate >= 20031126));
    /*printf("lResult %d\n", lResult);*/
    return lResult;
}

static int whiptailPresentOnly()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("whiptail");
    }
    return ret;
}

static char const *terminalName()
{
    static char ret[128] = "*";
    char lShellName[64] = "*";
    int *lArray;

    if (ret[0] == '*')
    {
        if (detectPresence("bash"))
        {
            strcpy(lShellName, "bash -c "); /*good for basic input*/
        }
        else if (strlen(dialogNameOnly()) || whiptailPresentOnly())
        {
            strcpy(lShellName, "sh -c "); /*good enough for dialog & whiptail*/
        }
        else
        {
            strcpy(ret, "");
            return NULL;
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
        else if (strcpy(ret, "gnome-terminal") && detectPresence(ret) && (lArray = getMajorMinorPatch(ret)) && ((lArray[0] < 3) || (lArray[0] == 3 && lArray[1] <= 6)))
        {
            strcat(ret, " --disable-factory -x ");
            strcat(ret, lShellName);
        }
        else
        {
            strcpy(ret, "");
        }
        /* bad: koi rxterm guake tilda vala-terminal qterminal
                aterm Terminal terminology sakura lilyterm weston-terminal
                roxterm termit xvt rxvt mrxvt urxvt */
    }
    if (strlen(ret))
    {
        return ret;
    }
    else
    {
        return NULL;
    }
}

static char const *dialogName()
{
    char const *ret;
    ret = dialogNameOnly();
    if (strlen(ret) && (isTerminalRunning() || terminalName()))
    {
        return ret;
    }
    else
    {
        return NULL;
    }
}

static int whiptailPresent()
{
    int ret;
    ret = whiptailPresentOnly();
    if (ret && (isTerminalRunning() || terminalName()))
    {
        return ret;
    }
    else
    {
        return 0;
    }
}

static int graphicMode()
{
    return !(tinyfd_forceConsole && (isTerminalRunning() || terminalName())) && (getenv("DISPLAY") || (isDarwin() && (!getenv("SSH_TTY") || getenv("DISPLAY"))));
}

static int pactlPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("pactl");
    }
    return ret;
}

static int speakertestPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("speaker-test");
    }
    return ret;
}

static int beepexePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("beep.exe");
    }
    return ret;
}

static int xmessagePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("xmessage"); /*if not tty,not on osxpath*/
    }
    return ret && graphicMode();
}

static int gxmessagePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("gxmessage");
    }
    return ret && graphicMode();
}

static int gmessagePresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("gmessage");
    }
    return ret && graphicMode();
}

static int notifysendPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("notify-send");
    }
    return ret && graphicMode();
}

static int perlPresent()
{
    static int ret = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;

    if (ret < 0)
    {
        ret = detectPresence("perl");
        if (ret)
        {
            lIn = popen("perl -MNet::DBus -e \"Net::DBus->session->get_service('org.freedesktop.Notifications')\" 2>&1", "r");
            if (fgets(lBuff, sizeof(lBuff), lIn) == NULL)
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

static int afplayPresent()
{
    static int ret = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;

    if (ret < 0)
    {
        ret = detectPresence("afplay");
        if (ret)
        {
            lIn = popen("test -e /System/Library/Sounds/Ping.aiff || echo Ping", "r");
            if (fgets(lBuff, sizeof(lBuff), lIn) == NULL)
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

static int xdialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("Xdialog");
    }
    return ret && graphicMode();
}

static int gdialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("gdialog");
    }
    return ret && graphicMode();
}

static int osascriptPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        gWarningDisplayed |= !!getenv("SSH_TTY");
        ret = detectPresence("osascript");
    }
    return ret && graphicMode() && !getenv("SSH_TTY");
}

static int qarmaPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("qarma");
    }
    return ret && graphicMode();
}

static int matedialogPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("matedialog");
    }
    return ret && graphicMode();
}

static int shellementaryPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = 0; /*detectPresence("shellementary"); shellementary is not ready yet */
    }
    return ret && graphicMode();
}

static int zenityPresent()
{
    static int ret = -1;
    if (ret < 0)
    {
        ret = detectPresence("zenity");
    }
    return ret && graphicMode();
}

static int zenity3Present()
{
    static int ret = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;
    int lIntTmp;

    if (ret < 0)
    {
        ret = 0;
        if (zenityPresent())
        {
            lIn = popen("zenity --version", "r");
            if (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
            {
                if (atoi(lBuff) >= 3)
                {
                    ret = 3;
                    lIntTmp = atoi(strtok(lBuff, ".") + 2);
                    if (lIntTmp >= 18)
                    {
                        ret = 5;
                    }
                    else if (lIntTmp >= 10)
                    {
                        ret = 4;
                    }
                }
                else if ((atoi(lBuff) == 2) && (atoi(strtok(lBuff, ".") + 2) >= 32))
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

static int kdialogPresent()
{
    static int ret = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;
    char *lDesktop;

    if (ret < 0)
    {
        if (zenityPresent())
        {
            lDesktop = getenv("XDG_SESSION_DESKTOP");
            if (!lDesktop || (strcmp(lDesktop, "KDE") && strcmp(lDesktop, "lxqt")))
            {
                ret = 0;
                return ret;
            }
        }

        ret = detectPresence("kdialog");
        if (ret && !getenv("SSH_TTY"))
        {
            lIn = popen("kdialog --attach 2>&1", "r");
            if (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
            {
                if (!strstr("Unknown", lBuff))
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
                if (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
                {
                    if (!strstr("Unknown", lBuff))
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

static int osx9orBetter()
{
    static int ret = -1;
    char lBuff[MAX_PATH_OR_CMD];
    FILE *lIn;
    int V, v;

    if (ret < 0)
    {
        ret = 0;
        lIn = popen("osascript -e 'set osver to system version of (system info)'", "r");
        if ((fgets(lBuff, sizeof(lBuff), lIn) != NULL) && (2 == sscanf(lBuff, "%d.%d", &V, &v)))
        {
            V = V * 100 + v;
            if (V >= 1009)
            {
                ret = 1;
            }
        }
        pclose(lIn);
        if (tinyfd_verbose)
            printf("Osx10 = %d, %d = %s\n", ret, V, lBuff);
    }
    return ret;
}

static int python2Present()
{
    static int ret = -1;
    int i;

    if (ret < 0)
    {
        ret = 0;
        strcpy(gPython2Name, "python2");
        if (detectPresence(gPython2Name))
            ret = 1;
        else
        {
            for (i = 9; i >= 0; i--)
            {
                sprintf(gPython2Name, "python2.%d", i);
                if (detectPresence(gPython2Name))
                {
                    ret = 1;
                    break;
                }
            }
            /*if ( ! ret )
            {
                strcpy(gPython2Name , "python" ) ;
                if ( detectPresence(gPython2Name) ) ret = 1;
            }*/
        }
        if (tinyfd_verbose)
            printf("python2Present %d\n", ret);
        if (tinyfd_verbose)
            printf("gPython2Name %s\n", gPython2Name);
    }
    return ret;
}

static int python3Present()
{
    static int ret = -1;
    int i;

    if (ret < 0)
    {
        ret = 0;
        strcpy(gPython3Name, "python3");
        if (detectPresence(gPython3Name))
            ret = 1;
        else
        {
            for (i = 9; i >= 0; i--)
            {
                sprintf(gPython3Name, "python3.%d", i);
                if (detectPresence(gPython3Name))
                {
                    ret = 1;
                    break;
                }
            }
            /*if ( ! ret )
            {
                strcpy(gPython3Name , "python" ) ;
                if ( detectPresence(gPython3Name) ) ret = 1;
            }*/
        }
        if (tinyfd_verbose)
            printf("python3Present %d\n", ret);
        if (tinyfd_verbose)
            printf("gPython3Name %s\n", gPython3Name);
    }
    return ret;
}

static int tkinter2Present()
{
    static int ret = -1;
    char lPythonCommand[256];
    char lPythonParams[256] =
        "-S -c \"try:\n\timport Tkinter;\nexcept:\n\tprint 0;\"";

    if (ret < 0)
    {
        ret = 0;
        if (python2Present())
        {
            sprintf(lPythonCommand, "%s %s", gPython2Name, lPythonParams);
            ret = tryCommand(lPythonCommand);
        }
        if (tinyfd_verbose)
            printf("tkinter2Present %d\n", ret);
    }
    return ret && graphicMode() && !(isDarwin() && getenv("SSH_TTY"));
}

static int tkinter3Present()
{
    static int ret = -1;
    char lPythonCommand[256];
    char lPythonParams[256] =
        "-S -c \"try:\n\timport tkinter;\nexcept:\n\tprint(0);\"";

    if (ret < 0)
    {
        ret = 0;
        if (python3Present())
        {
            sprintf(lPythonCommand, "%s %s", gPython3Name, lPythonParams);
            ret = tryCommand(lPythonCommand);
        }
        if (tinyfd_verbose)
            printf("tkinter3Present %d\n", ret);
    }
    return ret && graphicMode() && !(isDarwin() && getenv("SSH_TTY"));
}

static int pythonDbusPresent()
{
    static int ret = -1;
    char lPythonCommand[256];
    char lPythonParams[256] =
        "-c \"try:\n\timport dbus;bus=dbus.SessionBus();\
notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');\
notify=dbus.Interface(notif,'org.freedesktop.Notifications');\nexcept:\n\tprint(0);\"";

    if (ret < 0)
    {
        ret = 0;
        if (python2Present())
        {
            strcpy(gPythonName, gPython2Name);
            sprintf(lPythonCommand, "%s %s", gPythonName, lPythonParams);
            ret = tryCommand(lPythonCommand);
        }

        if (!ret && python3Present())
        {
            strcpy(gPythonName, gPython3Name);
            sprintf(lPythonCommand, "%s %s", gPythonName, lPythonParams);
            ret = tryCommand(lPythonCommand);
        }

        if (tinyfd_verbose)
            printf("dbusPresent %d\n", ret);
        if (tinyfd_verbose)
            printf("gPythonName %s\n", gPythonName);
    }
    return ret && graphicMode() && !(isDarwin() && getenv("SSH_TTY"));
}

static void sigHandler(int sig)
{
    FILE *lIn;
    if ((lIn = popen("pactl unload-module module-sine", "r")))
    {
        pclose(lIn);
    }
}

void tinyfd_beep()
{
    char str[256];
    FILE *lIn;

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
        signal(SIGINT, sigHandler);
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

    if ((lIn = popen(str, "r")))
    {
        pclose(lIn);
    }

    if (pactlPresent())
    {
        signal(SIGINT, SIG_DFL);
    }
}

int tinyfd_messageBox(
    char const *const aTitle,      /* NULL or "" */
    char const *const aMessage,    /* NULL or ""  may contain \n and \t */
    char const *const aDialogType, /* "ok" "okcancel" "yesno" "yesnocancel" */
    char const *const aIconType,   /* "info" "warning" "error" "question" */
    int const aDefaultButton)      /* 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel */
{
    char lBuff[MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char *str = NULL;
    char *lpDialogString;
    FILE *lIn;
    int lWasGraphicDialog = 0;
    int lWasXterm = 0;
    int lResult;
    char lChar;
    struct termios infoOri;
    struct termios info;
    size_t lTitleLen;
    size_t lMessageLen;

    lBuff[0] = '\0';

    lTitleLen = aTitle ? strlen(aTitle) : 0;
    lMessageLen = aMessage ? strlen(aMessage) : 0;
    if (!aTitle || !lQuery)
    {
        str = (char *)malloc(MAX_PATH_OR_CMD + lTitleLen + lMessageLen);
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
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" ");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "with title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        strcat(str, "with icon ");
        if (aIconType && !strcmp("error", aIconType))
        {
            strcat(str, "stop ");
        }
        else if (aIconType && !strcmp("warning", aIconType))
        {
            strcat(str, "caution ");
        }
        else /* question or info */
        {
            strcat(str, "note ");
        }
        if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "default button \"Cancel\" ");
            }
        }
        else if (aDialogType && !strcmp("yesno", aDialogType))
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
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
               "-e 'if vButton is \"Yes\" then' -e 'return 1'\
 -e 'else if vButton is \"OK\" then' -e 'return 1'\
 -e 'else if vButton is \"No\" then' -e 'return 2'\
 -e 'else' -e 'return 0' -e 'end if' ");

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
        if (aDialogType && (!strcmp("okcancel", aDialogType) || !strcmp("yesno", aDialogType) || !strcmp("yesnocancel", aDialogType)))
        {
            if (aIconType && (!strcmp("warning", aIconType) || !strcmp("error", aIconType)))
            {
                strcat(str, "warning");
            }
            if (!strcmp("yesnocancel", aDialogType))
            {
                strcat(str, "yesnocancel");
            }
            else
            {
                strcat(str, "yesno");
            }
        }
        else if (aIconType && !strcmp("error", aIconType))
        {
            strcat(str, "error");
        }
        else if (aIconType && !strcmp("warning", aIconType))
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
        if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            strcat(str,
                   " --yes-label Ok --no-label Cancel");
        }
        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }

        if (!strcmp("yesnocancel", aDialogType))
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
            if ((zenity3Present() >= 4) && !getenv("SSH_TTY"))
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

        if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            strcat(str,
                   "question --ok-label=Ok --cancel-label=Cancel");
        }
        else if (aDialogType && !strcmp("yesno", aDialogType))
        {
            strcat(str, "question");
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
            strcat(str, "list --column \"\" --hide-header \"Yes\" \"No\"");
        }
        else if (aIconType && !strcmp("error", aIconType))
        {
            strcat(str, "error");
        }
        else if (aIconType && !strcmp("warning", aIconType))
        {
            strcat(str, "warning");
        }
        else
        {
            strcat(str, "info");
        }
        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (aMessage && strlen(aMessage))
        {
            strcat(str, " --no-wrap --text=\"");
            strcat(str, aMessage);
            strcat(str, "\"");
        }
        if ((zenity3Present() >= 3) || (!zenityPresent() && (shellementaryPresent() || qarmaPresent())))
        {
            strcat(str, " --icon-name=dialog-");
            if (aIconType && (!strcmp("question", aIconType) || !strcmp("error", aIconType) || !strcmp("warning", aIconType)))
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

        if (!strcmp("yesnocancel", aDialogType))
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

        strcpy(str, gPython2Name);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }

        strcat(str,
               " -S -c \"import Tkinter,tkMessageBox;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "res=tkMessageBox.");
        if (aDialogType && !strcmp("okcancel", aDialogType))
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
        else if (aDialogType && !strcmp("yesno", aDialogType))
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
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
            }
        }
        else
        {
            strcat(str, "showinfo(");
        }

        strcat(str, "icon='");
        if (aIconType && (!strcmp("question", aIconType) || !strcmp("error", aIconType) || !strcmp("warning", aIconType)))
        {
            strcat(str, aIconType);
        }
        else
        {
            strcat(str, "info");
        }

        strcat(str, "',");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aMessage && strlen(aMessage))
        {
            strcat(str, "message='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "'");
        }

        if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
            strcat(str, ");\n\
if res is None :\n\tprint 0\n\
elif res is False :\n\tprint 2\n\
else :\n\tprint 1\n\"");
        }
        else
        {
            strcat(str, ");\n\
if res is False :\n\tprint 0\n\
else :\n\tprint 1\n\"");
        }
    }
    else if (!gxmessagePresent() && !gmessagePresent() && !gdialogPresent() && !xdialogPresent() && tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return 1;
        }

        strcpy(str, gPython3Name);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import messagebox;root=tkinter.Tk();root.withdraw();");

        strcat(str, "res=messagebox.");
        if (aDialogType && !strcmp("okcancel", aDialogType))
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
        else if (aDialogType && !strcmp("yesno", aDialogType))
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
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
            }
        }
        else
        {
            strcat(str, "showinfo(");
        }

        strcat(str, "icon='");
        if (aIconType && (!strcmp("question", aIconType) || !strcmp("error", aIconType) || !strcmp("warning", aIconType)))
        {
            strcat(str, aIconType);
        }
        else
        {
            strcat(str, "info");
        }

        strcat(str, "',");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aMessage && strlen(aMessage))
        {
            strcat(str, "message='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "'");
        }

        if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
            strcat(str, ");\n\
if res is None :\n\tprint(0)\n\
elif res is False :\n\tprint(2)\n\
else :\n\tprint(1)\n\"");
        }
        else
        {
            strcat(str, ");\n\
if res is False :\n\tprint(0)\n\
else :\n\tprint(1)\n\"");
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

        if (aDialogType && !strcmp("okcancel", aDialogType))
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
            }
        }
        else if (aDialogType && !strcmp("yesno", aDialogType))
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
            }
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
            }
        }
        else
        {
            strcat(str, " -buttons Ok:1");
            strcat(str, " -default Ok");
        }

        strcat(str, " -center \"");
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\"");
        if (aTitle && strlen(aTitle))
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
            lWasGraphicDialog = 1;
            strcpy(str, "(gdialog ");
        }
        else if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return 1;
            }
            lWasGraphicDialog = 1;
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
                lWasXterm = 1;
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
            lWasXterm = 1;
            strcpy(str, terminalName());
            strcat(str, "'(whiptail ");
        }

        if (aTitle && strlen(aTitle))
        {
            strcat(str, "--title \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }

        if (!xdialogPresent() && !gdialogPresent())
        {
            if (aDialogType && (!strcmp("okcancel", aDialogType) || !strcmp("yesno", aDialogType) || !strcmp("yesnocancel", aDialogType)))
            {
                strcat(str, "--backtitle \"");
                strcat(str, "tab: move focus");
                strcat(str, "\" ");
            }
        }

        if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "--defaultno ");
            }
            strcat(str,
                   "--yes-label \"Ok\" --no-label \"Cancel\" --yesno ");
        }
        else if (aDialogType && !strcmp("yesno", aDialogType))
        {
            if (!aDefaultButton)
            {
                strcat(str, "--defaultno ");
            }
            strcat(str, "--yesno ");
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" ");

        if (lWasGraphicDialog)
        {
            if (aDialogType && !strcmp("yesnocancel", aDialogType))
            {
                strcat(str, "0 60 0 Yes \"\" No \"\") 2>/tmp/tinyfd.txt;\
if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");
            }
            else
            {
                strcat(str,
                       "10 60 ) 2>&1;if [ $? = 0 ];then echo 1;else echo 0;fi");
            }
        }
        else
        {
            if (aDialogType && !strcmp("yesnocancel", aDialogType))
            {
                strcat(str, "0 60 0 Yes \"\" No \"\" >/dev/tty ) 2>/tmp/tinyfd.txt;\
                if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
                tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");

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
            gWarningDisplayed = 1;
            strcat(str, "echo \"");
            strcat(str, gTitle);
            strcat(str, "\";");
            strcat(str, "echo \"");
            strcat(str, tinyfd_needs);
            strcat(str, "\";echo;echo;");
        }
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "echo \"");
            strcat(str, aTitle);
            strcat(str, "\";echo;");
        }
        if (aMessage && strlen(aMessage))
        {
            strcat(str, "echo \"");
            strcat(str, aMessage);
            strcat(str, "\"; ");
        }
        if (aDialogType && !strcmp("yesno", aDialogType))
        {
            strcat(str, "echo -n \"y/n: \"; ");
            strcat(str, "stty sane -echo;");
            strcat(str,
                   "answer=$( while ! head -c 1 | grep -i [ny];do true ;done);");
            strcat(str,
                   "if echo \"$answer\" | grep -iq \"^y\";then\n");
            strcat(str, "\techo 1\nelse\n\techo 0\nfi");
        }
        else if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            strcat(str, "echo -n \"[O]kay/[C]ancel: \"; ");
            strcat(str, "stty sane -echo;");
            strcat(str,
                   "answer=$( while ! head -c 1 | grep -i [oc];do true ;done);");
            strcat(str,
                   "if echo \"$answer\" | grep -iq \"^o\";then\n");
            strcat(str, "\techo 1\nelse\n\techo 0\nfi");
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
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
    else if (!isTerminalRunning() && pythonDbusPresent() && !strcmp("ok", aDialogType))
    {
        if (lQuery)
        {
            response("python-dbus");
            return 1;
        }
        strcpy(str, gPythonName);
        strcat(str, " -c \"import dbus;bus=dbus.SessionBus();");
        strcat(str, "notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');");
        strcat(str, "notify=dbus.Interface(notif,'org.freedesktop.Notifications');");
        strcat(str, "notify.Notify('',0,'");
        if (aIconType && strlen(aIconType))
        {
            strcat(str, aIconType);
        }
        strcat(str, "','");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, aTitle);
        }
        strcat(str, "','");
        if (aMessage && strlen(aMessage))
        {
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
        }
        strcat(str, "','','',5000)\"");
    }
    else if (!isTerminalRunning() && (perlPresent() >= 2) && !strcmp("ok", aDialogType))
    {
        if (lQuery)
        {
            response("perl-dbus");
            return 1;
        }

        sprintf(str, "perl -e \"use Net::DBus;\
                                                                 my \\$sessionBus = Net::DBus->session;\
                                                                 my \\$notificationsService = \\$sessionBus->get_service('org.freedesktop.Notifications');\
                                                                 my \\$notificationsObject = \\$notificationsService->get_object('/org/freedesktop/Notifications',\
                                                                 'org.freedesktop.Notifications');\
                                                                 my \\$notificationId;\\$notificationId = \\$notificationsObject->Notify(shift, 0, '%s', '%s', '%s', [], {}, -1);\" ",
                aIconType ? aIconType : "", aTitle ? aTitle : "", aMessage ? aMessage : "");
    }
    else if (!isTerminalRunning() && notifysendPresent() && !strcmp("ok", aDialogType))
    {

        if (lQuery)
        {
            response("notifysend");
            return 1;
        }
        strcpy(str, "notify-send");
        if (aIconType && strlen(aIconType))
        {
            strcat(str, " -i '");
            strcat(str, aIconType);
            strcat(str, "'");
        }
        strcat(str, " \"");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, aTitle);
            strcat(str, " | ");
        }
        if (aMessage && strlen(aMessage))
        {
            replaceSubStr(aMessage, "\n\t", " |  ", lBuff);
            replaceSubStr(aMessage, "\n", " | ", lBuff);
            replaceSubStr(aMessage, "\t", "  ", lBuff);
            strcat(str, lBuff);
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
            gWarningDisplayed = 1;
            printf("\n\n%s\n", gTitle);
            printf("%s\n\n", tinyfd_needs);
        }
        if (aTitle && strlen(aTitle))
        {
            printf("\n%s\n", aTitle);
        }

        tcgetattr(0, &infoOri);
        tcgetattr(0, &info);
        info.c_lflag &= ~ICANON;
        info.c_cc[VMIN] = 1;
        info.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &info);
        if (aDialogType && !strcmp("yesno", aDialogType))
        {
            do
            {
                if (aMessage && strlen(aMessage))
                {
                    printf("\n%s\n", aMessage);
                }
                printf("y/n: ");
                fflush(stdout);
                lChar = tolower(getchar());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n');
            lResult = lChar == 'y' ? 1 : 0;
        }
        else if (aDialogType && !strcmp("okcancel", aDialogType))
        {
            do
            {
                if (aMessage && strlen(aMessage))
                {
                    printf("\n%s\n", aMessage);
                }
                printf("[O]kay/[C]ancel: ");
                fflush(stdout);
                lChar = tolower(getchar());
                printf("\n\n");
            } while (lChar != 'o' && lChar != 'c');
            lResult = lChar == 'o' ? 1 : 0;
        }
        else if (aDialogType && !strcmp("yesnocancel", aDialogType))
        {
            do
            {
                if (aMessage && strlen(aMessage))
                {
                    printf("\n%s\n", aMessage);
                }
                printf("[Y]es/[N]o/[C]ancel: ");
                fflush(stdout);
                lChar = tolower(getchar());
                printf("\n\n");
            } while (lChar != 'y' && lChar != 'n' && lChar != 'c');
            lResult = (lChar == 'y') ? 1 : (lChar == 'n') ? 2 : 0;
        }
        else
        {
            if (aMessage && strlen(aMessage))
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

    if (!(lIn = popen(str, "r")))
    {
        free(str);
        return 0;
    }
    while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
    {
    }

    pclose(lIn);

    /* printf( "lBuff: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }
    /* printf( "lBuff1: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */

    if (aDialogType && !strcmp("yesnocancel", aDialogType))
    {
        if (lBuff[0] == '1')
        {
            if (!strcmp(lBuff + 1, "Yes"))
                strcpy(lBuff, "1");
            else if (!strcmp(lBuff + 1, "No"))
                strcpy(lBuff, "2");
        }
    }
    /* printf( "lBuff2: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */

    lResult = !strcmp(lBuff, "2") ? 2 : !strcmp(lBuff, "1") ? 1 : 0;

    /* printf( "lResult: %d\n" , lResult ) ; */
    free(str);
    return lResult;
}

/* return has only meaning for tinyfd_query */
int tinyfd_notifyPopup(
    char const *const aTitle,    /* NULL or "" */
    char const *const aMessage,  /* NULL or ""  may contain \n and \t */
    char const *const aIconType) /* "info" "warning" "error" */
{
    char lBuff[MAX_PATH_OR_CMD];
    char *str = NULL;
    char *lpDialogString;
    FILE *lIn;
    size_t lTitleLen;
    size_t lMessageLen;
    int lQuery;

    if (getenv("SSH_TTY"))
    {
        return tinyfd_messageBox(aTitle, aMessage, "ok", aIconType, 0);
    }

    lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    lTitleLen = aTitle ? strlen(aTitle) : 0;
    lMessageLen = aMessage ? strlen(aMessage) : 0;
    if (!aTitle || lQuery)
    {
        str = (char *)malloc(MAX_PATH_OR_CMD + lTitleLen + lMessageLen);
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
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, " \" ");
        if (aTitle && strlen(aTitle))
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

        if (aIconType && strlen(aIconType))
        {
            strcat(str, " --icon '");
            strcat(str, aIconType);
            strcat(str, "'");
        }
        if (aTitle && strlen(aTitle))
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

        if (aIconType && strlen(aIconType))
        {
            strcat(str, " --window-icon '");
            strcat(str, aIconType);
            strcat(str, "'");
        }

        strcat(str, " --text \"");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, aTitle);
            strcat(str, "\n");
        }
        if (aMessage && strlen(aMessage))
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
        sprintf(str, "perl -e \"use Net::DBus;\
                                                                 my \\$sessionBus = Net::DBus->session;\
                                                                 my \\$notificationsService = \\$sessionBus->get_service('org.freedesktop.Notifications');\
                                                                 my \\$notificationsObject = \\$notificationsService->get_object('/org/freedesktop/Notifications',\
                                                                 'org.freedesktop.Notifications');\
                                                                 my \\$notificationId;\\$notificationId = \\$notificationsObject->Notify(shift, 0, '%s', '%s', '%s', [], {}, -1);\" ",
                aIconType ? aIconType : "", aTitle ? aTitle : "", aMessage ? aMessage : "");
    }
    else if (pythonDbusPresent())
    {
        if (lQuery)
        {
            response("python-dbus");
            return 1;
        }
        strcpy(str, gPythonName);
        strcat(str, " -c \"import dbus;bus=dbus.SessionBus();");
        strcat(str, "notif=bus.get_object('org.freedesktop.Notifications','/org/freedesktop/Notifications');");
        strcat(str, "notify=dbus.Interface(notif,'org.freedesktop.Notifications');");
        strcat(str, "notify.Notify('',0,'");
        if (aIconType && strlen(aIconType))
        {
            strcat(str, aIconType);
        }
        strcat(str, "','");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, aTitle);
        }
        strcat(str, "','");
        if (aMessage && strlen(aMessage))
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
        if (aIconType && strlen(aIconType))
        {
            strcat(str, " -i '");
            strcat(str, aIconType);
            strcat(str, "'");
        }
        strcat(str, " \"");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, aTitle);
            strcat(str, " | ");
        }
        if (aMessage && strlen(aMessage))
        {
            replaceSubStr(aMessage, "\n\t", " |  ", lBuff);
            replaceSubStr(aMessage, "\n", " | ", lBuff);
            replaceSubStr(aMessage, "\t", "  ", lBuff);
            strcat(str, lBuff);
        }
        strcat(str, "\"");
    }
    else
    {
        return tinyfd_messageBox(aTitle, aMessage, "ok", aIconType, 0);
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);

    if (!(lIn = popen(str, "r")))
    {
        free(str);
        return 0;
    }

    pclose(lIn);
    free(str);
    return 1;
}

/* returns NULL on cancel */
char const *tinyfd_inputBox(
    char const *const aTitle,        /* NULL or "" */
    char const *const aMessage,      /* NULL or "" may NOT contain \n nor \t */
    char const *const aDefaultInput) /* "" , if NULL it's a passwordBox */
{
    static char lBuff[MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char *str = NULL;
    char *lpDialogString;
    FILE *lIn;
    int lResult;
    int lWasGdialog = 0;
    int lWasGraphicDialog = 0;
    int lWasXterm = 0;
    int lWasBasicXterm = 0;
    struct termios oldt;
    struct termios newt;
    char *lEOF;
    size_t lTitleLen;
    size_t lMessageLen;

    lBuff[0] = '\0';

    lTitleLen = aTitle ? strlen(aTitle) : 0;
    lMessageLen = aMessage ? strlen(aMessage) : 0;
    if (!aTitle || !lQuery)
    {
        str = (char *)malloc(MAX_PATH_OR_CMD + lTitleLen + lMessageLen);
    }

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return (char const *)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'display dialog \"");
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" ");
        strcat(str, "default answer \"");
        if (aDefaultInput && strlen(aDefaultInput))
        {
            strcat(str, aDefaultInput);
        }
        strcat(str, "\" ");
        if (!aDefaultInput)
        {
            strcat(str, "hidden answer true ");
        }
        if (aTitle && strlen(aTitle))
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
            return (char const *)1;
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
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" \"");
        if (aDefaultInput && strlen(aDefaultInput))
        {
            strcat(str, aDefaultInput);
        }
        strcat(str, "\"");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        strcat(str,
               ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
    }
    else if (zenityPresent() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        if (zenityPresent())
        {
            if (lQuery)
            {
                response("zenity");
                return (char const *)1;
            }
            strcpy(str, "szAnswer=$(zenity");
            if ((zenity3Present() >= 4) && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return (char const *)1;
            }
            strcpy(str, "szAnswer=$(matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return (char const *)1;
            }
            strcpy(str, "szAnswer=$(shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return (char const *)1;
            }
            strcpy(str, "szAnswer=$(qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --entry");

        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (aMessage && strlen(aMessage))
        {
            strcat(str, " --text=\"");
            strcat(str, aMessage);
            strcat(str, "\"");
        }
        if (aDefaultInput && strlen(aDefaultInput))
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
        strcat(str,
               ");if [ $? = 0 ];then echo 1$szAnswer;else echo 0$szAnswer;fi");
    }
    else if (gxmessagePresent() || gmessagePresent())
    {
        if (gxmessagePresent())
        {
            if (lQuery)
            {
                response("gxmessage");
                return (char const *)1;
            }
            strcpy(str, "szAnswer=$(gxmessage -buttons Ok:1,Cancel:0 -center \"");
        }
        else
        {
            if (lQuery)
            {
                response("gmessage");
                return (char const *)1;
            }
            strcpy(str, "szAnswer=$(gmessage -buttons Ok:1,Cancel:0 -center \"");
        }

        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\"");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, " -title  \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        strcat(str, " -entrytext \"");
        if (aDefaultInput && strlen(aDefaultInput))
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
            return (char const *)1;
        }
        strcpy(str, gPython2Name);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }

        strcat(str,
               " -S -c \"import Tkinter,tkSimpleDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "res=tkSimpleDialog.askstring(");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aMessage && strlen(aMessage))
        {

            strcat(str, "prompt='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "',");
        }
        if (aDefaultInput)
        {
            if (strlen(aDefaultInput))
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
            return (char const *)1;
        }
        strcpy(str, gPython3Name);
        strcat(str,
               " -S -c \"import tkinter; from tkinter import simpledialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "res=simpledialog.askstring(");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aMessage && strlen(aMessage))
        {

            strcat(str, "prompt='");
            lpDialogString = str + strlen(str);
            replaceSubStr(aMessage, "\n", "\\n", lpDialogString);
            strcat(str, "',");
        }
        if (aDefaultInput)
        {
            if (strlen(aDefaultInput))
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
                return (char const *)1;
            }
            lWasGraphicDialog = 1;
            lWasGdialog = 1;
            strcpy(str, "(gdialog ");
        }
        else if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return (char const *)1;
            }
            lWasGraphicDialog = 1;
            strcpy(str, "(Xdialog ");
        }
        else if (dialogName())
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            if (isTerminalRunning())
            {
                strcpy(str, "(dialog ");
            }
            else
            {
                lWasXterm = 1;
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
                return (char const *)0;
            }
            strcpy(str, "(whiptail ");
        }
        else
        {
            if (lQuery)
            {
                response("whiptail");
                return (char const *)0;
            }
            lWasXterm = 1;
            strcpy(str, terminalName());
            strcat(str, "'(whiptail ");
        }

        if (aTitle && strlen(aTitle))
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
        if (aMessage && strlen(aMessage))
        {
            strcat(str, aMessage);
        }
        strcat(str, "\" 10 60 ");
        if (aDefaultInput && strlen(aDefaultInput))
        {
            strcat(str, "\"");
            strcat(str, aDefaultInput);
            strcat(str, "\" ");
        }
        if (lWasGraphicDialog)
        {
            strcat(str, ") 2>/tmp/tinyfd.txt;\
        if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
        tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");
        }
        else
        {
            strcat(str, ">/dev/tty ) 2>/tmp/tinyfd.txt;\
        if [ $? = 0 ];then tinyfdBool=1;else tinyfdBool=0;fi;\
        tinyfdRes=$(cat /tmp/tinyfd.txt);echo $tinyfdBool$tinyfdRes");

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
            return (char const *)0;
        }
        lWasBasicXterm = 1;
        strcpy(str, terminalName());
        strcat(str, "'");
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = 1;
            tinyfd_messageBox(gTitle, tinyfd_needs, "ok", "warning", 0);
        }
        if (aTitle && strlen(aTitle) && !tinyfd_forceConsole)
        {
            strcat(str, "echo \"");
            strcat(str, aTitle);
            strcat(str, "\";echo;");
        }

        strcat(str, "echo \"");
        if (aMessage && strlen(aMessage))
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
        gWarningDisplayed = 1;
        tinyfd_messageBox(gTitle, tinyfd_needs, "ok", "warning", 0);
        if (lQuery)
        {
            response("no_solution");
            return (char const *)0;
        }
        return NULL;
    }
    else
    {
        if (lQuery)
        {
            response("basicinput");
            return (char const *)0;
        }
        if (!gWarningDisplayed && !tinyfd_forceConsole)
        {
            gWarningDisplayed = 1;
            tinyfd_messageBox(gTitle, tinyfd_needs, "ok", "warning", 0);
        }
        if (aTitle && strlen(aTitle))
        {
            printf("\n%s\n", aTitle);
        }
        if (aMessage && strlen(aMessage))
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

        lEOF = fgets(lBuff, MAX_PATH_OR_CMD, stdin);
        /* printf("lbuff<%c><%d>\n",lBuff[0],lBuff[0]); */
        if (!lEOF || (lBuff[0] == '\0'))
        {
            free(str);
            return NULL;
        }

        if (lBuff[0] == '\n')
        {
            lEOF = fgets(lBuff, MAX_PATH_OR_CMD, stdin);
            /* printf("lbuff<%c><%d>\n",lBuff[0],lBuff[0]); */
            if (!lEOF || (lBuff[0] == '\0'))
            {
                free(str);
                return NULL;
            }
        }

        if (!aDefaultInput)
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            printf("\n");
        }
        printf("\n");
        if (strchr(lBuff, 27))
        {
            free(str);
            return NULL;
        }
        if (lBuff[strlen(lBuff) - 1] == '\n')
        {
            lBuff[strlen(lBuff) - 1] = '\0';
        }
        free(str);
        return lBuff;
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
        return NULL;
    }
    while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
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

    /* printf( "len Buff: %lu\n" , strlen(lBuff) ) ; */
    /* printf( "lBuff0: %s\n" , lBuff ) ; */
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }
    /* printf( "lBuff1: %s len: %lu \n" , lBuff , strlen(lBuff) ) ; */
    if (lWasBasicXterm)
    {
        if (strstr(lBuff, "^[")) /* esc was pressed */
        {
            free(str);
            return NULL;
        }
    }

    lResult = strncmp(lBuff, "1", 1) ? 0 : 1;
    /* printf( "lResult: %d \n" , lResult ) ; */
    if (!lResult)
    {
        free(str);
        return NULL;
    }
    /* printf( "lBuff+1: %s\n" , lBuff+1 ) ; */
    free(str);

    return lBuff + 1;
}

char const *tinyfd_saveFileDialog(
    char const *const aTitle,                   /* NULL or "" */
    char const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,             /* 0 */
    char const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    char const *const aSingleFilterDescription) /* NULL or "image files" */
{
    static char lBuff[MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char str[MAX_PATH_OR_CMD];
    char lString[MAX_PATH_OR_CMD];
    int i;
    int lWasGraphicDialog = 0;
    int lWasXterm = 0;
    char const *p;
    FILE *lIn;
    lBuff[0] = '\0';

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return (char const *)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"Finder\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'POSIX path of ( choose file name ");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "with prompt \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
        if (strlen(lString))
        {
            strcat(str, "default location \"");
            strcat(str, lString);
            strcat(str, "\" ");
        }
        getLastName(lString, aDefaultPathAndFile);
        if (strlen(lString))
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
            return (char const *)1;
        }

        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        strcat(str, " --getsavefilename ");

        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
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

        if (aNumOfFilterPatterns > 0)
        {
            int pattern; // otherwise MIME-type filter
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                pattern = strchr(aFilterPatterns[i], '*') != 0;
                if (pattern)
                    break;
            }
            strcat(str, " \"");
            if (pattern)
            {
                if (aSingleFilterDescription && strlen(aSingleFilterDescription))
                {
                    strcat(str, aSingleFilterDescription);
                }
                strcat(str, "(");
            }
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                if (i != 0)
                    strcat(str, " ");
                strcat(str, aFilterPatterns[i]);
            }
            if (pattern)
            {
                strcat(str, ")");
            }
            strcat(str, "\"");
        }
        if (aTitle && strlen(aTitle))
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
                return (char const *)1;
            }
            strcpy(str, "zenity");
            if ((zenity3Present() >= 4) && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return (char const *)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return (char const *)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return (char const *)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --file-selection --save --confirm-overwrite");

        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
        {
            strcat(str, " --filename=\"");
            strcat(str, aDefaultPathAndFile);
            strcat(str, "\"");
        }
        if (aNumOfFilterPatterns > 0)
        {
            strcat(str, " --file-filter='");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription))
            {
                strcat(str, aSingleFilterDescription);
                strcat(str, " | ");
            }
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, aFilterPatterns[i]);
                strcat(str, " ");
            }
            strcat(str, "' --file-filter='All files | *'");
        }
        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
    }
    else if (!xdialogPresent() && tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return (char const *)1;
        }
        strcpy(str, gPython2Name);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }
        strcat(str,
               " -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set\
 frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "print tkFileDialog.asksaveasfilename(");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }
        if ((aNumOfFilterPatterns > 1) || ((aNumOfFilterPatterns == 1) /* test because poor osx behaviour */
                                           && (aFilterPatterns[0][strlen(aFilterPatterns[0]) - 1] != '*')))
        {
            strcat(str, "filetypes=(");
            strcat(str, "('");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription))
            {
                strcat(str, aSingleFilterDescription);
            }
            strcat(str, "',(");
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, "'");
                strcat(str, aFilterPatterns[i]);
                strcat(str, "',");
            }
            strcat(str, ")),");
            strcat(str, "('All files','*'))");
        }
        strcat(str, ")\"");
    }
    else if (!xdialogPresent() && tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return (char const *)1;
        }
        strcpy(str, gPython3Name);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "print( filedialog.asksaveasfilename(");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }
        if ((aNumOfFilterPatterns > 1) || ((aNumOfFilterPatterns == 1) /* test because poor osx behaviour */
                                           && (aFilterPatterns[0][strlen(aFilterPatterns[0]) - 1] != '*')))
        {
            strcat(str, "filetypes=(");
            strcat(str, "('");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription))
            {
                strcat(str, aSingleFilterDescription);
            }
            strcat(str, "',(");
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, "'");
                strcat(str, aFilterPatterns[i]);
                strcat(str, "',");
            }
            strcat(str, ")),");
            strcat(str, "('All files','*'))");
        }
        strcat(str, "))\"");
    }
    else if (xdialogPresent() || dialogName())
    {
        if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return (char const *)1;
            }
            lWasGraphicDialog = 1;
            strcpy(str, "(Xdialog ");
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            strcpy(str, "(dialog ");
        }
        else
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            lWasXterm = 1;
            strcpy(str, terminalName());
            strcat(str, "'(");
            strcat(str, dialogName());
            strcat(str, " ");
        }

        if (aTitle && strlen(aTitle))
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
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
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
            return tinyfd_inputBox(aTitle, NULL, NULL);
        }
        p = tinyfd_inputBox(aTitle, "Save file", "");
        getPathWithoutFinalSlash(lString, p);
        if (strlen(lString) && !dirExists(lString))
        {
            return NULL;
        }
        getLastName(lString, p);
        if (!strlen(lString))
        {
            return NULL;
        }
        return p;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    if (!(lIn = popen(str, "r")))
    {
        return NULL;
    }
    while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
    {
    }
    pclose(lIn);
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    if (!strlen(lBuff))
    {
        return NULL;
    }
    getPathWithoutFinalSlash(lString, lBuff);
    if (strlen(lString) && !dirExists(lString))
    {
        return NULL;
    }
    getLastName(lString, lBuff);
    if (!filenameValid(lString))
    {
        return NULL;
    }
    return lBuff;
}

/* in case of multiple files, the separator is | */
char const *tinyfd_openFileDialog(
    char const *const aTitle,                   /* NULL or "" */
    char const *const aDefaultPathAndFile,      /* NULL or "" */
    int const aNumOfFilterPatterns,             /* 0 */
    char const *const *const aFilterPatterns,   /* NULL or {"*.jpg","*.png"} */
    char const *const aSingleFilterDescription, /* NULL or "image files" */
    int const aAllowMultipleSelects)            /* 0 or 1 */
{
    static char lBuff[MAX_MULTIPLE_FILES * MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char str[MAX_PATH_OR_CMD];
    char lString[MAX_PATH_OR_CMD];
    int i;
    FILE *lIn;
    char *p;
    char const *p2;
    int lWasKdialog = 0;
    int lWasGraphicDialog = 0;
    int lWasXterm = 0;
    lBuff[0] = '\0';

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return (char const *)1;
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
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "with prompt \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
        if (strlen(lString))
        {
            strcat(str, "default location \"");
            strcat(str, lString);
            strcat(str, "\" ");
        }
        if (aNumOfFilterPatterns > 0)
        {
            strcat(str, "of type {\"");
            strcat(str, aFilterPatterns[0] + 2);
            strcat(str, "\"");
            for (i = 1; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, ",\"");
                strcat(str, aFilterPatterns[i] + 2);
                strcat(str, "\"");
            }
            strcat(str, "} ");
        }
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
            return (char const *)1;
        }
        lWasKdialog = 1;

        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        strcat(str, " --getopenfilename ");

        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
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

        if (aNumOfFilterPatterns > 0)
        {
            int pattern; // otherwise MIME-type filter
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                pattern = strchr(aFilterPatterns[i], '*') != 0;
                if (pattern)
                    break;
            }
            strcat(str, " \"");
            if (pattern)
            {
                if (aSingleFilterDescription && strlen(aSingleFilterDescription))
                {
                    strcat(str, aSingleFilterDescription);
                }
                strcat(str, "(");
            }
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                if (i != 0)
                    strcat(str, " ");
                strcat(str, aFilterPatterns[i]);
            }
            if (pattern)
            {
                strcat(str, ")");
            }
            strcat(str, "\"");
        }
        if (aAllowMultipleSelects)
        {
            strcat(str, " --multiple --separate-output");
        }
        if (aTitle && strlen(aTitle))
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
                return (char const *)1;
            }
            strcpy(str, "zenity");
            if ((zenity3Present() >= 4) && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return (char const *)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return (char const *)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return (char const *)1;
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
        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
        {
            strcat(str, " --filename=\"");
            strcat(str, aDefaultPathAndFile);
            strcat(str, "\"");
        }
        if (aNumOfFilterPatterns > 0)
        {
            strcat(str, " --file-filter='");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription))
            {
                strcat(str, aSingleFilterDescription);
                strcat(str, " | ");
            }
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, aFilterPatterns[i]);
                strcat(str, " ");
            }
            strcat(str, "' --file-filter='All files | *'");
        }
        if (tinyfd_silent)
            strcat(str, " 2>/dev/null ");
    }
    else if (tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return (char const *)1;
        }
        strcpy(str, gPython2Name);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }
        strcat(str,
               " -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
        }
        strcat(str, "lFiles=tkFileDialog.askopenfilename(");
        if (aAllowMultipleSelects)
        {
            strcat(str, "multiple=1,");
        }
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }
        if ((aNumOfFilterPatterns > 1) || ((aNumOfFilterPatterns == 1) /*test because poor osx behaviour*/
                                           && (aFilterPatterns[0][strlen(aFilterPatterns[0]) - 1] != '*')))
        {
            strcat(str, "filetypes=(");
            strcat(str, "('");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription))
            {
                strcat(str, aSingleFilterDescription);
            }
            strcat(str, "',(");
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, "'");
                strcat(str, aFilterPatterns[i]);
                strcat(str, "',");
            }
            strcat(str, ")),");
            strcat(str, "('All files','*'))");
        }
        strcat(str, ");\
\nif not isinstance(lFiles, tuple):\n\tprint lFiles\nelse:\
\n\tlFilesString=''\n\tfor lFile in lFiles:\n\t\tlFilesString+=str(lFile)+'|'\
\n\tprint lFilesString[:-1]\n\"");
    }
    else if (tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return (char const *)1;
        }
        strcpy(str, gPython3Name);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "lFiles=filedialog.askopenfilename(");
        if (aAllowMultipleSelects)
        {
            strcat(str, "multiple=1,");
        }
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
        {
            getPathWithoutFinalSlash(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialdir='");
                strcat(str, lString);
                strcat(str, "',");
            }
            getLastName(lString, aDefaultPathAndFile);
            if (strlen(lString))
            {
                strcat(str, "initialfile='");
                strcat(str, lString);
                strcat(str, "',");
            }
        }
        if ((aNumOfFilterPatterns > 1) || ((aNumOfFilterPatterns == 1) /*test because poor osx behaviour*/
                                           && (aFilterPatterns[0][strlen(aFilterPatterns[0]) - 1] != '*')))
        {
            strcat(str, "filetypes=(");
            strcat(str, "('");
            if (aSingleFilterDescription && strlen(aSingleFilterDescription))
            {
                strcat(str, aSingleFilterDescription);
            }
            strcat(str, "',(");
            for (i = 0; i < aNumOfFilterPatterns; i++)
            {
                strcat(str, "'");
                strcat(str, aFilterPatterns[i]);
                strcat(str, "',");
            }
            strcat(str, ")),");
            strcat(str, "('All files','*'))");
        }
        strcat(str, ");\
\nif not isinstance(lFiles, tuple):\n\tprint(lFiles)\nelse:\
\n\tlFilesString=''\n\tfor lFile in lFiles:\n\t\tlFilesString+=str(lFile)+'|'\
\n\tprint(lFilesString[:-1])\n\"");
    }
    else if (xdialogPresent() || dialogName())
    {
        if (xdialogPresent())
        {
            if (lQuery)
            {
                response("xdialog");
                return (char const *)1;
            }
            lWasGraphicDialog = 1;
            strcpy(str, "(Xdialog ");
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            strcpy(str, "(dialog ");
        }
        else
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            lWasXterm = 1;
            strcpy(str, terminalName());
            strcat(str, "'(");
            strcat(str, dialogName());
            strcat(str, " ");
        }

        if (aTitle && strlen(aTitle))
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
        if (aDefaultPathAndFile && strlen(aDefaultPathAndFile))
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
            return tinyfd_inputBox(aTitle, NULL, NULL);
        }
        p2 = tinyfd_inputBox(aTitle, "Open file", "");
        if (!fileExists(p2))
        {
            return NULL;
        }
        return p2;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    if (!(lIn = popen(str, "r")))
    {
        return NULL;
    }
    lBuff[0] = '\0';
    p = lBuff;
    while (fgets(p, sizeof(lBuff), lIn) != NULL)
    {
        p += strlen(p);
    }
    pclose(lIn);
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    if (lWasKdialog && aAllowMultipleSelects)
    {
        p = lBuff;
        while ((p = strchr(p, '\n')))
            *p = '|';
    }
    /* printf( "lBuff2: %s\n" , lBuff ) ; */
    if (!strlen(lBuff))
    {
        return NULL;
    }
    if (aAllowMultipleSelects && strchr(lBuff, '|'))
    {
        p2 = ensureFilesExist(lBuff, lBuff);
    }
    else if (fileExists(lBuff))
    {
        p2 = lBuff;
    }
    else
    {
        return NULL;
    }
    /* printf( "lBuff3: %s\n" , p2 ) ; */

    return p2;
}

char const *tinyfd_selectFolderDialog(
    char const *const aTitle,       /* "" */
    char const *const aDefaultPath) /* "" */
{
    static char lBuff[MAX_PATH_OR_CMD];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char str[MAX_PATH_OR_CMD];
    FILE *lIn;
    char const *p;
    int lWasGraphicDialog = 0;
    int lWasXterm = 0;
    lBuff[0] = '\0';

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return (char const *)1;
        }
        strcpy(str, "osascript ");
        if (!osx9orBetter())
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
        strcat(str, " -e 'try' -e 'POSIX path of ( choose folder ");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "with prompt \"");
            strcat(str, aTitle);
            strcat(str, "\" ");
        }
        if (aDefaultPath && strlen(aDefaultPath))
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
            return (char const *)1;
        }
        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        strcat(str, " --getexistingdirectory ");

        if (aDefaultPath && strlen(aDefaultPath))
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

        if (aTitle && strlen(aTitle))
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
                return (char const *)1;
            }
            strcpy(str, "zenity");
            if ((zenity3Present() >= 4) && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return (char const *)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return (char const *)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return (char const *)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --file-selection --directory");

        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title=\"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
        if (aDefaultPath && strlen(aDefaultPath))
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
            return (char const *)1;
        }
        strcpy(str, gPython2Name);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }
        strcat(str,
               " -S -c \"import Tkinter,tkFileDialog;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''/usr/bin/osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "print tkFileDialog.askdirectory(");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aDefaultPath && strlen(aDefaultPath))
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
            return (char const *)1;
        }
        strcpy(str, gPython3Name);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import filedialog;root=tkinter.Tk();root.withdraw();");
        strcat(str, "print( filedialog.askdirectory(");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, "title='");
            strcat(str, aTitle);
            strcat(str, "',");
        }
        if (aDefaultPath && strlen(aDefaultPath))
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
                return (char const *)1;
            }
            lWasGraphicDialog = 1;
            strcpy(str, "(Xdialog ");
        }
        else if (isTerminalRunning())
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            strcpy(str, "(dialog ");
        }
        else
        {
            if (lQuery)
            {
                response("dialog");
                return (char const *)0;
            }
            lWasXterm = 1;
            strcpy(str, terminalName());
            strcat(str, "'(");
            strcat(str, dialogName());
            strcat(str, " ");
        }

        if (aTitle && strlen(aTitle))
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
        if (aDefaultPath && strlen(aDefaultPath))
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
            return tinyfd_inputBox(aTitle, NULL, NULL);
        }
        p = tinyfd_inputBox(aTitle, "Select folder", "");
        if (!p || !strlen(p) || !dirExists(p))
        {
            return NULL;
        }
        return p;
    }
    if (tinyfd_verbose)
        printf("str: %s\n", str);
    if (!(lIn = popen(str, "r")))
    {
        return NULL;
    }
    while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
    {
    }
    pclose(lIn);
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    if (!strlen(lBuff) || !dirExists(lBuff))
    {
        return NULL;
    }
    return lBuff;
}

/* returns the hexcolor as a string "#FF0000" */
/* aoResultRGB also contains the result */
/* aDefaultRGB is used only if aDefaultHexRGB is NULL */
/* aDefaultRGB and aoResultRGB can be the same array */
char const *tinyfd_colorChooser(
    char const *const aTitle,           /* NULL or "" */
    char const *const aDefaultHexRGB,   /* NULL or "#FF0000"*/
    unsigned char const aDefaultRGB[3], /* { 0 , 255 , 255 } */
    unsigned char aoResultRGB[3])       /* { 0 , 0 , 0 } */
{
    static char lBuff[128];
    int lQuery = aTitle && !strcmp(aTitle, "tinyfd_query");
    char lTmp[128];
    char str[MAX_PATH_OR_CMD];
    char lDefaultHexRGB[8];
    char *lpDefaultHexRGB;
    unsigned char lDefaultRGB[3];
    char const *p;
    FILE *lIn;
    int i;
    int lWasZenity3 = 0;
    int lWasOsascript = 0;
    int lWasXdialog = 0;
    lBuff[0] = '\0';

    if (aDefaultHexRGB)
    {
        Hex2RGB(aDefaultHexRGB, lDefaultRGB);
        lpDefaultHexRGB = (char *)aDefaultHexRGB;
    }
    else
    {
        lDefaultRGB[0] = aDefaultRGB[0];
        lDefaultRGB[1] = aDefaultRGB[1];
        lDefaultRGB[2] = aDefaultRGB[2];
        RGB2Hex(aDefaultRGB, lDefaultHexRGB);
        lpDefaultHexRGB = (char *)lDefaultHexRGB;
    }

    if (osascriptPresent())
    {
        if (lQuery)
        {
            response("applescript");
            return (char const *)1;
        }
        lWasOsascript = 1;
        strcpy(str, "osascript");

        if (!osx9orBetter())
        {
            strcat(str, " -e 'tell application \"System Events\"' -e 'Activate'");
            strcat(str, " -e 'try' -e 'set mycolor to choose color default color {");
        }
        else
        {
            strcat(str,
                   " -e 'try' -e 'tell app (path to frontmost application as Unicode text) \
to set mycolor to choose color default color {");
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
            return (char const *)1;
        }
        strcpy(str, "kdialog");
        if (kdialogPresent() == 2)
        {
            strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
        }
        sprintf(str + strlen(str), " --getcolor --default '%s'", lpDefaultHexRGB);

        if (aTitle && strlen(aTitle))
        {
            strcat(str, " --title \"");
            strcat(str, aTitle);
            strcat(str, "\"");
        }
    }
    else if (zenity3Present() || matedialogPresent() || shellementaryPresent() || qarmaPresent())
    {
        lWasZenity3 = 1;
        if (zenity3Present())
        {
            if (lQuery)
            {
                response("zenity3");
                return (char const *)1;
            }
            strcpy(str, "zenity");
            if ((zenity3Present() >= 4) && !getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(sleep .01;xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        else if (matedialogPresent())
        {
            if (lQuery)
            {
                response("matedialog");
                return (char const *)1;
            }
            strcpy(str, "matedialog");
        }
        else if (shellementaryPresent())
        {
            if (lQuery)
            {
                response("shellementary");
                return (char const *)1;
            }
            strcpy(str, "shellementary");
        }
        else
        {
            if (lQuery)
            {
                response("qarma");
                return (char const *)1;
            }
            strcpy(str, "qarma");
            if (!getenv("SSH_TTY"))
            {
                strcat(str, " --attach=$(xprop -root 32x '\t$0' _NET_ACTIVE_WINDOW | cut -f 2)"); /* contribution: Paul Rouget */
            }
        }
        strcat(str, " --color-selection --show-palette");
        sprintf(str + strlen(str), " --color=%s", lpDefaultHexRGB);

        if (aTitle && strlen(aTitle))
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
            return (char const *)1;
        }
        lWasXdialog = 1;
        strcpy(str, "Xdialog --colorsel \"");
        if (aTitle && strlen(aTitle))
        {
            strcat(str, aTitle);
        }
        strcat(str, "\" 0 60 ");
        sprintf(lTmp, "%hhu %hhu %hhu", lDefaultRGB[0],
                lDefaultRGB[1], lDefaultRGB[2]);
        strcat(str, lTmp);
        strcat(str, " 2>&1");
    }
    else if (tkinter2Present())
    {
        if (lQuery)
        {
            response("python2-tkinter");
            return (char const *)1;
        }
        strcpy(str, gPython2Name);
        if (!isTerminalRunning() && isDarwin())
        {
            strcat(str, " -i"); /* for osx without console */
        }

        strcat(str,
               " -S -c \"import Tkinter,tkColorChooser;root=Tkinter.Tk();root.withdraw();");

        if (isDarwin())
        {
            strcat(str,
                   "import os;os.system('''osascript -e 'tell app \\\"Finder\\\" to set \
frontmost of process \\\"Python\\\" to true' ''');");
        }

        strcat(str, "res=tkColorChooser.askcolor(color='");
        strcat(str, lpDefaultHexRGB);
        strcat(str, "'");

        if (aTitle && strlen(aTitle))
        {
            strcat(str, ",title='");
            strcat(str, aTitle);
            strcat(str, "'");
        }
        strcat(str, ");\
\nif res[1] is not None:\n\tprint res[1]\"");
    }
    else if (tkinter3Present())
    {
        if (lQuery)
        {
            response("python3-tkinter");
            return (char const *)1;
        }
        strcpy(str, gPython3Name);
        strcat(str,
               " -S -c \"import tkinter;from tkinter import colorchooser;root=tkinter.Tk();root.withdraw();");
        strcat(str, "res=colorchooser.askcolor(color='");
        strcat(str, lpDefaultHexRGB);
        strcat(str, "'");

        if (aTitle && strlen(aTitle))
        {
            strcat(str, ",title='");
            strcat(str, aTitle);
            strcat(str, "'");
        }
        strcat(str, ");\
\nif res[1] is not None:\n\tprint(res[1])\"");
    }
    else
    {
        if (lQuery)
        {
            return tinyfd_inputBox(aTitle, NULL, NULL);
        }
        p = tinyfd_inputBox(aTitle,
                            "Enter hex rgb color (i.e. #f5ca20)", lpDefaultHexRGB);
        if (!p || (strlen(p) != 7) || (p[0] != '#'))
        {
            return NULL;
        }
        for (i = 1; i < 7; i++)
        {
            if (!isxdigit(p[i]))
            {
                return NULL;
            }
        }
        Hex2RGB(p, aoResultRGB);
        return p;
    }

    if (tinyfd_verbose)
        printf("str: %s\n", str);
    if (!(lIn = popen(str, "r")))
    {
        return NULL;
    }
    while (fgets(lBuff, sizeof(lBuff), lIn) != NULL)
    {
    }
    pclose(lIn);
    if (!strlen(lBuff))
    {
        return NULL;
    }
    /* printf( "len Buff: %lu\n" , strlen(lBuff) ) ; */
    /* printf( "lBuff0: %s\n" , lBuff ) ; */
    if (lBuff[strlen(lBuff) - 1] == '\n')
    {
        lBuff[strlen(lBuff) - 1] = '\0';
    }

    if (lWasZenity3)
    {
        if (lBuff[0] == '#')
        {
            if (strlen(lBuff) > 7)
            {
                lBuff[3] = lBuff[5];
                lBuff[4] = lBuff[6];
                lBuff[5] = lBuff[9];
                lBuff[6] = lBuff[10];
                lBuff[7] = '\0';
            }
            Hex2RGB(lBuff, aoResultRGB);
        }
        else if (lBuff[3] == '(')
        {
            sscanf(lBuff, "rgb(%hhu,%hhu,%hhu",
                   &aoResultRGB[0], &aoResultRGB[1], &aoResultRGB[2]);
            RGB2Hex(aoResultRGB, lBuff);
        }
        else if (lBuff[4] == '(')
        {
            sscanf(lBuff, "rgba(%hhu,%hhu,%hhu",
                   &aoResultRGB[0], &aoResultRGB[1], &aoResultRGB[2]);
            RGB2Hex(aoResultRGB, lBuff);
        }
    }
    else if (lWasOsascript || lWasXdialog)
    {
        /* printf( "lBuff: %s\n" , lBuff ) ; */
        sscanf(lBuff, "%hhu %hhu %hhu",
               &aoResultRGB[0], &aoResultRGB[1], &aoResultRGB[2]);
        RGB2Hex(aoResultRGB, lBuff);
    }
    else
    {
        Hex2RGB(lBuff, aoResultRGB);
    }
    /* printf("%d %d %d\n", aoResultRGB[0],aoResultRGB[1],aoResultRGB[2]); */
    /* printf( "lBuff: %s\n" , lBuff ) ; */
    return lBuff;
}
#endif /* _WIN32 */

#ifdef _MSC_VER
#pragma warning(default : 4996)
#pragma warning(default : 4100)
#pragma warning(default : 4706)
#endif
