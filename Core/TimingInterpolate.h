//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <LinearAlgebraSolver.h>



/// @brief The interpolation methods available. All of them answer the same question: "the samples are at uneven times, what would the value have been at this other time?"
/// @param INTERP_LINEAR Straight line between the two neighbouring samples. Simplest, never overshoots, but has kinks, so it bends every curve a little (error grows with the curvature times step squared, `O(h^2)`).
/// @param INTERP_HERMITE Cubic piece between two neighbours, using the slope of a parabola through three points at every sample. Local (only nearby samples matter) and smooth in value and slope, error `O(h^3)` at the nodes' slopes.
/// @param INTERP_SPLINE Natural cubic spline: the one cubic-per-interval curve whose slope AND curvature are continuous everywhere. Smoothest, error `O(h^4)` in the middle, but global (one far-away sample slightly changes everything) and the ends are forced flat in curvature.
/// @param INTERP_PCHIP Monotone cubic (Fritsch-Carlson): like Hermite, but slopes are chosen so the curve never overshoots the data. Safe, but flattens true peaks and valleys, which hurts for oscillations.
/// @param INTERP_COUNT Not a method, only the number of methods.
typedef enum {
    INTERP_LINEAR,
    INTERP_HERMITE,
    INTERP_SPLINE,
    INTERP_PCHIP,
    INTERP_COUNT
} InterpolationMethod;



/// @brief What `AnalyzeTiming` found out about the time column of a path.
/// @param PointCount Number of rows.
/// @param IsIncreasing `true` if every time is strictly larger than the one before. Interpolation needs this.
/// @param IdealStep The step of the perfectly even grid with the same first and last time: `(t_last - t_first) / (PointCount - 1)`.
/// @param SmallestStep Smallest gap between two neighbouring times.
/// @param LargestStep Largest gap between two neighbouring times. Much more than `IdealStep` means missing samples (a hole in the data).
/// @param LargestDrift Largest distance of any time from its place on the even grid, in units of `IdealStep`.
/// @param DriftedCount How many rows are further from the even grid than the tolerance given to `AnalyzeTiming`.
typedef struct {
    size_t PointCount ;
    bool IsIncreasing ;
    double IdealStep, SmallestStep, LargestStep, LargestDrift ;
    size_t DriftedCount ;
} TimingReport;



/// @brief Gives a printable name for an interpolation method.
/// @param Method The method.
/// @return A short text like `"cubic spline"`.
const char* GetInterpolationName (InterpolationMethod Method)
{
    switch (Method)
    {
        case INTERP_LINEAR:  return "linear";
        case INTERP_HERMITE: return "cubic Hermite";
        case INTERP_SPLINE:  return "cubic spline";
        case INTERP_PCHIP:   return "PCHIP";
        default:             return "unknown";
    }
}



/// @brief Checks how even the time column of a path is. Row `k` "should" be at `t_first + k * IdealStep`; its drift is the distance from that place divided by `IdealStep`.
/// @param Path Address of a trajectory (columns `t, x, y`) with at least 2 rows.
/// @param Tolerance How much drift (in units of `IdealStep`) still counts as fine. `0.05` means 5 percent of a step.
/// @return A `TimingReport`.
/// @attention A row that has drifted only because of the clock's tiny jitter is not a problem; a hole in the data (one big gap) shows up as a large `LargestStep`, and also shifts the rows after it, so many rows become drifted.
TimingReport AnalyzeTiming (Matrix* Path, double Tolerance)
{
    TimingReport Report ;
    size_t RowCount = (Path->RowCount) ;
    double StartTime = GetMatrixCell(Path,0,0) ;
    double EndTime = GetMatrixCell(Path,RowCount-1,0) ;

    (Report.PointCount) = RowCount ;
    (Report.IsIncreasing) = true ;
    (Report.IdealStep) = (EndTime - StartTime) / (double)(RowCount - 1) ;
    (Report.SmallestStep) = GetMatrixCell(Path,1,0) - StartTime ;
    (Report.LargestStep) = (Report.SmallestStep) ;
    (Report.LargestDrift) = 0.0 ;
    (Report.DriftedCount) = 0 ;

    for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
    {
        if (ThisRow+1 < RowCount)
        {
            double Step = GetMatrixCell(Path,ThisRow+1,0) - GetMatrixCell(Path,ThisRow,0) ;
            if (Step <= 0.0) (Report.IsIncreasing) = false ;
            if (Step < (Report.SmallestStep)) (Report.SmallestStep) = Step ;
            if (Step > (Report.LargestStep)) (Report.LargestStep) = Step ;
        }

        double Drift = fabs(GetMatrixCell(Path,ThisRow,0) - (StartTime + (double)ThisRow*(Report.IdealStep))) / (Report.IdealStep) ;
        if (Drift > (Report.LargestDrift)) (Report.LargestDrift) = Drift ;
        if (Drift > Tolerance) (Report.DriftedCount)++ ;
    }

    return Report;
}



