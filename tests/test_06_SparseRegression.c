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
#if 0
#include "../Core/RandomNoiseGenerator.h"
#include "../Core/LinearAlgebraSolver.h"
#include "../Core/OdeSolverAndSystems.h"
#include "../Core/CandidateTermLibrary.h"
#include "../Core/SparseRegressionSTLSQ.h"
#include "../Core/GroundTruthCoefficients.h"
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

    printf("\n[Help] test_06_SparseRegression.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Checks SparseRegressionSTLSQ.h. Plain least squares (test_05) uses EVERY column of Theta, so terms that should be zero come out");
    printf("\n  as tiny leftovers. STLSQ fixes that: fit, throw out every coefficient below a threshold, refit the survivors, repeat.");
    printf("\n  The result is an equation with only a few terms, and the thrown out terms are exactly zero.");
    printf("\n  Here the derivatives are still EXACT (no noise yet), so this proves the algorithm itself before noise makes things hard.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_06_SparseRegression.exe [Threshold] [MaxDegree] [IncludeTrig] [TimeStep] [SampleCount] [Ridge] [MaxIterations]");
    printf("\n  test_06_SparseRegression.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  Threshold      Coefficients smaller than this (absolute value) are thrown out.                  Default: 0.05");
    printf("\n  MaxDegree      Highest total power in the library.                                              Default: 3");
    printf("\n  IncludeTrig    1 adds sin(x), cos(x), sin(y), cos(y). 0 leaves them out.                        Default: 1");
    printf("\n  TimeStep       RK4 step between two recorded points.                                            Default: 0.01");
    printf("\n  SampleCount    How many points to record.                                                       Default: 3001");
    printf("\n  Ridge          Tiny stabilizer for each least squares fit.                                      Default: 1e-10");
    printf("\n  MaxIterations  Most number of throw out and refit rounds.                                       Default: 10");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_06_SparseRegression.exe");
    printf("\n  test_06_SparseRegression.exe 0.2                 (a bigger threshold, so it is stricter)");
    printf("\n  test_06_SparseRegression.exe 0.05 2 0            (library too small, true terms are MISSING)");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [HandCheck]  A tiny problem you can do on paper: target = 2x + 0.01 x^2 on 9 points, library 1, x, x^2.");
    printf("\n  [Systems]    For all 5 systems: exact derivatives, Theta, STLSQ for dx and dy, then true and recovered equations side by side.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  [HandCheck]  Every line says OK. The constant and x^2 are thrown out (exactly 0) and x gets exactly 2.");
    printf("\n  [Systems]    Every system says SUPPORT OK (the set of non-zero terms equals the true set) and the biggest miss is tiny.");
    printf("\n               Unlike test_05 there are no small leftovers: a thrown out term is exactly zero.");
    printf("\n");
    printf("\nTRY THIS");
    printf("\n  Threshold 0.0001: nothing gets thrown out, so you get the leftovers of plain least squares back.");
    printf("\n  Threshold 5: too strict, even true terms (like 2 or 1.5) are small enough to be thrown out. The threshold must fit the problem.");
    printf("\n  MaxDegree 2 and IncludeTrig 0: three systems have MISSING terms, so SUPPORT WRONG is the correct and honest answer.");

    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    double Threshold = 0.05 ;
    int MaxDegree = 3 ;
    bool IncludeTrig = true ;
    double TimeStep = 0.01 ;
    size_t SampleCount = 3001 ;
    double Ridge = 1e-10 ;
    int MaxIterations = 10 ;
    if (argc > 1) Threshold     = atof(argv[1]) ;
    if (argc > 2) MaxDegree     = atoi(argv[2]) ;
    if (argc > 3) IncludeTrig   = (atoi(argv[3]) != 0) ;
    if (argc > 4) TimeStep      = atof(argv[4]) ;
    if (argc > 5) SampleCount   = (size_t) atoll(argv[5]) ;
    if (argc > 6) Ridge         = atof(argv[6]) ;
    if (argc > 7) MaxIterations = atoi(argv[7]) ;

    #if 1   //To check STLSQ on a tiny problem you can do on paper: target = 2x + 0.01 x^2 on 9 points, library 1, x, x^2.
    Matrix HandTheta = MatrixCreate(9, 3) ;
    double HandTarget[9] ;
    double HandAnswer[3] ;
    for (size_t ThisRow=0; ThisRow<9; ThisRow++)
    {
        double x = -2.0 + 0.5 * (double) ThisRow ;      //-2, -1.5, ..., 2
        SetMatrixCell(&HandTheta, ThisRow, 0, 1.0);
        SetMatrixCell(&HandTheta, ThisRow, 1, x);
        SetMatrixCell(&HandTheta, ThisRow, 2, x*x);
        HandTarget[ThisRow] = 2.0*x + 0.01*x*x ;
    }

    //Plain least squares first: finds (0, 2, 0.01) exactly, so the 0.01 survives
    LeastSquaresSolve(&HandTheta, HandTarget, 0.0, HandAnswer);
    printf("\n[HandCheck] plain least squares:  1: %9.6f   x: %9.6f   x^2: %9.6f   (expected 0, 2, 0.01)", HandAnswer[0], HandAnswer[1], HandAnswer[2]);

    //STLSQ with threshold 0.05: the constant (0) and x^2 (0.01) are below it, so both go. Refit on x alone gives exactly 2 (the sum of x^3 over symmetric points is 0).
    int Rounds = SparseRegressionSTLSQ(&HandTheta, HandTarget, 0.05, 0.0, 10, HandAnswer) ;
    printf("\n[HandCheck] STLSQ, threshold 0.05: 1: %9.6f   x: %9.6f   x^2: %9.6f   (expected 0, 2, 0)", HandAnswer[0], HandAnswer[1], HandAnswer[2]);
    printf("\n[HandCheck] constant thrown out to exactly 0:   %s", (HandAnswer[0] == 0.0) ? "OK" : "WRONG");
    printf("\n[HandCheck] x^2 thrown out to exactly 0:        %s", (HandAnswer[2] == 0.0) ? "OK" : "WRONG");
    printf("\n[HandCheck] x is exactly 2 after the refit:     %s", (fabs(HandAnswer[1] - 2.0) < 1e-12) ? "OK" : "WRONG");
    printf("\n[HandCheck] rounds that threw something out = %d (expected 1)  %s", Rounds, (Rounds == 1) ? "OK" : "WRONG");
    MatrixDestroy(&HandTheta);
    #endif

    #if 1   //To recover every system with STLSQ on EXACT derivatives, and show true and recovered equations side by side.
    TermLibrary Library = CreateTermLibrary(MaxDegree, IncludeTrig) ;
    for (size_t ThisSystem=0; ThisSystem<SYSTEM_COUNT; ThisSystem++)
    {
        SystemID System = (SystemID) ThisSystem ;
        double Params[MAX_SYSTEM_PARAMS] ;
        GetDefaultParams(System, Params);
        Matrix Path = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, SampleCount) ;
        Matrix TrueDerivatives = TrueDerivativesOfTrajectory(System, Params, &Path) ;
        Matrix Theta = BuildTermMatrix(&Library, &Path, 1, 2) ;

        size_t TermCount = (Library.TermCount) ;
        double* TrueDx = (double*) calloc(TermCount, sizeof(double)) ;
        double* TrueDy = (double*) calloc(TermCount, sizeof(double)) ;
        double* FoundDx = (double*) calloc(TermCount, sizeof(double)) ;
        double* FoundDy = (double*) calloc(TermCount, sizeof(double)) ;
        double* Target = (double*) calloc((Path.RowCount), sizeof(double)) ;
        int MissingCount = FillTrueCoefficients(System, Params, &Library, TrueDx, TrueDy) ;

        //One STLSQ problem per equation: Theta * coefficients = derivative
        for (size_t ThisRow=0; ThisRow<(Path.RowCount); ThisRow++) Target[ThisRow] = GetMatrixCell(&TrueDerivatives, ThisRow, 0);
        int RoundsX = SparseRegressionSTLSQ(&Theta, Target, Threshold, Ridge, MaxIterations, FoundDx) ;
        for (size_t ThisRow=0; ThisRow<(Path.RowCount); ThisRow++) Target[ThisRow] = GetMatrixCell(&TrueDerivatives, ThisRow, 1);
        int RoundsY = SparseRegressionSTLSQ(&Theta, Target, Threshold, Ridge, MaxIterations, FoundDy) ;

        //Grade: biggest miss on any coefficient, and how many terms are on the wrong side of zero (found but not true, or true but not found)
        double BiggestMiss = 0.0 ;
        int WrongSupportCount = 0 ;
        for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
        {
            double MissX = fabs(FoundDx[ThisTerm] - TrueDx[ThisTerm]) ;
            double MissY = fabs(FoundDy[ThisTerm] - TrueDy[ThisTerm]) ;
            if (MissX > BiggestMiss) BiggestMiss = MissX;
            if (MissY > BiggestMiss) BiggestMiss = MissY;
            if ((FoundDx[ThisTerm] != 0.0) != (TrueDx[ThisTerm] != 0.0)) WrongSupportCount++;
            if ((FoundDy[ThisTerm] != 0.0) != (TrueDy[ThisTerm] != 0.0)) WrongSupportCount++;
        }

        printf("\n\n[Systems] %s", GetSystemName(System));
        if (MissingCount > 0) printf("   (%d true term(s) MISSING from this library, so it cannot be exact)", MissingCount);
        printf("\n    true:       ");
        PrintEquation("dx/dt", &Library, TrueDx, 0.0, 3);
        printf("\n                ");
        PrintEquation("dy/dt", &Library, TrueDy, 0.0, 3);
        printf("\n    recovered:  ");
        PrintEquation("dx/dt", &Library, FoundDx, 0.0, 3);
        printf("\n                ");
        PrintEquation("dy/dt", &Library, FoundDy, 0.0, 3);
        printf("\n    rounds (dx, dy) = %d, %d    biggest miss on any coefficient = %e    wrong terms = %d    %s", RoundsX, RoundsY, BiggestMiss, WrongSupportCount, (WrongSupportCount == 0) ? "SUPPORT OK" : "SUPPORT WRONG");

        free(Target);
        free(FoundDy);
        free(FoundDx);
        free(TrueDy);
        free(TrueDx);
        MatrixDestroy(&Theta);
        MatrixDestroy(&TrueDerivatives);
        MatrixDestroy(&Path);
    }
    DestroyTermLibrary(&Library);
    #endif

    printf("\n");
    return 0;
}
