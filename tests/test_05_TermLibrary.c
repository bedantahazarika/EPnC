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
#include <GroundTruthCoefficients.h>
#if 0
#include "../Core/RandomNoiseGenerator.h"
#include "../Core/LinearAlgebraSolver.h"
#include "../Core/OdeSolverAndSystems.h"
#include "../Core/CandidateTermLibrary.h"
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

    printf("\n[Help] test_05_TermLibrary.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Checks CandidateTermLibrary.h. The library turns every (x, y) sample into a row of candidate terms: 1, x, y, x^2, x*y, y^2, ...");
    printf("\n  That table is called Theta. The regression later looks for the FEW columns of Theta that add up to the derivative.");
    printf("\n  Here we do not look for few yet. We use plain least squares on EXACT derivatives, to prove that Theta is built correctly.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_05_TermLibrary.exe [MaxDegree] [IncludeTrig] [TimeStep] [SampleCount] [Ridge]");
    printf("\n  test_05_TermLibrary.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  MaxDegree     Highest total power in the library. 3 means up to x^3, x^2*y, x*y^2, y^3.      Default: 3");
    printf("\n  IncludeTrig   1 adds sin(x), cos(x), sin(y), cos(y). 0 leaves them out.                      Default: 1");
    printf("\n  TimeStep      RK4 step between two recorded points.                                          Default: 0.01");
    printf("\n  SampleCount   How many points to record. Total time = (SampleCount-1) * TimeStep.            Default: 3001");
    printf("\n  Ridge         Tiny stabilizer for the least squares (see LeastSquaresSolve).                 Default: 1e-10");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_05_TermLibrary.exe");
    printf("\n  test_05_TermLibrary.exe 2 0            (polynomials up to degree 2 only, some systems become impossible)");
    printf("\n  test_05_TermLibrary.exe 4 1 0.01 5001");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [Library]      Lists the terms of the library you asked for, in column order, and checks how many there are.");
    printf("\n  [HandCheck]    Evaluates a degree 2 library with trig at the point (2, 3) and compares with numbers you can do on paper.");
    printf("\n  [FindTerm]     Checks the column lookup, including the answer -1 for a term the library does not have.");
    printf("\n  [LeastSquares] For all 5 systems: exact derivatives, Theta, plain least squares, then true and recovered equations side by side.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  [Library]       Term count matches the formula, and the names start 1, x, y, x^2, x*y, y^2, ...");
    printf("\n  [HandCheck]     Every line says OK.");
    printf("\n  [FindTerm]      Every line says OK.");
    printf("\n  [LeastSquares]  Recovered equation matches the true one. Small leftovers on terms that should be zero are normal: plain");
    printf("\n                  least squares has no way to say exactly zero. That is the job of the sparse regression (next step).");
    printf("\n");
    printf("\nTRY THIS");
    printf("\n  Run with MaxDegree 2 and IncludeTrig 0. Van der Pol and Duffing need x^3 terms and the pendulum needs sin(x), so the program");
    printf("\n  reports those true terms as MISSING from the library and the fit gets worse. That is the failure case: the method can only");
    printf("\n  find what the library contains.");
    printf("\n  Then run with MaxDegree 4. Lotka-Volterra now gets WORSE although all its true terms are in the library. One closed orbit is a");
    printf("\n  single curve in the plane, and a big library of high powers cannot tell its terms apart along one curve. More data that covers");
    printf("\n  more of the plane (several runs from different starts) is what helps, not a bigger library.");

    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    int MaxDegree = 3 ;
    bool IncludeTrig = true ;
    double TimeStep = 0.01 ;
    size_t SampleCount = 3001 ;
    double Ridge = 1e-10 ;
    if (argc > 1) MaxDegree   = atoi(argv[1]) ;
    if (argc > 2) IncludeTrig = (atoi(argv[2]) != 0) ;
    if (argc > 3) TimeStep    = atof(argv[3]) ;
    if (argc > 4) SampleCount = (size_t) atoll(argv[4]) ;
    if (argc > 5) Ridge       = atof(argv[5]) ;

    TermLibrary Library = CreateTermLibrary(MaxDegree, IncludeTrig) ;
    char Name[32] ;

    #if 1   //To list the terms of the library in column order and check how many there are.
    size_t ExpectedCount = (size_t)((MaxDegree + 1) * (MaxDegree + 2) / 2) + (IncludeTrig ? 4 : 0) ;
    printf("\n[Library] MaxDegree = %d, IncludeTrig = %d  ->  %zu terms (expected %zu)  %s", MaxDegree, IncludeTrig, (Library.TermCount), ExpectedCount, ((Library.TermCount) == ExpectedCount) ? "OK" : "WRONG");
    printf("\n[Library] Columns of Theta:  ");
    for (size_t ThisTerm=0; ThisTerm<(Library.TermCount); ThisTerm++)
    {
        GetTermName(&(Library.Terms[ThisTerm]), Name, sizeof(Name));
        printf("%s%s", Name, (ThisTerm+1 < (Library.TermCount)) ? ", " : "");
    }
    #endif

    #if 1   //To check Theta against numbers you can do on paper: a degree 2 library with trig, evaluated at the single point (x, y) = (2, 3).
    TermLibrary HandLibrary = CreateTermLibrary(2, 1) ;
    Matrix HandPoint = MatrixCreate(1, 3) ;     //Same layout as a trajectory: t, x, y
    SetMatrixCell(&HandPoint, 0, 0, 0.0);
    SetMatrixCell(&HandPoint, 0, 1, 2.0);
    SetMatrixCell(&HandPoint, 0, 2, 3.0);
    Matrix HandTheta = BuildTermMatrix(&HandLibrary, &HandPoint, 1, 2) ;

    //Order: 1, x, y, x^2, x*y, y^2, sin(x), cos(x), sin(y), cos(y)
    double HandExpected[10] = { 1.0, 2.0, 3.0, 4.0, 6.0, 9.0, sin(2.0), cos(2.0), sin(3.0), cos(3.0) } ;
    printf("\n[HandCheck] Theta has %zu row and %zu columns (expected 1 and 10)", (HandTheta.RowCount), (HandTheta.ColCount));
    for (size_t ThisTerm=0; ThisTerm<(HandLibrary.TermCount); ThisTerm++)
    {
        GetTermName(&(HandLibrary.Terms[ThisTerm]), Name, sizeof(Name));
        double Got = GetMatrixCell(&HandTheta, 0, ThisTerm) ;
        printf("\n[HandCheck] %-8s  got %10.6f  expected %10.6f  %s", Name, Got, HandExpected[ThisTerm], (fabs(Got - HandExpected[ThisTerm]) < 1e-12) ? "OK" : "WRONG");
    }
    MatrixDestroy(&HandTheta);
    MatrixDestroy(&HandPoint);
    DestroyTermLibrary(&HandLibrary);
    #endif

    #if 1   //To check the column lookup, including the answer -1 for a term the library does not have.
    TermLibrary SmallLibrary = CreateTermLibrary(1, 0) ;        //Only 1, x, y
    TermLibrary BigLibrary = CreateTermLibrary(3, 1) ;          //1, x, y, x^2, x*y, y^2, x^3, x^2*y, x*y^2, y^3, then trig
    int ColumnOfXy = FindTerm(&BigLibrary, TERM_POLYNOMIAL, 1, 1) ;
    int ColumnOfSinX = FindTerm(&BigLibrary, TERM_SIN_X, 0, 0) ;
    int ColumnOfXSquaredInSmall = FindTerm(&SmallLibrary, TERM_POLYNOMIAL, 2, 0) ;
    int ColumnOfSinXInSmall = FindTerm(&SmallLibrary, TERM_SIN_X, 0, 0) ;
    printf("\n[FindTerm] x*y in the degree 3 library:   column %2d (expected 4)   %s", ColumnOfXy, (ColumnOfXy == 4) ? "OK" : "WRONG");
    printf("\n[FindTerm] sin(x) in the same library:    column %2d (expected 10)  %s", ColumnOfSinX, (ColumnOfSinX == 10) ? "OK" : "WRONG");
    printf("\n[FindTerm] x^2 in the degree 1 library:   column %2d (expected -1)  %s", ColumnOfXSquaredInSmall, (ColumnOfXSquaredInSmall == -1) ? "OK" : "WRONG");
    printf("\n[FindTerm] sin(x) in the degree 1 library: column %2d (expected -1)  %s", ColumnOfSinXInSmall, (ColumnOfSinXInSmall == -1) ? "OK" : "WRONG");
    DestroyTermLibrary(&SmallLibrary);
    DestroyTermLibrary(&BigLibrary);
    #endif

    #if 1   //To recover every system with plain least squares on EXACT derivatives, and show true and recovered equations side by side.
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

        //One least squares problem per equation: Theta * coefficients = derivative
        for (size_t ThisRow=0; ThisRow<(Path.RowCount); ThisRow++) Target[ThisRow] = GetMatrixCell(&TrueDerivatives, ThisRow, 0);
        LeastSquaresSolve(&Theta, Target, Ridge, FoundDx);
        for (size_t ThisRow=0; ThisRow<(Path.RowCount); ThisRow++) Target[ThisRow] = GetMatrixCell(&TrueDerivatives, ThisRow, 1);
        LeastSquaresSolve(&Theta, Target, Ridge, FoundDy);

        //Grade: biggest miss on any coefficient, and biggest leftover on a term that should be exactly zero
        double BiggestMiss = 0.0 ;
        double BiggestLeftover = 0.0 ;
        for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
        {
            double MissX = fabs(FoundDx[ThisTerm] - TrueDx[ThisTerm]) ;
            double MissY = fabs(FoundDy[ThisTerm] - TrueDy[ThisTerm]) ;
            if (MissX > BiggestMiss) BiggestMiss = MissX;
            if (MissY > BiggestMiss) BiggestMiss = MissY;
            if (TrueDx[ThisTerm] == 0.0 && fabs(FoundDx[ThisTerm]) > BiggestLeftover) BiggestLeftover = fabs(FoundDx[ThisTerm]);
            if (TrueDy[ThisTerm] == 0.0 && fabs(FoundDy[ThisTerm]) > BiggestLeftover) BiggestLeftover = fabs(FoundDy[ThisTerm]);
        }

        printf("\n\n[LeastSquares] %s", GetSystemName(System));
        if (MissingCount > 0) printf("   (%d true term(s) MISSING from this library, so it cannot be exact)", MissingCount);
        printf("\n    true:       ");
        PrintEquation("dx/dt", &Library, TrueDx, 0.0, 3);
        printf("\n                ");
        PrintEquation("dy/dt", &Library, TrueDy, 0.0, 3);
        printf("\n    recovered:  ");
        PrintEquation("dx/dt", &Library, FoundDx, 0.0005, 3);
        printf("\n                ");
        PrintEquation("dy/dt", &Library, FoundDy, 0.0005, 3);
        printf("\n    biggest miss on any coefficient = %e,  biggest leftover on a term that should be zero = %e", BiggestMiss, BiggestLeftover);

        free(Target);
        free(FoundDy);
        free(FoundDx);
        free(TrueDy);
        free(TrueDx);
        MatrixDestroy(&Theta);
        MatrixDestroy(&TrueDerivatives);
        MatrixDestroy(&Path);
    }
    #endif

    DestroyTermLibrary(&Library);
    printf("\n");
    return 0;
}
