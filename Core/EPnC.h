//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

/// @file EPnC.h
/// @brief Umbrella header: `#include <EPnC.h>` pulls in the whole library (header-only, no raylib needed). The GUI lives in EPnC_Gui.h and is NOT part of this umbrella, because it needs raylib.
/// @attention The order below goes from the headers that need nothing to the ones that need the earlier ones. Every header also includes what it needs itself, so this order is a convenience, not a requirement.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include <RandomNoiseGenerator.h>
#include <LinearAlgebraSolver.h>
#include <OdeSolverAndSystems.h>
#include <CandidateTermLibrary.h>
#include <SparseRegressionSTLSQ.h>
#include <GroundTruthCoefficients.h>
#include <DerivativeEstimation.h>
#include <NeuralDerivativeNetwork.h>
#include <CsvPathLoader.h>
#include <TimingInterpolate.h>
