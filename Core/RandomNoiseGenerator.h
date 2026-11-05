//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define TWO_PI 6.283185307179586



/// @brief Holds the internal state of the random number generator.
/// @param State Current 64-bit state. Must never be 0.
typedef struct {
    uint64_t State;
} RandomState;



/// @brief Sets the starting point of the generator. Same seed gives the same numbers every run.
/// @param Gen Address of the generator.
/// @param Seed Any non-zero `uint64_t`.
/// @warning Seed 0 gets the generator stuck at 0 forever. No check is done.
void RandomSeed (RandomState* Gen, uint64_t Seed)
{
    (Gen->State) = Seed;
    return;
}



/// @brief Gives the next raw 64-bit random number (xorshift64).
/// @param Gen Address of the generator.
/// @return Next random number in `uint64_t`.
/// @attention Only shifts and XORs, so it is fast but not for cryptography.
uint64_t RandomNextU64 (RandomState* Gen)
{
    uint64_t x = (Gen->State) ;
    x ^= x << 13 ;
    x ^= x >> 7 ;
    x ^= x << 17 ;
    (Gen->State) = x;
    return x;
}



/// @brief Gives a uniform random number in [0, 1).
/// @param Gen Address of the generator.
/// @return Random number in `double`.
double RandomUniform (RandomState* Gen)
{
    //Keep top 53 bits (a double holds exactly 53 bits of fraction), then divide by 2^53
    return (double)(RandomNextU64(Gen) >> 11) * (1.0 / 9007199254740992.0) ;
}



/// @brief Gives a Gaussian (bell curve) random number using the Box-Muller transform.
/// @param Gen Address of the generator.
/// @param Mean Center of the bell curve in `double`.
/// @param StdDev Width of the bell curve in `double`.
/// @return Random number in `double`.
/// @attention Box-Muller makes two Gaussians per two uniforms. We only keep one, simple over clever.
double RandomGaussian (RandomState* Gen, double Mean, double StdDev)
{
    double U1 = 1.0 - RandomUniform(Gen) ;  //Now in (0,1], so log() never sees 0
    double U2 = RandomUniform(Gen) ;
    double Radius = sqrt(-2.0 * log(U1)) ;
    return Mean + StdDev * Radius * cos(TWO_PI * U2) ;
}
