//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
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

    printf("\n[Help] test_09_GuidedTour.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  A slow motion tour of the whole pipeline. Every chapter does ONE job with tiny numbers, writes the loops out by hand,");
    printf("\n  shows the first 3 iterations with every multiplication visible, then calls the REAL function from the headers and");
    printf("\n  checks that both agree (MATCH). So you see what is under the hood, and you see the headers are doing the same thing.");
    printf("\n  It pauses after each step: press Enter to go on, or type q and Enter to stop pausing and let the rest scroll by.");
    printf("\n");
    printf("\nCHAPTERS");
    printf("\n  1  RK4: how one time step of the ODE solver is built from K1, K2, K3, K4 (3 steps shown).");
    printf("\n  2  Noise: clean value plus a Gaussian random number gives the fake measurement (3 rows shown).");
    printf("\n  3  Derivatives from points: the finite difference formulas row by row, on clean and on noisy points.");
    printf("\n  4  The term matrix Theta: every cell is x^a * y^b, built row by row (3 rows shown).");
    printf("\n  5  Least squares: how Theta^T Theta and Theta^T b are summed up, then solved. Plain least squares leaves tiny leftovers.");
    printf("\n  6  STLSQ: fit, cut the small coefficients, refit, round by round. Ends with the true equation.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_09_GuidedTour.exe [Chapter] [Pause] [TimeStep] [Sigma] [Seed]");
    printf("\n  test_09_GuidedTour.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  Chapter    0 runs all chapters one after another, 1 to 6 runs just that one.            Default: 0");
    printf("\n  Pause      1 waits for Enter at every step, 0 never waits.                              Default: 1");
    printf("\n  TimeStep   Time between recorded points. Big on purpose so the numbers are easy to read. Default: 0.1");
    printf("\n  Sigma      Width of the measurement noise used in chapters 2, 3 and 6.                  Default: 0.002");
    printf("\n  Seed       Random seed, same seed gives the same noise.                                 Default: 1");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_09_GuidedTour.exe                    (everything, with pauses)");
    printf("\n  test_09_GuidedTour.exe 4                  (only the term matrix chapter)");
    printf("\n  test_09_GuidedTour.exe 0 0                (everything, no pauses, good for output redirection)");
    printf("\n  test_09_GuidedTour.exe 6 1 0.1 0.01       (STLSQ with more noise, watch which terms get cut)");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  Every check line says MATCH. The system used is the Linear Spiral (the same one used in the other tests).");
    printf("\n");
    printf("\nTRY THIS");
    printf("\n  Run chapter 3 with Sigma 0 and then with Sigma 0.02: the derivative estimate of the noisy points is far off,");
    printf("\n  because a small difference of two noisy numbers is divided by a small time step. That is the whole reason for the neural stage.");

    printf("\n");
    return true;
}



/// @brief Stops and waits for the person to press Enter. Typing `q` and Enter turns all later pauses off.
/// @param PauseOn Address of the pause switch. `true` = wait, `false` = do not wait. Set to `false` by `q` or by end of input.
void Pause (bool* PauseOn)
{
    if (!(*PauseOn)) return;

    printf("\n   >>> Enter = next step,  q + Enter = stop pausing ");
    fflush(stdout);
    int Key = getchar() ;
    if (Key == 'q' || Key == 'Q' || Key == EOF) (*PauseOn) = false;
    while (Key != '\n' && Key != EOF) Key = getchar();
    printf("\n");
    return;
}



/// @brief Prints a chapter title with a line around it.
/// @param Number Chapter number.
/// @param Title Chapter title text.
void PrintChapterTitle (int Number, const char* Title)
{
    printf("\n\n==============================================================================");
    printf("\n CHAPTER %d: %s", Number, Title);
    printf("\n==============================================================================\n");
    return;
}



