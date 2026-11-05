//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>



/// @brief A tiny neural network that IS the curve: one input (time), one output (a value like `x`), one hidden layer of `tanh` units. `N(s) = c + sum_j v_j * tanh(w_j*s + b_j)`.
/// @param HiddenCount How many `tanh` units there are (`H`). More units means a more flexible (and more noise chasing) curve.
/// @param Params All the numbers the network learns, in ONE flat array of `3*H + 1` values: `w_0..w_(H-1)`, then `b_0..b_(H-1)`, then `v_0..v_(H-1)`, then `c`. Gradients and Adam memory use the same layout.
/// @attention The flat layout is on purpose: a gradient is just another array of the same length, so the optimizer needs no special cases.
typedef struct {
    size_t HiddenCount ;
    double* Params ;
} CurveNetwork;



/// @brief Holds the running averages that Adam needs, one pair of numbers per learnable number of the network.
/// @param ParamCount Length of both arrays, same as `3*H + 1` of the network.
/// @param StepCount How many Adam steps were taken so far (needed for the bias correction).
/// @param FirstMoment Running average of the gradient (the "momentum").
/// @param SecondMoment Running average of the gradient squared (tells how big the usual gradient is, so every parameter gets a fair step size).
typedef struct {
    size_t ParamCount, StepCount ;
    double* FirstMoment ;
    double* SecondMoment ;
} AdamMemory;



/// @brief Makes a network with small random starting values.
/// @param HiddenCount Number of `tanh` units.
/// @param Gen Address of the random generator.
/// @return A new `CurveNetwork`. Starting values: `w` and `b` uniform in `[-3, 3]`, `v` uniform in `[-0.3, 0.3]`, `c = 0`.
/// @attention Starting `w` and `b` are spread out so the `tanh` steps sit at different places and different steepnesses. All units identical would mean all units learn identical things.
/// @warning Result needs a `CurveNetworkDestroy` later.
CurveNetwork CurveNetworkCreate (size_t HiddenCount, RandomState* Gen)
{
    CurveNetwork Net ;
    (Net.HiddenCount) = HiddenCount ;
    (Net.Params) = (double*) calloc(3*HiddenCount + 1, sizeof(double)) ;

    for (size_t ThisUnit=0; ThisUnit<HiddenCount; ThisUnit++)
    {
        (Net.Params)[ThisUnit] = 6.0*RandomUniform(Gen) - 3.0 ;                  //w
        (Net.Params)[HiddenCount + ThisUnit] = 6.0*RandomUniform(Gen) - 3.0 ;    //b
        (Net.Params)[2*HiddenCount + ThisUnit] = 0.6*RandomUniform(Gen) - 0.3 ;  //v
    }

    return Net;
}



/// @brief Frees the memory of a network.
/// @param Net Address of the network.
void CurveNetworkDestroy (CurveNetwork* Net)
{
    free(Net->Params);
    (Net->Params) = NULL ;
    (Net->HiddenCount) = 0 ;
    return;
}



/// @brief Computes the value of the curve, `N(s)`.
/// @param Net Address of the network.
/// @param Input The input `s` in `double` (time, ideally rescaled to about `[-1, 1]`).
/// @return `N(s)` in `double`.
double EvaluateCurve (CurveNetwork* Net, double Input)
{
    size_t HiddenCount = (Net->HiddenCount) ;
    double* Params = (Net->Params) ;
    double Output = Params[3*HiddenCount] ;

    for (size_t ThisUnit=0; ThisUnit<HiddenCount; ThisUnit++)
    {
        Output += Params[2*HiddenCount + ThisUnit] * tanh(Params[ThisUnit]*Input + Params[HiddenCount + ThisUnit]) ;
    }

    return Output;
}



