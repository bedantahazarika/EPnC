//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <LinearAlgebraSolver.h>



/// @brief Make a smaller `Matrix` that holds only the chosen columns of `Theta`.
/// @param Theta Address of the big `Matrix` (one row per sample, one column per candidate term).
/// @param IsActive One flag per column of `Theta`. `true` means keep this column, `false` means drop it.
/// @param ActiveCount How many flags are `1`. This becomes the column count of the result.
/// @return The new `Matrix` (`Theta->RowCount x ActiveCount`), columns in the original order.
/// @warning Needs a `MatrixDestroy` later, or the memory leaks. `ActiveCount` must really match the flags, nothing checks it.
Matrix PickActiveColumns (Matrix* Theta, bool* IsActive, size_t ActiveCount)
{
    Matrix Picked = MatrixCreate((Theta->RowCount), ActiveCount) ;
    size_t PickedCol = 0 ;

    for (size_t ThisCol=0; ThisCol<(Theta->ColCount); ThisCol++)
    {
        if (!IsActive[ThisCol]) continue;

        for (size_t ThisRow=0; ThisRow<(Theta->RowCount); ThisRow++)
        {
            SetMatrixCell(&Picked, ThisRow, PickedCol, GetMatrixCell(Theta,ThisRow,ThisCol));
        }
        PickedCol++;
    }

    return Picked;
}



/// @brief Least squares fit that uses only the active columns. Dropped columns get the coefficient exactly `0.0`.
/// @param Theta Address of the term matrix.
/// @param Target The derivative values, `Theta->RowCount` values in `double`.
/// @param IsActive One flag per column of `Theta` (`true` = may be used, `false` = forced to zero).
/// @param ActiveCount How many flags are `1`. Must be at least `1`.
/// @param Ridge Tiny stabilizer, see `LeastSquaresSolve`.
/// @param Coefficients Container for the answer, `Theta->ColCount` values in `double`. Every cell gets written.
void FitActiveColumns (Matrix* Theta, double* Target, bool* IsActive, size_t ActiveCount, double Ridge, double* Coefficients)
{
    Matrix Picked = PickActiveColumns(Theta, IsActive, ActiveCount) ;
    double* ShortAnswer = (double*) calloc(ActiveCount, sizeof(double)) ;

    LeastSquaresSolve(&Picked, Target, Ridge, ShortAnswer);

    //Put the short answer back to its place in the full list, zeros everywhere else
    size_t PickedCol = 0 ;
    for (size_t ThisTerm=0; ThisTerm<(Theta->ColCount); ThisTerm++)
    {
        if (IsActive[ThisTerm])
        {
            Coefficients[ThisTerm] = ShortAnswer[PickedCol] ;
            PickedCol++;
        } else
        {
            Coefficients[ThisTerm] = 0.0 ;
        }
    }

    free(ShortAnswer);
    MatrixDestroy(&Picked);
    return;
}



/// @brief A STLSQ run that can be advanced ONE round at a time, and that remembers every round. Made for animations: take a step, draw the snapshot, take the next step.
/// @param Theta Address of the term matrix (not copied, must stay alive while the job is used).
/// @param Target The derivative values (not copied, must stay alive too).
/// @param Threshold Any coefficient with absolute value below this is thrown out.
/// @param Ridge Tiny stabilizer for each least squares fit.
/// @param MaxRounds Most number of "throw out and refit" rounds.
/// @param TermCount How many candidate terms (columns of `Theta`).
/// @param ActiveCount How many terms are still alive.
/// @param IsActive Array of `TermCount`: which terms are still alive right now.
/// @param Coefficients Array of `TermCount`: the current answer. A thrown out term is exactly `0.0`.
/// @param RoundsDone How many rounds threw something out AND refit so far (same number `SparseRegressionSTLSQ` returns).
/// @param IsDone `true` once nothing more will change.
/// @param SnapshotCount How many snapshots exist (`1 + RoundsDone`).
/// @param History Array of `(MaxRounds + 1) * TermCount`. Snapshot `k` (row `k`) holds the coefficients straight after fit number `k`, BEFORE the next throwing out. So snapshot 0 is the fit with every term, and a term below the threshold in snapshot `k` is the one that dies in round `k + 1`.
/// @param ActiveHistory Same shape as `History`: which terms took part in fit number `k`.
typedef struct {
    Matrix* Theta ;
    double* Target ;
    double Threshold, Ridge ;
    int MaxRounds ;
    size_t TermCount, ActiveCount ;
    bool* IsActive ;
    double* Coefficients ;
    int RoundsDone ;
    bool IsDone ;
    size_t SnapshotCount ;
    double* History ;
    bool* ActiveHistory ;
} StlsqJob;



