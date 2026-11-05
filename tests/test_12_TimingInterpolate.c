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

    printf("\n[Help] test_12_TimingInterpolate.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Tests TimingInterpolate.h: how well do the four interpolation methods rebuild a signal whose clock was uneven,");
    printf("\n  and does making the time even help finite differences?");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_12_TimingInterpolate.exe [CsvFile] [OutFile]");
    printf("\n");
    printf("\nARGUMENTS");
    printf("\n  CsvFile   Optional. A file with columns t,x,y: its timing is analyzed.   Default: none");
    printf("\n  OutFile   Optional. If given too, the evened path (spline) is saved here. Default: none");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [Accuracy]  A sine wave with a jittery clock is put back on an even grid with each method. RMS error vs the exact sine.");
    printf("\n  [Slopes]    Finite difference slopes of a jittery circle: as given, and after making the time even with each method.");
    printf("\n  [Report]    The timing report of the jittery circle for several tolerances: how many rows count as drifted.");
    printf("\n  [File]      Timing report of CsvFile (and optional OutFile).");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  [Accuracy] cubic methods beat linear by a large factor. [Slopes] the slope error after evening is far below the one before.");

    printf("\n");
    return true;
}



/// @brief Makes a circle path `x = sin t`, `y = cos t` with a jittery clock: every sample time is shaken by a random fraction of the step. Values are exact for the shaken times.
/// @param PointCount Number of rows.
/// @param EndTime Last time (first is 0).
/// @param Jitter Largest shake as a fraction of the step, below `0.5` so the order of samples never changes.
/// @param Gen Address of the random generator.
/// @return A new `Matrix` (columns `t, x, y`). The first and last time are not shaken.
Matrix MakeJitteryCircle (size_t PointCount, double EndTime, double Jitter, RandomState* Gen)
{
    Matrix Path = MatrixCreate(PointCount, 3) ;
    double Step = EndTime / (double)(PointCount - 1) ;

    for (size_t ThisRow=0; ThisRow<PointCount; ThisRow++)
    {
        double Time = (double)ThisRow * Step ;
        if ((ThisRow > 0) && (ThisRow < PointCount-1)) Time += (2.0*RandomUniform(Gen) - 1.0) * Jitter * Step ;
        SetMatrixCell(&Path, ThisRow, 0, Time);
        SetMatrixCell(&Path, ThisRow, 1, sin(Time));
        SetMatrixCell(&Path, ThisRow, 2, cos(Time));
    }

    return Path;
}



/// @brief Root mean square error of the `x` and `y` columns of a path against the exact circle at the path's own times.
/// @param Path Address of the path.
/// @return RMS error in `double`.
double CircleValueError (Matrix* Path)
{
    double Sum = 0.0 ;
    for (size_t ThisRow=0; ThisRow<(Path->RowCount); ThisRow++)
    {
        double Time = GetMatrixCell(Path,ThisRow,0) ;
        double MissX = GetMatrixCell(Path,ThisRow,1) - sin(Time) ;
        double MissY = GetMatrixCell(Path,ThisRow,2) - cos(Time) ;
        Sum += MissX*MissX + MissY*MissY ;
    }
    return sqrt(Sum / (2.0*(double)(Path->RowCount)));
}



