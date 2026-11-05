//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>
#include <OdeSolverAndSystems.h>
#include <CandidateTermLibrary.h>
#include <SparseRegressionSTLSQ.h>
#include <GroundTruthCoefficients.h>
#include <DerivativeEstimation.h>
#if 0
#include "../Core/RandomNoiseGenerator.h"
#include "../Core/LinearAlgebraSolver.h"
#include "../Core/OdeSolverAndSystems.h"
#include "../Core/CandidateTermLibrary.h"
#include "../Core/SparseRegressionSTLSQ.h"
#include "../Core/GroundTruthCoefficients.h"
#include "../Core/DerivativeEstimation.h"
#endif



/// @brief Prints a full guide for this `.exe` if the first argument is `-h` or `--help` (both do the same thing).
/// @param argc Argument count, straight from `main`.
/// @param argv Argument list, straight from `main`.
/// @return `true` if the guide was printed (then `main` should quit right away), otherwise `false`.
/// @attention Only the first argument is looked at. This keeps the clutter out of `main`.
bool PrintHelpIfAsked (int argc, char** argv)
{
    if (argc < 2) return false;
    if (strcmp(argv[1],"-h") != 0 && strcmp(argv[1],"--help") != 0) return false;

    printf("\n[Help] test_07_FiniteDifference.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Checks DerivativeEstimation.h. Until now we gave the regression the EXACT derivatives. Real data only has recorded points,");
    printf("\n  so the derivatives must be estimated from the points. Two estimators are tested (both on CLEAN data, no noise yet):");
    printf("\n    FD  finite differences: slope between the two neighbours of a point.");
    printf("\n    SG  Savitzky-Golay: fit a small polynomial to the nearby points, and take its slope.");
    printf("\n  Then the estimated derivatives go into STLSQ, and we see if the true equations still come out.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_07_FiniteDifference.exe [TimeStep] [SampleCount] [Threshold] [MaxDegree] [IncludeTrig] [Ridge] [HalfWindow] [PolyOrder]");
    printf("\n  test_07_FiniteDifference.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  TimeStep      RK4 step between two recorded points.                                    Default: 0.01");
    printf("\n  SampleCount   How many points to record.                                               Default: 3001");
    printf("\n  Threshold     STLSQ throws out coefficients smaller than this.                         Default: 0.05");
    printf("\n  MaxDegree     Highest total power in the library.                                      Default: 3");
    printf("\n  IncludeTrig   1 adds sin(x), cos(x), sin(y), cos(y). 0 leaves them out.                Default: 1");
    printf("\n  Ridge         Tiny stabilizer for each least squares fit.                             Default: 1e-10");
    printf("\n  HalfWindow    Savitzky-Golay uses 2*HalfWindow+1 points around each point.             Default: 5");
    printf("\n  PolyOrder     Degree of the polynomial Savitzky-Golay fits.                            Default: 3");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_07_FiniteDifference.exe");
    printf("\n  test_07_FiniteDifference.exe 0.05 600       (coarse steps, finite differences get visibly worse)");
    printf("\n  test_07_FiniteDifference.exe 0.01 3001 0.05 3 1 1e-10 8 4");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [HandCheck]  x = t^2 and y = t^3 on 5 points. FD is exact for x, off by exactly h^2 for y. SG with order 3 is exact for both.");
    printf("\n  [Accuracy]   For all 5 systems: biggest derivative error of FD and SG, at TimeStep and at TimeStep/2 (same total time).");
    printf("\n  [Recovery]   For all 5 systems: estimated derivatives (FD and SG) into STLSQ, then true and recovered equations side by side.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  [HandCheck]  Every line says OK.");
    printf("\n  [Accuracy]   For FD the error ratio is about 4: halving the step makes the error 4 times smaller (error grows like h^2).");
    printf("\n               SG with order 3 is more accurate and its ratio is about 8 (error grows like h^3).");
    printf("\n  [Recovery]   On clean data both give SUPPORT OK for all 5 systems, with small errors on the coefficients. These errors");
    printf("\n               (about 1e-3 to 1e-4) are the price of estimating the derivative instead of knowing it.");
    printf("\n");
    printf("\nTRY THIS");
    printf("\n  Run with TimeStep 0.1 and SampleCount 300. FD errors grow about 100 times, and some systems start to fail.");
    printf("\n  Then notice what is NOT here: noise. test_08 adds it, and then the story changes completely.");

    printf("\n");
    return true;
}