/// @brief Computes the slope of the curve, `dN/ds`, exactly (not by finite differences).
/// @param Net Address of the network.
/// @param Input The input `s` in `double`.
/// @return `dN/ds = sum_j v_j * w_j * (1 - tanh^2(w_j*s + b_j))` in `double`.
/// @attention Uses `d tanh(z)/dz = 1 - tanh^2(z)`. Smooth everywhere, so no noise amplification from differencing.
/// @attention This is the slope with respect to the input `s`. If time was rescaled, the caller must scale it back.
double EvaluateCurveSlope (CurveNetwork* Net, double Input)
{
    size_t HiddenCount = (Net->HiddenCount) ;
    double* Params = (Net->Params) ;
    double Slope = 0.0 ;

    for (size_t ThisUnit=0; ThisUnit<HiddenCount; ThisUnit++)
    {
        double Activation = tanh(Params[ThisUnit]*Input + Params[HiddenCount + ThisUnit]) ;
        Slope += Params[2*HiddenCount + ThisUnit] * Params[ThisUnit] * (1.0 - Activation*Activation) ;
    }

    return Slope;
}



/// @brief Measures how badly the curve misses the data (the loss), and in the same pass computes how every learnable number should change to miss less (the gradient). This is backpropagation, written out by hand.
/// @param Net Address of the network.
/// @param Inputs Array of `SampleCount` inputs `s_k`.
/// @param Targets Array of `SampleCount` measured values `y_k`.
/// @param SampleCount How many data points there are.
/// @param Gradient Array of `3*H + 1` values. Overwritten with `dLoss/dParam`, same layout as `Params`.
/// @return The loss `L = (1 / 2K) * sum_k (N(s_k) - y_k)^2` in `double` (half the mean squared residual).
/// @attention The chain rule, per data point, with residual `r = N - y` and `a_j = tanh(w_j*s + b_j)`: `dL/dc = r`, `dL/dv_j = r*a_j`, `dL/db_j = r*v_j*(1 - a_j^2)`, `dL/dw_j = r*v_j*(1 - a_j^2)*s`. All divided by `K` at the end.
/// @attention The `1/2` in the loss is only there so the `2` from differentiating the square cancels.
/// @warning Nothing checks `Gradient` has enough room.
double CurveLossAndGradient (CurveNetwork* Net, double* Inputs, double* Targets, size_t SampleCount, double* Gradient)
{
    size_t HiddenCount = (Net->HiddenCount) ;
    size_t ParamCount = 3*HiddenCount + 1 ;
    double* Params = (Net->Params) ;
    double* Activations = (double*) calloc(HiddenCount, sizeof(double)) ;
    double Loss = 0.0 ;

    for (size_t ThisParam=0; ThisParam<ParamCount; ThisParam++)
    {
        Gradient[ThisParam] = 0.0 ;
    }

    for (size_t ThisSample=0; ThisSample<SampleCount; ThisSample++)
    {
        double Input = Inputs[ThisSample] ;
        double Output = Params[3*HiddenCount] ;

        //Forward pass: remember every tanh, the backward pass needs them
        for (size_t ThisUnit=0; ThisUnit<HiddenCount; ThisUnit++)
        {
            Activations[ThisUnit] = tanh(Params[ThisUnit]*Input + Params[HiddenCount + ThisUnit]) ;
            Output += Params[2*HiddenCount + ThisUnit] * Activations[ThisUnit] ;
        }

        double Residual = Output - Targets[ThisSample] ;
        Loss += 0.5 * Residual * Residual ;

        //Backward pass: chain rule, starting from the residual
        Gradient[3*HiddenCount] += Residual ;
        for (size_t ThisUnit=0; ThisUnit<HiddenCount; ThisUnit++)
        {
            double Activation = Activations[ThisUnit] ;
            double PushIntoUnit = Residual * Params[2*HiddenCount + ThisUnit] * (1.0 - Activation*Activation) ;
            Gradient[2*HiddenCount + ThisUnit] += Residual * Activation ;
            Gradient[HiddenCount + ThisUnit] += PushIntoUnit ;
            Gradient[ThisUnit] += PushIntoUnit * Input ;
        }
    }

    for (size_t ThisParam=0; ThisParam<ParamCount; ThisParam++)
    {
        Gradient[ThisParam] /= (double)SampleCount ;
    }

    free(Activations);
    return (Loss / (double)SampleCount);
}



