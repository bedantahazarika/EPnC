//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <EPnC_Gui.h>



/// @brief The EPnC app: drop a .csv file (columns t, x, y) and watch the equation being discovered. All the work lives in `EPnC_Gui.h`; run with `-h` for the command line switches.
int main (int argc, char** argv)
{
    #ifdef WITH_GUI
        return GuiRun(argc, argv);
    #else
        printf("\n[MainApp] Built without the GUI. Rebuild with -DWITH_GUI=ON (needs raylib).\n");
        return 0;
    #endif
}
