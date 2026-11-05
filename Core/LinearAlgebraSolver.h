//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdlib.h>
#include <math.h>



/// @brief A struct for storing a Matrix.
/// @param RowCount Number of Rows.
/// @param ColCount Number of Columns.
/// @param MatrixCells* Array of Values in the cells of the matrix.
/// @attention If the matrix is `N×N`, then first `N` elements of `MatrixCells*` are the elements of first row and next `N` elements are of the next row. That's the design.
typedef struct {
    size_t RowCount, ColCount ;
    double* MatrixCells ;
} Matrix;



/// @brief Create a `Matrix` filled with zeros.
/// @param RowCount Number of rows.
/// @param ColCount Number of columns.
/// @return The new `Matrix`.
/// @warning Needs a `MatrixDestroy` later, or the memory leaks.
Matrix MatrixCreate (size_t RowCount, size_t ColCount)
{
    Matrix Result;
    (Result.RowCount) = RowCount;
    (Result.ColCount) = ColCount;
    (Result.MatrixCells) = (double*) calloc(RowCount*ColCount, sizeof(double)) ;
    return Result;
}



/// @brief Free the memory of a `Matrix`.
/// @param TargetMatrix Address of the `Matrix`.
void MatrixDestroy (Matrix* TargetMatrix)
{
    free((TargetMatrix->MatrixCells));
    (TargetMatrix->MatrixCells) = NULL;
    (TargetMatrix->RowCount) = 0;
    (TargetMatrix->ColCount) = 0;
    return;
}



/// @brief Read one cell of a `Matrix`.
/// @param TargetMatrix Address of the `Matrix`.
/// @param RowPosition Row of the cell (starts from 0).
/// @param ColPosition Column of the cell (starts from 0).
/// @return Value of that cell in `double`.
/// @warning Doesn't check if the position is inside the matrix.
double GetMatrixCell (Matrix* TargetMatrix, size_t RowPosition, size_t ColPosition)
{
    return (TargetMatrix->MatrixCells[RowPosition*(TargetMatrix->ColCount) + ColPosition]) ;
}



/// @brief Write one cell of a `Matrix`.
/// @param TargetMatrix Address of the `Matrix`.
/// @param RowPosition Row of the cell (starts from 0).
/// @param ColPosition Column of the cell (starts from 0).
/// @param NewValue Value to store in `double`.
/// @warning Doesn't check if the position is inside the matrix.
void SetMatrixCell (Matrix* TargetMatrix, size_t RowPosition, size_t ColPosition, double NewValue)
{
    (TargetMatrix->MatrixCells[RowPosition*(TargetMatrix->ColCount) + ColPosition]) = NewValue;
    return;
}



/// @brief Flip a `Matrix` over its diagonal (rows become columns).
/// @param Source Address of the `Matrix` to transpose.
/// @return A new `Matrix` of size `ColCount x RowCount`.
/// @warning Result needs a `MatrixDestroy` later.
Matrix TransposeMatrix (Matrix* Source)
{
    Matrix Result = MatrixCreate((Source->ColCount), (Source->RowCount)) ;

    for (size_t ThisRow=0; ThisRow<(Source->RowCount); ThisRow++)
    {
        for (size_t ThisCol=0; ThisCol<(Source->ColCount); ThisCol++)
        {
            SetMatrixCell(&Result, ThisCol, ThisRow, GetMatrixCell(Source,ThisRow,ThisCol));
        }
    }

    return Result;
}



/// @brief Multiply two matrices: `Result = Left * Right`.
/// @param Left Address of the left `Matrix` (`m x k`).
/// @param Right Address of the right `Matrix` (`k x n`).
/// @return A new `Matrix` of size `m x n`.
/// @warning Doesn't check that `Left->ColCount == Right->RowCount`. Result needs a `MatrixDestroy` later.
Matrix MultiplyMatrices (Matrix* Left, Matrix* Right)
{
    Matrix Result = MatrixCreate((Left->RowCount), (Right->ColCount)) ;

    for (size_t ThisRow=0; ThisRow<(Result.RowCount); ThisRow++)
    {
        for (size_t ThisCol=0; ThisCol<(Result.ColCount); ThisCol++)
        {
            double Sum = 0.0 ;
            for (size_t Inner=0; Inner<(Left->ColCount); Inner++)
            {
                Sum += GetMatrixCell(Left,ThisRow,Inner) * GetMatrixCell(Right,Inner,ThisCol) ;
            }
            SetMatrixCell(&Result, ThisRow, ThisCol, Sum);
        }
    }

    return Result;
}