/// @brief Makes an `AdamMemory` with everything at zero.
/// @param ParamCount Number of learnable numbers in the network (`3*H + 1`).
/// @return A new `AdamMemory`.
/// @warning Result needs an `AdamMemoryDestroy` later.
AdamMemory AdamMemoryCreate (size_t ParamCount)
{
    AdamMemory Memory ;
    (Memory.ParamCount) = ParamCount ;
    (Memory.StepCount) = 0 ;
    (Memory.FirstMoment) = (double*) calloc(ParamCount, sizeof(double)) ;
    (Memory.SecondMoment) = (double*) calloc(ParamCount, sizeof(double)) ;
    return Memory;
}



/// @brief Frees the memory of an `AdamMemory`.
/// @param Memory Address of the memory.
void AdamMemoryDestroy (AdamMemory* Memory)
{
    free(Memory->FirstMoment);
    free(Memory->SecondMoment);
    (Memory->FirstMoment) = NULL ;
    (Memory->SecondMoment) = NULL ;
    (Memory->ParamCount) = 0 ;
    return;
}



/// @brief One step of Adam: moves every learnable number a little downhill, with a step size adapted to how big its gradient usually is.
/// @param Net Address of the network. Its `Params` are changed.
/// @param Memory Address of the Adam memory, updated too.
/// @param Gradient Array from `CurveLossAndGradient`.
/// @param LearningRate Overall step size in `double`. `0.01` is a good start here.
/// @attention Formulas (`g` = gradient, `p` = parameter): `m = 0.9*m + 0.1*g`, `v = 0.999*v + 0.001*g^2`, then `p -= LearningRate * mHat / (sqrt(vHat) + 1e-8)`, where `mHat = m / (1 - 0.9^t)` and `vHat = v / (1 - 0.999^t)` fix the bias from starting at zero.
/// @attention Compare with plain gradient descent `p -= LearningRate * g`: there, a parameter with a tiny gradient barely moves and one with a huge gradient overshoots. Adam divides by the typical gradient size, so each parameter moves by about `LearningRate`.
void AdamStep (CurveNetwork* Net, AdamMemory* Memory, double* Gradient, double LearningRate)
{
    (Memory->StepCount) += 1 ;
    double BiasFixFirst = 1.0 - pow(0.9, (double)(Memory->StepCount)) ;
    double BiasFixSecond = 1.0 - pow(0.999, (double)(Memory->StepCount)) ;

    for (size_t ThisParam=0; ThisParam<(Memory->ParamCount); ThisParam++)
    {
        (Memory->FirstMoment)[ThisParam] = 0.9*(Memory->FirstMoment)[ThisParam] + 0.1*Gradient[ThisParam] ;
        (Memory->SecondMoment)[ThisParam] = 0.999*(Memory->SecondMoment)[ThisParam] + 0.001*Gradient[ThisParam]*Gradient[ThisParam] ;

        double FirstHat = (Memory->FirstMoment)[ThisParam] / BiasFixFirst ;
        double SecondHat = (Memory->SecondMoment)[ThisParam] / BiasFixSecond ;
        (Net->Params)[ThisParam] -= LearningRate * FirstHat / (sqrt(SecondHat) + 1e-8) ;
    }

    return;
}



/// @brief One step of plain gradient descent: `p -= LearningRate * g` for every learnable number. The simplest possible optimizer, kept as an option to compare with Adam.
/// @param Net Address of the network. Its `Params` are changed.
/// @param Gradient Array from `CurveLossAndGradient`.
/// @param LearningRate Step size in `double`. Needs to be chosen more carefully than for Adam.
void GradientDescentStep (CurveNetwork* Net, double* Gradient, double LearningRate)
{
    size_t ParamCount = 3*(Net->HiddenCount) + 1 ;

    for (size_t ThisParam=0; ThisParam<ParamCount; ThisParam++)
    {
        (Net->Params)[ThisParam] -= LearningRate * Gradient[ThisParam] ;
    }

    return;
}



