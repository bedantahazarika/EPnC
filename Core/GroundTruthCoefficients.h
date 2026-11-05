//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdlib.h>
#include <math.h>
#include <OdeSolverAndSystems.h>
#include <CandidateTermLibrary.h>



/// @brief How well a recovered equation pair matches the true one, counted by terms (is a coefficient zero or not).
/// @param TrueTermCount How many terms the true equations have (`dx` and `dy` together).
/// @param FoundTrueCount How many of those true terms the regression kept (non-zero).
/// @param ExtraTermCount How many terms the regression kept that should be zero.
/// @param BiggestMiss Biggest absolute difference between a found and a true coefficient, over all terms and both equations.
typedef struct {
    int TrueTermCount, FoundTrueCount, ExtraTermCount ;
    double BiggestMiss ;
} SupportReport;



/// @brief Puts one true coefficient into its slot, if the library has that term.
/// @param Library Address of the term library.
/// @param Coefficients Container with one number per term (already filled with zeros).
/// @param Kind Which kind of term (`TERM_POLYNOMIAL`, `TERM_SIN_X`, ...).
/// @param PowerX Power of `x`.
/// @param PowerY Power of `y`.
/// @param Value The true coefficient.
/// @return `0` if placed, `1` if the library does not have that term (so it is missing).
int PlaceCoefficient (TermLibrary* Library, double* Coefficients, int Kind, int PowerX, int PowerY, double Value)
{
    int Column = FindTerm(Library, Kind, PowerX, PowerY) ;
    if (Column < 0) return 1;

    Coefficients[Column] = Value;
    return 0;
}



/// @brief Writes the true equations of a system as coefficients of the library terms. This is the answer key the regression is graded against.
/// @param System Which system.
/// @param Params Constants of the system (see `EvaluateSystem` for their meaning).
/// @param Library Address of the term library.
/// @param DxCoefficients Container for the `dx/dt` coefficients, one per term. Gets filled with zeros first.
/// @param DyCoefficients Container for the `dy/dt` coefficients, one per term. Gets filled with zeros first.
/// @return How many true terms the library does not have (and so could not be placed).
int FillTrueCoefficients (SystemID System, double* Params, TermLibrary* Library, double* DxCoefficients, double* DyCoefficients)
{
    int MissingCount = 0 ;
    for (size_t ThisTerm=0; ThisTerm<(Library->TermCount); ThisTerm++)
    {
        DxCoefficients[ThisTerm] = 0.0;
        DyCoefficients[ThisTerm] = 0.0;
    }

    switch (System)
    {
        case SYSTEM_LINEAR_SPIRAL:      //dx = a x + b y,   dy = c x + d y
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 1, 0,  Params[0]);
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 0, 1,  Params[1]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 1, 0,  Params[2]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 0, 1,  Params[3]);
            break;

        case SYSTEM_LOTKA_VOLTERRA:     //dx = a x - b x y,   dy = d x y - c y
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 1, 0,  Params[0]);
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 1, 1, -Params[1]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 1, 1,  Params[3]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 0, 1, -Params[2]);
            break;

        case SYSTEM_VAN_DER_POL:        //dx = y,   dy = Mu y - Mu x^2 y - x
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 0, 1,  1.0);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 0, 1,  Params[0]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 2, 1, -Params[0]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 1, 0, -1.0);
            break;

        case SYSTEM_DAMPED_PENDULUM:    //dx = y,   dy = -Freq2 sin(x) - Damping y
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 0, 1,  1.0);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_SIN_X,      0, 0, -Params[1]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 0, 1, -Params[0]);
            break;

        case SYSTEM_DUFFING:            //dx = y,   dy = -Damping y - Alpha x - Beta x^3
            MissingCount += PlaceCoefficient(Library, DxCoefficients, TERM_POLYNOMIAL, 0, 1,  1.0);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 0, 1, -Params[0]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 1, 0, -Params[1]);
            MissingCount += PlaceCoefficient(Library, DyCoefficients, TERM_POLYNOMIAL, 3, 0, -Params[2]);
            break;

        default:
            break;
    }

    return MissingCount;
}



/// @brief Compares recovered coefficients with the true ones, term by term. A term counts as "kept" when its coefficient is not exactly zero (STLSQ gives exact zeros).
/// @param TrueDx True `dx/dt` coefficients, one per term (from `FillTrueCoefficients`).
/// @param TrueDy True `dy/dt` coefficients, one per term.
/// @param FoundDx Recovered `dx/dt` coefficients, one per term.
/// @param FoundDy Recovered `dy/dt` coefficients, one per term.
/// @param TermCount Number of terms in the library.
/// @return A `SupportReport`. The recovery is exactly right when `FoundTrueCount == TrueTermCount` and `ExtraTermCount == 0`.
/// @attention A true term that is missing from the library is not in `TrueTermCount`, since it has no slot. Check the `FillTrueCoefficients` return value for that.
SupportReport CompareWithTruth (double* TrueDx, double* TrueDy, double* FoundDx, double* FoundDy, size_t TermCount)
{
    SupportReport Report = {0, 0, 0, 0.0} ;

    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
    {
        double MissX = fabs(FoundDx[ThisTerm] - TrueDx[ThisTerm]) ;
        double MissY = fabs(FoundDy[ThisTerm] - TrueDy[ThisTerm]) ;
        if (MissX > (Report.BiggestMiss)) (Report.BiggestMiss) = MissX;
        if (MissY > (Report.BiggestMiss)) (Report.BiggestMiss) = MissY;

        if (TrueDx[ThisTerm] != 0.0) { (Report.TrueTermCount)++; if (FoundDx[ThisTerm] != 0.0) (Report.FoundTrueCount)++; }
        else if (FoundDx[ThisTerm] != 0.0) (Report.ExtraTermCount)++;

        if (TrueDy[ThisTerm] != 0.0) { (Report.TrueTermCount)++; if (FoundDy[ThisTerm] != 0.0) (Report.FoundTrueCount)++; }
        else if (FoundDy[ThisTerm] != 0.0) (Report.ExtraTermCount)++;
    }

    return Report;
}