/// @brief Finds the interval that contains a query time, by halving the search range each time (binary search).
/// @param Times Array of `PointCount` strictly increasing times.
/// @param PointCount Length of `Times`.
/// @param Query The time to look for.
/// @return The index `i` such that `Times[i] <= Query < Times[i+1]`. Queries outside the data give the first or the last interval.
size_t FindInterval (double* Times, size_t PointCount, double Query)
{
    size_t Low = 0 ;
    size_t High = PointCount - 1 ;

    while (High - Low > 1)
    {
        size_t Middle = (Low + High) / 2 ;
        if (Times[Middle] <= Query) Low = Middle ;
        else High = Middle ;
    }

    return Low;
}



/// @brief The slope rule of PCHIP at one END of the data (Fritsch-Carlson, shape preserving three point formula).
/// @param WidthNear Width of the interval touching the end.
/// @param WidthNext Width of the next interval.
/// @param SecantNear Secant slope of the interval touching the end.
/// @param SecantNext Secant slope of the next interval.
/// @return The slope at the end point in `double`.
double PchipEndSlope (double WidthNear, double WidthNext, double SecantNear, double SecantNext)
{
    double Slope = ((2.0*WidthNear + WidthNext)*SecantNear - WidthNear*SecantNext) / (WidthNear + WidthNext) ;
    if (Slope*SecantNear <= 0.0) return 0.0;
    if ((SecantNear*SecantNext <= 0.0) && (fabs(Slope) > 3.0*fabs(SecantNear))) return 3.0*SecantNear;
    return Slope;
}