/// @brief Trains the network so its curve passes through the middle of the data points. Repeats: measure the loss and gradient, then take one optimizer step.
/// @param Net Address of the network. Trained in place.
/// @param Inputs Array of `SampleCount` inputs `s_k` (rescaled to about `[-1, 1]`).
/// @param Targets Array of `SampleCount` values `y_k` (rescaled to about `O(1)`).
/// @param SampleCount How many data points there are.
/// @param StepCount How many training steps to take.
/// @param LearningRate Step size in `double`. `0.01` is a good start for Adam.
/// @param LossHistory Array of `StepCount` values that receives the loss before each step (the loss curve). Can be `NULL` if you do not want it.
/// @attention More steps is not always better: once the curve starts chasing the noise, the fit looks better but the slope gets worse.
void FitCurveToSamples (CurveNetwork* Net, double* Inputs, double* Targets, size_t SampleCount, size_t StepCount, double LearningRate, double* LossHistory)
{
    size_t ParamCount = 3*(Net->HiddenCount) + 1 ;
    double* Gradient = (double*) calloc(ParamCount, sizeof(double)) ;
    AdamMemory Memory = AdamMemoryCreate(ParamCount) ;

    for (size_t ThisStep=0; ThisStep<StepCount; ThisStep++)
    {
        double Loss = CurveLossAndGradient(Net, Inputs, Targets, SampleCount, Gradient) ;
        if (LossHistory != NULL)
        {
            LossHistory[ThisStep] = Loss ;
        }

        #if 1   //To train with Adam.
            AdamStep(Net, &Memory, Gradient, LearningRate);
        #elif 0 //To train with plain gradient descent.
            GradientDescentStep(Net, Gradient, LearningRate);
        #endif
    }

    AdamMemoryDestroy(&Memory);
    free(Gradient);
    return;
}



/// @brief Estimates `dx/dt` and `dy/dt` by fitting a small network to the noisy points (one network for `x`, one for `y`) and differentiating the network itself. Drop-in replacement for `FiniteDifferenceDerivatives`.
/// @param Path Address of a trajectory (columns `t, x, y`), noisy is fine. Rows do not need to be evenly spaced.
/// @param HiddenCount Number of `tanh` units per network. Acts like the Savitzky-Golay window: fewer units, stiffer curve, more noise averaged out.
/// @param StepCount Number of training steps per network.
/// @param LearningRate Adam step size. `0.01` is a good start.
/// @param Gen Address of the random generator (for the starting values of the networks).
/// @param LossHistory Array of `StepCount` values that receives the loss curve of the `x` network (in rescaled units). Can be `NULL`.
/// @return A new `Matrix` of size `RowCount x 2`. Columns are `dx/dt, dy/dt`, same layout as `TrueDerivativesOfTrajectory`.
/// @attention Rescaling, like nondimensionalizing a physics problem: time is mapped to `s` in `[-1, 1]` and each of `x`, `y` is shifted and divided by its standard deviation. Without it `tanh` saturates and learning stalls. The slope is converted back with `dx/dt = StdDev * (dN/ds) / HalfSpan`.
/// @warning Needs a path that is not constant (a `StdDev` of 0 divides by zero). Result needs a `MatrixDestroy` later.
/// @warning Cost per training step grows with `RowCount * HiddenCount`. Very long paths with many oscillations are hard for a smooth network to fit (it learns slow wiggles first), so check the loss curve.
Matrix NeuralDerivatives (Matrix* Path, size_t HiddenCount, size_t StepCount, double LearningRate, RandomState* Gen, double* LossHistory)
{
    size_t RowCount = (Path->RowCount) ;
    double StartTime = GetMatrixCell(Path,0,0) ;
    double EndTime = GetMatrixCell(Path,RowCount-1,0) ;
    double MiddleTime = 0.5*(StartTime + EndTime) ;
    double HalfSpan = 0.5*(EndTime - StartTime) ;

    Matrix Derivatives = MatrixCreate(RowCount, 2) ;
    double* Inputs = (double*) calloc(RowCount, sizeof(double)) ;
    double* Targets = (double*) calloc(RowCount, sizeof(double)) ;

    for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
    {
        Inputs[ThisRow] = (GetMatrixCell(Path,ThisRow,0) - MiddleTime) / HalfSpan ;
    }

    for (size_t ThisCol=1; ThisCol<=2; ThisCol++)
    {
        double Mean = 0.0 ;
        double Variance = 0.0 ;
        for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
        {
            Mean += GetMatrixCell(Path,ThisRow,ThisCol) ;
        }
        Mean /= (double)RowCount ;
        for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
        {
            double Offset = GetMatrixCell(Path,ThisRow,ThisCol) - Mean ;
            Variance += Offset*Offset ;
        }
        double StdDev = sqrt(Variance / (double)RowCount) ;

        for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
        {
            Targets[ThisRow] = (GetMatrixCell(Path,ThisRow,ThisCol) - Mean) / StdDev ;
        }

        CurveNetwork Net = CurveNetworkCreate(HiddenCount, Gen) ;
        FitCurveToSamples(&Net, Inputs, Targets, RowCount, StepCount, LearningRate, ((ThisCol==1) ? LossHistory : NULL));

        for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
        {
            double Slope = StdDev * EvaluateCurveSlope(&Net, Inputs[ThisRow]) / HalfSpan ;
            SetMatrixCell(&Derivatives, ThisRow, ThisCol-1, Slope);
        }

        CurveNetworkDestroy(&Net);
    }

    free(Inputs);
    free(Targets);
    return Derivatives;
}