/// @brief Chapter 1. One RK4 time step written out as K1, K2, K3, K4, three times in a row (the loop inside `TraceTrajectory`).
/// @param PauseOn Address of the pause switch.
/// @param TimeStep The time step `h`.
void ChapterRk4 (bool* PauseOn, double TimeStep)
{
    PrintChapterTitle(1, "RK4, one small time step at a time");
    SystemID System = SYSTEM_LINEAR_SPIRAL ;
    double Params[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(System, Params);
    Position2D State = GetDefaultStartState(System) ;

    printf("System: %s.  We know the slope (dx/dt, dy/dt) at any point, but we want to know where we are one step LATER.", GetSystemName(System));
    printf("\nOne straight step along the slope is too crude, so RK4 samples the slope 4 times and mixes them:");
    printf("\n   K1 = slope here");
    printf("\n   K2 = slope at the middle of the step, if we had followed K1 for half a step");
    printf("\n   K3 = slope at the middle again, but following K2 (a better guess)");
    printf("\n   K4 = slope at the end, if we had followed K3 for a whole step");
    printf("\n   new = old + h/6 * (K1 + 2 K2 + 2 K3 + K4)         h = %g", TimeStep);
    Pause(PauseOn);

    for (int ThisStep=0; ThisStep<3; ThisStep++)
    {
        printf("---- loop iteration %d: time t = %g, we stand at x = %.6f, y = %.6f ----\n", ThisStep, ThisStep*TimeStep, (State.x), (State.y));

        Slope2D K1 = EvaluateSystem(System, State, Params) ;
        printf("K1 = slope at (%.6f, %.6f)              = (%.6f, %.6f)\n", (State.x), (State.y), (K1.dx_by_dt), (K1.dy_by_dt));

        Position2D Probe2 = { (State.x) + 0.5*TimeStep*(K1.dx_by_dt), (State.y) + 0.5*TimeStep*(K1.dy_by_dt) } ;
        Slope2D K2 = EvaluateSystem(System, Probe2, Params) ;
        printf("K2 = slope at (x + h/2 K1x, y + h/2 K1y) = slope at (%.6f, %.6f) = (%.6f, %.6f)\n", (Probe2.x), (Probe2.y), (K2.dx_by_dt), (K2.dy_by_dt));

        Position2D Probe3 = { (State.x) + 0.5*TimeStep*(K2.dx_by_dt), (State.y) + 0.5*TimeStep*(K2.dy_by_dt) } ;
        Slope2D K3 = EvaluateSystem(System, Probe3, Params) ;
        printf("K3 = slope at (x + h/2 K2x, y + h/2 K2y) = slope at (%.6f, %.6f) = (%.6f, %.6f)\n", (Probe3.x), (Probe3.y), (K3.dx_by_dt), (K3.dy_by_dt));

        Position2D Probe4 = { (State.x) + TimeStep*(K3.dx_by_dt), (State.y) + TimeStep*(K3.dy_by_dt) } ;
        Slope2D K4 = EvaluateSystem(System, Probe4, Params) ;
        printf("K4 = slope at (x + h K3x, y + h K3y)     = slope at (%.6f, %.6f) = (%.6f, %.6f)\n", (Probe4.x), (Probe4.y), (K4.dx_by_dt), (K4.dy_by_dt));

        double MixX = (K1.dx_by_dt) + 2.0*(K2.dx_by_dt) + 2.0*(K3.dx_by_dt) + (K4.dx_by_dt) ;
        double MixY = (K1.dy_by_dt) + 2.0*(K2.dy_by_dt) + 2.0*(K3.dy_by_dt) + (K4.dy_by_dt) ;
        double NewX = (State.x) + TimeStep/6.0 * MixX ;
        double NewY = (State.y) + TimeStep/6.0 * MixY ;
        printf("mix x = K1 + 2 K2 + 2 K3 + K4 = %.6f,   new x = %.6f + %g/6 * %.6f = %.6f\n", MixX, (State.x), TimeStep, MixX, NewX);
        printf("mix y = K1 + 2 K2 + 2 K3 + K4 = %.6f,   new y = %.6f + %g/6 * %.6f = %.6f\n", MixY, (State.y), TimeStep, MixY, NewY);

        Position2D Real = State ;
        Rk4Step(System, Params, &Real, TimeStep);
        printf("Real Rk4Step gives (%.6f, %.6f)   %s\n", (Real.x), (Real.y), (fabs(Real.x - NewX) < 1e-12 && fabs(Real.y - NewY) < 1e-12) ? "MATCH" : "DIFFERENT");

        (State.x) = NewX;
        (State.y) = NewY;
        Pause(PauseOn);
    }
    return;
}



/// @brief Chapter 2. Clean value plus a Gaussian random number gives the noisy measurement, for the first 3 rows.
/// @param PauseOn Address of the pause switch.
/// @param TimeStep Time step of the path.
/// @param Sigma Noise width.
/// @param Seed Random seed.
void ChapterNoise (bool* PauseOn, double TimeStep, double Sigma, uint64_t Seed)
{
    PrintChapterTitle(2, "Noise: making a fake measurement");
    SystemID System = SYSTEM_LINEAR_SPIRAL ;
    double Params[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(System, Params);
    Matrix Clean = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, 6) ;

    printf("A real instrument never reads the exact value. We imitate that: measured = clean + a random number from a bell curve");
    printf("\nwith width sigma = %g (most random numbers are within +-sigma, a few within +-2 sigma).", Sigma);
    printf("\nTime is left alone (we assume a perfect clock). Each row uses two random numbers, one for x and one for y.");
    Pause(PauseOn);

    RandomState Gen ;
    RandomSeed(&Gen, Seed);
    Matrix Noisy = MatrixCreate(6, 3) ;
    for (int ThisRow=0; ThisRow<3; ThisRow++)
    {
        double DrawX = RandomGaussian(&Gen, 0.0, Sigma) ;
        double DrawY = RandomGaussian(&Gen, 0.0, Sigma) ;
        double CleanX = GetMatrixCell(&Clean,(size_t)ThisRow,1) ;
        double CleanY = GetMatrixCell(&Clean,(size_t)ThisRow,2) ;
        printf("---- loop iteration (row) %d ----\n", ThisRow);
        printf("x: clean %.6f + random %+.6f = noisy %.6f\n", CleanX, DrawX, CleanX + DrawX);
        printf("y: clean %.6f + random %+.6f = noisy %.6f\n", CleanY, DrawY, CleanY + DrawY);
        SetMatrixCell(&Noisy, (size_t)ThisRow, 1, CleanX + DrawX);
        SetMatrixCell(&Noisy, (size_t)ThisRow, 2, CleanY + DrawY);
        Pause(PauseOn);
    }

    //Real function, same seed, so it must draw the same random numbers in the same order
    Matrix Real = MatrixCreate(6, 3) ;
    for (size_t ThisCell=0; ThisCell<18; ThisCell++) Real.MatrixCells[ThisCell] = Clean.MatrixCells[ThisCell];
    RandomSeed(&Gen, Seed);
    AddGaussianNoise(&Real, Sigma, &Gen);
    double Worst = 0.0 ;
    for (size_t ThisRow=0; ThisRow<3; ThisRow++)
    {
        Worst = fmax(Worst, fabs(GetMatrixCell(&Real,ThisRow,1) - GetMatrixCell(&Noisy,ThisRow,1)));
        Worst = fmax(Worst, fabs(GetMatrixCell(&Real,ThisRow,2) - GetMatrixCell(&Noisy,ThisRow,2)));
    }
    printf("Real AddGaussianNoise with the same seed, first 3 rows differ from ours by at most %e   %s\n", Worst, (Worst < 1e-15) ? "MATCH" : "DIFFERENT");

    MatrixDestroy(&Real);
    MatrixDestroy(&Noisy);
    MatrixDestroy(&Clean);
    return;
}



/// @brief Chapter 3. The finite difference formulas written out for rows 0, 1, 2 (x only), then the same on noisy points.
/// @param PauseOn Address of the pause switch.
/// @param TimeStep Time step of the path.
/// @param Sigma Noise width.
/// @param Seed Random seed.
void ChapterDerivatives (bool* PauseOn, double TimeStep, double Sigma, uint64_t Seed)
{
    PrintChapterTitle(3, "Derivatives from points");
    SystemID System = SYSTEM_LINEAR_SPIRAL ;
    double Params[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(System, Params);
    Matrix Clean = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, 8) ;
    Matrix Exact = TrueDerivativesOfTrajectory(System, Params, &Clean) ;
    RandomState Gen ;
    RandomSeed(&Gen, Seed);
    Matrix Noisy = MatrixCreate(8, 3) ;
    for (size_t ThisCell=0; ThisCell<24; ThisCell++) Noisy.MatrixCells[ThisCell] = Clean.MatrixCells[ThisCell];
    AddGaussianNoise(&Noisy, Sigma, &Gen);

    printf("We only have the recorded points. The slope at a point is estimated from its neighbours (h = %g):", TimeStep);
    printf("\n   inside the data:  slope = (next - previous) / (2 h)                     (central difference)");
    printf("\n   first row:        slope = (-3 first + 4 second - third) / (2 h)         (only one side exists)");
    printf("\nShown for x. y works the same way.");
    Pause(PauseOn);

    for (int ThisPass=0; ThisPass<2; ThisPass++)
    {
        Matrix* Points = (ThisPass == 0) ? &Clean : &Noisy ;
        printf("######## %s points ########\n", (ThisPass == 0) ? "CLEAN" : "NOISY");
        for (int ThisRow=0; ThisRow<3; ThisRow++)
        {
            double Slope ;
            printf("---- loop iteration (row) %d ----\n", ThisRow);
            if (ThisRow == 0)
            {
                double f0 = GetMatrixCell(Points,0,1), f1 = GetMatrixCell(Points,1,1), f2 = GetMatrixCell(Points,2,1) ;
                Slope = (-3.0*f0 + 4.0*f1 - f2) / (2.0*TimeStep) ;
                printf("first row: (-3 * %.6f + 4 * %.6f - %.6f) / (2 * %g) = %.6f\n", f0, f1, f2, TimeStep, Slope);
            } else
            {
                double Previous = GetMatrixCell(Points,(size_t)ThisRow-1,1), Next = GetMatrixCell(Points,(size_t)ThisRow+1,1) ;
                Slope = (Next - Previous) / (2.0*TimeStep) ;
                printf("inside:    (%.6f - %.6f) / (2 * %g) = %.6f\n", Next, Previous, TimeStep, Slope);
            }
            double Truth = GetMatrixCell(&Exact,(size_t)ThisRow,0) ;
            printf("true dx/dt from the equation = %.6f,   our estimate is off by %+.6f\n", Truth, Slope - Truth);
            Pause(PauseOn);
        }

        Matrix Fd = FiniteDifferenceDerivatives(Points) ;
        double Worst = 0.0 ;
        //Re-do the three x slopes quickly to compare with the real function
        Worst = fmax(Worst, fabs(GetMatrixCell(&Fd,0,0) - (-3.0*GetMatrixCell(Points,0,1) + 4.0*GetMatrixCell(Points,1,1) - GetMatrixCell(Points,2,1))/(2.0*TimeStep)));
        Worst = fmax(Worst, fabs(GetMatrixCell(&Fd,1,0) - (GetMatrixCell(Points,2,1) - GetMatrixCell(Points,0,1))/(2.0*TimeStep)));
        Worst = fmax(Worst, fabs(GetMatrixCell(&Fd,2,0) - (GetMatrixCell(Points,3,1) - GetMatrixCell(Points,1,1))/(2.0*TimeStep)));
        printf("Real FiniteDifferenceDerivatives, first 3 rows of dx/dt, differ from ours by at most %e   %s\n", Worst, (Worst < 1e-12) ? "MATCH" : "DIFFERENT");
        MatrixDestroy(&Fd);
        if (ThisPass == 0) printf("On clean points the error is tiny (it comes only from the formula, about h^2).\n");
        else printf("On noisy points the error is about sigma / (sqrt(2) h) = %g / %.3f = %.4f, MUCH bigger than the clean error.\n", Sigma, sqrt(2.0)*TimeStep, Sigma/(sqrt(2.0)*TimeStep));
        Pause(PauseOn);
    }

    MatrixDestroy(&Noisy);
    MatrixDestroy(&Exact);
    MatrixDestroy(&Clean);
    return;
}



/// @brief Chapter 4. The term matrix Theta, cell by cell for the first 3 rows (library of degree 2, no trig).
/// @param PauseOn Address of the pause switch.
/// @param TimeStep Time step of the path.
void ChapterThetaMatrix (bool* PauseOn, double TimeStep)
{
    PrintChapterTitle(4, "The term matrix Theta");
    SystemID System = SYSTEM_LINEAR_SPIRAL ;
    double Params[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(System, Params);
    Matrix Path = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, 6) ;
    TermLibrary Library = CreateTermLibrary(2, 0) ;
    Matrix Theta = BuildTermMatrix(&Library, &Path, 1, 2) ;

    printf("We do not know which terms are in the equation, so we write down a menu of candidate terms and let the regression pick.");
    printf("\nMenu (degree 2, no trig), %zu terms: ", (Library.TermCount));
    for (size_t ThisTerm=0; ThisTerm<(Library.TermCount); ThisTerm++)
    {
        char Name[32] ;
        GetTermName(&(Library.Terms[ThisTerm]), Name, sizeof(Name));
        printf("%s%s", Name, (ThisTerm+1 < (Library.TermCount)) ? ",  " : "");
    }
    printf("\nTheta has one ROW per recorded point and one COLUMN per term. Cell (row, term) = the term evaluated at that row's (x, y).");
    Pause(PauseOn);

    double Worst = 0.0 ;
    for (size_t ThisRow=0; ThisRow<3; ThisRow++)
    {
        double x = GetMatrixCell(&Path,ThisRow,1), y = GetMatrixCell(&Path,ThisRow,2) ;
        printf("---- loop iteration (row) %zu: point (x, y) = (%.6f, %.6f) ----\n", ThisRow, x, y);
        for (size_t ThisTerm=0; ThisTerm<(Library.TermCount); ThisTerm++)
        {
            Term* TheTerm = &(Library.Terms[ThisTerm]) ;
            char Name[32] ;
            GetTermName(TheTerm, Name, sizeof(Name));
            double ByHand = pow(x, (double)(TheTerm->PowerX)) * pow(y, (double)(TheTerm->PowerY)) ;
            printf("  column %zu  %-5s = x^%d * y^%d = %.6f^%d * %.6f^%d = %.6f\n", ThisTerm, Name, (TheTerm->PowerX), (TheTerm->PowerY), x, (TheTerm->PowerX), y, (TheTerm->PowerY), ByHand);
            Worst = fmax(Worst, fabs(ByHand - GetMatrixCell(&Theta,ThisRow,ThisTerm)));
        }
        Pause(PauseOn);
    }
    printf("Real BuildTermMatrix, first 3 rows, differs from ours by at most %e   %s\n", Worst, (Worst < 1e-12) ? "MATCH" : "DIFFERENT");
    printf("(Anything to the power 0 is 1, so the first column is all ones: that is the constant term.)\n");

    MatrixDestroy(&Theta);
    DestroyTermLibrary(&Library);
    MatrixDestroy(&Path);
    return;
}



/// @brief Chapter 5. Least squares: the sums that make `Theta^T Theta` and `Theta^T b`, then the solve, on exact derivatives.
/// @param PauseOn Address of the pause switch.
/// @param TimeStep Time step of the path.
void ChapterLeastSquares (bool* PauseOn, double TimeStep)
{
    PrintChapterTitle(5, "Least squares: plain fit of all terms");
    SystemID System = SYSTEM_LINEAR_SPIRAL ;
    double Params[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(System, Params);
    size_t SampleCount = 60 ;
    Matrix Path = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, SampleCount) ;
    Matrix Exact = TrueDerivativesOfTrajectory(System, Params, &Path) ;
    TermLibrary Library = CreateTermLibrary(2, 0) ;
    Matrix Theta = BuildTermMatrix(&Library, &Path, 1, 2) ;
    size_t TermCount = (Library.TermCount) ;
    double* Target = (double*) calloc(SampleCount, sizeof(double)) ;
    for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++) Target[ThisRow] = GetMatrixCell(&Exact, ThisRow, 0);

    printf("Goal: find numbers c so that  Theta * c  is as close as possible to the derivative b = dx/dt  (here %zu rows, %zu terms).", SampleCount, TermCount);
    printf("\n'As close as possible' means: smallest sum of squared misses. Calculus gives the 'normal equations':");
    printf("\n   (Theta^T Theta) c = Theta^T b");
    printf("\nTheta^T Theta is a small square table (%zu x %zu). Its cell (a, b) is the sum over ALL rows of  Theta[row][a] * Theta[row][b].", TermCount, TermCount);
    Pause(PauseOn);

    Matrix ThetaT = TransposeMatrix(&Theta) ;
    Matrix ThetaTTheta = MultiplyMatrices(&ThetaT, &Theta) ;
    size_t A = 1, B = 2 ;
    char NameA[32], NameB[32] ;
    GetTermName(&(Library.Terms[A]), NameA, sizeof(NameA));
    GetTermName(&(Library.Terms[B]), NameB, sizeof(NameB));
    printf("Take cell (%s, %s). The loop over rows, first 3 iterations:\n", NameA, NameB);
    double RunningSum = 0.0 ;
    for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++)
    {
        double Product = GetMatrixCell(&Theta,ThisRow,A) * GetMatrixCell(&Theta,ThisRow,B) ;
        RunningSum += Product;
        if (ThisRow < 3) printf("  row %zu: %.6f * %.6f = %+.6f     running sum = %+.6f\n", ThisRow, GetMatrixCell(&Theta,ThisRow,A), GetMatrixCell(&Theta,ThisRow,B), Product, RunningSum);
        if (ThisRow == 3) printf("  ... (%zu more rows) ...\n", SampleCount - 3);
    }
    printf("total = %.6f, the real MultiplyMatrices gives %.6f   %s\n", RunningSum, GetMatrixCell(&ThetaTTheta,A,B), (fabs(RunningSum - GetMatrixCell(&ThetaTTheta,A,B)) < 1e-9) ? "MATCH" : "DIFFERENT");
    Pause(PauseOn);

    printf("The whole Theta^T Theta table:\n");
    for (size_t ThisRow=0; ThisRow<TermCount; ThisRow++)
    {
        printf("  ");
        for (size_t ThisCol=0; ThisCol<TermCount; ThisCol++) printf("%10.4f ", GetMatrixCell(&ThetaTTheta,ThisRow,ThisCol));
        printf("\n");
    }
    printf("It is symmetric (cell (a,b) = cell (b,a)), because the product Theta[row][a] * Theta[row][b] does not care about the order.\n");
    Pause(PauseOn);

    printf("Solving the %zu x %zu system (this is the Gauss elimination you wrote in LinearAlgebraSolver.h):\n", TermCount, TermCount);
    double* Coefficients = (double*) calloc(TermCount, sizeof(double)) ;
    LeastSquaresSolve(&Theta, Target, 1e-10, Coefficients);
    printf("\n  term     found c        true c\n");
    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
    {
        char Name[32] ;
        GetTermName(&(Library.Terms[ThisTerm]), Name, sizeof(Name));
        double TrueValue = (Library.Terms[ThisTerm].PowerX == 1 && Library.Terms[ThisTerm].PowerY == 0) ? -0.1 : ((Library.Terms[ThisTerm].PowerX == 0 && Library.Terms[ThisTerm].PowerY == 1) ? 2.0 : 0.0) ;
        printf("  %-5s %+12.3e   %+10.3f\n", Name, Coefficients[ThisTerm], TrueValue);
    }
    printf("\nThe two true terms come out right. The others should be exactly 0, but plain least squares leaves tiny leftovers (rounding,");
    printf("\nand the ridge safety number). With noise the leftovers get much bigger. That is why the next chapter cuts them off.\n");

    free(Coefficients);
    MatrixDestroy(&ThetaTTheta);
    MatrixDestroy(&ThetaT);
    free(Target);
    MatrixDestroy(&Theta);
    DestroyTermLibrary(&Library);
    MatrixDestroy(&Exact);
    MatrixDestroy(&Path);
    return;
}



