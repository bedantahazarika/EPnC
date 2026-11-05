//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdlib.h>
#include <math.h>
#include <LinearAlgebraSolver.h>



/// @brief Estimates `dx/dt` and `dy/dt` from the recorded points alone, using finite differences. This is the baseline derivative estimator, no neural network.
/// @param Path Address of a trajectory (columns `t, x, y`), at least 3 rows, rows evenly spaced in time. Noisy paths are fine, that is the point.
/// @return A new `Matrix` of size `RowCount x 2`. Columns are `dx/dt, dy/dt`, same layout as `TrueDerivativesOfTrajectory`.
/// @attention Inside the path: central difference `(f[k+1] - f[k-1]) / (2 h)`. At the first and last row there is only one side, so a second order one sided formula is used (`(-3 f[0] + 4 f[1] - f[2]) / (2 h)` at the start, mirrored at the end). Both have error of order `h^2`.
/// @warning Noise gets amplified: noise of width `s` in the points becomes noise of about `s / (sqrt(2) h)` in the derivative. Halving `h` makes it worse. That is exactly the problem the neural stage tries to solve.
/// @warning Result needs a `MatrixDestroy` later.
Matrix FiniteDifferenceDerivatives (Matrix* Path)
{
    size_t RowCount = (Path->RowCount) ;
    double TimeStep = GetMatrixCell(Path,1,0) - GetMatrixCell(Path,0,0) ;
    Matrix Derivatives = MatrixCreate(RowCount, 2) ;

    for (size_t ThisCol=1; ThisCol<=2; ThisCol++)
    {
        for (size_t ThisRow=1; ThisRow+1<RowCount; ThisRow++)
        {
            double Slope = (GetMatrixCell(Path,ThisRow+1,ThisCol) - GetMatrixCell(Path,ThisRow-1,ThisCol)) / (2.0*TimeStep) ;
            SetMatrixCell(&Derivatives, ThisRow, ThisCol-1, Slope);
        }

        double FirstSlope = (-3.0*GetMatrixCell(Path,0,ThisCol) + 4.0*GetMatrixCell(Path,1,ThisCol) - GetMatrixCell(Path,2,ThisCol)) / (2.0*TimeStep) ;
        double LastSlope = (3.0*GetMatrixCell(Path,RowCount-1,ThisCol) - 4.0*GetMatrixCell(Path,RowCount-2,ThisCol) + GetMatrixCell(Path,RowCount-3,ThisCol)) / (2.0*TimeStep) ;
        SetMatrixCell(&Derivatives, 0, ThisCol-1, FirstSlope);
        SetMatrixCell(&Derivatives, RowCount-1, ThisCol-1, LastSlope);
    }

    return Derivatives;
}



/// @brief Estimates `dx/dt` and `dy/dt` with a Savitzky-Golay filter: around every row, fit a small polynomial to the nearby points by least squares, and read the slope of that polynomial at the row.
/// @param Path Address of a trajectory (columns `t, x, y`), rows evenly spaced in time.
/// @param HalfWindow How many points on each side of a row are used. The window has `2*HalfWindow + 1` points.
/// @param PolyOrder Degree of the polynomial that is fitted. Must be smaller than the window size. `2` or `3` is typical.
/// @return A new `Matrix` of size `RowCount x 2`. Columns are `dx/dt, dy/dt`.
/// @attention A bigger window smooths more noise but blurs real wiggles. Any signal that is itself a polynomial of degree `PolyOrder` or lower is differentiated exactly (even at the edges).
/// @attention Near the ends the window slides inwards so it always stays inside the data, and the slope is read at the row's own position (not the window's center).
/// @warning Needs at least `2*HalfWindow + 1` rows. Result needs a `MatrixDestroy` later.
Matrix SavitzkyGolayDerivatives (Matrix* Path, int HalfWindow, int PolyOrder)
{
    long RowCount = (long)(Path->RowCount) ;
    long WindowSize = 2L*HalfWindow + 1L ;
    size_t UnknownCount = (size_t)(PolyOrder + 1) ;
    double TimeStep = GetMatrixCell(Path,1,0) - GetMatrixCell(Path,0,0) ;

    Matrix Derivatives = MatrixCreate((Path->RowCount), 2) ;
    Matrix Window = MatrixCreate((size_t)WindowSize, UnknownCount) ;
    double* Values = (double*) calloc((size_t)WindowSize, sizeof(double)) ;
    double* Fit = (double*) calloc(UnknownCount, sizeof(double)) ;

    for (long ThisRow=0; ThisRow<RowCount; ThisRow++)
    {
        long Start = ThisRow - HalfWindow ;
        if (Start < 0) Start = 0;
        if (Start + WindowSize > RowCount) Start = RowCount - WindowSize;

        //Fit  value = c0 + c1 u + c2 u^2 + ...  where u = (distance from this row in samples) / HalfWindow. Dividing keeps u between -1 and 1, so the numbers stay tame.
        for (long ThisSlot=0; ThisSlot<WindowSize; ThisSlot++)
        {
            double u = (double)(Start + ThisSlot - ThisRow) / (double) HalfWindow ;
            for (size_t ThisPower=0; ThisPower<UnknownCount; ThisPower++)
            {
                SetMatrixCell(&Window, (size_t)ThisSlot, ThisPower, pow(u,(double)ThisPower));
            }
        }

        for (size_t ThisCol=1; ThisCol<=2; ThisCol++)
        {
            for (long ThisSlot=0; ThisSlot<WindowSize; ThisSlot++) Values[ThisSlot] = GetMatrixCell(Path, (size_t)(Start + ThisSlot), ThisCol);
            LeastSquaresSolve(&Window, Values, 0.0, Fit);

            //Slope at u = 0 is c1, and u counts in steps of (HalfWindow * TimeStep) of real time
            SetMatrixCell(&Derivatives, (size_t)ThisRow, ThisCol-1, Fit[1] / ((double)HalfWindow * TimeStep));
        }
    }

    free(Fit);
    free(Values);
    MatrixDestroy(&Window);
    return Derivatives;
}
