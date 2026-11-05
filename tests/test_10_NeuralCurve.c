//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>
#include <OdeSolverAndSystems.h>
#include <DerivativeEstimation.h>
#include <NeuralDerivativeNetwork.h>



/// @brief Prints a full guide for this program if the first argument is `-h` or `--help`.
/// @param argc Argument count from `main`.
/// @param argv Argument values from `main`.
/// @return `1` if the guide was printed (the caller should stop), `0` otherwise.
int PrintHelpIfAsked (int argc, char** argv)
{
    if (argc < 2) return 0;
    if ((strcmp(argv[1],"-h") != 0) && (strcmp(argv[1],"--help") != 0)) return 0;

    printf("\nTEST 10: NEURAL CURVE (Week 3, Idea B: the network is the curve)");
    printf("\n");
    printf("\nWhat it does:");
    printf("\n  Block 1 checks the hand-worked example (1 hidden unit, 1 data point) and");
    printf("\n  compares the backprop gradient with a numerical gradient.");
    printf("\n  Block 2 fits a small network to a noisy circle (x = sin t, y = cos t),");
    printf("\n  and compares the slope error of finite differences, Savitzky-Golay and the network.");
    printf("\n");
    printf("\nUsage:");
    printf("\n  test_10_NeuralCurve.exe [HiddenCount] [StepCount] [NoiseSigma] [Seed]");
    printf("\n");
    printf("\nArguments:");
    printf("\n  HiddenCount  tanh units per network, default 8");
    printf("\n  StepCount    training steps per network, default 2000");
    printf("\n  NoiseSigma   width of the noise added to x and y, default 0.2");
    printf("\n  Seed         random seed, default 7");
    printf("\n");
    printf("\nExamples:");
    printf("\n  test_10_NeuralCurve.exe");
    printf("\n  test_10_NeuralCurve.exe 40 20000 0.2    (too many units, long training: overfit)");
    printf("\n");
    printf("\nWhat a good result looks like:");
    printf("\n  Block 1: every line says MATCH, analytic and numeric gradients agree to about 8 digits.");
    printf("\n  Block 2: the loss goes down, and the network slope error is smaller than the finite difference one.");
    printf("\n");
    return 1;
}



/// @brief Root mean square difference between two `RowCount x 2` derivative tables.
/// @param Estimate Address of the estimated derivatives.
/// @param Truth Address of the true derivatives.
/// @return RMS error over all rows and both columns, in `double`.
double RmsSlopeError (Matrix* Estimate, Matrix* Truth)
{
    double Sum = 0.0 ;
    for (size_t ThisRow=0; ThisRow<(Truth->RowCount); ThisRow++)
    {
        for (size_t ThisCol=0; ThisCol<2; ThisCol++)
        {
            double Miss = GetMatrixCell(Estimate,ThisRow,ThisCol) - GetMatrixCell(Truth,ThisRow,ThisCol) ;
            Sum += Miss*Miss ;
        }
    }
    return sqrt(Sum / (2.0*(double)(Truth->RowCount)));
}