/// @brief A chunked neural derivative estimation that can be advanced a little at a time (for animations and progress bars). For every chunk of the path, one small network is fitted for `x` and one for `y`; only the middle third of each chunk's slopes is kept.
/// @param Path Address of the trajectory (columns `t, x, y`). Borrowed, not copied: it must stay alive and unchanged while the job runs.
/// @param RowCount Number of rows of `Path`.
/// @param ChunkSize Rows per chunk (the whole path if it is shorter).
/// @param KeepSize Rows kept from every chunk (`ChunkSize / 3`).
/// @param HiddenCount Number of `tanh` units per network.
/// @param StepsPerChunk Training steps per chunk.
/// @param LearningRate Adam step size.
/// @param Gen Address of the random generator (for the starting values of every network).
/// @param Derivatives The result, `RowCount x 2` (`dx/dt, dy/dt`), filled chunk by chunk. Owned by the caller once the job is done (`NeuralJobDestroy` does NOT free it).
/// @param ChunkCount How many chunks there are.
/// @param ChunkIndex Which chunk is being fitted now (equals `ChunkCount` when done).
/// @param ChunkStart First row of the current chunk.
/// @param KeepStart First row of the current chunk whose slopes are kept.
/// @param StepsDone Training steps done on the current chunk.
/// @param IsDone `true` when every chunk is finished and `Derivatives` is complete.
/// @param MiddleTime Middle of the current chunk's time range (time is rescaled to `[-1, 1]` around it).
/// @param HalfSpan Half of the current chunk's time range.
/// @param MeanX Mean of `x` over the current chunk (values are rescaled to zero mean and unit spread).
/// @param StdX Standard deviation of `x` over the current chunk.
/// @param MeanY Mean of `y` over the current chunk.
/// @param StdY Standard deviation of `y` over the current chunk.
/// @param Inputs The rescaled times of the current chunk.
/// @param TargetsX The rescaled `x` values of the current chunk.
/// @param TargetsY The rescaled `y` values of the current chunk.
/// @param NetX The network being fitted to `x` (valid while not done).
/// @param NetY The network being fitted to `y`.
/// @param MemX Adam memory of `NetX`.
/// @param MemY Adam memory of `NetY`.
/// @param GradientX Gradient buffer of `NetX`.
/// @param GradientY Gradient buffer of `NetY`.
/// @param LossX Last loss of `NetX` (rescaled units).
/// @param LossY Last loss of `NetY`.
typedef struct {
    Matrix* Path ;
    size_t RowCount, ChunkSize, KeepSize, HiddenCount, StepsPerChunk ;
    double LearningRate ;
    RandomState* Gen ;
    Matrix Derivatives ;
    size_t ChunkCount, ChunkIndex, ChunkStart, KeepStart, StepsDone ;
    bool IsDone ;
    double MiddleTime, HalfSpan, MeanX, StdX, MeanY, StdY ;
    double* Inputs ;
    double* TargetsX ;
    double* TargetsY ;
    CurveNetwork NetX, NetY ;
    AdamMemory MemX, MemY ;
    double* GradientX ;
    double* GradientY ;
    double LossX, LossY ;
} NeuralJob;



