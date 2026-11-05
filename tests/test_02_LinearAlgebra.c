//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>
#if 0
#include "../Core/RandomNoiseGenerator.h"
#include "../Core/LinearAlgebraSolver.h"
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

    printf("\n[Help] test_02_LinearAlgebra.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Checks LinearAlgebraSolver.h: solving a 3x3 system exactly, and fitting a straight line to noisy points (least squares).");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_02_LinearAlgebra.exe [NoiseSigma] [PointCount]");
    printf("\n  test_02_LinearAlgebra.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  NoiseSigma   Width of the noise added to the line points.   Default: 0.1");
    printf("\n  PointCount   How many noisy points to fit.                  Default: 200");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_02_LinearAlgebra.exe");
    printf("\n  test_02_LinearAlgebra.exe 0.5");
    printf("\n  test_02_LinearAlgebra.exe 0.5 1000");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild)");
    printf("\n  [SolveLinearSystem] block 1   ON. Exact 3x3 that needs row swapping. NOT diagonally dominant, so Gauss-Seidel can fail here.");
    printf("\n  [SolveLinearSystem] block 2   ON. Diagonally dominant 3x3. Safe for both methods.");
    printf("\n  [LeastSquaresSolve]           ON. Fits y = 1 + 2t to noisy points.");
    printf("\n");
    printf("\nSWITCHING THE SOLVER");
    printf("\n  The method (Gauss Elimination or Gauss-Seidel) is chosen by the #if / #elif chain inside SolveLinearSystem in Core/LinearAlgebraSolver.h. Flip it there and rebuild.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  Both 3x3 blocks print x = 2, y = 3, z = -1.");
    printf("\n  The line fit prints intercept about 1 and slope about 2. Bigger NoiseSigma or fewer points means a bigger miss.");


    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    double NoiseSigma = 0.1 ;
    size_t PointCount = 200 ;
    if (argc > 1) NoiseSigma = atof(argv[1]) ;
    if (argc > 2) PointCount = (size_t) atoll(argv[2]) ;

    #if 1   //To solve an exact 3x3 that needs row swapping (NOT diagonally dominant, Gauss-Seidel can fail here).
    //Exact 3x3 system. Answer should be x = 2, y = 3, z = -1
    Matrix A = MatrixCreate(3,3) ;
    double Values[9] = { 2,1,-1,  -3,-1,2,  -2,1,2 } ;
    double b[3] = { 8,-11,-3 } ;
    double x[3] = { 0,0,0 } ;
    for (size_t ThisNum=0; ThisNum<9; ThisNum++) A.MatrixCells[ThisNum] = Values[ThisNum];

    SolveLinearSystem(&A, b, x);
    printf("\n[SolveLinearSystem] x = %f, %f, %f (expect 2, 3, -1)", x[0], x[1], x[2]);
    MatrixDestroy(&A);
    #endif

    #if 1   //To solve a diagonally dominant 3x3 (safe for Gauss-Seidel too). Answer should be x = 2, y = 3, z = -1
    Matrix Dominant = MatrixCreate(3,3) ;
    double DominantValues[9] = { 4,1,-1,  2,5,1,  -1,1,3 } ;
    double DominantB[3] = { 12,18,-2 } ;
    double DominantX[3] = { 0,0,0 } ;
    for (size_t ThisNum=0; ThisNum<9; ThisNum++) Dominant.MatrixCells[ThisNum] = DominantValues[ThisNum];

    SolveLinearSystem(&Dominant, DominantB, DominantX);
    printf("\n[SolveLinearSystem] x = %f, %f, %f (expect 2, 3, -1)", DominantX[0], DominantX[1], DominantX[2]);
    MatrixDestroy(&Dominant);
    #endif

    #if 1   //To fit a line to noisy points with least squares.
    //Fit a line y = 1 + 2t to noisy points. Columns of the matrix are [1, t]. Expect intercept ~1, slope ~2
    RandomState Gen;
    RandomSeed(&Gen, 12345);

    Matrix Design = MatrixCreate(PointCount, 2) ;
    double* Target = (double*) calloc(PointCount, sizeof(double)) ;
    double Answer[2] = { 0,0 } ;

    for (size_t ThisPoint=0; ThisPoint<PointCount; ThisPoint++)
    {
        double t = 0.1 * ThisPoint ;
        SetMatrixCell(&Design, ThisPoint, 0, 1.0);
        SetMatrixCell(&Design, ThisPoint, 1, t);
        Target[ThisPoint] = 1.0 + 2.0*t + RandomGaussian(&Gen, 0.0, NoiseSigma) ;
    }

    LeastSquaresSolve(&Design, Target, 0.0, Answer);
    printf("\n[LeastSquaresSolve] intercept = %f (expect 1), slope = %f (expect 2)", Answer[0], Answer[1]);
    free(Target);
    MatrixDestroy(&Design);
    #endif

    printf("\n");
    return 0;
}
