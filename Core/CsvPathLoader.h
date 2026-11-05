//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <LinearAlgebraSolver.h>



/// @brief Tries to read exactly three numbers (`t`, `x`, `y`) from the start of one text line.
/// @param Line The text line (a `\0` ended string).
/// @param Numbers Array of 3 `double`, filled with `t, x, y` when the line is good.
/// @return `true` if three real numbers were found, `false` otherwise (header line, blank line, text, `nan`, `inf`).
/// @attention Numbers may be separated by commas, semicolons, spaces or tabs, in any mix. Extra columns after the third are ignored. Scientific notation (`1.5e-3`) works.
/// @attention A header like `t,x,y` fails on purpose, so headers are skipped without any special code.
bool ParseThreeNumbers (const char* Line, double* Numbers)
{
    const char* Here = Line ;

    for (int ThisNumber=0; ThisNumber<3; ThisNumber++)
    {
        while ((*Here == ',') || (*Here == ';') || (*Here == ' ') || (*Here == '\t'))
        {
            Here++ ;
        }

        char* After ;
        Numbers[ThisNumber] = strtod(Here, &After) ;
        if (After == Here) return false;
        if (!isfinite(Numbers[ThisNumber])) return false;
        Here = After ;
    }

    return true;
}



/// @brief Loads a trajectory from a CSV file with columns `t,x,y`. Same layout as `TraceTrajectory`, so the result goes straight into `FiniteDifferenceDerivatives`, `NeuralDerivatives`, `BuildTermMatrix` and so on.
/// @param FilePath Path of the file, like `"data.csv"`.
/// @return A new `Matrix` of size `RowCount x 3` (columns `t, x, y`). If the file cannot be opened or has no good rows, `RowCount` is `0`, so check `(Path.RowCount) == 0` before using it.
/// @attention Reads the file twice: first counts the good rows, then fills the matrix. Lines that are not three numbers (header, blank, comments) are skipped silently.
/// @warning Does not check that `t` is increasing or evenly spaced. Lines longer than 1023 characters are cut. Result needs a `MatrixDestroy` later.
Matrix LoadPathFromCsv (const char* FilePath)
{
    char Line[1024] ;
    double Numbers[3] ;
    size_t RowCount = 0 ;

    FILE* File = fopen(FilePath, "r") ;
    if (File == NULL) return MatrixCreate(0, 3);

    while (fgets(Line, sizeof(Line), File) != NULL)
    {
        if (ParseThreeNumbers(Line, Numbers)) RowCount++ ;
    }
    rewind(File);

    Matrix Path = MatrixCreate(RowCount, 3) ;
    size_t ThisRow = 0 ;
    while (fgets(Line, sizeof(Line), File) != NULL)
    {
        if (ParseThreeNumbers(Line, Numbers))
        {
            for (size_t ThisCol=0; ThisCol<3; ThisCol++)
            {
                SetMatrixCell(&Path, ThisRow, ThisCol, Numbers[ThisCol]);
            }
            ThisRow++ ;
        }
    }

    fclose(File);
    return Path;
}



/// @brief Saves a trajectory to a CSV file with a `t,x,y` header line.
/// @param Path Address of the trajectory (columns `t, x, y`).
/// @param FilePath Path of the file to write (an existing file is overwritten).
/// @attention Numbers are written with 17 significant digits, so loading the file back gives exactly the same `double` values.
/// @warning If the file cannot be opened, nothing is written and nothing is reported.
void SavePathToCsv (Matrix* Path, const char* FilePath)
{
    FILE* File = fopen(FilePath, "w") ;
    if (File == NULL) return;

    fprintf(File, "t,x,y\n");
    for (size_t ThisRow=0; ThisRow<(Path->RowCount); ThisRow++)
    {
        fprintf(File, "%.17g,%.17g,%.17g\n", GetMatrixCell(Path,ThisRow,0), GetMatrixCell(Path,ThisRow,1), GetMatrixCell(Path,ThisRow,2));
    }

    fclose(File);
    return;
}