/// @brief Rescales one column of the current chunk to zero mean and unit spread.
/// @param Job Address of the job.
/// @param Col Which column of the path (1 for `x`, 2 for `y`).
/// @param Targets Receives the rescaled values.
/// @param Mean Receives the mean.
/// @param StdDev Receives the standard deviation.
void NeuralJobRescaleColumn (NeuralJob* Job, size_t Col, double* Targets, double* Mean, double* StdDev)
{
    double Sum = 0.0 ;
    double Variance = 0.0 ;
    for (size_t ThisRow=0; ThisRow<(Job->ChunkSize); ThisRow++) Sum += GetMatrixCell((Job->Path),(Job->ChunkStart)+ThisRow,Col) ;
    (*Mean) = Sum / (double)(Job->ChunkSize) ;
    for (size_t ThisRow=0; ThisRow<(Job->ChunkSize); ThisRow++)
    {
        double Offset = GetMatrixCell((Job->Path),(Job->ChunkStart)+ThisRow,Col) - (*Mean) ;
        Variance += Offset*Offset ;
    }
    (*StdDev) = sqrt(Variance / (double)(Job->ChunkSize)) ;
    for (size_t ThisRow=0; ThisRow<(Job->ChunkSize); ThisRow++) Targets[ThisRow] = (GetMatrixCell((Job->Path),(Job->ChunkStart)+ThisRow,Col) - (*Mean)) / (*StdDev) ;
    return;
}



/// @brief Prepares the chunk `ChunkIndex`: finds its rows, rescales its data and creates fresh networks.
/// @param Job Address of the job.
void NeuralJobBeginChunk (NeuralJob* Job)
{
    (Job->KeepStart) = (Job->ChunkIndex) * (Job->KeepSize) ;
    (Job->ChunkStart) = ((Job->KeepStart) > (Job->KeepSize)) ? ((Job->KeepStart) - (Job->KeepSize)) : 0 ;
    if ((Job->ChunkStart) + (Job->ChunkSize) > (Job->RowCount)) (Job->ChunkStart) = (Job->RowCount) - (Job->ChunkSize) ;

    double StartTime = GetMatrixCell((Job->Path), (Job->ChunkStart), 0) ;
    double EndTime = GetMatrixCell((Job->Path), (Job->ChunkStart)+(Job->ChunkSize)-1, 0) ;
    (Job->MiddleTime) = 0.5*(StartTime + EndTime) ;
    (Job->HalfSpan) = 0.5*(EndTime - StartTime) ;
    for (size_t ThisRow=0; ThisRow<(Job->ChunkSize); ThisRow++)
    {
        (Job->Inputs)[ThisRow] = (GetMatrixCell((Job->Path),(Job->ChunkStart)+ThisRow,0) - (Job->MiddleTime)) / (Job->HalfSpan) ;
    }
    NeuralJobRescaleColumn(Job, 1, (Job->TargetsX), &(Job->MeanX), &(Job->StdX));
    NeuralJobRescaleColumn(Job, 2, (Job->TargetsY), &(Job->MeanY), &(Job->StdY));

    (Job->NetX) = CurveNetworkCreate((Job->HiddenCount), (Job->Gen)) ;
    (Job->NetY) = CurveNetworkCreate((Job->HiddenCount), (Job->Gen)) ;
    (Job->MemX) = AdamMemoryCreate(3*(Job->HiddenCount) + 1) ;
    (Job->MemY) = AdamMemoryCreate(3*(Job->HiddenCount) + 1) ;
    (Job->StepsDone) = 0 ;
    return;
}



