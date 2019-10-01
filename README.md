## tinyfiledialogs

Native dialog library for Windows, macOS, GTK+, Qt, console & more.
SSH supported via automatic switch to console mode or X11 forwarding.

Originally pure C89. To build in betterC mode, enable `BC` subconfiguration.

For examples and info take a look at [the documentation](https://tinyfiledialogs.dpldocs.info/tinyfiledialogs.html).

Short example of how to open a save dialog and show the name of chosen file in a message box:
```D
import tinyfiledialogs;

// pairs of pattern list and optional description
const TFD_Filter[] filters = [
    { ["*.png"           ],   "PNG image" },
    { ["*.jpg", "*.jpeg" ],  "JPEG image" },
    { ["*.tif", "*.tiff" ],  "TIFF image" },
    { ["*.tga"           ], "TarGA image" },
];
// it blocks until the dialog is closed
const char* filename = tinyfd_saveFileDialog("Save as...", "Untitled.png", filters);
if (filename)
{
    // now we can save our file physically
    tinyfd_messageBox("The filename is", filename, "ok", "info", 1);
}
```

**NOTE: the main purpose of this package is to implement native file and color dialogs in [beamui](https://github.com/dayllenger/beamui), so don't expect much support and development.**
