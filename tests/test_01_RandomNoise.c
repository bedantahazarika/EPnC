//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <RandomNoiseGenerator.h>
#if 0
#include "../Core/RandomNoiseGenerator.h"
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

    printf("\n[Help] test_01_RandomNoise.exe");
    printf("\n");
    printf("\nWHAT IT DOES");
    printf("\n  Checks RandomNoiseGenerator.h. Draws many random numbers and compares them with what the math promises.");
    printf("\n");
    printf("\nUSAGE");
    printf("\n  test_01_RandomNoise.exe [Seed] [Count]");
    printf("\n  test_01_RandomNoise.exe -h        (or --help, shows this guide)");
    printf("\n");
    printf("\nARGUMENTS (all optional, but they go in this order)");
    printf("\n  Seed    Starting number of the generator. Same seed gives the same numbers every run. Never use 0.   Default: 12345");
    printf("\n  Count   How many numbers to draw. More numbers, closer to the exact values.                          Default: 100000");
    printf("\n");
    printf("\nEXAMPLES");
    printf("\n  test_01_RandomNoise.exe");
    printf("\n  test_01_RandomNoise.exe 777");
    printf("\n  test_01_RandomNoise.exe 777 1000000");
    printf("\n");
    printf("\nBLOCKS INSIDE main (flip #if 1 / #if 0 in the .c file, then rebuild)");
    printf("\n  [Gaussian]   ON by default. Draws Count numbers from a bell curve (mean 2, StdDev 0.5) and prints the measured Mean, StdDev, and the fraction inside 1 sigma.");
    printf("\n  [Uniform]    OFF by default. Prints 5 numbers, all should be in [0, 1).");
    printf("\n");
    printf("\nWHAT A GOOD RESULT LOOKS LIKE");
    printf("\n  Mean about 2.0, StdDev about 0.5, Inside 1 sigma about 0.6827 (the famous 68.27 percent rule).");
    printf("\n  The gap shrinks like 1/sqrt(Count), so 100 times more numbers gives about 10 times closer.");


    printf("\n");
    return true;
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    uint64_t Seed = 12345 ;
    size_t Count = 100000 ;
    if (argc > 1) Seed  = (uint64_t) atoll(argv[1]) ;
    if (argc > 2) Count = (size_t) atoll(argv[2]) ;

    RandomState Gen;
    RandomSeed(&Gen, Seed);

    #if 1
    //Bell curve check: mean should be ~2, StdDev ~0.5, and ~68.27% should land within 1 StdDev
    double Sum = 0.0 ;
    double SumSquares = 0.0 ;
    size_t InsideOneSigma = 0 ;

    for (size_t ThisSample=0; ThisSample<Count; ThisSample++)
    {
        double Value = RandomGaussian(&Gen, 2.0, 0.5) ;
        Sum += Value;
        SumSquares += Value * Value;
        if (fabs(Value - 2.0) < 0.5) InsideOneSigma++;
    }

    double Mean = Sum / Count ;
    double StdDev = sqrt(SumSquares / Count - Mean * Mean) ;
    printf("\n[Gaussian] Mean = %f (expect 2.0)", Mean);
    printf("\n[Gaussian] StdDev = %f (expect 0.5)", StdDev);
    printf("\n[Gaussian] Inside 1 sigma = %f (expect 0.6827)", (double)InsideOneSigma / Count);
    #endif

    #if 0
    //First few uniform numbers, should all be in [0,1)
    for (size_t ThisSample=0; ThisSample<5; ThisSample++)
    {
        printf("\n[Uniform] %f", RandomUniform(&Gen));
    }
    #endif

    printf("\n");
    return 0;
}
