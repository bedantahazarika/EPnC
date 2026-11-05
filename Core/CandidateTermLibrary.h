//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <LinearAlgebraSolver.h>



/// @brief One candidate term, like `x*y` or `sin(x)`. It holds both WHAT KIND of term it is and the powers, so there is only one type to remember.
/// @param Kind Which kind of term. One of the names below.
/// @param PowerX Power of `x`. Only used by `TERM_POLYNOMIAL`.
/// @param PowerY Power of `y`. Only used by `TERM_POLYNOMIAL`.
/// @attention The kinds: `TERM_POLYNOMIAL` is `x^PowerX * y^PowerY` (both powers 0 is the constant `1`), `TERM_SIN_X` is `sin(x)`, `TERM_COS_X` is `cos(x)`, `TERM_SIN_Y` is `sin(y)`, `TERM_COS_Y` is `cos(y)`.
/// @attention To add a new kind: add it to the list inside the struct, then add a `case` in `EvaluateTerm` and in `GetTermName`, and list it in `CreateTermLibrary`.
/// @attention The kind names are global, so you write `TERM_SIN_X` and not `Term.TERM_SIN_X`. Functions that take a kind as an argument (like `FindTerm`) take it as a plain `int`.
typedef struct {
    enum { TERM_POLYNOMIAL, TERM_SIN_X, TERM_COS_X, TERM_SIN_Y, TERM_COS_Y } Kind ;
    int PowerX, PowerY ;
} Term;



/// @brief The full list of candidate terms we let the regression choose from. One term = one column of the term matrix.
/// @param TermCount How many terms.
/// @param Terms The terms themselves.
typedef struct {
    size_t TermCount ;
    Term* Terms ;
} TermLibrary;



/// @brief Builds the list of candidate terms.
/// @param MaxDegree Highest total power allowed, like `3` for everything up to `x^3`, `x^2*y`, `x*y^2`, `y^3`.
/// @param IncludeTrig `true` to also add `sin(x)`, `cos(x)`, `sin(y)`, `cos(y)`. `false` for polynomials only.
/// @return A new `TermLibrary`. Order is by degree: `1, x, y, x^2, x*y, y^2, x^3, x^2*y, x*y^2, y^3, ...`, then the trig terms.
/// @attention Polynomial count is `(MaxDegree+1)*(MaxDegree+2)/2`. Degree 1 gives 3 terms, degree 2 gives 6, degree 3 gives 10.
/// @warning Result needs a `DestroyTermLibrary` later.
TermLibrary CreateTermLibrary (int MaxDegree, bool IncludeTrig)
{
    TermLibrary Library;
    size_t PolynomialCount = (size_t)((MaxDegree + 1) * (MaxDegree + 2) / 2) ;
    (Library.TermCount) = PolynomialCount + (IncludeTrig ? 4 : 0) ;
    (Library.Terms) = (Term*) calloc((Library.TermCount), sizeof(Term)) ;

    size_t NextTerm = 0 ;
    for (int ThisDegree=0; ThisDegree<=MaxDegree; ThisDegree++)
    {
        for (int PowerX=ThisDegree; PowerX>=0; PowerX--)    //Within one degree, the most x comes first: x^2, then x*y, then y^2
        {
            (Library.Terms[NextTerm].Kind) = TERM_POLYNOMIAL ;
            (Library.Terms[NextTerm].PowerX) = PowerX ;
            (Library.Terms[NextTerm].PowerY) = ThisDegree - PowerX ;
            NextTerm++;
        }
    }

    if (IncludeTrig)
    {
        (Library.Terms[NextTerm++].Kind) = TERM_SIN_X ;
        (Library.Terms[NextTerm++].Kind) = TERM_COS_X ;
        (Library.Terms[NextTerm++].Kind) = TERM_SIN_Y ;
        (Library.Terms[NextTerm++].Kind) = TERM_COS_Y ;
    }

    return Library;
}



