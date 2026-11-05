//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdlib.h>
#include <math.h>
#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>

#define MAX_SYSTEM_PARAMS 4



/// @brief Which 2D system we are talking about. Used with `switch` everywhere below.
/// @param SYSTEM_LINEAR_SPIRAL `dx = a*x + b*y`, `dy = c*x + d*y`. Spirals in or out (or circles).
/// @param SYSTEM_LOTKA_VOLTERRA Predator and prey. `dx = a*x - b*x*y`, `dy = d*x*y - c*y`.
/// @param SYSTEM_VAN_DER_POL Self-sustained oscillator. `dx = y`, `dy = Mu*(1 - x*x)*y - x`.
/// @param SYSTEM_DAMPED_PENDULUM Pendulum with friction. `dx = y`, `dy = -Freq2*sin(x) - Damping*y`.
/// @param SYSTEM_DUFFING Double well with friction (no driving force). `dx = y`, `dy = -Damping*y - Alpha*x - Beta*x*x*x`.
/// @param SYSTEM_COUNT Not a system. Just how many systems exist, handy for loops.
/// @attention `Params` for each system are listed in `EvaluateSystem`.
typedef enum {
    SYSTEM_LINEAR_SPIRAL,
    SYSTEM_LOTKA_VOLTERRA,
    SYSTEM_VAN_DER_POL,
    SYSTEM_DAMPED_PENDULUM,
    SYSTEM_DUFFING,
    SYSTEM_COUNT
} SystemID;



/// @brief A point in the 2D plane: where the system is right now.
/// @param x First variable.
/// @param y Second variable.
typedef struct {
    double x, y ;
} Position2D;



/// @brief How fast the system is changing at a point: the right hand side of the ODE. Same numbers as a `Position2D`, but a different physical thing (a rate, not a position), so it gets its own type.
/// @param dx_by_dt Rate of change of `x` per unit time.
/// @param dy_by_dt Rate of change of `y` per unit time.
typedef struct {
    double dx_by_dt, dy_by_dt ;
} Slope2D;



/// @brief Gives the human readable name of a system.
/// @param System Which system.
/// @return Name as a string.
const char* GetSystemName (SystemID System)
{
    switch (System)
    {
        case SYSTEM_LINEAR_SPIRAL:
            return "Linear Spiral";
        case SYSTEM_LOTKA_VOLTERRA:
            return "Lotka-Volterra";
        case SYSTEM_VAN_DER_POL:
            return "Van der Pol";
        case SYSTEM_DAMPED_PENDULUM:
            return "Damped Pendulum";
        case SYSTEM_DUFFING:
            return "Duffing";
        default:
            return "Unknown";
    }
}



/// @brief The right hand side of the ODE: given where we are, tells how fast we are moving, `(dx/dt, dy/dt)`.
/// @param System Which system.
/// @param State Where we are right now.
/// @param Params Constants of the system, in `double`.
/// @attention Meaning of `Params` per system:
/// @attention `SYSTEM_LINEAR_SPIRAL`: `Params[0..3] = a, b, c, d`.
/// @attention `SYSTEM_LOTKA_VOLTERRA`: `Params[0..3] = a, b, c, d`.
/// @attention `SYSTEM_VAN_DER_POL`: `Params[0] = Mu`.
/// @attention `SYSTEM_DAMPED_PENDULUM`: `Params[0] = Damping`, `Params[1] = Freq2` (that's `g/L`).
/// @attention `SYSTEM_DUFFING`: `Params[0..2] = Damping, Alpha, Beta`.
/// @return The slope as a `Slope2D`.
/// @warning Doesn't check that `Params` has enough values. `MAX_SYSTEM_PARAMS` is always enough.
Slope2D EvaluateSystem (SystemID System, Position2D State, double* Params)
{
    Slope2D Slope;
    double x = (State.x) ;
    double y = (State.y) ;

    switch (System)
    {
        case SYSTEM_LINEAR_SPIRAL:
            (Slope.dx_by_dt) = Params[0]*x + Params[1]*y ;
            (Slope.dy_by_dt) = Params[2]*x + Params[3]*y ;
            break;

        case SYSTEM_LOTKA_VOLTERRA:
            (Slope.dx_by_dt) = Params[0]*x - Params[1]*x*y ;
            (Slope.dy_by_dt) = Params[3]*x*y - Params[2]*y ;
            break;

        case SYSTEM_VAN_DER_POL:
            (Slope.dx_by_dt) = y ;
            (Slope.dy_by_dt) = Params[0]*(1.0 - x*x)*y - x ;
            break;

        case SYSTEM_DAMPED_PENDULUM:
            (Slope.dx_by_dt) = y ;
            (Slope.dy_by_dt) = -Params[1]*sin(x) - Params[0]*y ;
            break;

        case SYSTEM_DUFFING:
            (Slope.dx_by_dt) = y ;
            (Slope.dy_by_dt) = -Params[0]*y - Params[1]*x - Params[2]*x*x*x ;
            break;

        default:
            (Slope.dx_by_dt) = 0.0;
            (Slope.dy_by_dt) = 0.0;
            break;
    }

    return Slope;
}