/// @brief Root mean square error of finite difference slopes against the exact slopes of the circle (`cos t` and `-sin t`) at the path's own times.
/// @param Path Address of the path.
/// @return RMS error in `double`.
double CircleSlopeError (Matrix* Path)
{
    Matrix Slopes = FiniteDifferenceDerivatives(Path) ;
    double Sum = 0.0 ;
    for (size_t ThisRow=0; ThisRow<(Path->RowCount); ThisRow++)
    {
        double Time = GetMatrixCell(Path,ThisRow,0) ;
        double MissX = GetMatrixCell(&Slopes,ThisRow,0) - cos(Time) ;
        double MissY = GetMatrixCell(&Slopes,ThisRow,1) + sin(Time) ;
        Sum += MissX*MissX + MissY*MissY ;
    }
    MatrixDestroy(&Slopes);
    return sqrt(Sum / (2.0*(double)(Path->RowCount)));
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    RandomState Gen ;
    RandomSeed(&Gen, 7);
    double TwoPi = 2.0*3.14159265358979323846 ;

    #if 1   //To rebuild a jittery sine wave on an even grid with every method and measure the error.
    {
        Matrix Path = MakeJitteryCircle(40, TwoPi, 0.3, &Gen) ;
        TimingReport Report = AnalyzeTiming(&Path, 0.0) ;
        printf("\n[Accuracy] 40 points over one turn, clock jitter up to 30 percent of a step (largest drift seen %.2f steps)", Report.LargestDrift);
        Matrix Snapped = MakeTimeEven(&Path, INTERP_LINEAR, 1e9, NULL) ;
        printf("\n[Accuracy]   %-14s RMS error %.6f   (values left alone, only the times snapped to the grid)", "no interpolation", CircleValueError(&Snapped));
        MatrixDestroy(&Snapped);
        for (int ThisMethod=0; ThisMethod<INTERP_COUNT; ThisMethod++)
        {
            Matrix Evened = MakeTimeEven(&Path, (InterpolationMethod)ThisMethod, 0.0, NULL) ;
            printf("\n[Accuracy]   %-14s RMS error %.6f", GetInterpolationName((InterpolationMethod)ThisMethod), CircleValueError(&Evened));
            MatrixDestroy(&Evened);
        }
        MatrixDestroy(&Path);
    }
    #endif

    #if 1   //To see if finite difference slopes improve after making the time even.
    {
        Matrix Path = MakeJitteryCircle(400, 2.0*TwoPi, 0.3, &Gen) ;
        printf("\n\n[Slopes] 400 points over two turns, jitter up to 30 percent of a step, no noise");
        printf("\n[Slopes]   RMS slope error as given:                 %.5f", CircleSlopeError(&Path));
        for (int ThisMethod=0; ThisMethod<INTERP_COUNT; ThisMethod++)
        {
            Matrix Evened = MakeTimeEven(&Path, (InterpolationMethod)ThisMethod, 0.05, NULL) ;
            printf("\n[Slopes]   RMS slope error after %-14s %.5f", GetInterpolationName((InterpolationMethod)ThisMethod), CircleSlopeError(&Evened));
            MatrixDestroy(&Evened);
        }

        printf("\n\n[Report] Timing of the same path for several tolerances (units of one step)");
        double Tolerances[] = { 0.0, 0.05, 0.1, 0.2, 0.3 } ;
        for (size_t ThisTolerance=0; ThisTolerance<5; ThisTolerance++)
        {
            TimingReport Report = AnalyzeTiming(&Path, Tolerances[ThisTolerance]) ;
            printf("\n[Report]   tolerance %.2f: %3zu of %zu rows drifted", Tolerances[ThisTolerance], Report.DriftedCount, Report.PointCount);
        }
        TimingReport Report = AnalyzeTiming(&Path, 0.05) ;
        printf("\n[Report]   ideal step %.5f, smallest step %.5f, largest step %.5f, increasing: %s", Report.IdealStep, Report.SmallestStep, Report.LargestStep, Report.IsIncreasing ? "yes" : "no");
        MatrixDestroy(&Path);
    }
    #endif

    #if 1   //To analyze the timing of the file given as the first argument, and optionally save the evened version.
    if (argc > 1)
    {
        Matrix Path = LoadPathFromCsv(argv[1]) ;
        if (Path.RowCount < 3)
        {
            printf("\n\n[File] %s: fewer than 3 good rows, nothing to analyze", argv[1]);
        }
        else
        {
            TimingReport Report = AnalyzeTiming(&Path, 0.05) ;
            printf("\n\n[File] %s: %zu rows, increasing: %s", argv[1], Report.PointCount, Report.IsIncreasing ? "yes" : "NO (sort it first, interpolation needs it)");
            printf("\n[File]   ideal step %g, smallest step %g, largest step %g (%.1f times the ideal)", Report.IdealStep, Report.SmallestStep, Report.LargestStep, Report.LargestStep/Report.IdealStep);
            printf("\n[File]   largest drift %.3f steps, %zu rows drifted more than 0.05 steps", Report.LargestDrift, Report.DriftedCount);
            if ((argc > 2) && (Report.IsIncreasing))
            {
                Matrix Evened = MakeTimeEven(&Path, INTERP_SPLINE, 0.05, NULL) ;
                SavePathToCsv(&Evened, argv[2]);
                printf("\n[File]   saved the evened path (spline) to %s", argv[2]);
                MatrixDestroy(&Evened);
            }
        }
        MatrixDestroy(&Path);
    }
    #endif

    printf("\n");
    return 0;
}