/// @brief Frees the memory of a term library.
/// @param Library Address of the library.
void DestroyTermLibrary (TermLibrary* Library)
{
    free(Library->Terms);
    (Library->Terms) = NULL;
    (Library->TermCount) = 0;
    return;
}



/// @brief Gives the value of one term at one point.
/// @param TheTerm Address of the term.
/// @param x Value of `x`.
/// @param y Value of `y`.
/// @return The value, like `x*y` evaluated at `(2, 3)` gives `6`.
/// @attention Powers are done by multiplying, not with `pow`, so `x^0` is exactly `1` even when `x` is `0`.
double EvaluateTerm (Term* TheTerm, double x, double y)
{
    double Value = 1.0 ;

    switch (TheTerm->Kind)
    {
        case TERM_POLYNOMIAL:
            for (int ThisPower=0; ThisPower<(TheTerm->PowerX); ThisPower++) Value *= x;
            for (int ThisPower=0; ThisPower<(TheTerm->PowerY); ThisPower++) Value *= y;
            return Value;

        case TERM_SIN_X:
            return sin(x);

        case TERM_COS_X:
            return cos(x);

        case TERM_SIN_Y:
            return sin(y);

        case TERM_COS_Y:
            return cos(y);

        default:
            return 0.0;
    }
}



/// @brief Builds the term matrix (often called Theta): one row per sample, one column per term. Each cell is that term evaluated at that sample's `(x, y)`.
/// @param Library Address of the term library. Its order decides the column order.
/// @param Data Address of the samples, for example from `TraceTrajectory` (columns `t, x, y`).
/// @param XCol Column of `Data` that holds `x` (for `TraceTrajectory` that is `1`).
/// @param YCol Column of `Data` that holds `y` (for `TraceTrajectory` that is `2`).
/// @return A new `Matrix` of size `Data->RowCount x Library->TermCount`.
/// @attention The regression later asks: which few columns, mixed with which weights, add up to the derivative?
/// @warning Result needs a `MatrixDestroy` later.
Matrix BuildTermMatrix (TermLibrary* Library, Matrix* Data, size_t XCol, size_t YCol)
{
    Matrix Theta = MatrixCreate((Data->RowCount), (Library->TermCount)) ;

    for (size_t ThisRow=0; ThisRow<(Data->RowCount); ThisRow++)
    {
        double x = GetMatrixCell(Data, ThisRow, XCol) ;
        double y = GetMatrixCell(Data, ThisRow, YCol) ;

        for (size_t ThisTerm=0; ThisTerm<(Library->TermCount); ThisTerm++)
        {
            SetMatrixCell(&Theta, ThisRow, ThisTerm, EvaluateTerm(&(Library->Terms[ThisTerm]), x, y));
        }
    }

    return Theta;
}



/// @brief Writes a readable name of a term into a text buffer, like `1`, `x`, `x^2*y` or `sin(x)`.
/// @param TheTerm Address of the term.
/// @param Buffer Where the name is written.
/// @param BufferSize Size of `Buffer` in bytes. 32 is plenty.
void GetTermName (Term* TheTerm, char* Buffer, size_t BufferSize)
{
    char XPart[24] ;
    char YPart[24] ;

    switch (TheTerm->Kind)
    {
        case TERM_POLYNOMIAL:
            if ((TheTerm->PowerX) == 0)         XPart[0] = '\0';
            else if ((TheTerm->PowerX) == 1)    snprintf(XPart, sizeof(XPart), "x");
            else                                snprintf(XPart, sizeof(XPart), "x^%d", (TheTerm->PowerX));

            if ((TheTerm->PowerY) == 0)         YPart[0] = '\0';
            else if ((TheTerm->PowerY) == 1)    snprintf(YPart, sizeof(YPart), "y");
            else                                snprintf(YPart, sizeof(YPart), "y^%d", (TheTerm->PowerY));

            if ((TheTerm->PowerX) > 0 && (TheTerm->PowerY) > 0)     snprintf(Buffer, BufferSize, "%s*%s", XPart, YPart);
            else if ((TheTerm->PowerX) > 0)                         snprintf(Buffer, BufferSize, "%s", XPart);
            else if ((TheTerm->PowerY) > 0)                         snprintf(Buffer, BufferSize, "%s", YPart);
            else                                                    snprintf(Buffer, BufferSize, "1");
            break;

        case TERM_SIN_X:
            snprintf(Buffer, BufferSize, "sin(x)");
            break;

        case TERM_COS_X:
            snprintf(Buffer, BufferSize, "cos(x)");
            break;

        case TERM_SIN_Y:
            snprintf(Buffer, BufferSize, "sin(y)");
            break;

        case TERM_COS_Y:
            snprintf(Buffer, BufferSize, "cos(y)");
            break;

        default:
            snprintf(Buffer, BufferSize, "?");
            break;
    }

    return;
}



