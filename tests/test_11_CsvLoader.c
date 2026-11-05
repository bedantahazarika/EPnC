//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <EPnC.h>



/// @brief Prints a full guide for this program if the first argument is `-h` or `--help`.
/// @param argc Argument count from `main`.
/// @param argv Argument values from `main`.
/// @return `true` if the guide was printed (the caller should stop), `false` otherwise.
bool PrintHelpIfAsked (int argc, char** argv)
{
    if (argc < 2) return false;
    if (strcmp(argv[1],"-h") != 0 && strcmp(argv[1],"--help") != 0) return false;

    printf("\n[Help] test_11_CsvLoader.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Tests the CSV loader (CsvPathLoader.h) and, because it only includes <EPnC.h>, the umbrella header too.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_11_CsvLoader.exe [CsvFile]");
    printf("\n");
    printf("\nARGUMENTS");
    printf("\n  CsvFile   Optional. A file with columns t,x,y to load and describe. Default: none.");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [RoundTrip]  Saves a simulated path, loads it back, and checks every number is identical.");
    printf("\n  [Messy]      Loads a deliberately messy file (header, blank line, semicolons, tabs, comment, CRLF, scientific notation).");
    printf("\n  [Missing]    Loads a file that does not exist: should give 0 rows, not a crash.");
    printf("\n  [File]       Loads CsvFile if one was given.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  Biggest difference 0, the messy file gives 3 rows with the right numbers, the missing file gives 0 rows.");

    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    #if 1   //To save a simulated path to CSV, load it back and compare every number.
    {
        double Params[MAX_SYSTEM_PARAMS] ;
        GetDefaultParams(SYSTEM_LINEAR_SPIRAL, Params);
        Matrix Original = TraceTrajectory(SYSTEM_LINEAR_SPIRAL, Params, GetDefaultStartState(SYSTEM_LINEAR_SPIRAL), 0.01, 200) ;
        SavePathToCsv(&Original, "test_11_RoundTrip.csv");
        Matrix Loaded = LoadPathFromCsv("test_11_RoundTrip.csv") ;

        double BiggestDifference = 0.0 ;
        for (size_t ThisCell=0; ThisCell<(Original.RowCount)*3; ThisCell++)
        {
            double Difference = fabs(Original.MatrixCells[ThisCell] - Loaded.MatrixCells[ThisCell]) ;
            if (Difference > BiggestDifference) BiggestDifference = Difference ;
        }
        printf("\n[RoundTrip] saved %zu rows, loaded %zu rows, biggest difference %g", Original.RowCount, Loaded.RowCount, BiggestDifference);

        MatrixDestroy(&Original);
        MatrixDestroy(&Loaded);
    }
    #endif

    #if 1   //To load a deliberately messy file.
    {
        FILE* Messy = fopen("test_11_Messy.csv", "wb") ;
        fprintf(Messy, "# my data\r\n");
        fprintf(Messy, "time,x,y\r\n");
        fprintf(Messy, "0.0,1.0,2.0\r\n");
        fprintf(Messy, "\r\n");
        fprintf(Messy, "0.5; 1.5e-1 ;-2.5\r\n");
        fprintf(Messy, "1.0\t3\t4\textra,columns\r\n");
        fprintf(Messy, "oops,1,2\r\n");
        fclose(Messy);

        Matrix Path = LoadPathFromCsv("test_11_Messy.csv") ;
        printf("\n[Messy] rows = %zu (expected 3)", Path.RowCount);
        for (size_t ThisRow=0; ThisRow<(Path.RowCount); ThisRow++)
        {
            printf("\n[Messy]   t = %g   x = %g   y = %g", GetMatrixCell(&Path,ThisRow,0), GetMatrixCell(&Path,ThisRow,1), GetMatrixCell(&Path,ThisRow,2));
        }
        printf("\n[Messy] expected: (0, 1, 2)  (0.5, 0.15, -2.5)  (1, 3, 4)");
        MatrixDestroy(&Path);
    }
    #endif

    #if 1   //To load a file that does not exist.
    {
        Matrix Path = LoadPathFromCsv("this_file_does_not_exist.csv") ;
        printf("\n[Missing] rows = %zu (expected 0)", Path.RowCount);
        MatrixDestroy(&Path);
    }
    #endif

    #if 1   //To load and describe the file given as the first argument.
    if (argc > 1)
    {
        Matrix Path = LoadPathFromCsv(argv[1]) ;
        printf("\n[File] %s: %zu rows", argv[1], Path.RowCount);
        if (Path.RowCount > 1)
        {
            printf("\n[File]   first row: t = %g   x = %g   y = %g", GetMatrixCell(&Path,0,0), GetMatrixCell(&Path,0,1), GetMatrixCell(&Path,0,2));
            printf("\n[File]   last row:  t = %g   x = %g   y = %g", GetMatrixCell(&Path,Path.RowCount-1,0), GetMatrixCell(&Path,Path.RowCount-1,1), GetMatrixCell(&Path,Path.RowCount-1,2));
            printf("\n[File]   average time step: %g", (GetMatrixCell(&Path,Path.RowCount-1,0) - GetMatrixCell(&Path,0,0)) / (double)(Path.RowCount-1));
        }
        MatrixDestroy(&Path);
    }
    #endif

    printf("\n");
    return 0;
}