/// @brief Computes the slope of the interpolating curve at every sample. This is the only part that differs between the cubic methods; the curve between two samples is then always the same cubic Hermite piece (see `EvaluateInterpolant`).
/// @param Method The method. `INTERP_LINEAR` needs no slopes, so nothing is done for it.
/// @param Times Array of `PointCount` strictly increasing times.
/// @param Values Array of `PointCount` values (`x` or `y`).
/// @param PointCount Length of both arrays, at least 3.
/// @param Slopes Array of `PointCount` values that receives the slopes.
/// @attention With widths `h_i = t_(i+1) - t_i` and secants `d_i = (y_(i+1) - y_i) / h_i`:
/// @attention HERMITE: inside, `m_i = (h_i*d_(i-1) + h_(i-1)*d_i) / (h_(i-1) + h_i)`, the slope of the parabola through three neighbours. At the ends, the parabola through the first (or last) three points.
/// @attention PCHIP: if `d_(i-1)` and `d_i` have different signs (a peak or valley), `m_i = 0`. Otherwise a weighted harmonic mean of the two secants, which can never be larger than the larger secant, so there is no overshoot.
/// @attention SPLINE: slopes that make the curvature continuous solve a tridiagonal system, `h_i*m_(i-1) + 2*(h_(i-1) + h_i)*m_i + h_(i-1)*m_(i+1) = 3*(h_i*d_(i-1) + h_(i-1)*d_i)`, with "natural" ends `2*m_0 + m_1 = 3*d_0` (curvature zero). Solved with the Thomas algorithm (Gauss elimination that only touches 3 diagonals, so it costs `O(PointCount)`).
void ComputeNodeSlopes (InterpolationMethod Method, double* Times, double* Values, size_t PointCount, double* Slopes)
{
    if (Method == INTERP_LINEAR) return;

    size_t IntervalCount = PointCount - 1 ;
    double* Widths = (double*) calloc(IntervalCount, sizeof(double)) ;
    double* Secants = (double*) calloc(IntervalCount, sizeof(double)) ;
    for (size_t ThisInterval=0; ThisInterval<IntervalCount; ThisInterval++)
    {
        Widths[ThisInterval] = Times[ThisInterval+1] - Times[ThisInterval] ;
        Secants[ThisInterval] = (Values[ThisInterval+1] - Values[ThisInterval]) / Widths[ThisInterval] ;
    }

    switch (Method)
    {
        case INTERP_HERMITE:
            for (size_t ThisNode=1; ThisNode<IntervalCount; ThisNode++)
            {
                Slopes[ThisNode] = (Widths[ThisNode]*Secants[ThisNode-1] + Widths[ThisNode-1]*Secants[ThisNode]) / (Widths[ThisNode-1] + Widths[ThisNode]) ;
            }
            Slopes[0] = Secants[0] + (Secants[0] - Secants[1]) * Widths[0] / (Widths[0] + Widths[1]) ;
            Slopes[IntervalCount] = Secants[IntervalCount-1] + (Secants[IntervalCount-1] - Secants[IntervalCount-2]) * Widths[IntervalCount-1] / (Widths[IntervalCount-2] + Widths[IntervalCount-1]) ;
            break;

        case INTERP_PCHIP:
            for (size_t ThisNode=1; ThisNode<IntervalCount; ThisNode++)
            {
                if (Secants[ThisNode-1]*Secants[ThisNode] <= 0.0)
                {
                    Slopes[ThisNode] = 0.0 ;
                }
                else
                {
                    double WeightLeft = 2.0*Widths[ThisNode] + Widths[ThisNode-1] ;
                    double WeightRight = Widths[ThisNode] + 2.0*Widths[ThisNode-1] ;
                    Slopes[ThisNode] = (WeightLeft + WeightRight) / (WeightLeft/Secants[ThisNode-1] + WeightRight/Secants[ThisNode]) ;
                }
            }
            Slopes[0] = PchipEndSlope(Widths[0], Widths[1], Secants[0], Secants[1]) ;
            Slopes[IntervalCount] = PchipEndSlope(Widths[IntervalCount-1], Widths[IntervalCount-2], Secants[IntervalCount-1], Secants[IntervalCount-2]) ;
            break;

        case INTERP_SPLINE:
        {
            double* Lower = (double*) calloc(PointCount, sizeof(double)) ;
            double* Diagonal = (double*) calloc(PointCount, sizeof(double)) ;
            double* Upper = (double*) calloc(PointCount, sizeof(double)) ;
            double* Right = (double*) calloc(PointCount, sizeof(double)) ;

            Diagonal[0] = 2.0 ;
            Upper[0] = 1.0 ;
            Right[0] = 3.0*Secants[0] ;
            for (size_t ThisNode=1; ThisNode<IntervalCount; ThisNode++)
            {
                Lower[ThisNode] = Widths[ThisNode] ;
                Diagonal[ThisNode] = 2.0*(Widths[ThisNode-1] + Widths[ThisNode]) ;
                Upper[ThisNode] = Widths[ThisNode-1] ;
                Right[ThisNode] = 3.0*(Widths[ThisNode]*Secants[ThisNode-1] + Widths[ThisNode-1]*Secants[ThisNode]) ;
            }
            Lower[IntervalCount] = 1.0 ;
            Diagonal[IntervalCount] = 2.0 ;
            Right[IntervalCount] = 3.0*Secants[IntervalCount-1] ;

            //Thomas algorithm: eliminate the lower diagonal going down, then substitute back going up
            for (size_t ThisNode=1; ThisNode<PointCount; ThisNode++)
            {
                double Factor = Lower[ThisNode] / Diagonal[ThisNode-1] ;
                Diagonal[ThisNode] -= Factor*Upper[ThisNode-1] ;
                Right[ThisNode] -= Factor*Right[ThisNode-1] ;
            }
            Slopes[IntervalCount] = Right[IntervalCount] / Diagonal[IntervalCount] ;
            for (size_t ThisNode=IntervalCount; ThisNode-- > 0; )
            {
                Slopes[ThisNode] = (Right[ThisNode] - Upper[ThisNode]*Slopes[ThisNode+1]) / Diagonal[ThisNode] ;
            }

            free(Lower);
            free(Diagonal);
            free(Upper);
            free(Right);
            break;
        }

        default:
            break;
    }

    free(Widths);
    free(Secants);
    return;
}