/// @brief Finds which column of the term matrix belongs to a given term.
/// @param Library Address of the term library.
/// @param Kind Which kind of term (`TERM_POLYNOMIAL`, `TERM_SIN_X`, ...).
/// @param PowerX Power of `x` (use `0` for the trig kinds, it is ignored for them).
/// @param PowerY Power of `y` (use `0` for the trig kinds, it is ignored for them).
/// @return The column number, or `-1` if the library does not contain that term.
/// @attention The `-1` is the "true term is missing from the library" case. Check for it before using the result as an index.
int FindTerm (TermLibrary* Library, int Kind, int PowerX, int PowerY)
{
    for (size_t ThisTerm=0; ThisTerm<(Library->TermCount); ThisTerm++)
    {
        Term* Candidate = &(Library->Terms[ThisTerm]) ;
        if ((int)(Candidate->Kind) != Kind) continue;
        if (Kind != TERM_POLYNOMIAL) return (int) ThisTerm;
        if ((Candidate->PowerX) == PowerX && (Candidate->PowerY) == PowerY) return (int) ThisTerm;
    }

    return -1;
}



/// @brief Prints one equation like `dx/dt = -0.500x + 1.990y`, using the term names. Terms whose coefficient is tiny are left out.
/// @param LeftSide Text before the equals sign, like `"dx/dt"`.
/// @param Library Address of the term library (gives the names).
/// @param Coefficients One number per term, in the library's order.
/// @param ZeroBelow Coefficients with absolute value below this are treated as zero and not printed. Exact zeros are never printed.
/// @param Decimals How many digits after the decimal point.
/// @attention Prints no newline at the start or at the end, so you decide the layout. If every coefficient is zero, it prints `0`.
void PrintEquation (const char* LeftSide, TermLibrary* Library, double* Coefficients, double ZeroBelow, int Decimals)
{
    char Name[32] ;
    int PrintedCount = 0 ;

    printf("%s =", LeftSide);

    for (size_t ThisTerm=0; ThisTerm<(Library->TermCount); ThisTerm++)
    {
        double Coefficient = Coefficients[ThisTerm] ;
        if (Coefficient == 0.0 || fabs(Coefficient) < ZeroBelow) continue;

        Term* TheTerm = &(Library->Terms[ThisTerm]) ;
        int IsConstant = ((TheTerm->Kind) == TERM_POLYNOMIAL && (TheTerm->PowerX) == 0 && (TheTerm->PowerY) == 0) ;
        GetTermName(TheTerm, Name, sizeof(Name));

        if (PrintedCount == 0)  printf(" %s%.*f", (Coefficient < 0.0) ? "-" : "", Decimals, fabs(Coefficient));
        else                    printf(" %s %.*f", (Coefficient < 0.0) ? "-" : "+", Decimals, fabs(Coefficient));

        if (!IsConstant) printf("%s", Name);     //The constant term is just the number
        PrintedCount++;
    }

    if (PrintedCount == 0) printf(" 0");
    return;
}