/// @brief Biggest absolute difference between two derivative matrices, over all rows and both columns.
/// @param Estimated Address of the estimated derivatives (`RowCount x 2`).
/// @param Exact Address of the exact derivatives (`RowCount x 2`).
/// @return The biggest `|Estimated - Exact|`.
double BiggestDerivativeError (Matrix* Estimated, Matrix* Exact)
{
    double Biggest = 0.0 ;
    for (size_t ThisRow=0; ThisRow<(Estimated->RowCount); ThisRow++)
    {
        for (size_t ThisCol=0; ThisCol<2; ThisCol++)
        {
            double Miss = fabs(GetMatrixCell(Estimated,ThisRow,ThisCol) - GetMatrixCell(Exact,ThisRow,ThisCol)) ;
            if (Miss > Biggest) Biggest = Miss;
        }
    }
    return Biggest;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    double TimeStep = 0.01 ;
    size_t SampleCount = 3001 ;
    double Threshold = 0.05 ;
    int MaxDegree = 3 ;
    bool IncludeTrig = true ;
    double Ridge = 1e-10 ;
    int HalfWindow = 5 ;
    int PolyOrder = 3 ;
    if (argc > 1) TimeStep    = atof(argv[1]) ;
    if (argc > 2) SampleCount = (size_t) atoll(argv[2]) ;
    if (argc > 3) Threshold   = atof(argv[3]) ;
    if (argc > 4) MaxDegree   = atoi(argv[4]) ;
    if (argc > 5) IncludeTrig = (atoi(argv[5]) != 0) ;
    if (argc > 6) Ridge       = atof(argv[6]) ;
    if (argc > 7) HalfWindow  = atoi(argv[7]) ;
    if (argc > 8) PolyOrder   = atoi(argv[8]) ;

    #if 1   //To check both estimators on x = t^2 and y = t^3 (5 points, h = 0.1) against numbers you can do on paper.
    Matrix HandPath = MatrixCreate(5, 3) ;
    for (size_t ThisRow=0; ThisRow<5; ThisRow++)
    {
        double t = 0.1 * (double) ThisRow ;
        SetMatrixCell(&HandPath, ThisRow, 0, t);
        SetMatrixCell(&HandPath, ThisRow, 1, t*t);
        SetMatrixCell(&HandPath, ThisRow, 2, t*t*t);
    }
    Matrix HandFd = FiniteDifferenceDerivatives(&HandPath) ;
    Matrix HandSg = SavitzkyGolayDerivatives(&HandPath, 2, 3) ;

    //FD, x = t^2: central difference of a parabola is exact, and so is the one sided formula. Expected 2t at every row.
    double MissX = 0.0 ;
    for (size_t ThisRow=0; ThisRow<5; ThisRow++) MissX = fmax(MissX, fabs(GetMatrixCell(&HandFd,ThisRow,0) - 2.0*(0.1*(double)ThisRow)));
    printf("\n[HandCheck] FD  dx/dt of t^2: biggest miss = %e (expected 0)   %s", MissX, (MissX < 1e-12) ? "OK" : "WRONG");

    //FD, y = t^3 at t = 0.2: (0.3^3 - 0.1^3) / 0.2 = 0.13, while the true slope is 3*0.2^2 = 0.12. The central difference is off by exactly h^2 = 0.01.
    double FdY = GetMatrixCell(&HandFd,2,1) ;
    printf("\n[HandCheck] FD  dy/dt of t^3 at t = 0.2: got %.6f, expected 0.130000 (true slope 0.12, off by h^2)   %s", FdY, (fabs(FdY - 0.13) < 1e-12) ? "OK" : "WRONG");

    //SG with order 3 and 5 points reproduces any cubic exactly, edges included.
    double MissSg = 0.0 ;
    for (size_t ThisRow=0; ThisRow<5; ThisRow++)
    {
        double t = 0.1 * (double) ThisRow ;
        MissSg = fmax(MissSg, fabs(GetMatrixCell(&HandSg,ThisRow,0) - 2.0*t));
        MissSg = fmax(MissSg, fabs(GetMatrixCell(&HandSg,ThisRow,1) - 3.0*t*t));
    }
    printf("\n[HandCheck] SG  order 3, 5 points, exact on t^2 and t^3: biggest miss = %e (expected 0)   %s", MissSg, (MissSg < 1e-9) ? "OK" : "WRONG");
    MatrixDestroy(&HandSg);
    MatrixDestroy(&HandFd);
    MatrixDestroy(&HandPath);
    #endif

    #if 1   //To measure the derivative error of FD and SG, at TimeStep and at TimeStep/2 over the same total time.
    printf("\n\n[Accuracy] SG uses HalfWindow = %d, PolyOrder = %d", HalfWindow, PolyOrder);
    printf("\n[Accuracy] %-16s  %12s %12s %7s    %12s %12s %7s", "system", "FD err (h)", "FD err (h/2)", "ratio", "SG err (h)", "SG err (h/2)", "ratio");
    for (size_t ThisSystem=0; ThisSystem<SYSTEM_COUNT; ThisSystem++)
    {
        SystemID System = (SystemID) ThisSystem ;
        double Params[MAX_SYSTEM_PARAMS] ;
        GetDefaultParams(System, Params);

        double FdErr[2], SgErr[2] ;
        for (int ThisHalving=0; ThisHalving<2; ThisHalving++)
        {
            double Step = (ThisHalving == 0) ? TimeStep : (TimeStep / 2.0) ;
            size_t Count = (ThisHalving == 0) ? SampleCount : (2*SampleCount - 1) ;
            Matrix Path = TraceTrajectory(System, Params, GetDefaultStartState(System), Step, Count) ;
            Matrix Exact = TrueDerivativesOfTrajectory(System, Params, &Path) ;
            Matrix Fd = FiniteDifferenceDerivatives(&Path) ;
            Matrix Sg = SavitzkyGolayDerivatives(&Path, HalfWindow, PolyOrder) ;
            FdErr[ThisHalving] = BiggestDerivativeError(&Fd, &Exact) ;
            SgErr[ThisHalving] = BiggestDerivativeError(&Sg, &Exact) ;
            MatrixDestroy(&Sg);
            MatrixDestroy(&Fd);
            MatrixDestroy(&Exact);
            MatrixDestroy(&Path);
        }

        printf("\n[Accuracy] %-16s  %12.3e %12.3e %7.2f    %12.3e %12.3e %7.2f", GetSystemName(System), FdErr[0], FdErr[1], FdErr[0]/FdErr[1], SgErr[0], SgErr[1], SgErr[0]/SgErr[1]);
    }
    #endif

    #if 1   //To put estimated derivatives (FD, then SG) of CLEAN paths into STLSQ for all 5 systems, and show true and recovered equations side by side.
    TermLibrary Library = CreateTermLibrary(MaxDegree, IncludeTrig) ;
    size_t TermCount = (Library.TermCount) ;
    double* TrueDx = (double*) calloc(TermCount, sizeof(double)) ;
    double* TrueDy = (double*) calloc(TermCount, sizeof(double)) ;
    double* FoundDx = (double*) calloc(TermCount, sizeof(double)) ;
    double* FoundDy = (double*) calloc(TermCount, sizeof(double)) ;
    double* Target = (double*) calloc(SampleCount, sizeof(double)) ;

    for (size_t ThisSystem=0; ThisSystem<SYSTEM_COUNT; ThisSystem++)
    {
        SystemID System = (SystemID) ThisSystem ;
        double Params[MAX_SYSTEM_PARAMS] ;
        GetDefaultParams(System, Params);
        Matrix Path = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, SampleCount) ;
        Matrix Theta = BuildTermMatrix(&Library, &Path, 1, 2) ;
        int MissingCount = FillTrueCoefficients(System, Params, &Library, TrueDx, TrueDy) ;

        printf("\n\n[Recovery] %s", GetSystemName(System));
        if (MissingCount > 0) printf("   (%d true term(s) MISSING from this library, so it cannot be exact)", MissingCount);
        printf("\n    true:          ");
        PrintEquation("dx/dt", &Library, TrueDx, 0.0, 3);
        printf("\n                   ");
        PrintEquation("dy/dt", &Library, TrueDy, 0.0, 3);

        for (int ThisMethod=0; ThisMethod<2; ThisMethod++)
        {
            Matrix Estimated = (ThisMethod == 0) ? FiniteDifferenceDerivatives(&Path) : SavitzkyGolayDerivatives(&Path, HalfWindow, PolyOrder) ;
            for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++) Target[ThisRow] = GetMatrixCell(&Estimated, ThisRow, 0);
            SparseRegressionSTLSQ(&Theta, Target, Threshold, Ridge, 10, FoundDx);
            for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++) Target[ThisRow] = GetMatrixCell(&Estimated, ThisRow, 1);
            SparseRegressionSTLSQ(&Theta, Target, Threshold, Ridge, 10, FoundDy);

            SupportReport Report = CompareWithTruth(TrueDx, TrueDy, FoundDx, FoundDy, TermCount) ;
            printf("\n    recovered %s:  ", (ThisMethod == 0) ? "FD" : "SG");
            PrintEquation("dx/dt", &Library, FoundDx, 0.0, 3);
            printf("\n                   ");
            PrintEquation("dy/dt", &Library, FoundDy, 0.0, 3);
            printf("\n        true terms found = %d of %d,  extra terms = %d,  biggest miss on any coefficient = %e    %s", (Report.FoundTrueCount), (Report.TrueTermCount), (Report.ExtraTermCount), (Report.BiggestMiss), (Report.FoundTrueCount == Report.TrueTermCount && Report.ExtraTermCount == 0) ? "SUPPORT OK" : "SUPPORT WRONG");
            MatrixDestroy(&Estimated);
        }

        MatrixDestroy(&Theta);
        MatrixDestroy(&Path);
    }

    free(Target);
    free(FoundDy);
    free(FoundDx);
    free(TrueDy);
    free(TrueDx);
    DestroyTermLibrary(&Library);
    #endif

    printf("\n");
    return 0;
}