/// @brief Starts a chunked neural derivative estimation.
/// @param Path Address of the trajectory (columns `t, x, y`). Not copied.
/// @param ChunkSize Rows per chunk, `150` is a good start. A shorter path is one chunk.
/// @param HiddenCount Number of `tanh` units per network.
/// @param StepsPerChunk Training steps per chunk.
/// @param LearningRate Adam step size, `0.01` is a good start.
/// @param Gen Address of the random generator.
/// @return The new `NeuralJob`. Call `NeuralJobWork` until `IsDone`.
/// @warning Result needs a `NeuralJobDestroy` later. `Path` and `Gen` must stay alive meanwhile.
NeuralJob NeuralJobStart (Matrix* Path, size_t ChunkSize, size_t HiddenCount, size_t StepsPerChunk, double LearningRate, RandomState* Gen)
{
    NeuralJob Job ;
    memset(&Job, 0, sizeof(Job));
    (Job.Path) = Path ;
    (Job.RowCount) = (Path->RowCount) ;
    (Job.ChunkSize) = (ChunkSize < (Job.RowCount)) ? ChunkSize : (Job.RowCount) ;
    (Job.KeepSize) = ((Job.ChunkSize) < (Job.RowCount)) ? ((Job.ChunkSize)/3) : (Job.RowCount) ;
    (Job.HiddenCount) = HiddenCount ;
    (Job.StepsPerChunk) = StepsPerChunk ;
    (Job.LearningRate) = LearningRate ;
    (Job.Gen) = Gen ;
    (Job.Derivatives) = MatrixCreate((Job.RowCount), 2) ;
    (Job.ChunkCount) = ((Job.RowCount) + (Job.KeepSize) - 1) / (Job.KeepSize) ;
    (Job.ChunkIndex) = 0 ;
    (Job.IsDone) = false ;
    (Job.Inputs) = (double*) calloc((Job.ChunkSize), sizeof(double)) ;
    (Job.TargetsX) = (double*) calloc((Job.ChunkSize), sizeof(double)) ;
    (Job.TargetsY) = (double*) calloc((Job.ChunkSize), sizeof(double)) ;
    (Job.GradientX) = (double*) calloc(3*HiddenCount + 1, sizeof(double)) ;
    (Job.GradientY) = (double*) calloc(3*HiddenCount + 1, sizeof(double)) ;
    NeuralJobBeginChunk(&Job);
    return Job;
}



/// @brief Finishes the current chunk: writes the slopes of its kept rows into `Derivatives`, then starts the next chunk (or marks the job done).
/// @param Job Address of the job.
void NeuralJobFinishChunk (NeuralJob* Job)
{
    for (size_t ThisRow=(Job->KeepStart); (ThisRow<(Job->KeepStart)+(Job->KeepSize)) && (ThisRow<(Job->RowCount)); ThisRow++)
    {
        double Input = (Job->Inputs)[ThisRow - (Job->ChunkStart)] ;
        SetMatrixCell(&(Job->Derivatives), ThisRow, 0, (Job->StdX)*EvaluateCurveSlope(&(Job->NetX), Input)/(Job->HalfSpan));
        SetMatrixCell(&(Job->Derivatives), ThisRow, 1, (Job->StdY)*EvaluateCurveSlope(&(Job->NetY), Input)/(Job->HalfSpan));
    }

    CurveNetworkDestroy(&(Job->NetX));
    CurveNetworkDestroy(&(Job->NetY));
    AdamMemoryDestroy(&(Job->MemX));
    AdamMemoryDestroy(&(Job->MemY));
    (Job->ChunkIndex)++ ;
    if ((Job->ChunkIndex) >= (Job->ChunkCount)) (Job->IsDone) = true ;
    else NeuralJobBeginChunk(Job);
    return;
}



