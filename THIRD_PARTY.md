# Third-party notices

EPnC itself is licensed under the GNU GPL v3 or later (see `LICENSE`). It uses the following third-party material, all of which is compatible with that license.

## Raylib (used by the app only)

- Project: https://www.raylib.com (https://github.com/raysan5/raylib), version 5.5
- License: zlib
- Raylib is **not copied into this repository**. CMake downloads it at build time when `WITH_GUI=ON`, or uses a copy installed on your computer. The core library (everything except `Core/EPnC_Gui.h`) does not use it.

## Inter Regular (embedded in `Core/PlotFontData.h`)

- Project: https://github.com/rsms/inter, version 4.1
- Copyright (c) 2016 The Inter Project Authors
- License: SIL Open Font License, Version 1.1 (https://openfontlicense.org)
- Only a subset of the characters (Basic Latin, Greek letters and a few symbols) is embedded, converted to a C array. Nothing was redrawn.

## Method credit

The equation-discovery idea follows SINDy: S. L. Brunton, J. L. Proctor, J. N. Kutz, "Discovering governing equations from data by sparse identification of nonlinear dynamical systems", *PNAS* 113 (15), 2016. EPnC is an independent re-implementation written from scratch in C; no code from the SINDy authors is used.
