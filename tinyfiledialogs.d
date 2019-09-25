/*________
 /         \ This is a custom D port of tinyfiledialogs v3.3.9 [Apr 14, 2019]
 |tiny file| http://tinyfiledialogs.sourceforge.net
 | dialogs | Copyright (c) 2014-2019 Guillaume Vareille
 \____  ___/ Copyright (c) 2019 dayllenger
      \|

Native dialog library for Windows, macOS, GTK+, Qt, console & more.
SSH supported via automatic switch to console mode or X11 forwarding.

8 functions:
- beep
- notify popup (tray)
- message & question
- input & password
- save file
- open file(s)
- select folder
- color picker

Each function is documented with examples.
The dialogs can be forced into console mode.

Windows XP to 10:
- native code & vbs create the graphic dialogs
- enhanced console mode can use dialog.exe from
http://andrear.altervista.org/home/cdialog.php
- basic console input

Unix (using command line calls):
- applescript, kdialog, zenity
- python (2 or 3) + tkinter + python-dbus (optional)
- dialog (opens a console if needed)
- basic console input
The same executable can run across desktops & distributions.

- Notes -



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
module tinyfiledialogs;

extern (C) nothrow @nogc:

alias c_str = const(char*);

/// No params, no return value, just beep
void tinyfd_beep();

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
int tinyfd_notifyPopup(c_str title, c_str message, c_str iconType);

/**	Params:
        title = C-string or null
        message = C-string or null, can be multiline
        dialogType = "ok" "okcancel" "yesno" "yesnocancel"
        iconType = "info" "warning" "error" "question"
        defaultButton = 0 for cancel/no , 1 for ok/yes , 2 for no in yesnocancel

    Returns:
        0 for cancel/no, 1 for ok/yes, 2 for no in yesnocancel.

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
);

/** Params:
        title = C-string or null
        message = C-string or null, may NOT contain \ n or \ t on windows
        defaultInput = C-string, if null it's a password box

    Returns:
        Entered text, `null` on cancel.

    Example:
    ---
    c_str passwd = tinyfd_inputBox("a password box",
        "your password will be revealed", null);
    if (passwd) printf("your password is: %s\ n", passwd);
    ---
*/
c_str tinyfd_inputBox(c_str title, c_str message, c_str defaultInput);

/** Params:
        title = C-string or null
        defaultPathAndFile = C-string or null
        numOfFilterPatterns = 0
        filterPatterns = `["*.jpg", "*.png"]`, or (sometimes) MIME type `["audio/mp3"]`, or null
        singleFilterDescription = "Text files" or null

    Returns:
        Selected file name, `null` on cancel.

    Example:
    ---
    const c_str[] patterns = ["application/x-dsrc"];
    c_str filename = tinyfd_saveFileDialog(
        "Save D source file", "mod.d",
        cast(int)patterns.length, patterns.ptr, null);
    ---
*/
c_str tinyfd_saveFileDialog(
    c_str title,
    c_str defaultPathAndFile,
    int numOfFilterPatterns,
    c_str* filterPatterns,
    c_str singleFilterDescription,
);

/** Params:
        title = C-string or null
        defaultPathAndFile = C-string or null
        numOfFilterPatterns = 0
        filterPatterns = `["*.jpg", "*.png"]`, or (sometimes) MIME type `["audio/mp3"]`, or null
        singleFilterDescription = "Image files" or null
        allowMultipleSelects = 0 or 1

    Returns:
        Selected file name or `null` on cancel. In case of multiple files, the separator is |.

    Example:
    ---
    const c_str[] patterns = ["*.cpp *.cc *.C *.cxx *.c++"];
    c_str filename = tinyfd_openFileDialog(
        "Open a C++ File", null,
        cast(int)patterns.length, patterns.ptr,
        "C++ source code", 0);
    ---
*/
c_str tinyfd_openFileDialog(
    c_str title,
    c_str defaultPathAndFile,
    int numOfFilterPatterns,
    c_str* filterPatterns,
    c_str singleFilterDescription,
    int allowMultipleSelects,
);

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
    else
        tinyfd_messageBox("Error", "Folder name is null", "ok", "error", 1);
    ---
*/
c_str tinyfd_selectFolderDialog(c_str title, c_str defaultPath);

/** Params:
        title = C-string or null
        defaultHexRGB = default color, C-string or null
        defaultRGB = used only if the previous parameter is null
        resultRGB = also contains the result

    Returns:
        The hexcolor as a string like "#FF0000" or `null` on cancel.

    Example:
    ---
    ubyte[3] rgbColor;
    c_str hexColor = tinyfd_colorChooser("Choose a nice color", "#FF0077",
        rgbColor, rgbColor);
    if (hexColor)
        tinyfd_messageBox("The selected hexcolor is", hexColor, "ok", "info", 1);
    else
        tinyfd_messageBox("Error", "hexcolor is null", "ok", "error", 1);
    ---
*/
c_str tinyfd_colorChooser(
    c_str title,
    c_str defaultHexRGB,
    ref const ubyte[3] defaultRGB,
    ref ubyte[3] resultRGB,
);

/**************************** IMPLEMENTATION ****************************/