int main (int argc, char** argv)
{
    if (PrintHelpIfAsked(argc, argv)) return 0;

    size_t HiddenCount = (argc>1) ? (size_t)atoi(argv[1]) : 8 ;
    size_t StepCount = (argc>2) ? (size_t)atoi(argv[2]) : 2000 ;
    double NoiseSigma = (argc>3) ? atof(argv[3]) : 0.2 ;
    int Seed = (argc>4) ? atoi(argv[4]) : 7 ;

    RandomState Gen ;
    RandomSeed(&Gen, (uint64_t)Seed);

    #if 1   //To check the hand-worked example and the backprop gradient.
        printf("\n[Block 1] Hand-worked example: 1 unit, w=0.5, b=0, v=2, c=0.1, one point s=2, target 1.5");
        CurveNetwork Hand = CurveNetworkCreate(1, &Gen) ;
        (Hand.Params)[0] = 0.5 ;    //w
        (Hand.Params)[1] = 0.0 ;    //b
        (Hand.Params)[2] = 2.0 ;    //v
        (Hand.Params)[3] = 0.1 ;    //c

        double HandInput = 2.0 ;
        double HandTarget = 1.5 ;
        double HandGradient[4] ;
        double HandLoss = CurveLossAndGradient(&Hand, &HandInput, &HandTarget, 1, HandGradient) ;

        printf("\n  N(2)    = %.6f   (by hand 1.623188)", EvaluateCurve(&Hand, HandInput));
        printf("\n  N'(2)   = %.6f   (by hand 0.419974)", EvaluateCurveSlope(&Hand, HandInput));
        printf("\n  Loss    = %.6f   (by hand 0.007588)", HandLoss);

        printf("\n  Gradient check, order: w, b, v, c  (analytic from backprop vs numeric from nudging)");
        for (size_t ThisParam=0; ThisParam<4; ThisParam++)
        {
            double Nudge = 1e-6 ;
            double Scratch[4] ;
            double Saved = (Hand.Params)[ThisParam] ;
            (Hand.Params)[ThisParam] = Saved + Nudge ;
            double LossUp = CurveLossAndGradient(&Hand, &HandInput, &HandTarget, 1, Scratch) ;
            (Hand.Params)[ThisParam] = Saved - Nudge ;
            double LossDown = CurveLossAndGradient(&Hand, &HandInput, &HandTarget, 1, Scratch) ;
            (Hand.Params)[ThisParam] = Saved ;

            double Numeric = (LossUp - LossDown) / (2.0*Nudge) ;
            printf("\n    %.8f   %.8f   %s", HandGradient[ThisParam], Numeric, (fabs(HandGradient[ThisParam]-Numeric) < 1e-7) ? "MATCH" : "MISMATCH");
        }
        CurveNetworkDestroy(&Hand);
    #endif



    #if 1   //To compare finite differences, Savitzky-Golay and the neural curve on a noisy circle.
        size_t PointCount = 60 ;
        Matrix Path = MatrixCreate(PointCount, 3) ;
        Matrix Truth = MatrixCreate(PointCount, 2) ;
        for (size_t ThisPoint=0; ThisPoint<PointCount; ThisPoint++)
        {
            double Time = 2.0*3.14159265358979323846*(double)ThisPoint/(double)(PointCount-1) ;
            SetMatrixCell(&Path, ThisPoint, 0, Time);
            SetMatrixCell(&Path, ThisPoint, 1, sin(Time));
            SetMatrixCell(&Path, ThisPoint, 2, cos(Time));
            SetMatrixCell(&Truth, ThisPoint, 0, cos(Time));
            SetMatrixCell(&Truth, ThisPoint, 1, -sin(Time));
        }
        AddGaussianNoise(&Path, NoiseSigma, &Gen);

        double* LossHistory = (double*) calloc(StepCount, sizeof(double)) ;
        Matrix FromFd = FiniteDifferenceDerivatives(&Path) ;
        Matrix FromSg = SavitzkyGolayDerivatives(&Path, 5, 2) ;
        Matrix FromNn = NeuralDerivatives(&Path, HiddenCount, StepCount, 0.01, &Gen, LossHistory) ;

        printf("\n\n[Block 2] Noisy circle, %zu points, noise sigma %.3f, %zu hidden units, %zu steps", PointCount, NoiseSigma, HiddenCount, StepCount);
        printf("\n  Loss curve of the x network (rescaled units):");
        for (size_t ThisMark=0; ThisMark<5; ThisMark++)
        {
            size_t ThisStep = (ThisMark*(StepCount-1)) / 4 ;
            printf("\n    step %6zu   loss %.6f", ThisStep, LossHistory[ThisStep]);
        }
        printf("\n  RMS slope error (smaller is better, true slopes are cos t and -sin t):");
        printf("\n    Finite differences   %.4f", RmsSlopeError(&FromFd, &Truth));
        printf("\n    Savitzky-Golay       %.4f", RmsSlopeError(&FromSg, &Truth));
        printf("\n    Neural curve         %.4f", RmsSlopeError(&FromNn, &Truth));
        printf("\n");

        free(LossHistory);
        MatrixDestroy(&FromFd);
        MatrixDestroy(&FromSg);
        MatrixDestroy(&FromNn);
        MatrixDestroy(&Truth);
        MatrixDestroy(&Path);
    #endif

    return 0;
}