/// @brief Saves the current coefficients and active flags as the next snapshot of a job.
/// @param Job Address of the job.
void StlsqJobSaveSnapshot (StlsqJob* Job)
{
    size_t Row = (Job->SnapshotCount) ;
    for (size_t ThisTerm=0; ThisTerm<(Job->TermCount); ThisTerm++)
    {
        (Job->History)[Row*(Job->TermCount) + ThisTerm] = (Job->Coefficients)[ThisTerm] ;
        (Job->ActiveHistory)[Row*(Job->TermCount) + ThisTerm] = (Job->IsActive)[ThisTerm] ;
    }
    (Job->SnapshotCount)++ ;
    return;
}



/// @brief Starts a STLSQ job: all terms alive, first fit done, snapshot 0 saved.
/// @param Theta Address of the term matrix.
/// @param Target The derivative values, `Theta->RowCount` of them.
/// @param Threshold Terms with absolute coefficient below this are thrown out.
/// @param Ridge Tiny stabilizer, `1e-10` to start.
/// @param MaxRounds Most number of rounds, `10` is plenty.
/// @return The new `StlsqJob`. Call `StlsqJobStep` until `IsDone`.
/// @warning Result needs a `StlsqJobDestroy` later. `Theta` and `Target` are borrowed, not copied.
StlsqJob StlsqJobStart (Matrix* Theta, double* Target, double Threshold, double Ridge, int MaxRounds)
{
    StlsqJob Job ;
    size_t TermCount = (Theta->ColCount) ;
    size_t SnapshotRoom = (size_t)(MaxRounds + 1) ;

    (Job.Theta) = Theta ;
    (Job.Target) = Target ;
    (Job.Threshold) = Threshold ;
    (Job.Ridge) = Ridge ;
    (Job.MaxRounds) = MaxRounds ;
    (Job.TermCount) = TermCount ;
    (Job.ActiveCount) = TermCount ;
    (Job.IsActive) = (bool*) calloc(TermCount, sizeof(bool)) ;
    (Job.Coefficients) = (double*) calloc(TermCount, sizeof(double)) ;
    (Job.RoundsDone) = 0 ;
    (Job.IsDone) = (MaxRounds <= 0) ;
    (Job.SnapshotCount) = 0 ;
    (Job.History) = (double*) calloc(SnapshotRoom*TermCount, sizeof(double)) ;
    (Job.ActiveHistory) = (bool*) calloc(SnapshotRoom*TermCount, sizeof(bool)) ;

    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++) (Job.IsActive)[ThisTerm] = true ;

    //First fit uses everything
    FitActiveColumns(Theta, Target, (Job.IsActive), (Job.ActiveCount), Ridge, (Job.Coefficients));
    StlsqJobSaveSnapshot(&Job);
    return Job;
}