/// @brief Fills `Params` with nice constants that make a good looking trajectory for that system.
/// @param System Which system.
/// @param Params Container for the constants, needs room for `MAX_SYSTEM_PARAMS` values in `double`.
/// @attention Slots not used by that system are left untouched.
void GetDefaultParams (SystemID System, double* Params)
{
    switch (System)
    {
        case SYSTEM_LINEAR_SPIRAL:  //Shrinking spiral: decay 0.1, turning speed 2
            Params[0] = -0.1;
            Params[1] = 2.0;
            Params[2] = -2.0;
            Params[3] = -0.1;
            break;

        case SYSTEM_LOTKA_VOLTERRA: //Balance point sits at (2, 2)
            Params[0] = 1.0;
            Params[1] = 0.5;
            Params[2] = 1.0;
            Params[3] = 0.5;
            break;

        case SYSTEM_VAN_DER_POL:
            Params[0] = 1.0;
            break;

        case SYSTEM_DAMPED_PENDULUM:
            Params[0] = 0.2;
            Params[1] = 1.0;
            break;

        case SYSTEM_DUFFING:        //Negative Alpha makes the double well
            Params[0] = 0.2;
            Params[1] = -1.0;
            Params[2] = 1.0;
            break;

        default:
            break;
    }

    return;
}



/// @brief Gives a starting point that makes an interesting trajectory for that system.
/// @param System Which system.
/// @return The starting `Position2D`.
Position2D GetDefaultStartState (SystemID System)
{
    Position2D Start = { 1.0, 0.0 } ;

    switch (System)
    {
        case SYSTEM_LINEAR_SPIRAL:
            Start.x = 1.0;
            Start.y = 0.0;
            break;

        case SYSTEM_LOTKA_VOLTERRA:
            Start.x = 1.0;
            Start.y = 1.0;
            break;

        case SYSTEM_VAN_DER_POL:
            Start.x = 2.0;
            Start.y = 0.0;
            break;

        case SYSTEM_DAMPED_PENDULUM:
            Start.x = 2.0;
            Start.y = 0.0;
            break;

        case SYSTEM_DUFFING:    //Starts high up the side of the well, so the path swings wide (nearly reaching the barrier at x = 0) before settling. A path that explores more of the plane is much easier to recover the equation from.
            Start.x = 1.5;
            Start.y = 0.0;
            break;

        default:
            break;
    }

    return Start;
}



/// @brief Moves the state forward in time by one step using Runge-Kutta 4 (RK4).
/// @param System Which system.
/// @param Params Constants of the system (see `EvaluateSystem`).
/// @param State Address of the current state. It gets overwritten with the state after `TimeStep`.
/// @param TimeStep Step size in `double`.
/// @attention RK4 asks the system for its slope 4 times per step: at the start (`K1`), twice in the middle (`K2`, `K3`), and at the end (`K4`). The middle ones are better guesses each time. Final slope is the weighted mix `(K1 + 2*K2 + 2*K3 + K4) / 6`. Error shrinks like `TimeStep^4`.
void Rk4Step (SystemID System, double* Params, Position2D* State, double TimeStep)
{
    Position2D Probe;

    //K1: slope right here
    Slope2D K1 = EvaluateSystem(System, (*State), Params) ;

    //K2: slope in the middle of the step, if we had followed K1
    (Probe.x) = (State->x) + 0.5*TimeStep*(K1.dx_by_dt) ;
    (Probe.y) = (State->y) + 0.5*TimeStep*(K1.dy_by_dt) ;
    Slope2D K2 = EvaluateSystem(System, Probe, Params) ;

    //K3: slope in the middle again, but now following K2 (a better guess)
    (Probe.x) = (State->x) + 0.5*TimeStep*(K2.dx_by_dt) ;
    (Probe.y) = (State->y) + 0.5*TimeStep*(K2.dy_by_dt) ;
    Slope2D K3 = EvaluateSystem(System, Probe, Params) ;

    //K4: slope at the end of the step, if we had followed K3 all the way
    (Probe.x) = (State->x) + TimeStep*(K3.dx_by_dt) ;
    (Probe.y) = (State->y) + TimeStep*(K3.dy_by_dt) ;
    Slope2D K4 = EvaluateSystem(System, Probe, Params) ;

    (State->x) += TimeStep/6.0 * ((K1.dx_by_dt) + 2.0*(K2.dx_by_dt) + 2.0*(K3.dx_by_dt) + (K4.dx_by_dt)) ;
    (State->y) += TimeStep/6.0 * ((K1.dy_by_dt) + 2.0*(K2.dy_by_dt) + 2.0*(K3.dy_by_dt) + (K4.dy_by_dt)) ;
    return;
}



