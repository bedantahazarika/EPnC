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
#include <NeuralDerivativeNetwork.h>
#if 0
#include "../Core/RandomNoiseGenerator.h"
#include "../Core/LinearAlgebraSolver.h"
#include "../Core/OdeSolverAndSystems.h"
#include "../Core/CandidateTermLibrary.h"
#include "../Core/SparseRegressionSTLSQ.h"
#include "../Core/GroundTruthCoefficients.h"
#include "../Core/DerivativeEstimation.h"
#include "../Core/NeuralDerivativeNetwork.h"
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

    printf("\n[Help] test_08_NoiseSweep.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Makes the BASELINE curve of the talk: how well does derivatives-from-points + STLSQ recover the true equation as the");
    printf("\n  measurement noise grows? For every system and every noise width sigma it repeats the whole pipeline many times with");
    printf("\n  different random noise: clean path, add noise, estimate derivatives from the NOISY points, build Theta from the NOISY");
    printf("\n  points, STLSQ, compare with the true equation.");
    printf("\n  Method 2 swaps in the neural curve (Week 3), so its curve can be set next to the FD and SG baselines.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_08_NoiseSweep.exe [Trials] [Method] [Threshold] [HalfWindow] [PolyOrder] [MaxDegree] [IncludeTrig] [TimeStep] [SampleCount] [Seed] [WriteCsv] [NeuralHidden] [NeuralSteps] [NeuralChunk]");
    printf("\n  test_08_NoiseSweep.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  Trials        Random noise draws per system and per sigma.                              Default: 20");
    printf("\n  Method        0 = finite differences (FD), 1 = Savitzky-Golay (SG), 2 = neural curve.  Default: 0");
    printf("\n  Threshold     STLSQ throws out coefficients smaller than this.                         Default: 0.05");
    printf("\n  HalfWindow    Savitzky-Golay uses 2*HalfWindow+1 points around each point.             Default: 10");
    printf("\n  PolyOrder     Degree of the polynomial Savitzky-Golay fits.                            Default: 3");
    printf("\n  MaxDegree     Highest total power in the library.                                      Default: 3");
    printf("\n  IncludeTrig   1 adds sin(x), cos(x), sin(y), cos(y). 0 leaves them out.                Default: 1");
    printf("\n  TimeStep      RK4 step between two recorded points.                                    Default: 0.01");
    printf("\n  SampleCount   How many points to record.                                               Default: 3001");
    printf("\n  Seed          First random seed. Trial k uses Seed + k, so a run can be repeated.      Default: 1");
    printf("\n  WriteCsv      1 writes NoiseSweep.csv (NoiseSweepNN.csv for Method 2), 0 does not.     Default: 1");
    printf("\n  NeuralHidden  Method 2 only: tanh units per network.                                   Default: 8");
    printf("\n  NeuralSteps   Method 2 only: training steps per network.                               Default: 1500");
    printf("\n  NeuralChunk   Method 2 only: rows per chunk (long paths are cut into chunks, see header).  Default: 150");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_08_NoiseSweep.exe                         (finite differences)");
    printf("\n  test_08_NoiseSweep.exe 20 1                    (Savitzky-Golay, the stronger baseline)");
    printf("\n  test_08_NoiseSweep.exe 50 1 0.05 15 3          (more trials, wider window)");
    printf("\n  test_08_NoiseSweep.exe 3 2                     (neural curve, 3 trials only, it is slow: minutes)");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild). All are ON by default.");
    printf("\n  [Sweep]  The table: for each system and each sigma, three numbers over the trials.");
    printf("\n           exact   share of trials where the set of kept terms is exactly the true set (100 means always right).");
    printf("\n           found   average share of the TRUE terms that were kept.");
    printf("\n           extra   average number of WRONG terms that were kept (should be zero).");
    printf("\n  [Csv]    Same numbers in NoiseSweep.csv: system,sigma,exact,found,extra.");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  At sigma 0 every system is 100 exact. As sigma grows the exact column falls. Finite differences amplify noise by about");
    printf("\n  1 / (sqrt(2) * TimeStep) = 70 times, so with TimeStep 0.01 even sigma 0.001 gives derivative noise of about 0.07.");
    printf("\n  Savitzky-Golay averages over the window, so it survives to bigger sigma. Neither is a bug: this is the honest baseline.");
    printf("\n  The noise levels tested are in the SigmaList array inside main.");
    printf("\n");
    printf("\nTRY THIS");
    printf("\n  Compare Method 0 and Method 1. Then try a larger HalfWindow with Method 1: more smoothing survives more noise, until the");
    printf("\n  window is so wide that it blurs the real curve, and the exact column falls again from the other side.");

    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    int Trials = 20 ;
    int Method = 0 ;
    double Threshold = 0.05 ;
    int HalfWindow = 10 ;
    int PolyOrder = 3 ;
    int MaxDegree = 3 ;
    bool IncludeTrig = true ;
    double TimeStep = 0.01 ;
    size_t SampleCount = 3001 ;
    uint64_t Seed = 1 ;
    bool WriteCsv = true ;
    size_t NeuralHidden = 8 ;
    size_t NeuralSteps = 1500 ;
    size_t NeuralChunk = 150 ;
    if (argc > 1)  Trials      = atoi(argv[1]) ;
    if (argc > 2)  Method      = atoi(argv[2]) ;
    if (argc > 3)  Threshold   = atof(argv[3]) ;
    if (argc > 4)  HalfWindow  = atoi(argv[4]) ;
    if (argc > 5)  PolyOrder   = atoi(argv[5]) ;
    if (argc > 6)  MaxDegree   = atoi(argv[6]) ;
    if (argc > 7)  IncludeTrig = (atoi(argv[7]) != 0) ;
    if (argc > 8)  TimeStep    = atof(argv[8]) ;
    if (argc > 9)  SampleCount = (size_t) atoll(argv[9]) ;
    if (argc > 10) Seed        = (uint64_t) atoll(argv[10]) ;
    if (argc > 11) WriteCsv    = (atoi(argv[11]) != 0) ;
    if (argc > 12) NeuralHidden = (size_t) atoll(argv[12]) ;
    if (argc > 13) NeuralSteps  = (size_t) atoll(argv[13]) ;
    if (argc > 14) NeuralChunk  = (size_t) atoll(argv[14]) ;

    double SigmaList[] = { 0.0, 0.0001, 0.001, 0.003, 0.01, 0.03, 0.1 } ;
    size_t SigmaCount = sizeof(SigmaList) / sizeof(SigmaList[0]) ;
    double Ridge = 1e-10 ;

    #if 1   //To run the whole noisy pipeline many times per system and per sigma, and print the baseline table.
    FILE* Csv = NULL ;
    if (WriteCsv) Csv = fopen((Method == 2) ? "NoiseSweepNN.csv" : "NoiseSweep.csv", "w");
    if (Csv) fprintf(Csv, "system,sigma,exact_percent,found_percent,extra_terms\n");

    TermLibrary Library = CreateTermLibrary(MaxDegree, IncludeTrig) ;
    size_t TermCount = (Library.TermCount) ;
    double* TrueDx = (double*) calloc(TermCount, sizeof(double)) ;
    double* TrueDy = (double*) calloc(TermCount, sizeof(double)) ;
    double* FoundDx = (double*) calloc(TermCount, sizeof(double)) ;
    double* FoundDy = (double*) calloc(TermCount, sizeof(double)) ;
    double* Target = (double*) calloc(SampleCount, sizeof(double)) ;
    RandomState Gen ;

    printf("\n[Sweep] Method = %s,  Trials = %d,  Threshold = %g,  %zu terms in the library", (Method == 0) ? "FD" : (Method == 1) ? "SG" : "NN", Trials, Threshold, TermCount);
    if (Method == 1) printf(",  SG window = %d points, order %d", 2*HalfWindow + 1, PolyOrder);
    if (Method == 2) printf(",  %zu hidden units, %zu steps, chunks of %zu rows", NeuralHidden, NeuralSteps, NeuralChunk);

    for (size_t ThisSystem=0; ThisSystem<SYSTEM_COUNT; ThisSystem++)
    {
        SystemID System = (SystemID) ThisSystem ;
        double Params[MAX_SYSTEM_PARAMS] ;
        GetDefaultParams(System, Params);
        Matrix CleanPath = TraceTrajectory(System, Params, GetDefaultStartState(System), TimeStep, SampleCount) ;
        int MissingCount = FillTrueCoefficients(System, Params, &Library, TrueDx, TrueDy) ;

        printf("\n\n[Sweep] %s", GetSystemName(System));
        if (MissingCount > 0) printf("   (%d true term(s) MISSING from this library, so it cannot be exact)", MissingCount);
        printf("\n[Sweep]   %10s  %8s  %8s  %8s", "sigma", "exact", "found", "extra");

        for (size_t ThisSigma=0; ThisSigma<SigmaCount; ThisSigma++)
        {
            int ExactCount = 0 ;
            double FoundShareSum = 0.0 ;
            double ExtraSum = 0.0 ;

            for (int ThisTrial=0; ThisTrial<Trials; ThisTrial++)
            {
                RandomSeed(&Gen, Seed + (uint64_t) ThisTrial);

                //The measurement: a fresh noisy copy of the clean path. Derivatives AND Theta both come from the noisy points only.
                Matrix NoisyPath = MatrixCreate((CleanPath.RowCount), 3) ;
                for (size_t ThisCell=0; ThisCell<(CleanPath.RowCount)*3; ThisCell++) NoisyPath.MatrixCells[ThisCell] = CleanPath.MatrixCells[ThisCell];
                AddGaussianNoise(&NoisyPath, SigmaList[ThisSigma], &Gen);

                Matrix Estimated ;
                switch (Method)
                {
                    case 1:
                        Estimated = SavitzkyGolayDerivatives(&NoisyPath, HalfWindow, PolyOrder) ;
                        break;
                    case 2:
                        Estimated = NeuralDerivativesChunked(&NoisyPath, NeuralChunk, NeuralHidden, NeuralSteps, 0.01, &Gen) ;
                        break;
                    default:
                        Estimated = FiniteDifferenceDerivatives(&NoisyPath) ;
                        break;
                }
                Matrix Theta = BuildTermMatrix(&Library, &NoisyPath, 1, 2) ;

                for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++) Target[ThisRow] = GetMatrixCell(&Estimated, ThisRow, 0);
                SparseRegressionSTLSQ(&Theta, Target, Threshold, Ridge, 10, FoundDx);
                for (size_t ThisRow=0; ThisRow<SampleCount; ThisRow++) Target[ThisRow] = GetMatrixCell(&Estimated, ThisRow, 1);
                SparseRegressionSTLSQ(&Theta, Target, Threshold, Ridge, 10, FoundDy);

                SupportReport Report = CompareWithTruth(TrueDx, TrueDy, FoundDx, FoundDy, TermCount) ;
                if (Report.FoundTrueCount == Report.TrueTermCount && Report.ExtraTermCount == 0) ExactCount++;
                FoundShareSum += (double)(Report.FoundTrueCount) / (double)(Report.TrueTermCount) ;
                ExtraSum += (double)(Report.ExtraTermCount) ;

                MatrixDestroy(&Theta);
                MatrixDestroy(&Estimated);
                MatrixDestroy(&NoisyPath);
            }

            double ExactPercent = 100.0 * (double) ExactCount / (double) Trials ;
            double FoundPercent = 100.0 * FoundShareSum / (double) Trials ;
            double ExtraAverage = ExtraSum / (double) Trials ;
            printf("\n[Sweep]   %10g  %7.1f%%  %7.1f%%  %8.2f", SigmaList[ThisSigma], ExactPercent, FoundPercent, ExtraAverage);
            if (Csv) fprintf(Csv, "%s,%g,%.2f,%.2f,%.3f\n", GetSystemName(System), SigmaList[ThisSigma], ExactPercent, FoundPercent, ExtraAverage);
        }

        MatrixDestroy(&CleanPath);
    }

    free(Target);
    free(FoundDy);
    free(FoundDx);
    free(TrueDy);
    free(TrueDx);
    DestroyTermLibrary(&Library);
    if (Csv)
    {
        fclose(Csv);
        printf("\n\n[Csv] Wrote %s", (Method == 2) ? "NoiseSweepNN.csv" : "NoiseSweep.csv");
    }
    #endif

    printf("\n");
    return 0;
}