/// @brief Does ONE round of STLSQ: throw out the small terms, then refit the survivors and save a snapshot. Does nothing if the job is already done.
/// @param Job Address of the job.
/// @attention The job becomes done when (a) nobody was below the threshold, (b) everything was thrown out, or (c) `MaxRounds` rounds were used. Rounds (a) and (b) do not make a new snapshot.
void StlsqJobStep (StlsqJob* Job)
{
    if (Job->IsDone) return;

    //Throw out the small ones
    size_t ThrownOutCount = 0 ;
    for (size_t ThisTerm=0; ThisTerm<(Job->TermCount); ThisTerm++)
    {
        if ((Job->IsActive)[ThisTerm] && fabs((Job->Coefficients)[ThisTerm]) < (Job->Threshold))
        {
            (Job->IsActive)[ThisTerm] = false ;
            (Job->Coefficients)[ThisTerm] = 0.0 ;
            ThrownOutCount++ ;
        }
    }

    if (ThrownOutCount == 0)    //Nobody left to throw out, we are done
    {
        (Job->IsDone) = true ;
        return;
    }
    (Job->ActiveCount) -= ThrownOutCount ;
    if ((Job->ActiveCount) == 0)    //Everything is gone, all coefficients are already zero
    {
        (Job->IsDone) = true ;
        return;
    }

    //Refit the survivors, their values change a bit now that the small ones are gone
    FitActiveColumns((Job->Theta), (Job->Target), (Job->IsActive), (Job->ActiveCount), (Job->Ridge), (Job->Coefficients));
    (Job->RoundsDone)++ ;
    StlsqJobSaveSnapshot(Job);
    if ((Job->RoundsDone) >= (Job->MaxRounds)) (Job->IsDone) = true ;
    return;
}



/// @brief Frees the memory of a job (not `Theta` or `Target`, those were only borrowed).
/// @param Job Address of the job.
void StlsqJobDestroy (StlsqJob* Job)
{
    free(Job->IsActive);
    free(Job->Coefficients);
    free(Job->History);
    free(Job->ActiveHistory);
    (Job->IsActive) = NULL ;
    (Job->Coefficients) = NULL ;
    (Job->History) = NULL ;
    (Job->ActiveHistory) = NULL ;
    return;
}



/// @brief Sequentially Thresholded Least Squares (STLSQ), the heart of SINDy. Finds FEW terms of `Theta` that add up to `Target`.
/// @param Theta Address of the term matrix (from `BuildTermMatrix`). One row per sample, one column per candidate term.
/// @param Target The derivative values (`dx` or `dy`), `Theta->RowCount` values in `double`.
/// @param Threshold Any coefficient with absolute value below this is thrown out. Try `0.05` to start.
/// @param Ridge Tiny stabilizer for each least squares fit, see `LeastSquaresSolve`. Use `1e-10` to start.
/// @param MaxIterations Most number of "throw out small ones and refit" rounds. `10` is plenty, it usually settles in 2 or 3.
/// @param Coefficients Container for the answer, `Theta->ColCount` values in `double`. A thrown out term is exactly `0.0`.
/// @return How many rounds actually threw something out. `0` means the first fit already had nothing below the threshold.
/// @attention Steps: (1) fit all columns, (2) zero every coefficient below `Threshold` and forget its column, (3) refit the survivors, (4) repeat until nobody gets thrown out. Run it once for `dx` and once for `dy`.
/// @attention `Threshold` is absolute, so it should be judged against the size of the true coefficients. Columns of very different size (like `x^3` next to `1`) make one number unfair to all of them.
/// @warning If every term gets thrown out, the answer is all zeros. No error is reported.
int SparseRegressionSTLSQ (Matrix* Theta, double* Target, double Threshold, double Ridge, int MaxIterations, double* Coefficients)
{
    //The same algorithm as the stepped job, just run to the end in one go (one source of truth)
    StlsqJob Job = StlsqJobStart(Theta, Target, Threshold, Ridge, MaxIterations) ;
    while (!(Job.IsDone))
    {
        StlsqJobStep(&Job);
    }

    for (size_t ThisTerm=0; ThisTerm<(Job.TermCount); ThisTerm++)
    {
        Coefficients[ThisTerm] = (Job.Coefficients)[ThisTerm] ;
    }

    int RoundsDone = (Job.RoundsDone) ;
    StlsqJobDestroy(&Job);
    return RoundsDone;
}