/// @brief Does up to `MaxSteps` training steps (one step trains both networks of the current chunk once), moving on to the next chunk when one is finished.
/// @param Job Address of the job.
/// @param MaxSteps Most number of steps to do in this call.
void NeuralJobWork (NeuralJob* Job, size_t MaxSteps)
{
    for (size_t ThisStep=0; (ThisStep<MaxSteps) && !(Job->IsDone); ThisStep++)
    {
        (Job->LossX) = CurveLossAndGradient(&(Job->NetX), (Job->Inputs), (Job->TargetsX), (Job->ChunkSize), (Job->GradientX)) ;
        AdamStep(&(Job->NetX), &(Job->MemX), (Job->GradientX), (Job->LearningRate));
        (Job->LossY) = CurveLossAndGradient(&(Job->NetY), (Job->Inputs), (Job->TargetsY), (Job->ChunkSize), (Job->GradientY)) ;
        AdamStep(&(Job->NetY), &(Job->MemY), (Job->GradientY), (Job->LearningRate));
        (Job->StepsDone)++ ;
        if ((Job->StepsDone) >= (Job->StepsPerChunk)) NeuralJobFinishChunk(Job);
    }
    return;
}



/// @brief Frees everything a job allocated EXCEPT `Derivatives` (the caller owns that matrix). If the job was stopped early, the unfinished chunk's networks are freed too.
/// @param Job Address of the job.
void NeuralJobDestroy (NeuralJob* Job)
{
    if (!(Job->IsDone))
    {
        CurveNetworkDestroy(&(Job->NetX));
        CurveNetworkDestroy(&(Job->NetY));
        AdamMemoryDestroy(&(Job->MemX));
        AdamMemoryDestroy(&(Job->MemY));
    }
    free(Job->Inputs);
    free(Job->TargetsX);
    free(Job->TargetsY);
    free(Job->GradientX);
    free(Job->GradientY);
    (Job->Inputs) = NULL ;
    return;
}



/// @brief Like `NeuralDerivatives`, but for LONG paths: the path is cut into overlapping chunks, a small network is fitted to each chunk, and only the middle third of every chunk is kept. Drop-in replacement for `FiniteDifferenceDerivatives`.
/// @param Path Address of a trajectory (columns `t, x, y`), noisy is fine.
/// @param ChunkSize Rows per chunk. `150` is a good start. If the path is not longer than this, it is one chunk.
/// @param HiddenCount Number of `tanh` units per network.
/// @param StepCount Number of training steps per network.
/// @param LearningRate Adam step size. `0.01` is a good start.
/// @param Gen Address of the random generator.
/// @return A new `Matrix` of size `RowCount x 2`. Columns are `dx/dt, dy/dt`.
/// @attention Why: one small smooth network cannot follow many oscillations (it learns slow wiggles first). Measured on the Linear Spiral with 3001 points: one network over the whole path gives a slope error of 0.58, chunks of 150 rows give 0.025. A short piece of curve is simple, even for 8 units.
/// @attention Why only the middle third: a curve fit is least trustworthy at the two ends of its data, so those slopes are thrown away and the neighbouring chunk supplies them. Only the very first and very last rows have to use a chunk end.
/// @attention This is just `NeuralJob` run to the end in one go (one source of truth).
/// @warning Costs about 3 times more than the chunk sizes alone suggest, because every row is fitted in 3 overlapping chunks. Result needs a `MatrixDestroy` later.
Matrix NeuralDerivativesChunked (Matrix* Path, size_t ChunkSize, size_t HiddenCount, size_t StepCount, double LearningRate, RandomState* Gen)
{
    NeuralJob Job = NeuralJobStart(Path, ChunkSize, HiddenCount, StepCount, LearningRate, Gen) ;
    while (!(Job.IsDone))
    {
        NeuralJobWork(&Job, 1000);
    }
    Matrix Derivatives = (Job.Derivatives) ;
    NeuralJobDestroy(&Job);
    return Derivatives;
}