/// @brief Evaluates the interpolating curve at one query time.
/// @param Method The method. Must be the same as used for `ComputeNodeSlopes`.
/// @param Times Array of `PointCount` strictly increasing times.
/// @param Values Array of `PointCount` values.
/// @param Slopes Array from `ComputeNodeSlopes` (not read for `INTERP_LINEAR`).
/// @param PointCount Length of the arrays.
/// @param Query The time where the value is wanted.
/// @return The interpolated value in `double`.
/// @attention Inside an interval of width `h`, with `s = (Query - t_i) / h` between 0 and 1, the cubic Hermite piece is `y = H00(s)*y_i + H10(s)*h*m_i + H01(s)*y_(i+1) + H11(s)*h*m_(i+1)`, where `H00 = 2s^3 - 3s^2 + 1`, `H10 = s^3 - 2s^2 + s`, `H01 = -2s^3 + 3s^2`, `H11 = s^3 - s^2`. Check: at `s = 0` it gives `y_i`, at `s = 1` it gives `y_(i+1)`, and its slope there is `m_i` and `m_(i+1)`.
double EvaluateInterpolant (InterpolationMethod Method, double* Times, double* Values, double* Slopes, size_t PointCount, double Query)
{
    size_t Interval = FindInterval(Times, PointCount, Query) ;
    double Width = Times[Interval+1] - Times[Interval] ;
    double S = (Query - Times[Interval]) / Width ;

    if (Method == INTERP_LINEAR)
    {
        return (Values[Interval] + S*(Values[Interval+1] - Values[Interval]));
    }

    double S2 = S*S ;
    double S3 = S2*S ;
    return ((2.0*S3 - 3.0*S2 + 1.0)*Values[Interval]
          + (S3 - 2.0*S2 + S)*Width*Slopes[Interval]
          + (-2.0*S3 + 3.0*S2)*Values[Interval+1]
          + (S3 - S2)*Width*Slopes[Interval+1]);
}



/// @brief Makes the time column of a path perfectly even, so methods that assume one fixed time step (finite differences, Savitzky-Golay) can be trusted. Rows whose time is close to the even grid are only snapped to it; rows that have drifted more than the tolerance get a new `x` and `y`, interpolated at their grid time.
/// @param Path Address of a trajectory (columns `t, x, y`) with at least 3 rows and strictly increasing time (see `AnalyzeTiming`).
/// @param Method Which interpolation to use for the drifted rows.
/// @param Tolerance Drift (in units of the ideal step) that is still left alone. `0.05` is a good start. `0` re-interpolates every row.
/// @param WasInterpolated Array of `RowCount` that receives `true` for rows that were interpolated and `false` for rows that were only snapped. Can be `NULL`.
/// @return A new `Matrix`, same size as `Path`, with times `t_first + k*IdealStep`.
/// @attention The even grid has the same first and last time as the data and the same number of rows, so no query ever lies outside the data: this is interpolation, never extrapolation.
/// @attention Snapping a row that drifted less than the tolerance moves its sample in time by at most `Tolerance * IdealStep`, which acts like a small timing error. Smaller tolerance means fewer such errors but more interpolated (smoothed, slightly invented) values.
/// @warning Interpolation invents values. Across a big hole in the data (see `LargestStep` of `AnalyzeTiming`) the interpolant is a guess. Noise in `x` and `y` is interpolated as if it were signal. Result needs a `MatrixDestroy` later.
Matrix MakeTimeEven (Matrix* Path, InterpolationMethod Method, double Tolerance, bool* WasInterpolated)
{
    size_t RowCount = (Path->RowCount) ;
    Matrix Evened = MatrixCreate(RowCount, 3) ;
    double StartTime = GetMatrixCell(Path,0,0) ;
    double IdealStep = (GetMatrixCell(Path,RowCount-1,0) - StartTime) / (double)(RowCount - 1) ;

    double* Times = (double*) calloc(RowCount, sizeof(double)) ;
    double* Values = (double*) calloc(RowCount, sizeof(double)) ;
    double* Slopes = (double*) calloc(RowCount, sizeof(double)) ;
    for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
    {
        Times[ThisRow] = GetMatrixCell(Path,ThisRow,0) ;
        SetMatrixCell(&Evened, ThisRow, 0, StartTime + (double)ThisRow*IdealStep);
        if (WasInterpolated != NULL) WasInterpolated[ThisRow] = false ;
    }

    for (size_t ThisCol=1; ThisCol<=2; ThisCol++)
    {
        for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
        {
            Values[ThisRow] = GetMatrixCell(Path,ThisRow,ThisCol) ;
        }
        ComputeNodeSlopes(Method, Times, Values, RowCount, Slopes);

        for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
        {
            double GridTime = GetMatrixCell(&Evened,ThisRow,0) ;
            double Value = Values[ThisRow] ;
            if (fabs(Times[ThisRow] - GridTime) > Tolerance*IdealStep)
            {
                Value = EvaluateInterpolant(Method, Times, Values, Slopes, RowCount, GridTime) ;
                if (WasInterpolated != NULL) WasInterpolated[ThisRow] = true ;
            }
            SetMatrixCell(&Evened, ThisRow, ThisCol, Value);
        }
    }

    free(Times);
    free(Values);
    free(Slopes);
    return Evened;
}