/// @brief Solve the square system `A * x = b`. Two methods live inside, flip the `#if` / `#elif` to choose: Gauss Elimination or Gauss-Seidel.
/// @param A Address of the square `Matrix` (`n x n`).
/// @param b Right hand side, `n` values in `double`.
/// @param x Container for the answer, `n` values in `double`.
/// @attention Gauss Elimination is direct (exact up to rounding). Gauss-Seidel guesses and keeps improving the guess until it stops changing.
/// @warning Gauss Elimination destroys (overwrites) `A` and `b`. Gauss-Seidel leaves them untouched.
/// @warning A singular `A` (no unique answer) gives `inf` or `nan` in Gauss Elimination. No check is done.
/// @warning Gauss-Seidel only surely works if `A` is diagonally dominant or symmetric positive definite (`At*A` is). Otherwise it can blow up, and it is slow for badly scaled columns.
void SolveLinearSystem (Matrix* A, double* b, double* x)
{
    size_t Size = (A->RowCount) ;

    #if 1       //To solve by Gauss Elimination (direct, accurate, destroys `A` and `b`).

    //Forward elimination: make everything below the diagonal zero
    for (size_t Pivot=0; Pivot<Size; Pivot++)
    {
        //Find the row (at or below Pivot) with the biggest value in this column. Dividing by big numbers keeps errors small.
        size_t BestRow = Pivot ;
        for (size_t ThisRow=Pivot+1; ThisRow<Size; ThisRow++)
        {
            if (fabs(GetMatrixCell(A,ThisRow,Pivot)) > fabs(GetMatrixCell(A,BestRow,Pivot))) BestRow = ThisRow;
        }

        //Swap that row up to the Pivot position (in both A and b)
        if (BestRow != Pivot)
        {
            for (size_t ThisCol=0; ThisCol<Size; ThisCol++)
            {
                double tempHolder = GetMatrixCell(A,Pivot,ThisCol) ;
                SetMatrixCell(A, Pivot, ThisCol, GetMatrixCell(A,BestRow,ThisCol));
                SetMatrixCell(A, BestRow, ThisCol, tempHolder);
            }
            double tempHolderB = b[Pivot] ;
            b[Pivot] = b[BestRow];
            b[BestRow] = tempHolderB;
        }

        //Subtract a multiple of the Pivot row from each row below, so their Pivot column becomes 0
        for (size_t ThisRow=Pivot+1; ThisRow<Size; ThisRow++)
        {
            double Factor = GetMatrixCell(A,ThisRow,Pivot) / GetMatrixCell(A,Pivot,Pivot) ;
            for (size_t ThisCol=Pivot; ThisCol<Size; ThisCol++)
            {
                SetMatrixCell ( //Arguments too long LoL 😭
                    A,
                    ThisRow,
                    ThisCol,
                    GetMatrixCell(A,ThisRow,ThisCol) - Factor*GetMatrixCell(A,Pivot,ThisCol)
                );
            }
            b[ThisRow] -= Factor * b[Pivot] ;
        }
    }

    //Back substitution: last row has one unknown, solve it, then walk upward
    for (size_t Count=0; Count<Size; Count++)
    {
        size_t ThisRow = Size-1-Count ;  //Counting down this way avoids unsigned underflow
        double Sum = b[ThisRow] ;
        for (size_t ThisCol=ThisRow+1; ThisCol<Size; ThisCol++)
        {
            Sum -= GetMatrixCell(A,ThisRow,ThisCol) * x[ThisCol] ;
        }
        x[ThisRow] = Sum / GetMatrixCell(A,ThisRow,ThisRow) ;
    }

    #elif 0     //To solve by Gauss-Seidel (iterative, simple, keeps `A` and `b`).

    size_t MaxSweeps = 100000 ;
    double Tolerance = 1e-12 ;

    //Starting guess: everything is zero
    for (size_t ThisNum=0; ThisNum<Size; ThisNum++)
    {
        x[ThisNum] = 0.0;
    }

    for (size_t ThisSweep=0; ThisSweep<MaxSweeps; ThisSweep++)
    {
        double BiggestChange = 0.0 ;

        for (size_t ThisRow=0; ThisRow<Size; ThisRow++)
        {
            //Row says: A[r][r]*x[r] + (all other terms) = b[r]. Solve it for x[r], using the newest x values we have.
            double Sum = b[ThisRow] ;
            for (size_t ThisCol=0; ThisCol<Size; ThisCol++)
            {
                if (ThisCol != ThisRow) Sum -= GetMatrixCell(A,ThisRow,ThisCol) * x[ThisCol] ;
            }

            double NewValue = Sum / GetMatrixCell(A,ThisRow,ThisRow) ;
            if (fabs(NewValue - x[ThisRow]) > BiggestChange) BiggestChange = fabs(NewValue - x[ThisRow]);
            x[ThisRow] = NewValue;
        }

        //Guess stopped changing, so we are done
        if (BiggestChange < Tolerance) break;
    }

    #endif

    return;
}



/// @brief Find the `x` that makes `A * x` as close to `b` as possible (least squares), via the normal equations `(At*A) x = At*b`.
/// @param A Address of the `Matrix` (`RowCount x ColCount`, `RowCount >= ColCount`).
/// @param b Target values, `RowCount` values in `double`.
/// @param Ridge Small number added to the diagonal of `At*A` for stability. Use `0.0` for plain least squares.
/// @param x Container for the answer, `ColCount` values in `double`.
/// @attention `A` and `b` stay untouched.
/// @warning Squaring `A` makes badly scaled columns worse, so use `Ridge` (like `1e-10`) if answers look wild.
void LeastSquaresSolve (Matrix* A, double* b, double Ridge, double* x)
{
    Matrix At = TransposeMatrix(A) ;
    Matrix AtA = MultiplyMatrices(&At, A) ;
    double* Atb = (double*) calloc((A->ColCount), sizeof(double)) ;

    for (size_t ThisCol=0; ThisCol<(A->ColCount); ThisCol++)
    {
        for (size_t ThisRow=0; ThisRow<(A->RowCount); ThisRow++)
        {
            Atb[ThisCol] += GetMatrixCell(&At,ThisCol,ThisRow) * b[ThisRow] ;
        }
        SetMatrixCell(&AtA, ThisCol, ThisCol, GetMatrixCell(&AtA,ThisCol,ThisCol) + Ridge);
    }

    SolveLinearSystem(&AtA, Atb, x);

    free(Atb);
    MatrixDestroy(&At);
    MatrixDestroy(&AtA);
    return;
}
