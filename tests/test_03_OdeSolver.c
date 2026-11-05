//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>
#include <OdeSolverAndSystems.h>
#if 0
#include "../Core/RandomNoiseGenerator.h"
#include "../Core/LinearAlgebraSolver.h"
#include "../Core/OdeSolverAndSystems.h"
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

    printf("\n[Help] test_03_OdeSolver.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Checks OdeSolverAndSystems.h. Runs the RK4 solver on the test systems and compares it with exact answers and conservation laws.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_03_OdeSolver.exe [TimeStep] [SampleCount] [NoiseSigma]");
    printf("\n  test_03_OdeSolver.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  TimeStep      Time between two recorded points (the RK4 step).             Default: 0.01");
    printf("\n  SampleCount   How many points to record. Total time = (SampleCount-1) * TimeStep.     Default: 1001 (so t = 10)");
    printf("\n  NoiseSigma    Width of the Gaussian noise for the noise check.             Default: 0.05");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_03_OdeSolver.exe");
    printf("\n  test_03_OdeSolver.exe 0.05");
    printf("\n  test_03_OdeSolver.exe 0.01 2001 0.2");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [Spiral]            Linear spiral vs its exact formula. Runs twice (step h and h/2) and prints the error ratio.");
    printf("\n  [LotkaVolterra]     Checks the conserved quantity V stays constant along the orbit.");
    printf("\n  [Pendulum]          Friction switched off. Checks the energy stays constant.");
    printf("\n  [AllSystems]        Runs all 5 systems with default settings and prints the final point, so you can spot a blow-up.");
    printf("\n  [Noise]             Adds noise to a Van der Pol path and measures its width and the time column.");
    printf("\n  [TrueDerivatives]   Exact derivatives vs a central finite difference on the clean spiral.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  [Spiral]            Ratio about 16 (RK4 error goes like h^4, so halving h gives 2^4 = 16).");
    printf("\n  [LotkaVolterra]     Drift near 0 (about 1e-10 with the defaults).");
    printf("\n  [Pendulum]          Drift near 0 (about 1e-10 with the defaults).");
    printf("\n  [AllSystems]        Every line says OK.");
    printf("\n  [Noise]             Measured sigma close to NoiseSigma, time change exactly 0.");
    printf("\n  [TrueDerivatives]   Small gap (finite difference has its own error of about h^2).");
    printf("\n");
    printf("\nTRY THIS");
    printf("\n  Run with TimeStep 0.5 and SampleCount 21 (same total time). Errors and drifts jump from about 1e-9 to about 1e-2, and the Spiral ratio slips from 16 to about 15.5. That is RK4 getting rough when steps are too big.");


    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    double TimeStep = 0.01 ;
    size_t SampleCount = 1001 ;
    double NoiseSigma = 0.05 ;
    if (argc > 1) TimeStep    = atof(argv[1]) ;
    if (argc > 2) SampleCount = (size_t) atoll(argv[2]) ;
    if (argc > 3) NoiseSigma  = atof(argv[3]) ;

    #if 1   //To compare RK4 with the exact spiral solution, and check the error drops ~16x when the step is halved.
    //Exact answer for a = d = -Alpha, b = Omega, c = -Omega:  x = e^(-Alpha t) (x0 cos(Omega t) + y0 sin(Omega t)),  y = e^(-Alpha t) (y0 cos(Omega t) - x0 sin(Omega t))
    double SpiralParams[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(SYSTEM_LINEAR_SPIRAL, SpiralParams);
    Position2D SpiralStart = GetDefaultStartState(SYSTEM_LINEAR_SPIRAL) ;
    double Alpha = -SpiralParams[0] ;
    double Omega = SpiralParams[1] ;
    double SpiralError[2] = { 0,0 } ;

    for (size_t ThisPass=0; ThisPass<2; ThisPass++)
    {
        size_t Refine = (ThisPass == 0) ? 1 : 2 ;   //Second pass uses half the step, same total time
        Matrix SpiralPath = TraceTrajectory(SYSTEM_LINEAR_SPIRAL, SpiralParams, SpiralStart, TimeStep/Refine, (SampleCount-1)*Refine + 1) ;

        for (size_t ThisSample=0; ThisSample<(SpiralPath.RowCount); ThisSample++)
        {
            double t = GetMatrixCell(&SpiralPath,ThisSample,0) ;
            double Decay = exp(-Alpha*t) ;
            double ExactX = Decay * ((SpiralStart.x)*cos(Omega*t) + (SpiralStart.y)*sin(Omega*t)) ;
            double ExactY = Decay * ((SpiralStart.y)*cos(Omega*t) - (SpiralStart.x)*sin(Omega*t)) ;
            double ErrorX = fabs(GetMatrixCell(&SpiralPath,ThisSample,1) - ExactX) ;
            double ErrorY = fabs(GetMatrixCell(&SpiralPath,ThisSample,2) - ExactY) ;
            if (ErrorX > SpiralError[ThisPass]) SpiralError[ThisPass] = ErrorX;
            if (ErrorY > SpiralError[ThisPass]) SpiralError[ThisPass] = ErrorY;
        }
        MatrixDestroy(&SpiralPath);
    }

    printf("\n[Spiral] Biggest error with step h   = %e", SpiralError[0]);
    printf("\n[Spiral] Biggest error with step h/2 = %e", SpiralError[1]);
    printf("\n[Spiral] Ratio = %f (expect ~16, because RK4 error goes like h^4)", SpiralError[0]/SpiralError[1]);
    #endif

    #if 1   //To check Lotka-Volterra keeps its conserved quantity V = d*x - c*ln(x) + b*y - a*ln(y).
    double LvParams[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(SYSTEM_LOTKA_VOLTERRA, LvParams);
    Position2D LvStart = GetDefaultStartState(SYSTEM_LOTKA_VOLTERRA) ;
    Matrix LvPath = TraceTrajectory(SYSTEM_LOTKA_VOLTERRA, LvParams, LvStart, TimeStep, SampleCount) ;

    double LvFirstV = 0.0 ;
    double LvBiggestDrift = 0.0 ;
    for (size_t ThisSample=0; ThisSample<(LvPath.RowCount); ThisSample++)
    {
        double x = GetMatrixCell(&LvPath,ThisSample,1) ;
        double y = GetMatrixCell(&LvPath,ThisSample,2) ;
        double V = LvParams[3]*x - LvParams[2]*log(x) + LvParams[1]*y - LvParams[0]*log(y) ;
        if (ThisSample == 0) LvFirstV = V;
        if (fabs(V - LvFirstV) > LvBiggestDrift) LvBiggestDrift = fabs(V - LvFirstV);
    }
    printf("\n[LotkaVolterra] Conserved V starts at %f, biggest drift = %e (expect ~0)", LvFirstV, LvBiggestDrift);
    MatrixDestroy(&LvPath);
    #endif

    #if 1   //To check an UNDAMPED pendulum keeps its energy E = y^2/2 - Freq2*cos(x).
    double PendParams[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(SYSTEM_DAMPED_PENDULUM, PendParams);
    PendParams[0] = 0.0;    //Switch the friction off
    Position2D PendStart = GetDefaultStartState(SYSTEM_DAMPED_PENDULUM) ;
    Matrix PendPath = TraceTrajectory(SYSTEM_DAMPED_PENDULUM, PendParams, PendStart, TimeStep, SampleCount) ;

    double PendFirstE = 0.0 ;
    double PendBiggestDrift = 0.0 ;
    for (size_t ThisSample=0; ThisSample<(PendPath.RowCount); ThisSample++)
    {
        double x = GetMatrixCell(&PendPath,ThisSample,1) ;
        double y = GetMatrixCell(&PendPath,ThisSample,2) ;
        double E = 0.5*y*y - PendParams[1]*cos(x) ;
        if (ThisSample == 0) PendFirstE = E;
        if (fabs(E - PendFirstE) > PendBiggestDrift) PendBiggestDrift = fabs(E - PendFirstE);
    }
    printf("\n[Pendulum] Energy starts at %f, biggest drift = %e (expect ~0)", PendFirstE, PendBiggestDrift);
    MatrixDestroy(&PendPath);
    #endif

    #if 1   //To run all five systems with default settings and make sure nothing blows up.
    for (size_t ThisSystem=0; ThisSystem<SYSTEM_COUNT; ThisSystem++)
    {
        SystemID System = (SystemID) ThisSystem ;
        double AllParams[MAX_SYSTEM_PARAMS] ;
        GetDefaultParams(System, AllParams);
        Matrix AllPath = TraceTrajectory(System, AllParams, GetDefaultStartState(System), TimeStep, SampleCount) ;

        size_t Last = (AllPath.RowCount) - 1 ;
        double LastX = GetMatrixCell(&AllPath,Last,1) ;
        double LastY = GetMatrixCell(&AllPath,Last,2) ;
        printf("\n[AllSystems] %-16s at t = %.2f:  x = %9.5f, y = %9.5f  %s", GetSystemName(System), GetMatrixCell(&AllPath,Last,0), LastX, LastY, (isfinite(LastX) && isfinite(LastY)) ? "OK" : "BLEW UP");
        MatrixDestroy(&AllPath);
    }
    #endif

    #if 1   //To add noise to a clean path and check the noise has the right width and time stays clean.
    RandomState Gen;
    RandomSeed(&Gen, 12345);
    double NoiseParams[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(SYSTEM_VAN_DER_POL, NoiseParams);
    Matrix CleanPath = TraceTrajectory(SYSTEM_VAN_DER_POL, NoiseParams, GetDefaultStartState(SYSTEM_VAN_DER_POL), TimeStep, SampleCount) ;
    Matrix NoisyPath = TraceTrajectory(SYSTEM_VAN_DER_POL, NoiseParams, GetDefaultStartState(SYSTEM_VAN_DER_POL), TimeStep, SampleCount) ;
    AddGaussianNoise(&NoisyPath, NoiseSigma, &Gen);

    double SumSquares = 0.0 ;
    double BiggestTimeChange = 0.0 ;
    for (size_t ThisSample=0; ThisSample<(CleanPath.RowCount); ThisSample++)
    {
        double DiffX = GetMatrixCell(&NoisyPath,ThisSample,1) - GetMatrixCell(&CleanPath,ThisSample,1) ;
        double DiffY = GetMatrixCell(&NoisyPath,ThisSample,2) - GetMatrixCell(&CleanPath,ThisSample,2) ;
        double DiffT = fabs(GetMatrixCell(&NoisyPath,ThisSample,0) - GetMatrixCell(&CleanPath,ThisSample,0)) ;
        SumSquares += DiffX*DiffX + DiffY*DiffY;
        if (DiffT > BiggestTimeChange) BiggestTimeChange = DiffT;
    }
    printf("\n[Noise] Measured sigma = %f (expect %f)", sqrt(SumSquares / (2.0 * (CleanPath.RowCount))), NoiseSigma);
    printf("\n[Noise] Biggest change in time column = %e (expect 0)", BiggestTimeChange);
    MatrixDestroy(&CleanPath);
    MatrixDestroy(&NoisyPath);
    #endif

    #if 1   //To check the exact derivatives against a plain finite difference on the clean spiral path.
    double DerivParams[MAX_SYSTEM_PARAMS] ;
    GetDefaultParams(SYSTEM_LINEAR_SPIRAL, DerivParams);
    Matrix DerivPath = TraceTrajectory(SYSTEM_LINEAR_SPIRAL, DerivParams, GetDefaultStartState(SYSTEM_LINEAR_SPIRAL), TimeStep, SampleCount) ;
    Matrix TrueDerivs = TrueDerivativesOfTrajectory(SYSTEM_LINEAR_SPIRAL, DerivParams, &DerivPath) ;

    //Central difference: (next - previous) / (2 h). Skip the first and last row, they have no neighbor on one side.
    double BiggestGap = 0.0 ;
    for (size_t ThisSample=1; ThisSample<(DerivPath.RowCount)-1; ThisSample++)
    {
        double FdX = (GetMatrixCell(&DerivPath,ThisSample+1,1) - GetMatrixCell(&DerivPath,ThisSample-1,1)) / (2.0*TimeStep) ;
        double FdY = (GetMatrixCell(&DerivPath,ThisSample+1,2) - GetMatrixCell(&DerivPath,ThisSample-1,2)) / (2.0*TimeStep) ;
        if (fabs(FdX - GetMatrixCell(&TrueDerivs,ThisSample,0)) > BiggestGap) BiggestGap = fabs(FdX - GetMatrixCell(&TrueDerivs,ThisSample,0));
        if (fabs(FdY - GetMatrixCell(&TrueDerivs,ThisSample,1)) > BiggestGap) BiggestGap = fabs(FdY - GetMatrixCell(&TrueDerivs,ThisSample,1));
    }
    printf("\n[TrueDerivatives] Biggest gap to finite difference = %e (expect small, ~h^2)", BiggestGap);
    MatrixDestroy(&DerivPath);
    MatrixDestroy(&TrueDerivs);
    #endif

    printf("\n");
    return 0;
}