/// @brief Chapter 6. STLSQ round by round on NOISY data: fit, cut the small coefficients, refit.
/// @param PauseOn Address of the pause switch.
/// @param TimeStep Time step of the path.
/// @param Sigma Noise width.
/// @param Seed Random seed.
void ChapterStlsq (bool* PauseOn, double TimeStep, double Sigma, uint64_t Seed)
{
    PrintChapterTitle(6, "STLSQ: fit, cut, refit");
    SystemID System = SYSTEM_LINEAR_SPIRAL ;
    double Params[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(System, Params);
    size_t SampleCount = 60 ;
    double Threshold = 0.05 ;
    Matrix Path = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, SampleCount) ;
    RandomState Gen ;
    RandomSeed(&Gen, Seed);
    AddGaussianNoise(&Path, Sigma, &Gen);
    Matrix Estimated = FiniteDifferenceDerivatives(&Path) ;
    TermLibrary Library = CreateTermLibrary(2, 0) ;
    Matrix Theta = BuildTermMatrix(&Library, &Path, 1, 2) ;
    size_t TermCount = (Library.TermCount) ;
    double* Target = (double*) calloc(SampleCount, sizeof(double)) ;
    for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++) Target[ThisRow] = GetMatrixCell(&Estimated, ThisRow, 0);
    double* Coefficients = (double*) calloc(TermCount, sizeof(double)) ;
    bool* IsActive = (bool*) calloc(TermCount, sizeof(bool)) ;
    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++) IsActive[ThisTerm] = true ;
    size_t ActiveCount = TermCount ;

    printf("Data: Linear Spiral, noise sigma = %g, derivatives from finite differences, threshold = %g.", Sigma, Threshold);
    printf("\nTrue equation:  dx/dt = -0.100x + 2.000y   (only 2 of the %zu menu terms are real).", TermCount);
    printf("\nSTLSQ: (1) fit every term, (2) any coefficient smaller than the threshold is cut to exactly 0 and its column is forgotten,");
    printf("\n(3) refit only the survivors, (4) repeat until nothing more is cut.");
    Pause(PauseOn);

    FitActiveColumns(&Theta, Target, IsActive, ActiveCount, 1e-10, Coefficients);
    for (int ThisRound=0; ThisRound<10; ThisRound++)
    {
        printf("---- fit number %d, using %zu terms ----\n", ThisRound, ActiveCount);
        size_t CutCount = 0 ;
        for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
        {
            char Name[32] ;
            GetTermName(&(Library.Terms[ThisTerm]), Name, sizeof(Name));
            if (IsActive[ThisTerm] == 0) { printf("  %-5s  (already cut)\n", Name); continue; }
            bool Cut = (fabs(Coefficients[ThisTerm]) < Threshold) ;
            printf("  %-5s  c = %+10.6f   |c| %s %g   %s\n", Name, Coefficients[ThisTerm], Cut ? "<" : ">=", Threshold, Cut ? "-> CUT" : "-> keep");
            if (Cut) { IsActive[ThisTerm] = false; Coefficients[ThisTerm] = 0.0; CutCount++; }
        }
        if (CutCount == 0) { printf("Nobody was cut, so we are done.\n"); break; }
        ActiveCount -= CutCount;
        if (ActiveCount == 0) { printf("Everything was cut, answer is all zeros.\n"); break; }
        printf("Cut %zu term(s). Refit with the %zu survivors (their values shift a little):\n", CutCount, ActiveCount);
        Pause(PauseOn);
        FitActiveColumns(&Theta, Target, IsActive, ActiveCount, 1e-10, Coefficients);
    }

    double Real[6] ;
    SparseRegressionSTLSQ(&Theta, Target, Threshold, 1e-10, 10, Real);
    double Worst = 0.0 ;
    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++) Worst = fmax(Worst, fabs(Real[ThisTerm] - Coefficients[ThisTerm]));
    printf("\nOur result:   ");
    PrintEquation("dx/dt", &Library, Coefficients, 0.0, 3);
    printf("\nReal SparseRegressionSTLSQ differs from ours by at most %e   %s\n", Worst, (Worst < 1e-12) ? "MATCH" : "DIFFERENT");
    printf("True:         dx/dt = -0.100x + 2.000y\n");

    free(IsActive);
    free(Coefficients);
    free(Target);
    MatrixDestroy(&Theta);
    DestroyTermLibrary(&Library);
    MatrixDestroy(&Estimated);
    MatrixDestroy(&Path);
    return;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    int Chapter = 0 ;
    bool PauseOn = true ;
    double TimeStep = 0.1 ;
    double Sigma = 0.002 ;
    uint64_t Seed = 1 ;
    if (argc > 1) Chapter  = atoi(argv[1]) ;
    if (argc > 2) PauseOn  = (atoi(argv[2]) != 0) ;
    if (argc > 3) TimeStep = atof(argv[3]) ;
    if (argc > 4) Sigma    = atof(argv[4]) ;
    if (argc > 5) Seed     = (uint64_t) atoll(argv[5]) ;

    #if 1   //To run the chapters of the guided tour, one after another or just the chosen one.
    if (Chapter == 0 || Chapter == 1) ChapterRk4(&PauseOn, TimeStep);
    if (Chapter == 0 || Chapter == 2) ChapterNoise(&PauseOn, TimeStep, Sigma, Seed);
    if (Chapter == 0 || Chapter == 3) ChapterDerivatives(&PauseOn, TimeStep, Sigma, Seed);
    if (Chapter == 0 || Chapter == 4) ChapterThetaMatrix(&PauseOn, TimeStep);
    if (Chapter == 0 || Chapter == 5) ChapterLeastSquares(&PauseOn, TimeStep);
    if (Chapter == 0 || Chapter == 6) ChapterStlsq(&PauseOn, TimeStep, Sigma, Seed);
    #endif

    printf("\n");
    return 0;
}