/// @brief Runs the system from a starting point and records the whole path.
/// @param System Which system.
/// @param Params Constants of the system (see `EvaluateSystem`).
/// @param StartState Where the path begins.
/// @param TimeStep Time between two recorded rows, in `double`.
/// @param SampleCount How many rows to record (the start point is row 0).
/// @return A new `Matrix` of size `SampleCount x 3`. Columns are `t, x, y`, same as the planned CSV.
/// @attention Row `k` is at time `k*TimeStep`. Time is computed fresh each row, not added up, so rounding errors don't pile up.
/// @warning Result needs a `MatrixDestroy` later.
Matrix TraceTrajectory (SystemID System, double* Params, Position2D StartState, double TimeStep, size_t SampleCount)
{
    Matrix Path = MatrixCreate(SampleCount, 3) ;
    Position2D Current = StartState ;

    for (size_t ThisSample=0; ThisSample<SampleCount; ThisSample++)
    {
        SetMatrixCell(&Path, ThisSample, 0, ThisSample*TimeStep);
        SetMatrixCell(&Path, ThisSample, 1, (Current.x));
        SetMatrixCell(&Path, ThisSample, 2, (Current.y));
        Rk4Step(System, Params, &Current, TimeStep);
    }

    return Path;
}



/// @brief Gives the exact derivatives `(dx/dt, dy/dt)` at every row of a trajectory, straight from the equations (no guessing).
/// @param System Which system.
/// @param Params Constants of the system (see `EvaluateSystem`).
/// @param Path Address of a trajectory from `TraceTrajectory` (columns `t, x, y`).
/// @return A new `Matrix` of size `RowCount x 2`. Columns are `dx/dt, dy/dt`.
/// @attention Use this on the CLEAN path, before adding noise. These are the "answers" for the neural network, and the cheat sheet for testing the regression.
/// @warning Result needs a `MatrixDestroy` later.
Matrix TrueDerivativesOfTrajectory (SystemID System, double* Params, Matrix* Path)
{
    Matrix Derivatives = MatrixCreate((Path->RowCount), 2) ;

    for (size_t ThisSample=0; ThisSample<(Path->RowCount); ThisSample++)
    {
        Position2D Here = { GetMatrixCell(Path,ThisSample,1), GetMatrixCell(Path,ThisSample,2) } ;
        Slope2D Slope = EvaluateSystem(System, Here, Params) ;
        SetMatrixCell(&Derivatives, ThisSample, 0, (Slope.dx_by_dt));
        SetMatrixCell(&Derivatives, ThisSample, 1, (Slope.dy_by_dt));
    }

    return Derivatives;
}



/// @brief Adds Gaussian noise to `x` and `y` of a trajectory. This is the "illusion of raw data" (a fake measurement).
/// @param Path Address of the trajectory (columns `t, x, y`). Changed in place.
/// @param NoiseSigma Width of the noise in `double`, in the same units as `x` and `y` (not a percentage).
/// @param Gen Address of the random generator.
/// @attention Column 0 (time) is left alone, we assume the clock is perfect.
void AddGaussianNoise (Matrix* Path, double NoiseSigma, RandomState* Gen)
{
    for (size_t ThisSample=0; ThisSample<(Path->RowCount); ThisSample++)
    {
        for (size_t ThisCol=1; ThisCol<(Path->ColCount); ThisCol++)
        {
            double NoisyValue = GetMatrixCell(Path,ThisSample,ThisCol) + RandomGaussian(Gen, 0.0, NoiseSigma) ;
            SetMatrixCell(Path, ThisSample, ThisCol, NoisyValue);
        }
    }

    return;
}
