//SPDX-License-Identifier: GPL-3.0-or-later
//Copyright (C) 2026 Bedanta Hazarika (@bedantahazarika). Part of EPnC, see the LICENSE file.

#pragma once

//Everything in this file only exists when built with -DWITH_GUI=ON. Without it, this header is empty and the rest of the Core still builds.
//Raylib is allowed ONLY in this header (that keeps the Core free of any graphics library).
#ifdef WITH_GUI

#include <EPnC.h>
#include <PlotFontData.h>

//Raylib has its own type called Matrix (4x4, for 3D graphics). It fights with ours, so we rename theirs only while including.
//Never include raymath.h anywhere, because it uses that type name too.
#define Matrix RaylibMatrix
#include <raylib.h>
#include <rlgl.h>
#undef Matrix



/// @brief Size in pixels of the normal text (everything else scales with it). Change it here, or press the A- and A+ buttons while the app runs.
#define DEFAULT_FONT_SIZE 16

/// @brief Which theme the app starts with: `0` = Light, `1` = Coffee, `2` = Dark. Press the theme button while the app runs to switch.
#define DEFAULT_THEME 1

/// @brief The text size the layout was designed with. Everything is drawn at this size times `DEFAULT_FONT_SIZE / GUI_DESIGN_FONT_SIZE`.
#define GUI_DESIGN_FONT_SIZE 14

/// @brief Helper for writing colors in the theme table (red, green, blue, fully opaque).
#define GUI_RGB(R,G,B) { R, G, B, 255 }



/// @brief All the colors of one theme.
/// @param Background Window background behind the panels.
/// @param Panel The big middle panel and the popups.
/// @param SidePanel The thin left and right panels.
/// @param Text Normal text.
/// @param Muted Secondary text, axis numbers.
/// @param Grid Thin lines inside plots and under titles.
/// @param Frame Borders.
/// @param Accent Selected things, the first curve (x), good bars.
/// @param Warm The second curve (y), weak bars.
/// @param Good Text meaning "fine".
/// @param Bad Text and lines meaning "wrong" (also the threshold lines).
/// @param Widget Resting color of buttons and slider tracks.
/// @param OnAccent Text color on top of `Accent`.
/// @param AccentHover `Accent` when the mouse is over a main button.
typedef struct {
    Color Background, Panel, SidePanel, Text, Muted, Grid, Frame ;
    Color Accent, Warm, Good, Bad, Widget, OnAccent, AccentHover ;
} GuiThemeColors;

#define GUI_THEME_COUNT 3
const char* GuiThemeNames[GUI_THEME_COUNT] = { "Light", "Coffee", "Dark" } ;

//The Coffee theme uses six colors: dark-coffee #432818, dark-wine #6f1d1b, olive-wood #7f5e38, chocolate-brown #99582a, camel #bb9457, light-apricot #ffe6a7.
//Grid, SidePanel, Widget and AccentHover are mixes of those (apricot with camel, chocolate with coffee).
GuiThemeColors GuiThemes[GUI_THEME_COUNT] = {
    {   //Light (good for slides and print)
        GUI_RGB(236,238,242), GUI_RGB(252,252,253), GUI_RGB(246,247,249), GUI_RGB( 35, 38, 45), GUI_RGB(118,124,134), GUI_RGB(228,230,234), GUI_RGB(196,200,208),
        GUI_RGB( 37, 99,235), GUI_RGB(234, 88, 12), GUI_RGB( 22,163, 74), GUI_RGB(220, 38, 38), GUI_RGB(226,229,235), GUI_RGB(255,255,255), GUI_RGB( 29, 78,216)
    },
    {   //Coffee
        GUI_RGB(187,148, 87), GUI_RGB(255,242,210), GUI_RGB(255,242,210), GUI_RGB( 67, 40, 24), GUI_RGB(127, 94, 56), GUI_RGB(238,210,147), GUI_RGB(187,148, 87),
        GUI_RGB(153, 88, 42), GUI_RGB(111, 29, 27), GUI_RGB(127, 94, 56), GUI_RGB(111, 29, 27), GUI_RGB(231,201,139), GUI_RGB(255,230,167), GUI_RGB(119, 69, 35)
    },
    {   //Dark (nice on screen)
        GUI_RGB( 18, 20, 25), GUI_RGB( 28, 31, 38), GUI_RGB( 24, 27, 33), GUI_RGB(228,230,235), GUI_RGB(140,146,158), GUI_RGB( 46, 50, 60), GUI_RGB( 70, 76, 90),
        GUI_RGB( 96,165,250), GUI_RGB(251,146, 60), GUI_RGB( 74,222,128), GUI_RGB(248,113,113), GUI_RGB( 48, 53, 64), GUI_RGB( 18, 20, 25), GUI_RGB(147,197,253)
    }
} ;

int GuiThemeIndex = DEFAULT_THEME ;

#define GUI_BACKGROUND    (GuiThemes[GuiThemeIndex].Background)
#define GUI_PANEL         (GuiThemes[GuiThemeIndex].Panel)
#define GUI_SIDE_PANEL    (GuiThemes[GuiThemeIndex].SidePanel)
#define GUI_TEXT          (GuiThemes[GuiThemeIndex].Text)
#define GUI_MUTED         (GuiThemes[GuiThemeIndex].Muted)
#define GUI_GRID          (GuiThemes[GuiThemeIndex].Grid)
#define GUI_FRAME         (GuiThemes[GuiThemeIndex].Frame)
#define GUI_ACCENT        (GuiThemes[GuiThemeIndex].Accent)
#define GUI_WARM          (GuiThemes[GuiThemeIndex].Warm)
#define GUI_GOOD          (GuiThemes[GuiThemeIndex].Good)
#define GUI_BAD           (GuiThemes[GuiThemeIndex].Bad)
#define GUI_WIDGET        (GuiThemes[GuiThemeIndex].Widget)
#define GUI_ON_ACCENT     (GuiThemes[GuiThemeIndex].OnAccent)
#define GUI_ACCENT_HOVER  (GuiThemes[GuiThemeIndex].AccentHover)



//The text is drawn with the embedded Inter font (see PlotFontData.h). A font made for one size looks sharpest at that size, so we load a few and pick the closest.
//Several sizes are loaded so that text stays sharp when the user makes it bigger.
#define GUI_FONT_COUNT 12
int GuiFontSizes[GUI_FONT_COUNT] = { 11, 12, 14, 16, 18, 20, 22, 26, 30, 36, 44, 52 } ;
Font GuiFonts[GUI_FONT_COUNT] ;

//The whole interface is drawn at a scale: 1.0 means the design size. Layout code works in "logical pixels"; the scale turns them into screen pixels.
int GuiFontSize = DEFAULT_FONT_SIZE ;
double GuiScale = (double)DEFAULT_FONT_SIZE / (double)GUI_DESIGN_FONT_SIZE ;



/// @brief The steps of the pipeline, in order. Each step keeps its result, so changing an option only re-runs the steps from the first one that depends on it.
/// @param GUI_STAGE_TIMING Make the time even (interpolation). Depends on: mode, interpolation method, drift tolerance.
/// @param GUI_STAGE_DERIVATIVES Estimate `dx/dt` and `dy/dt`. Depends on: derivative method, window, order.
/// @param GUI_STAGE_LIBRARY Build the candidate term matrix. Depends on: max degree, trig toggle.
/// @param GUI_STAGE_SPARSE STLSQ for `dx` and `dy`. Depends on: threshold. Cheap, so it re-runs by itself while a slider moves.
/// @param GUI_STAGE_COUNT Not a stage. Also the value of `DirtyFrom` when everything is up to date.
typedef enum {
    GUI_STAGE_TIMING,
    GUI_STAGE_DERIVATIVES,
    GUI_STAGE_LIBRARY,
    GUI_STAGE_SPARSE,
    GUI_STAGE_COUNT
} GuiStage;



/// @brief Everything the user can change in the left panel.
/// @param Mode `0` = Interpolated SINDy (works now), `1` = SINDy + filtered (neural curve, not built into the GUI yet).
/// @param InterpMethod An `InterpolationMethod` as `int`.
/// @param DriftTolerance Drift (in time steps) that is still left alone by the interpolation.
/// @param DerivMethod `0` = finite differences, `1` = Savitzky-Golay.
/// @param SgHalfWindow Savitzky-Golay half window (window is `2*HalfWindow + 1` points). A `double` only because sliders store `double`.
/// @param SgOrder Savitzky-Golay polynomial order. A `double` for the same reason.
/// @param MaxDegree Highest polynomial degree in the library. A `double` for the same reason.
/// @param IncludeTrig Whether `sin` and `cos` terms are in the library.
/// @param Threshold STLSQ threshold.
/// @param NeuralHidden Number of `tanh` units per network in the filtered mode (stored as `double` for the slider).
/// @param NeuralSteps Training steps per chunk in the filtered mode.
/// @param NeuralChunk Rows per chunk in the filtered mode.
/// @param DemoNoiseExp Base 10 logarithm of the noise width of demo data.
/// @param DemoJitter Clock jitter of demo data, as a fraction of a time step.
typedef struct {
    int Mode ;
    int InterpMethod ;
    double DriftTolerance ;
    int DerivMethod ;
    double SgHalfWindow, SgOrder ;
    double MaxDegree ;
    bool IncludeTrig ;
    double Threshold ;
    double NeuralHidden, NeuralSteps, NeuralChunk ;
    double DemoNoiseExp, DemoJitter ;
} GuiOptions;



/// @brief The whole state of the app: options, the loaded data, the cached result of every pipeline stage, and the view (tab, animation).
/// @param HasData `true` once a file or demo was loaded. Before that the drop popup is shown.
/// @param AskForFile `true` while the popup is open on top of already loaded data (the "Load another file" button).
/// @param SourceName Text shown in the right panel (file name or demo name).
/// @param RawPath The data exactly as loaded (columns `t, x, y`).
/// @param Timing Timing report of `RawPath` at the current drift tolerance.
/// @param TimeIsUsable `true` if there are at least 3 rows and time is strictly increasing (needed for interpolation).
/// @param StageReady Which stage results currently exist.
/// @param DirtyFrom First stage that must be re-run. `GUI_STAGE_COUNT` means everything is up to date.
/// @param EvenPath Path with even time (a plain copy of `RawPath` if the stage had nothing to do).
/// @param WasInterpolated Array with one flag per row of `EvenPath`.
/// @param InterpolatedCount How many rows were interpolated.
/// @param Slopes Estimated derivatives, `RowCount x 2`.
/// @param Library The candidate terms.
/// @param Theta The term matrix built from `EvenPath`.
/// @param TargetDx The `dx/dt` values as a plain array (for STLSQ).
/// @param TargetDy The `dy/dt` values as a plain array.
/// @param JobDx The finished STLSQ run for `dx/dt`, with every round remembered.
/// @param JobDy The finished STLSQ run for `dy/dt`.
/// @param StageMilliseconds How long each stage took last time.
/// @param IsDemo `true` if the data came from a built-in system (so the true equation is known).
/// @param DemoSystem Which system, if `IsDemo`.
/// @param DemoParams The constants of that system.
/// @param HasSupport `true` if `Support` holds a comparison with the truth.
/// @param Support Found versus true terms (demo data only).
/// @param PlotTab `0` = data, `1` = phase portrait, `2` = slopes.
/// @param Playing Whether the STLSQ animation is advancing by itself.
/// @param AnimClock Seconds since the last animation advance.
/// @param AnimRound Which snapshot (round) of STLSQ is shown.
/// @param Processing `true` while the pipeline runs stage by stage (one stage per frame, with the blocking overlay on screen).
/// @param StageToRun The stage that runs next while `Processing`.
/// @param Job The neural fitting job (only valid while `NeuralActive`).
/// @param NeuralActive `true` while the neural networks are being fitted (the middle panel then shows the fitting live).
/// @param NeuralSkip `true` when the user pressed "Skip animation": the fitting then runs without time limit per frame.
/// @param FdSlopes Slopes from plain finite differences of the raw data, kept only to compare with the neural ones on screen.
/// @param NeuralGen The random generator of the neural job.
/// @param NeuralStartTime When the neural fitting started (seconds).
/// @param NoiseDirty `true` when a demo data slider moved and the demo must be regenerated as soon as the mouse button is released.
/// @param LeftScroll How far the options panel is scrolled (logical pixels).
/// @param LeftContentHeight Height of everything in the options panel (logical pixels).
/// @param WantedFontSize The text size asked for by the A- and A+ buttons; applied at the start of the next frame.
/// @param SimStates Remembered simulations of the discovered equations: for each snapshot `k` and each row, `x` then `y`. Filled on demand.
/// @param SimReady One flag per snapshot: is its simulation computed.
/// @param SimError One value per snapshot: RMS gap between simulation and data (`NaN` if the simulation blew up).
/// @param SimCount Number of snapshots the simulation arrays have room for.
/// @param Message One line of feedback for the user (errors). Empty if nothing to say.
/// @param ShotRequested `true` when a screenshot should be saved at the end of this frame.
/// @param ShotFile Name of the screenshot file to write.
/// @param ShotCount How many screenshots were saved (used to number files).
typedef struct {
    GuiOptions Options ;
    bool HasData ;
    bool AskForFile ;
    char SourceName[256] ;
    Matrix RawPath ;
    TimingReport Timing ;
    bool TimeIsUsable ;
    bool StageReady[GUI_STAGE_COUNT] ;
    int DirtyFrom ;
    Matrix EvenPath ;
    bool* WasInterpolated ;
    size_t InterpolatedCount ;
    Matrix Slopes ;
    TermLibrary Library ;
    Matrix Theta ;
    double* TargetDx ;
    double* TargetDy ;
    StlsqJob JobDx, JobDy ;
    double StageMilliseconds[GUI_STAGE_COUNT] ;
    bool IsDemo ;
    SystemID DemoSystem ;
    double DemoParams[MAX_SYSTEM_PARAMS] ;
    bool HasSupport ;
    SupportReport Support ;
    int PlotTab ;
    bool Processing ;
    int StageToRun ;
    NeuralJob Job ;
    bool NeuralActive, NeuralSkip ;
    Matrix FdSlopes ;
    RandomState NeuralGen ;
    double NeuralStartTime ;
    bool NoiseDirty ;
    float LeftScroll, LeftContentHeight ;
    int WantedFontSize ;
    double* SimStates ;
    bool* SimReady ;
    double* SimError ;
    size_t SimCount ;
    bool Playing ;
    double AnimClock ;
    int AnimRound ;
    char Message[200] ;
    bool ShotRequested ;
    char ShotFile[256] ;
    int ShotCount ;
} GuiApp;



/// @brief The mouse as seen by the widgets this frame, plus which slider is being dragged.
/// @param Mouse Mouse position in pixels.
/// @param Wheel How far the mouse wheel turned this frame.
/// @param Clicked `true` on the frame the left button went down.
/// @param Held `true` while the left button is down.
/// @param ActiveId Id of the slider being dragged (`0` = none).
/// @param Blocked `true` while a popup is open: panels then ignore the mouse.
typedef struct {
    Vector2 Mouse ;
    float Wheel ;
    bool Clicked, Held ;
    int ActiveId ;
    bool Blocked ;
} GuiInput;

GuiInput GuiNow ;

//While a scrolling area is drawn, widgets outside of it must not react to the mouse (they are hidden by the clip).
Rectangle GuiClipBox ;
bool GuiClipActive = false ;



/// @brief Where the three panels are on screen.
/// @param Left The thin panel with the options.
/// @param Middle The big panel with the plots.
/// @param Right The thin panel with the information.
typedef struct {
    Rectangle Left, Middle, Right ;
} GuiLayout;



/// @brief A plot area: where it is on screen and which data range it shows.
/// @param Box Pixel rectangle of the data area (axes and numbers are drawn outside of it).
/// @param XMin Data value at the left edge.
/// @param XMax Data value at the right edge.
/// @param YMin Data value at the bottom edge.
/// @param YMax Data value at the top edge.
typedef struct {
    Rectangle Box ;
    double XMin, XMax, YMin, YMax ;
} GuiPlotBox;





//====================================================================================================
//Text
//====================================================================================================

/// @brief Picks the loaded font whose size is closest to the wanted size.
/// @param FontSize Wanted height of the letters in pixels.
/// @return The best `Font`.
Font GuiFontFor (int FontSize)
{
    int Wanted = (int)((double)FontSize * GuiScale + 0.5) ;
    int BestIndex = 0 ;
    for (int ThisFont=1; ThisFont<GUI_FONT_COUNT; ThisFont++)
    {
        if (abs(GuiFontSizes[ThisFont] - Wanted) < abs(GuiFontSizes[BestIndex] - Wanted)) BestIndex = ThisFont;
    }
    return GuiFonts[BestIndex];
}



/// @brief Draws text with the embedded font. Use this, never raylib's `DrawText`.
/// @param x Pixel column of the left edge.
/// @param y Pixel row of the top edge.
/// @param FontSize Height of the letters in pixels.
/// @param Shade Color of the text.
/// @param Text The text.
void GuiDrawText (float x, float y, int FontSize, Color Shade, const char* Text)
{
    Vector2 Where = { x, y } ;
    DrawTextEx(GuiFontFor(FontSize), Text, Where, (float)FontSize, 0.0f, Shade);
    return;
}



/// @brief Width of a text in pixels.
/// @param FontSize Height of the letters in pixels.
/// @param Text The text.
/// @return Width in pixels.
float GuiTextWidth (int FontSize, const char* Text)
{
    return MeasureTextEx(GuiFontFor(FontSize), Text, (float)FontSize, 0.0f).x ;
}



/// @brief Draws text so that it ends at a given pixel column.
/// @param xRight Pixel column of the right edge.
/// @param y Pixel row of the top edge.
/// @param FontSize Height of the letters in pixels.
/// @param Shade Color of the text.
/// @param Text The text.
void GuiDrawTextRight (float xRight, float y, int FontSize, Color Shade, const char* Text)
{
    GuiDrawText(xRight - GuiTextWidth(FontSize,Text), y, FontSize, Shade, Text);
    return;
}



/// @brief Draws text so that its middle is at a given pixel column.
/// @param xMiddle Pixel column of the middle.
/// @param y Pixel row of the top edge.
/// @param FontSize Height of the letters in pixels.
/// @param Shade Color of the text.
/// @param Text The text.
void GuiDrawTextCentered (float xMiddle, float y, int FontSize, Color Shade, const char* Text)
{
    GuiDrawText(xMiddle - 0.5f*GuiTextWidth(FontSize,Text), y, FontSize, Shade, Text);
    return;
}



/// @brief Draws text that is broken into lines at spaces so that no line is wider than `MaxWidth`.
/// @param x Pixel column of the left edge.
/// @param y Pixel row of the top edge of the first line.
/// @param MaxWidth Widest allowed line in pixels.
/// @param FontSize Height of the letters in pixels.
/// @param Shade Color of the text.
/// @param Text The text.
/// @return The pixel row just below the last line.
/// @warning Lines are cut at 255 characters. A single word wider than `MaxWidth` is not split.
float GuiDrawWrapped (float x, float y, float MaxWidth, int FontSize, Color Shade, const char* Text)
{
    char Line[384] ;
    size_t LineLength = 0 ;
    size_t TextLength = strlen(Text) ;
    size_t WordStart = 0 ;
    Line[0] = '\0' ;

    for (size_t ThisChar=0; ThisChar<=TextLength; ThisChar++)
    {
        if ((Text[ThisChar] == ' ') || (Text[ThisChar] == '\0'))
        {
            char Word[128] ;
            size_t WordLength = ThisChar - WordStart ;
            if (WordLength > 127) WordLength = 127 ;
            memcpy(Word, Text + WordStart, WordLength);
            Word[WordLength] = '\0' ;
            WordStart = ThisChar + 1 ;

            char Candidate[520] ;
            snprintf(Candidate, sizeof(Candidate), "%s%s%s", Line, (LineLength > 0) ? " " : "", Word);
            if ((LineLength > 0) && (GuiTextWidth(FontSize,Candidate) > MaxWidth))
            {
                GuiDrawText(x, y, FontSize, Shade, Line);
                y += (float)FontSize + 4.0f ;
                snprintf(Line, sizeof(Line), "%s", Word);
            }
            else
            {
                snprintf(Line, sizeof(Line), "%.380s", Candidate);
            }
            LineLength = strlen(Line) ;
        }
    }

    GuiDrawText(x, y, FontSize, Shade, Line);
    return (y + (float)FontSize + 4.0f);
}





//====================================================================================================
//Window
//====================================================================================================

/// @brief Opens the window and loads the fonts. Call once at the start.
/// @param Width Window width in pixels.
/// @param Height Window height in pixels.
/// @param Title Text in the title bar.
void GuiOpenWindow (int Width, int Height, const char* Title)
{
    SetTraceLogLevel(LOG_ERROR);                                    //Raylib is chatty by default
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);      //Smoother lines, and the user may resize
    InitWindow(Width, Height, Title);
    SetWindowMinSize((int)(940.0*GuiScale), (int)(620.0*GuiScale));
    SetTargetFPS(60);

    //Which characters to prepare: all normal keyboard characters, then a few for physics. Must match what PlotFontData.h contains.
    int Codepoints[160] ;
    int CodepointCount = 0 ;
    for (int ThisChar=32; ThisChar<=126; ThisChar++) Codepoints[CodepointCount++] = ThisChar;
    int Extras[] = { 176, 178, 179, 183, 215, 916, 931, 937, 8211, 8230, 8722 } ;   //degree, squared, cubed, middle dot, times, Delta, Sigma, Omega, en dash, ellipsis, minus
    for (int ThisExtra=0; ThisExtra<(int)(sizeof(Extras)/sizeof(Extras[0])); ThisExtra++) Codepoints[CodepointCount++] = Extras[ThisExtra];
    for (int ThisChar=945; ThisChar<=969; ThisChar++) Codepoints[CodepointCount++] = ThisChar;   //Greek lowercase: alpha ... omega

    for (int ThisFont=0; ThisFont<GUI_FONT_COUNT; ThisFont++)
    {
        GuiFonts[ThisFont] = LoadFontFromMemory(".ttf", PlotFontData, PlotFontDataSize, GuiFontSizes[ThisFont], Codepoints, CodepointCount) ;
        SetTextureFilter(GuiFonts[ThisFont].texture, TEXTURE_FILTER_BILINEAR);
    }
    return;
}



/// @brief Changes the text size (and with it the size of the whole interface) while the app runs.
/// @param NewSize New normal text size in pixels, limited to 11 through 26, and also limited so that the smallest allowed window (940 by 620 logical pixels) still fits on this monitor.
void GuiSetFontSize (int NewSize)
{
    int Monitor = GetCurrentMonitor() ;
    double FitWidth = (double)GetMonitorWidth(Monitor) / 940.0 ;
    double FitHeight = (double)GetMonitorHeight(Monitor) / 620.0 ;
    int BiggestThatFits = (int)((double)GUI_DESIGN_FONT_SIZE * ((FitWidth < FitHeight) ? FitWidth : FitHeight)) ;
    if (NewSize > BiggestThatFits) NewSize = BiggestThatFits ;
    if (NewSize < 11) NewSize = 11 ;
    if (NewSize > 26) NewSize = 26 ;
    GuiFontSize = NewSize ;
    GuiScale = (double)NewSize / (double)GUI_DESIGN_FONT_SIZE ;
    SetWindowMinSize((int)(940.0*GuiScale), (int)(620.0*GuiScale));
    return;
}



/// @brief Width of the window in logical pixels (screen pixels divided by the scale).
/// @return Width in logical pixels.
float GuiScreenWidth (void)
{
    return (float)((double)GetScreenWidth() / GuiScale);
}



/// @brief Height of the window in logical pixels.
/// @return Height in logical pixels.
float GuiScreenHeight (void)
{
    return (float)((double)GetScreenHeight() / GuiScale);
}



/// @brief Closes the window. Call once at the end.
void GuiCloseWindow (void)
{
    for (int ThisFont=0; ThisFont<GUI_FONT_COUNT; ThisFont++) UnloadFont(GuiFonts[ThisFont]);
    CloseWindow();
    return;
}



/// @brief Works out where the three panels go for the current window size. Thin panels left and right, the big one in the middle.
/// @return The `GuiLayout`.
GuiLayout GuiMakeLayout (void)
{
    float Margin = 8.0f ;
    float Width = GuiScreenWidth() ;
    float Height = GuiScreenHeight() ;
    float LeftWidth = (Width < 1180.0f) ? 250.0f : 280.0f ;
    float RightWidth = (Width < 1180.0f) ? 280.0f : 320.0f ;

    GuiLayout Layout ;
    (Layout.Left) = (Rectangle){ Margin, Margin, LeftWidth, Height - 2.0f*Margin } ;
    (Layout.Right) = (Rectangle){ Width - Margin - RightWidth, Margin, RightWidth, Height - 2.0f*Margin } ;
    (Layout.Middle) = (Rectangle){ Margin + LeftWidth + Margin, Margin, Width - 4.0f*Margin - LeftWidth - RightWidth, Height - 2.0f*Margin } ;
    return Layout;
}



/// @brief Draws a panel: filled box, border and a title.
/// @param Box Where the panel is.
/// @param Title Text at the top left.
/// @param Fill Background color.
void GuiDrawPanel (Rectangle Box, const char* Title, Color Fill)
{
    DrawRectangleRec(Box, Fill);
    DrawRectangleLinesEx(Box, 1.0f, GUI_FRAME);
    GuiDrawText(Box.x + 14.0f, Box.y + 10.0f, 22, GUI_TEXT, Title);
    DrawLineEx((Vector2){ Box.x + 14.0f, Box.y + 40.0f }, (Vector2){ Box.x + Box.width - 14.0f, Box.y + 40.0f }, 1.0f, GUI_GRID);
    return;
}





//====================================================================================================
//Widgets (immediate mode: the call draws the widget AND tells you what the mouse did)
//====================================================================================================

/// @brief Reads the mouse once per frame. Call first thing in every frame.
void GuiBeginFrame (void)
{
    Vector2 ScreenMouse = GetMousePosition() ;
    (GuiNow.Mouse) = (Vector2){ (float)((double)ScreenMouse.x / GuiScale), (float)((double)ScreenMouse.y / GuiScale) } ;     //Back to logical pixels
    (GuiNow.Clicked) = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ;
    (GuiNow.Wheel) = GetMouseWheelMove() ;
    (GuiNow.Held) = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ;
    if (!(GuiNow.Held)) (GuiNow.ActiveId) = 0 ;
    (GuiNow.Blocked) = false ;
    return;
}



/// @brief Tells if the mouse is over a rectangle and the widgets are allowed to react.
/// @param Box The rectangle.
/// @return `true` if the mouse is inside and no popup is blocking.
bool GuiMouseOver (Rectangle Box)
{
    if (GuiNow.Blocked) return false;
    if (GuiClipActive && !CheckCollisionPointRec(GuiNow.Mouse, GuiClipBox)) return false;
    return CheckCollisionPointRec(GuiNow.Mouse, Box);
}



/// @brief A push button.
/// @param Box Where the button is.
/// @param Label Text on the button.
/// @param Enabled `false` draws it greyed out and ignores clicks.
/// @param Primary `true` draws it in the accent color (the button to press next).
/// @return `true` on the frame the button was clicked.
bool GuiButton (Rectangle Box, const char* Label, bool Enabled, bool Primary)
{
    bool Over = Enabled && GuiMouseOver(Box) ;
    Color Fill = Primary ? GUI_ACCENT : GUI_WIDGET ;
    if (Over) Fill = Primary ? GUI_ACCENT_HOVER : GUI_FRAME ;
    if (!Enabled) Fill = GUI_WIDGET ;

    DrawRectangleRec(Box, Fill);
    Color TextShade = Primary ? GUI_ON_ACCENT : GUI_TEXT ;
    if (!Enabled) TextShade = GUI_MUTED ;
    GuiDrawTextCentered(Box.x + 0.5f*Box.width, Box.y + 0.5f*Box.height - 8.0f, 16, TextShade, Label);
    return (Over && (GuiNow.Clicked));
}



/// @brief A row of buttons of which exactly one is selected (like radio buttons).
/// @param Box Where the whole row is.
/// @param Count How many choices.
/// @param Labels Array of `Count` texts.
/// @param Selected Address of the selected index. Changed when the user clicks another one.
/// @param DisabledMask Bit `k` set means choice `k` is greyed out and cannot be picked. Use `0` for none.
/// @return `true` on the frame the selection changed.
bool GuiChoice (Rectangle Box, int Count, const char** Labels, int* Selected, unsigned DisabledMask)
{
    bool Changed = false ;
    float PieceWidth = Box.width / (float)Count ;

    for (int ThisChoice=0; ThisChoice<Count; ThisChoice++)
    {
        Rectangle Piece = { Box.x + PieceWidth*(float)ThisChoice, Box.y, PieceWidth - 2.0f, Box.height } ;
        bool IsDisabled = ((DisabledMask >> ThisChoice) & 1u) != 0 ;
        bool IsSelected = (ThisChoice == (*Selected)) ;
        bool Over = (!IsDisabled) && GuiMouseOver(Piece) ;

        Color Fill = IsSelected ? GUI_ACCENT : GUI_WIDGET ;
        if (Over && !IsSelected) Fill = GUI_FRAME ;
        DrawRectangleRec(Piece, Fill);

        Color TextShade = IsSelected ? GUI_ON_ACCENT : GUI_TEXT ;
        if (IsDisabled) TextShade = GUI_MUTED ;
        GuiDrawTextCentered(Piece.x + 0.5f*Piece.width, Piece.y + 0.5f*Piece.height - 7.0f, 14, TextShade, Labels[ThisChoice]);

        if (Over && (GuiNow.Clicked) && !IsSelected)
        {
            (*Selected) = ThisChoice ;
            Changed = true ;
        }
    }
    return Changed;
}



/// @brief A slider with a label on the left and the value on the right.
/// @param Id A number that is different for every slider (`1`, `2`, ...). Needed to remember which one is being dragged.
/// @param Box Where the slider is. Make it about 40 pixels tall.
/// @param Label Text above the track.
/// @param Value Address of the value. Changed while dragging.
/// @param Min Value at the left end.
/// @param Max Value at the right end.
/// @param Decimals How many decimals to show. `0` also makes the slider move in whole numbers.
/// @param ShownValue Text to show at the right instead of the number (like `"1.0e-04"` for a slider that holds an exponent). `NULL` shows the number.
/// @return `true` on every frame the value changed.
bool GuiSliderCore (int Id, Rectangle Box, const char* Label, double* Value, double Min, double Max, int Decimals, const char* ShownValue)
{
    bool Changed = false ;
    Rectangle Track = { Box.x, Box.y + 26.0f, Box.width, 6.0f } ;
    Rectangle Hit = { Box.x - 6.0f, Box.y + 16.0f, Box.width + 12.0f, 26.0f } ;

    if (GuiMouseOver(Hit) && (GuiNow.Clicked)) (GuiNow.ActiveId) = Id ;
    if ((GuiNow.ActiveId) == Id)
    {
        double Fraction = ((double)(GuiNow.Mouse.x) - (double)Track.x) / (double)Track.width ;
        if (Fraction < 0.0) Fraction = 0.0 ;
        if (Fraction > 1.0) Fraction = 1.0 ;
        double NewValue = Min + Fraction*(Max - Min) ;
        if (Decimals == 0) NewValue = floor(NewValue + 0.5) ;
        if (NewValue != (*Value))
        {
            (*Value) = NewValue ;
            Changed = true ;
        }
    }

    char ValueText[48] ;
    if (ShownValue != NULL) snprintf(ValueText, sizeof(ValueText), "%s", ShownValue);
    else snprintf(ValueText, sizeof(ValueText), "%.*f", Decimals, (*Value));
    GuiDrawText(Box.x, Box.y, 14, GUI_TEXT, Label);
    GuiDrawTextRight(Box.x + Box.width, Box.y, 14, GUI_MUTED, ValueText);

    float Fraction = (float)(((*Value) - Min) / (Max - Min)) ;
    DrawRectangleRec(Track, GUI_WIDGET);
    DrawRectangleRec((Rectangle){ Track.x, Track.y, Track.width*Fraction, Track.height }, GUI_ACCENT);
    DrawCircleV((Vector2){ Track.x + Track.width*Fraction, Track.y + 3.0f }, 8.0f, GUI_ACCENT);
    DrawCircleV((Vector2){ Track.x + Track.width*Fraction, Track.y + 3.0f }, 4.0f, GUI_PANEL);
    return Changed;
}



/// @brief A slider that shows its number. See `GuiSliderCore` for the arguments.
/// @param Id A number that is different for every slider.
/// @param Box Where the slider is.
/// @param Label Text above the track.
/// @param Value Address of the value.
/// @param Min Value at the left end.
/// @param Max Value at the right end.
/// @param Decimals Decimals to show (`0` = whole numbers).
/// @return `true` on every frame the value changed.
bool GuiSlider (int Id, Rectangle Box, const char* Label, double* Value, double Min, double Max, int Decimals)
{
    return GuiSliderCore(Id, Box, Label, Value, Min, Max, Decimals, NULL);
}



/// @brief A checkbox with a label.
/// @param Box Where the checkbox row is (about 24 pixels tall).
/// @param Label Text next to the box.
/// @param Value Address of the on/off value. Flipped when the user clicks.
/// @return `true` on the frame the value changed.
bool GuiToggle (Rectangle Box, const char* Label, bool* Value)
{
    Rectangle Square = { Box.x, Box.y + 2.0f, 18.0f, 18.0f } ;
    bool Changed = false ;

    DrawRectangleRec(Square, (*Value) ? GUI_ACCENT : GUI_WIDGET);
    if (*Value)
    {
        DrawLineEx((Vector2){ Square.x + 4.0f, Square.y + 9.0f }, (Vector2){ Square.x + 8.0f, Square.y + 13.0f }, 2.0f, GUI_ON_ACCENT);
        DrawLineEx((Vector2){ Square.x + 8.0f, Square.y + 13.0f }, (Vector2){ Square.x + 14.0f, Square.y + 5.0f }, 2.0f, GUI_ON_ACCENT);
    }
    GuiDrawText(Box.x + 28.0f, Box.y + 3.0f, 14, GUI_TEXT, Label);

    if (GuiMouseOver(Box) && (GuiNow.Clicked))
    {
        (*Value) = !(*Value) ;
        Changed = true ;
    }
    return Changed;
}





//====================================================================================================
//Plotting helpers
//====================================================================================================

/// @brief Starts clipping: everything drawn until `EndScissorMode` is only visible inside the rectangle.
/// @param Box The rectangle in logical pixels (the scale is applied here, because the clip works on screen pixels).
void GuiBeginClip (Rectangle Box)
{
    BeginScissorMode((int)((double)Box.x * GuiScale), (int)((double)Box.y * GuiScale), (int)((double)Box.width * GuiScale) + 1, (int)((double)Box.height * GuiScale) + 1);
    return;
}



/// @brief Picks a "nice" distance between axis ticks (1, 2 or 5 times a power of ten).
/// @param Range Width of the data range.
/// @param WantedTicks Roughly how many ticks are wanted.
/// @return The tick distance in `double`.
double GuiNiceStep (double Range, int WantedTicks)
{
    double Rough = Range / (double)WantedTicks ;
    double Magnitude = pow(10.0, floor(log10(Rough))) ;
    double Normal = Rough / Magnitude ;
    double Factor = 10.0 ;
    if (Normal < 1.5) Factor = 1.0 ;
    else if (Normal < 3.5) Factor = 2.0 ;
    else if (Normal < 7.5) Factor = 5.0 ;
    return (Factor * Magnitude);
}



/// @brief Finds the smallest and largest value in one column of a matrix, with a little padding so curves do not touch the frame.
/// @param Data Address of the matrix.
/// @param Col Which column.
/// @param SkipEnds How many rows to ignore at each end (the ends of a derivative estimate can be wild).
/// @param Min Receives the padded minimum.
/// @param Max Receives the padded maximum.
void GuiFindRange (Matrix* Data, size_t Col, size_t SkipEnds, double* Min, double* Max)
{
    double Low = 1e300 ;
    double High = -1e300 ;
    for (size_t ThisRow=SkipEnds; ThisRow<(Data->RowCount)-SkipEnds; ThisRow++)
    {
        double Value = GetMatrixCell(Data, ThisRow, Col) ;
        if (Value < Low) Low = Value ;
        if (Value > High) High = Value ;
    }
    if (High - Low < 1e-12)
    {
        High += 0.5 ;
        Low -= 0.5 ;
    }
    double Pad = 0.06 * (High - Low) ;
    (*Min) = Low - Pad ;
    (*Max) = High + Pad ;
    return;
}



/// @brief Converts a data point to a pixel position inside a plot.
/// @param Plot Address of the plot area.
/// @param x Data value on the horizontal axis.
/// @param y Data value on the vertical axis.
/// @return The pixel position.
Vector2 GuiToPixel (GuiPlotBox* Plot, double x, double y)
{
    float Px = (Plot->Box.x) + (float)((x - (Plot->XMin)) / ((Plot->XMax) - (Plot->XMin))) * (Plot->Box.width) ;
    float Py = (Plot->Box.y) + (Plot->Box.height) - (float)((y - (Plot->YMin)) / ((Plot->YMax) - (Plot->YMin))) * (Plot->Box.height) ;
    return (Vector2){ Px, Py };
}



/// @brief Draws the frame, grid, tick numbers and axis labels of a plot.
/// @param Plot Address of the plot area.
/// @param XLabel Text under the horizontal axis (can be empty).
/// @param YLabel Text above the vertical axis (can be empty).
void GuiDrawAxes (GuiPlotBox* Plot, const char* XLabel, const char* YLabel)
{
    DrawRectangleRec((Plot->Box), GUI_PANEL);

    double XStep = GuiNiceStep((Plot->XMax) - (Plot->XMin), 6) ;
    double YStep = GuiNiceStep((Plot->YMax) - (Plot->YMin), 4) ;
    char Number[32] ;

    double FirstXTick = ceil((Plot->XMin)/XStep) ;
    for (int TickCount=0; TickCount<40; TickCount++)
    {
        double XTick = (FirstXTick + (double)TickCount + 0.0) * XStep ;     //The + 0.0 turns a minus zero into a plain zero
        if (XTick > (Plot->XMax)) break;
        Vector2 Where = GuiToPixel(Plot, XTick, (Plot->YMin)) ;
        DrawLineEx(Where, (Vector2){ Where.x, (Plot->Box.y) }, 1.0f, GUI_GRID);
        snprintf(Number, sizeof(Number), "%.4g", XTick);
        GuiDrawTextCentered(Where.x, Where.y + 4.0f, 12, GUI_MUTED, Number);
    }
    double FirstYTick = ceil((Plot->YMin)/YStep) ;
    for (int TickCount=0; TickCount<40; TickCount++)
    {
        double YTick = (FirstYTick + (double)TickCount + 0.0) * YStep ;
        if (YTick > (Plot->YMax)) break;
        Vector2 Where = GuiToPixel(Plot, (Plot->XMin), YTick) ;
        DrawLineEx(Where, (Vector2){ (Plot->Box.x) + (Plot->Box.width), Where.y }, 1.0f, (fabs(YTick) < 1e-9*YStep) ? GUI_FRAME : GUI_GRID);
        snprintf(Number, sizeof(Number), "%.4g", YTick);
        GuiDrawTextRight(Where.x - 6.0f, Where.y - 7.0f, 12, GUI_MUTED, Number);
    }

    DrawRectangleLinesEx((Plot->Box), 1.0f, GUI_FRAME);
    GuiDrawTextCentered((Plot->Box.x) + 0.5f*(Plot->Box.width), (Plot->Box.y) + (Plot->Box.height) + 18.0f, 14, GUI_MUTED, XLabel);
    GuiDrawText((Plot->Box.x), (Plot->Box.y) - 20.0f, 14, GUI_MUTED, YLabel);
    return;
}



/// @brief Draws a curve made of two matrix columns (only some rows of them), clipped to the plot area.
/// @param Plot Address of the plot area.
/// @param XData Address of the matrix holding the horizontal values.
/// @param XCol Column of the horizontal values.
/// @param YData Address of the matrix holding the vertical values (can be the same matrix).
/// @param YCol Column of the vertical values.
/// @param FromRow First row to draw.
/// @param ToRow One past the last row to draw. Fewer than 2 rows draws nothing.
/// @param Shade Color of the curve.
/// @param Thickness Line thickness in pixels.
void GuiDrawSeriesRange (GuiPlotBox* Plot, Matrix* XData, size_t XCol, Matrix* YData, size_t YCol, size_t FromRow, size_t ToRow, Color Shade, float Thickness)
{
    if (ToRow < FromRow + 2) return;
    GuiBeginClip(Plot->Box);
    Vector2 Before = GuiToPixel(Plot, GetMatrixCell(XData,FromRow,XCol), GetMatrixCell(YData,FromRow,YCol)) ;
    for (size_t ThisRow=FromRow+1; ThisRow<ToRow; ThisRow++)
    {
        Vector2 Here = GuiToPixel(Plot, GetMatrixCell(XData,ThisRow,XCol), GetMatrixCell(YData,ThisRow,YCol)) ;
        DrawLineEx(Before, Here, Thickness, Shade);
        Before = Here ;
    }
    EndScissorMode();
    return;
}



/// @brief Draws a curve made of two matrix columns (all rows), clipped to the plot area.
/// @param Plot Address of the plot area.
/// @param XData Address of the matrix holding the horizontal values.
/// @param XCol Column of the horizontal values.
/// @param YData Address of the matrix holding the vertical values (can be the same matrix).
/// @param YCol Column of the vertical values.
/// @param Shade Color of the curve.
/// @param Thickness Line thickness in pixels.
void GuiDrawSeries (GuiPlotBox* Plot, Matrix* XData, size_t XCol, Matrix* YData, size_t YCol, Color Shade, float Thickness)
{
    GuiDrawSeriesRange(Plot, XData, XCol, YData, YCol, 0, (XData->RowCount), Shade, Thickness);
    return;
}





//====================================================================================================
//The pipeline (data in, equation out). Every stage keeps its result.
//====================================================================================================

/// @brief Gives the app a clean starting state with the default options.
/// @return A new `GuiApp` with no data.
GuiApp GuiAppCreate (void)
{
    GuiApp App ;
    memset(&App, 0, sizeof(App));
    (App.Options.Mode) = 0 ;
    (App.Options.InterpMethod) = INTERP_SPLINE ;
    (App.Options.DriftTolerance) = 0.05 ;
    (App.Options.DerivMethod) = 1 ;
    (App.Options.SgHalfWindow) = 10.0 ;
    (App.Options.SgOrder) = 3.0 ;
    (App.Options.MaxDegree) = 3.0 ;
    (App.Options.IncludeTrig) = true ;
    (App.Options.Threshold) = 0.1 ;
    (App.Options.NeuralHidden) = 16.0 ;
    (App.Options.NeuralSteps) = 1024.0 ;
    (App.Options.NeuralChunk) = 128.0 ;
    (App.Options.DemoNoiseExp) = -4.0 ;
    (App.Options.DemoJitter) = 0.0 ;
    (App.DirtyFrom) = GUI_STAGE_COUNT ;
    (App.Playing) = true ;
    return App;
}



/// @brief Throws away the results of a stage and every stage after it.
/// @param App Address of the app.
/// @param FirstStage The first stage to throw away.
/// @attention Stages are freed from the last to the first, because later ones borrow memory from earlier ones (the STLSQ jobs borrow `Theta`).
void GuiFreeResultsFrom (GuiApp* App, int FirstStage)
{
    if ((FirstStage <= GUI_STAGE_SPARSE) && (App->StageReady[GUI_STAGE_SPARSE]))
    {
        free(App->SimStates);
        free(App->SimReady);
        free(App->SimError);
        (App->SimStates) = NULL ;
        (App->SimReady) = NULL ;
        (App->SimError) = NULL ;
        (App->SimCount) = 0 ;
        StlsqJobDestroy(&(App->JobDx));
        StlsqJobDestroy(&(App->JobDy));
        (App->StageReady[GUI_STAGE_SPARSE]) = false ;
        (App->HasSupport) = false ;
    }
    if ((FirstStage <= GUI_STAGE_LIBRARY) && (App->StageReady[GUI_STAGE_LIBRARY]))
    {
        free(App->TargetDx);
        free(App->TargetDy);
        MatrixDestroy(&(App->Theta));
        DestroyTermLibrary(&(App->Library));
        (App->StageReady[GUI_STAGE_LIBRARY]) = false ;
    }
    if ((FirstStage <= GUI_STAGE_DERIVATIVES) && (App->NeuralActive))
    {
        MatrixDestroy(&(App->Job.Derivatives));
        NeuralJobDestroy(&(App->Job));
        MatrixDestroy(&(App->FdSlopes));
        (App->NeuralActive) = false ;
    }
    if ((FirstStage <= GUI_STAGE_DERIVATIVES) && (App->StageReady[GUI_STAGE_DERIVATIVES]))
    {
        MatrixDestroy(&(App->Slopes));
        (App->StageReady[GUI_STAGE_DERIVATIVES]) = false ;
    }
    if ((FirstStage <= GUI_STAGE_TIMING) && (App->StageReady[GUI_STAGE_TIMING]))
    {
        MatrixDestroy(&(App->EvenPath));
        free(App->WasInterpolated);
        (App->WasInterpolated) = NULL ;
        (App->StageReady[GUI_STAGE_TIMING]) = false ;
    }
    return;
}



/// @brief Says that an option changed, so this stage and every later one are out of date.
/// @param App Address of the app.
/// @param Stage The first stage that depends on the option.
void GuiMarkDirty (GuiApp* App, int Stage)
{
    if (Stage < (App->DirtyFrom)) (App->DirtyFrom) = Stage ;
    return;
}



/// @brief The names of the stages, for the processing overlay.
const char* GuiStageNames[GUI_STAGE_COUNT] = { "Making the time even", "Estimating derivatives", "Building the candidate terms", "Sparse regression" } ;



/// @brief Runs ONE stage of the pipeline (and throws away the old results of that stage and every later one first).
/// @param App Address of the app.
/// @param Stage Which stage.
/// @warning Needs `HasData` and the earlier stages already done.
void GuiRunStage (GuiApp* App, int Stage)
{
    GuiFreeResultsFrom(App, Stage);
    double Start = GetTime() ;

    switch (Stage)
    {
        case GUI_STAGE_TIMING:
        {
            size_t RowCount = (App->RawPath.RowCount) ;
            (App->Timing) = AnalyzeTiming(&(App->RawPath), (App->Options.DriftTolerance)) ;
            (App->WasInterpolated) = (bool*) calloc(RowCount, sizeof(bool)) ;
            (App->InterpolatedCount) = 0 ;

            if ((App->TimeIsUsable) && ((App->Options.Mode) == 0))
            {
                (App->EvenPath) = MakeTimeEven(&(App->RawPath), (InterpolationMethod)(App->Options.InterpMethod), (App->Options.DriftTolerance), (App->WasInterpolated)) ;
                for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
                {
                    if ((App->WasInterpolated)[ThisRow]) (App->InterpolatedCount)++ ;
                }
            }
            else
            {
                (App->EvenPath) = MatrixCreate(RowCount, 3) ;
                for (size_t ThisCell=0; ThisCell<RowCount*3; ThisCell++) (App->EvenPath.MatrixCells)[ThisCell] = (App->RawPath.MatrixCells)[ThisCell] ;
            }
            break;
        }

        case GUI_STAGE_DERIVATIVES:
        {
            int HalfWindow = (int)((App->Options.SgHalfWindow) + 0.5) ;
            int MaxHalfWindow = (int)(((App->EvenPath.RowCount) - 1) / 2) ;
            if (HalfWindow > MaxHalfWindow) HalfWindow = MaxHalfWindow ;
            int Order = (int)((App->Options.SgOrder) + 0.5) ;
            if (Order > 2*HalfWindow) Order = 2*HalfWindow ;

            if ((App->Options.DerivMethod) == 0) (App->Slopes) = FiniteDifferenceDerivatives(&(App->EvenPath)) ;
            else (App->Slopes) = SavitzkyGolayDerivatives(&(App->EvenPath), HalfWindow, Order) ;
            break;
        }

        case GUI_STAGE_LIBRARY:
        {
            size_t RowCount = (App->EvenPath.RowCount) ;
            (App->Library) = CreateTermLibrary((int)((App->Options.MaxDegree) + 0.5), (App->Options.IncludeTrig)) ;
            (App->Theta) = BuildTermMatrix(&(App->Library), &(App->EvenPath), 1, 2) ;
            (App->TargetDx) = (double*) calloc(RowCount, sizeof(double)) ;
            (App->TargetDy) = (double*) calloc(RowCount, sizeof(double)) ;
            for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
            {
                (App->TargetDx)[ThisRow] = GetMatrixCell(&(App->Slopes), ThisRow, 0) ;
                (App->TargetDy)[ThisRow] = GetMatrixCell(&(App->Slopes), ThisRow, 1) ;
            }
            break;
        }

        case GUI_STAGE_SPARSE:
        {
            (App->JobDx) = StlsqJobStart(&(App->Theta), (App->TargetDx), (App->Options.Threshold), 1e-10, 10) ;
            while (!(App->JobDx.IsDone)) StlsqJobStep(&(App->JobDx));
            (App->JobDy) = StlsqJobStart(&(App->Theta), (App->TargetDy), (App->Options.Threshold), 1e-10, 10) ;
            while (!(App->JobDy.IsDone)) StlsqJobStep(&(App->JobDy));

            //Room for one simulation per snapshot (they are computed later, only for the snapshot on screen)
            size_t MostSnapshots = ((App->JobDx.SnapshotCount) > (App->JobDy.SnapshotCount)) ? (App->JobDx.SnapshotCount) : (App->JobDy.SnapshotCount) ;
            (App->SimCount) = MostSnapshots ;
            (App->SimStates) = (double*) calloc(MostSnapshots*(App->EvenPath.RowCount)*2, sizeof(double)) ;
            (App->SimReady) = (bool*) calloc(MostSnapshots, sizeof(bool)) ;
            (App->SimError) = (double*) calloc(MostSnapshots, sizeof(double)) ;

            (App->HasSupport) = false ;
            if (App->IsDemo)
            {
                size_t TermCount = (App->Library.TermCount) ;
                double* TrueDx = (double*) calloc(TermCount, sizeof(double)) ;
                double* TrueDy = (double*) calloc(TermCount, sizeof(double)) ;
                FillTrueCoefficients((App->DemoSystem), (App->DemoParams), &(App->Library), TrueDx, TrueDy);
                (App->Support) = CompareWithTruth(TrueDx, TrueDy, (App->JobDx.Coefficients), (App->JobDy.Coefficients), TermCount) ;
                (App->HasSupport) = true ;
                free(TrueDx);
                free(TrueDy);
            }

            (App->AnimRound) = 0 ;
            (App->AnimClock) = 0.0 ;
            (App->Playing) = true ;
            break;
        }

        default:
            break;
    }

    (App->StageReady[Stage]) = true ;
    (App->StageMilliseconds[Stage]) = 1000.0*(GetTime() - Start) ;
    return;
}



/// @brief Starts the out of date stages. They then run one per frame (see `GuiStepProcessing`) while the blocking overlay is shown.
/// @param App Address of the app.
void GuiStartProcessing (GuiApp* App)
{
    if (!(App->HasData)) return;
    if ((App->DirtyFrom) >= GUI_STAGE_COUNT) return;
    (App->Processing) = true ;
    (App->StageToRun) = (App->DirtyFrom) ;
    return;
}



/// @brief Runs the next stage if the pipeline is processing. Call once per frame, BEFORE drawing, so the overlay of the previous frame stays on screen while the stage works.
/// @param App Address of the app.
/// @attention The neural fitting is the one stage that takes many frames: every call trains for about 12 milliseconds, and the middle panel shows the networks learning. When the user presses "Skip animation" it trains without that limit.
void GuiStepProcessing (GuiApp* App)
{
    if (!(App->Processing)) return;

    if (((App->StageToRun) == GUI_STAGE_DERIVATIVES) && ((App->Options.Mode) == 1))
    {
        if (!(App->NeuralActive))
        {
            GuiFreeResultsFrom(App, GUI_STAGE_DERIVATIVES);
            RandomSeed(&(App->NeuralGen), 4242);
            (App->FdSlopes) = FiniteDifferenceDerivatives(&(App->EvenPath)) ;
            (App->Job) = NeuralJobStart(&(App->EvenPath), (size_t)((App->Options.NeuralChunk) + 0.5), (size_t)((App->Options.NeuralHidden) + 0.5), (size_t)((App->Options.NeuralSteps) + 0.5), 0.01, &(App->NeuralGen)) ;
            (App->NeuralActive) = true ;
            (App->NeuralSkip) = false ;
            (App->NeuralStartTime) = GetTime() ;
            return;
        }

        double Start = GetTime() ;
        do
        {
            NeuralJobWork(&(App->Job), 4);
        } while (!(App->Job.IsDone) && ((App->NeuralSkip) || (GetTime() - Start < 0.012)));

        if (App->Job.IsDone)
        {
            (App->Slopes) = (App->Job.Derivatives) ;
            NeuralJobDestroy(&(App->Job));
            MatrixDestroy(&(App->FdSlopes));
            (App->NeuralActive) = false ;
            (App->NeuralSkip) = false ;
            (App->StageReady[GUI_STAGE_DERIVATIVES]) = true ;
            (App->StageMilliseconds[GUI_STAGE_DERIVATIVES]) = 1000.0*(GetTime() - (App->NeuralStartTime)) ;
            (App->StageToRun)++ ;
        }
        return;
    }

    GuiRunStage(App, (App->StageToRun));
    (App->StageToRun)++ ;
    if ((App->StageToRun) >= GUI_STAGE_COUNT)
    {
        (App->Processing) = false ;
        (App->DirtyFrom) = GUI_STAGE_COUNT ;
    }
    return;
}



/// @brief Runs all out of date stages right now, without drawing in between. Used for the cheap regression stage and for automatic tests.
/// @param App Address of the app.
void GuiRunPipeline (GuiApp* App)
{
    GuiStartProcessing(App);
    while (App->Processing) GuiStepProcessing(App);
    return;
}



//====================================================================================================
//Simulating the discovered equations (the plots on top follow the round shown in the bar charts)
//====================================================================================================

/// @brief Evaluates the discovered model: `dx/dt` and `dy/dt` for a given state.
/// @param Library Address of the term library.
/// @param CoefX Coefficients of the `dx/dt` equation, one per term.
/// @param CoefY Coefficients of the `dy/dt` equation, one per term.
/// @param x Current `x`.
/// @param y Current `y`.
/// @param Dx Receives `dx/dt`.
/// @param Dy Receives `dy/dt`.
void GuiModelSlope (TermLibrary* Library, double* CoefX, double* CoefY, double x, double y, double* Dx, double* Dy)
{
    double SumX = 0.0 ;
    double SumY = 0.0 ;
    for (size_t ThisTerm=0; ThisTerm<(Library->TermCount); ThisTerm++)
    {
        if ((CoefX[ThisTerm] == 0.0) && (CoefY[ThisTerm] == 0.0)) continue;
        double Value = EvaluateTerm(&(Library->Terms[ThisTerm]), x, y) ;
        SumX += CoefX[ThisTerm] * Value ;
        SumY += CoefY[ThisTerm] * Value ;
    }
    (*Dx) = SumX ;
    (*Dy) = SumY ;
    return;
}



/// @brief Makes sure the simulation of one snapshot exists: starts at the first data point and follows the equations of that round with RK4, one step per data row.
/// @param App Address of the app (needs the sparse regression stage).
/// @param Snapshot Which snapshot (round).
/// @attention A model with wrong terms can blow up. When a value becomes huge or not finite, the rest of that simulation is marked `NaN` (not drawn) and its error is `NaN`.
void GuiEnsureSimulation (GuiApp* App, int Snapshot)
{
    if ((App->SimReady) == NULL) return;
    if ((App->SimReady)[Snapshot]) return;

    size_t RowCount = (App->EvenPath.RowCount) ;
    size_t TermCount = (App->Library.TermCount) ;
    int ShownX = ((size_t)Snapshot < (App->JobDx.SnapshotCount)) ? Snapshot : (int)(App->JobDx.SnapshotCount) - 1 ;
    int ShownY = ((size_t)Snapshot < (App->JobDy.SnapshotCount)) ? Snapshot : (int)(App->JobDy.SnapshotCount) - 1 ;
    double* CoefX = (App->JobDx.History) + (size_t)ShownX*TermCount ;
    double* CoefY = (App->JobDy.History) + (size_t)ShownY*TermCount ;
    double* Sim = (App->SimStates) + (size_t)Snapshot*RowCount*2 ;

    double BiggestData = 0.0 ;
    for (size_t ThisRow=0; ThisRow<RowCount; ThisRow++)
    {
        if (fabs(GetMatrixCell(&(App->EvenPath),ThisRow,1)) > BiggestData) BiggestData = fabs(GetMatrixCell(&(App->EvenPath),ThisRow,1)) ;
        if (fabs(GetMatrixCell(&(App->EvenPath),ThisRow,2)) > BiggestData) BiggestData = fabs(GetMatrixCell(&(App->EvenPath),ThisRow,2)) ;
    }
    double Limit = 100.0 * (1.0 + BiggestData) ;

    double x = GetMatrixCell(&(App->EvenPath), 0, 1) ;
    double y = GetMatrixCell(&(App->EvenPath), 0, 2) ;
    Sim[0] = x ;
    Sim[1] = y ;
    bool Blewup = false ;
    double SumSquares = 0.0 ;

    for (size_t ThisRow=0; ThisRow+1<RowCount; ThisRow++)
    {
        if (!Blewup)
        {
            double Step = GetMatrixCell(&(App->EvenPath),ThisRow+1,0) - GetMatrixCell(&(App->EvenPath),ThisRow,0) ;
            double Dx1, Dy1, Dx2, Dy2, Dx3, Dy3, Dx4, Dy4 ;
            GuiModelSlope(&(App->Library), CoefX, CoefY, x, y, &Dx1, &Dy1);
            GuiModelSlope(&(App->Library), CoefX, CoefY, x + 0.5*Step*Dx1, y + 0.5*Step*Dy1, &Dx2, &Dy2);
            GuiModelSlope(&(App->Library), CoefX, CoefY, x + 0.5*Step*Dx2, y + 0.5*Step*Dy2, &Dx3, &Dy3);
            GuiModelSlope(&(App->Library), CoefX, CoefY, x + Step*Dx3, y + Step*Dy3, &Dx4, &Dy4);
            x += Step*(Dx1 + 2.0*Dx2 + 2.0*Dx3 + Dx4)/6.0 ;
            y += Step*(Dy1 + 2.0*Dy2 + 2.0*Dy3 + Dy4)/6.0 ;
            if ((!isfinite(x)) || (!isfinite(y)) || (fabs(x) > Limit) || (fabs(y) > Limit)) Blewup = true ;
        }

        if (Blewup)
        {
            Sim[2*(ThisRow+1)] = NAN ;
            Sim[2*(ThisRow+1)+1] = NAN ;
        }
        else
        {
            Sim[2*(ThisRow+1)] = x ;
            Sim[2*(ThisRow+1)+1] = y ;
            double MissX = x - GetMatrixCell(&(App->EvenPath),ThisRow+1,1) ;
            double MissY = y - GetMatrixCell(&(App->EvenPath),ThisRow+1,2) ;
            SumSquares += MissX*MissX + MissY*MissY ;
        }
    }

    (App->SimError)[Snapshot] = Blewup ? NAN : sqrt(SumSquares / (2.0*(double)RowCount)) ;
    (App->SimReady)[Snapshot] = true ;
    return;
}



/// @brief Draws the simulated curves of the snapshot on screen over a plot, as dashed lines, plus a one line legend.
/// @param App Address of the app.
/// @param Plot Address of the plot area.
/// @param View `0` = `x(t)` and `y(t)` against time, `1` = phase portrait (`y` against `x`).
void GuiDrawSimulation (GuiApp* App, GuiPlotBox* Plot, int View)
{
    int MostSnapshots = (int)(App->SimCount) ;
    int Shown = ((App->AnimRound) < MostSnapshots) ? (App->AnimRound) : MostSnapshots - 1 ;
    GuiEnsureSimulation(App, Shown);

    size_t RowCount = (App->EvenPath.RowCount) ;
    double* Sim = (App->SimStates) + (size_t)Shown*RowCount*2 ;

    GuiBeginClip(Plot->Box);
    for (size_t ThisRow=0; ThisRow+1<RowCount; ThisRow++)
    {
        if ((ThisRow / 6) % 2 == 1) continue;       //Gaps make the line dashed
        if ((!isfinite(Sim[2*(ThisRow+1)])) || (!isfinite(Sim[2*(ThisRow+1)+1]))) break;

        double Time0 = GetMatrixCell(&(App->EvenPath), ThisRow, 0) ;
        double Time1 = GetMatrixCell(&(App->EvenPath), ThisRow+1, 0) ;
        if (View == 1)
        {
            DrawLineEx(GuiToPixel(Plot, Sim[2*ThisRow], Sim[2*ThisRow+1]), GuiToPixel(Plot, Sim[2*(ThisRow+1)], Sim[2*(ThisRow+1)+1]), 2.5f, GUI_TEXT);
        }
        else
        {
            DrawLineEx(GuiToPixel(Plot, Time0, Sim[2*ThisRow]), GuiToPixel(Plot, Time1, Sim[2*(ThisRow+1)]), 2.5f, GUI_TEXT);
            DrawLineEx(GuiToPixel(Plot, Time0, Sim[2*ThisRow+1]), GuiToPixel(Plot, Time1, Sim[2*(ThisRow+1)+1]), 2.5f, GUI_TEXT);
        }
    }
    EndScissorMode();

    char Legend[160] ;
    if (isnan((App->SimError)[Shown])) snprintf(Legend, sizeof(Legend), "dashed: simulation from the equations of fit %d blew up (wrong terms)", Shown+1);
    else snprintf(Legend, sizeof(Legend), "dashed: simulated from the equations of fit %d, average gap to the data %.3g", Shown+1, (App->SimError)[Shown]);
    DrawRectangleRec((Rectangle){ Plot->Box.x + 6.0f, Plot->Box.y + Plot->Box.height - 26.0f, GuiTextWidth(14, Legend) + 8.0f, 20.0f }, Fade(GUI_PANEL, 0.85f));
    GuiDrawText(Plot->Box.x + 10.0f, Plot->Box.y + Plot->Box.height - 23.0f, 14, isnan((App->SimError)[Shown]) ? GUI_BAD : GUI_TEXT, Legend);
    return;
}





/// @brief Gives the app a new dataset (it takes over the memory of `Path`) and starts the whole pipeline on it (it then runs while the overlay is shown).
/// @param App Address of the app.
/// @param Path The data, columns `t, x, y`, at least 3 rows. The app owns it from now on.
/// @param Name Text to show as the source.
/// @warning Does not check the row count, the caller must.
void GuiSetData (GuiApp* App, Matrix Path, const char* Name)
{
    if (App->HasData)
    {
        GuiFreeResultsFrom(App, GUI_STAGE_TIMING);
        MatrixDestroy(&(App->RawPath));
    }
    (App->RawPath) = Path ;
    (App->HasData) = true ;
    snprintf((App->SourceName), sizeof(App->SourceName), "%s", Name);
    (App->Timing) = AnalyzeTiming(&Path, 0.0) ;
    (App->TimeIsUsable) = (Path.RowCount >= 3) && (App->Timing.IsIncreasing) ;
    (App->Message[0]) = '\0' ;
    (App->AskForFile) = false ;
    (App->DirtyFrom) = GUI_STAGE_TIMING ;
    GuiStartProcessing(App);
    return;
}



/// @brief Loads a CSV file (columns `t, x, y`) into the app.
/// @param App Address of the app.
/// @param FilePath Path of the file.
/// @attention If fewer than 3 good rows are found, the old data stays and a message is shown.
void GuiLoadFile (GuiApp* App, const char* FilePath)
{
    Matrix Path = LoadPathFromCsv(FilePath) ;
    if (Path.RowCount < 3)
    {
        snprintf((App->Message), sizeof(App->Message), "Could not read 3 or more rows of t, x, y from %s", GetFileName(FilePath));
        MatrixDestroy(&Path);
        return;
    }
    (App->IsDemo) = false ;
    GuiSetData(App, Path, GetFileName(FilePath));
    return;
}



/// @brief Makes the data of a demo: a built-in system simulated for 30 time units (3001 rows), with optional clock jitter and noise.
/// @param System Which system.
/// @param Params The constants of the system.
/// @param NoiseSigma Width of the noise added to `x` and `y`.
/// @param Jitter Clock jitter as a fraction of the time step (`0` = perfectly even time). The values are exact for the shaken times (read from a much finer simulation with a spline).
/// @return A new `Matrix` (columns `t, x, y`).
Matrix GuiMakeDemoPath (SystemID System, double* Params, double NoiseSigma, double Jitter)
{
    RandomState Gen ;
    RandomSeed(&Gen, 2026);
    Matrix Path ;

    if (Jitter <= 0.0)
    {
        Path = TraceTrajectory(System, Params, GetDefaultStartState(System), 0.01, 3001) ;
    }
    else
    {
        size_t FineCount = 30001 ;
        Matrix Fine = TraceTrajectory(System, Params, GetDefaultStartState(System), 0.001, FineCount) ;
        double* Times = (double*) calloc(FineCount, sizeof(double)) ;
        double* ValuesX = (double*) calloc(FineCount, sizeof(double)) ;
        double* ValuesY = (double*) calloc(FineCount, sizeof(double)) ;
        double* SlopesX = (double*) calloc(FineCount, sizeof(double)) ;
        double* SlopesY = (double*) calloc(FineCount, sizeof(double)) ;
        for (size_t ThisRow=0; ThisRow<FineCount; ThisRow++)
        {
            Times[ThisRow] = GetMatrixCell(&Fine,ThisRow,0) ;
            ValuesX[ThisRow] = GetMatrixCell(&Fine,ThisRow,1) ;
            ValuesY[ThisRow] = GetMatrixCell(&Fine,ThisRow,2) ;
        }
        ComputeNodeSlopes(INTERP_SPLINE, Times, ValuesX, FineCount, SlopesX);
        ComputeNodeSlopes(INTERP_SPLINE, Times, ValuesY, FineCount, SlopesY);

        Path = MatrixCreate(3001, 3) ;
        for (size_t ThisRow=0; ThisRow<3001; ThisRow++)
        {
            double Time = 0.01 * (double)ThisRow ;
            if ((ThisRow > 0) && (ThisRow < 3000)) Time += (2.0*RandomUniform(&Gen) - 1.0) * Jitter * 0.01 ;
            SetMatrixCell(&Path, ThisRow, 0, Time);
            SetMatrixCell(&Path, ThisRow, 1, EvaluateInterpolant(INTERP_SPLINE, Times, ValuesX, SlopesX, FineCount, Time));
            SetMatrixCell(&Path, ThisRow, 2, EvaluateInterpolant(INTERP_SPLINE, Times, ValuesY, SlopesY, FineCount, Time));
        }
        free(Times);
        free(ValuesX);
        free(ValuesY);
        free(SlopesX);
        free(SlopesY);
        MatrixDestroy(&Fine);
    }

    AddGaussianNoise(&Path, NoiseSigma, &Gen);
    return Path;
}



/// @brief Simulates one of the built-in systems (with the clock jitter currently set in the options) and loads it as the data.
/// @param App Address of the app.
/// @param System Which system.
/// @param NoiseSigma Width of the noise added to `x` and `y`.
void GuiLoadDemo (GuiApp* App, SystemID System, double NoiseSigma)
{
    (App->DemoSystem) = System ;
    GetDefaultParams(System, (App->DemoParams));
    (App->Options.DemoNoiseExp) = log10(NoiseSigma) ;
    Matrix Path = GuiMakeDemoPath(System, (App->DemoParams), NoiseSigma, (App->Options.DemoJitter)) ;

    char Name[256] ;
    if ((App->Options.DemoJitter) > 0.0) snprintf(Name, sizeof(Name), "Demo: %s (noise %.1e, clock jitter %.2f)", GetSystemName(System), NoiseSigma, (App->Options.DemoJitter));
    else snprintf(Name, sizeof(Name), "Demo: %s (noise %.1e)", GetSystemName(System), NoiseSigma);
    (App->IsDemo) = true ;
    GuiSetData(App, Path, Name);
    return;
}





//====================================================================================================
//Text for the information panel
//====================================================================================================

/// @brief Writes an equation like `dx/dt = 1.000 x - 0.500 x*y` into a text buffer.
/// @param Buffer Where to write.
/// @param BufferSize Size of the buffer.
/// @param LeftSide Text before the `=`, like `"dx/dt"`.
/// @param Library Address of the term library.
/// @param Coefficients One coefficient per term.
/// @param ZeroBelow Coefficients smaller than this (in absolute value) are not written.
void GuiFormatEquation (char* Buffer, size_t BufferSize, const char* LeftSide, TermLibrary* Library, double* Coefficients, double ZeroBelow)
{
    char Name[32] ;
    int PrintedCount = 0 ;
    snprintf(Buffer, BufferSize, "%s =", LeftSide);

    for (size_t ThisTerm=0; ThisTerm<(Library->TermCount); ThisTerm++)
    {
        double Value = Coefficients[ThisTerm] ;
        if (fabs(Value) < ZeroBelow) continue;

        GetTermName(&(Library->Terms[ThisTerm]), Name, sizeof(Name));
        size_t Used = strlen(Buffer) ;
        const char* Sign = (Value < 0.0) ? " -" : " +" ;
        if (PrintedCount == 0) Sign = (Value < 0.0) ? " -" : " " ;
        if (strcmp(Name, "1") == 0) snprintf(Buffer + Used, BufferSize - Used, "%s%s%.3f", Sign, (PrintedCount == 0) ? "" : " ", fabs(Value));
        else snprintf(Buffer + Used, BufferSize - Used, "%s%s%.3f %s", Sign, (PrintedCount == 0) ? "" : " ", fabs(Value), Name);
        PrintedCount++ ;
    }
    if (PrintedCount == 0) strncat(Buffer, " 0", BufferSize - strlen(Buffer) - 1);
    return;
}



/// @brief Describes in plain words what is on screen for one STLSQ snapshot.
/// @param Job Address of the finished job.
/// @param Snapshot Which snapshot is shown.
/// @param Threshold The threshold in use.
/// @param Buffer Where to write the sentence.
/// @param BufferSize Size of the buffer.
void GuiDescribeSnapshot (StlsqJob* Job, int Snapshot, double Threshold, char* Buffer, size_t BufferSize)
{
    size_t AliveCount = 0 ;
    size_t WeakCount = 0 ;
    for (size_t ThisTerm=0; ThisTerm<(Job->TermCount); ThisTerm++)
    {
        size_t Place = (size_t)Snapshot*(Job->TermCount) + ThisTerm ;
        if ((Job->ActiveHistory)[Place])
        {
            AliveCount++ ;
            if (fabs((Job->History)[Place]) < Threshold) WeakCount++ ;
        }
    }

    bool IsLast = ((size_t)Snapshot + 1 == (Job->SnapshotCount)) ;
    if (IsLast && (Job->IsDone) && (WeakCount == 0)) snprintf(Buffer, BufferSize, "Fit %d of %zu: %zu terms alive and none is below the threshold, so it stops here.", Snapshot+1, (Job->SnapshotCount), AliveCount);
    else if (WeakCount > 0) snprintf(Buffer, BufferSize, "Fit %d of %zu: %zu terms alive, %zu are below the threshold and will be removed next.", Snapshot+1, (Job->SnapshotCount), AliveCount, WeakCount);
    else snprintf(Buffer, BufferSize, "Fit %d of %zu: %zu terms alive.", Snapshot+1, (Job->SnapshotCount), AliveCount);
    return;
}





//====================================================================================================
//The panels
//====================================================================================================

/// @brief Draws the left panel (all the options) and reacts to them. The options can be scrolled with the mouse wheel when they do not all fit.
/// @param App Address of the app.
/// @param Box Where the panel is.
void GuiDrawLeftPanel (GuiApp* App, Rectangle Box)
{
    GuiDrawPanel(Box, "Options", GUI_SIDE_PANEL);
    GuiOptions* Options = &(App->Options) ;

    //Look of the app: theme, smaller text, bigger text (in the title row)
    float TitleY = Box.y + 8.0f ;
    float RightEdge = Box.x + Box.width - 12.0f ;
    if (GuiButton((Rectangle){ RightEdge - 34.0f, TitleY, 34.0f, 26.0f }, "A+", true, false)) (App->WantedFontSize) = GuiFontSize + 1 ;
    if (GuiButton((Rectangle){ RightEdge - 72.0f, TitleY, 34.0f, 26.0f }, "A-", true, false)) (App->WantedFontSize) = GuiFontSize - 1 ;
    if (GuiButton((Rectangle){ RightEdge - 150.0f, TitleY, 74.0f, 26.0f }, GuiThemeNames[GuiThemeIndex], true, false)) GuiThemeIndex = (GuiThemeIndex + 1) % GUI_THEME_COUNT ;

    //The scrolling area is everything between the title and the three buttons at the bottom
    float ButtonsTop = Box.y + Box.height - 12.0f - 3.0f*30.0f - 2.0f*6.0f ;
    Rectangle View = { Box.x + 1.0f, Box.y + 46.0f, Box.width - 2.0f, ButtonsTop - (Box.y + 46.0f) - 6.0f } ;
    float MaxScroll = (App->LeftContentHeight) - View.height ;
    if (MaxScroll < 0.0f) MaxScroll = 0.0f ;
    if (GuiMouseOver(Box) && (GuiNow.Wheel != 0.0f)) (App->LeftScroll) -= 40.0f * (GuiNow.Wheel) ;
    if ((App->LeftScroll) > MaxScroll) (App->LeftScroll) = MaxScroll ;
    if ((App->LeftScroll) < 0.0f) (App->LeftScroll) = 0.0f ;

    float X = Box.x + 14.0f ;
    float W = Box.width - 34.0f ;
    float Y = View.y + 4.0f - (App->LeftScroll) ;
    float ContentTop = Y ;
    GuiBeginClip(View);
    GuiClipBox = View ;
    GuiClipActive = true ;

    //Mode
    GuiDrawText(X, Y, 12, GUI_MUTED, "MODE");
    Y += 16.0f ;
    const char* ModeLabels[2] = { "Interpolated SINDy", "SINDy + filtered" } ;
    if (GuiChoice((Rectangle){ X, Y, W, 26.0f }, 2, ModeLabels, &(Options->Mode), 0u)) GuiMarkDirty(App, GUI_STAGE_TIMING);
    Y += 38.0f ;

    if ((Options->Mode) == 0)
    {
        //Timing
        GuiDrawText(X, Y, 12, GUI_MUTED, "TIMING (make the time even)");
        Y += 16.0f ;
        const char* InterpLabels[4] = { "Linear", "Hermite", "Spline", "PCHIP" } ;
        if (GuiChoice((Rectangle){ X, Y, W, 26.0f }, 4, InterpLabels, &(Options->InterpMethod), 0u)) GuiMarkDirty(App, GUI_STAGE_TIMING);
        Y += 34.0f ;
        if (GuiSlider(1, (Rectangle){ X, Y, W, 38.0f }, "Drift tolerance (steps)", &(Options->DriftTolerance), 0.0, 0.5, 2)) GuiMarkDirty(App, GUI_STAGE_TIMING);
        Y += 48.0f ;

        //Derivatives
        GuiDrawText(X, Y, 12, GUI_MUTED, "DERIVATIVES");
        Y += 16.0f ;
        const char* DerivLabels[2] = { "Finite differences", "Savitzky-Golay" } ;
        if (GuiChoice((Rectangle){ X, Y, W, 26.0f }, 2, DerivLabels, &(Options->DerivMethod), 0u)) GuiMarkDirty(App, GUI_STAGE_DERIVATIVES);
        Y += 34.0f ;
        if ((Options->DerivMethod) == 1)
        {
            if (GuiSlider(2, (Rectangle){ X, Y, W, 38.0f }, "Half window (points)", &(Options->SgHalfWindow), 3.0, 40.0, 0)) GuiMarkDirty(App, GUI_STAGE_DERIVATIVES);
            Y += 44.0f ;
            if (GuiSlider(3, (Rectangle){ X, Y, W, 38.0f }, "Polynomial order", &(Options->SgOrder), 2.0, 5.0, 0)) GuiMarkDirty(App, GUI_STAGE_DERIVATIVES);
            Y += 48.0f ;
        }
        else
        {
            Y += 4.0f ;
        }
    }
    else
    {
        //Filtered mode: the time stamps are used as they are, and a small network per chunk replaces the derivative estimate
        GuiDrawText(X, Y, 12, GUI_MUTED, "TIMING");
        Y += 16.0f ;
        Y = GuiDrawWrapped(X, Y, W, 14, GUI_TEXT, "Time spacing is ignored: the network fits any time stamps.") + 8.0f ;

        GuiDrawText(X, Y, 12, GUI_MUTED, "NEURAL FILTER (experimental)");
        Y += 16.0f ;
        if (GuiSlider(6, (Rectangle){ X, Y, W, 38.0f }, "Hidden units per network", &(Options->NeuralHidden), 4.0, 32.0, 0)) GuiMarkDirty(App, GUI_STAGE_DERIVATIVES);
        Y += 44.0f ;
        if (GuiSlider(7, (Rectangle){ X, Y, W, 38.0f }, "Training steps per chunk", &(Options->NeuralSteps), 200.0, 4000.0, 0)) GuiMarkDirty(App, GUI_STAGE_DERIVATIVES);
        Y += 44.0f ;
        if (GuiSlider(8, (Rectangle){ X, Y, W, 38.0f }, "Rows per chunk", &(Options->NeuralChunk), 60.0, 400.0, 0)) GuiMarkDirty(App, GUI_STAGE_DERIVATIVES);
        Y += 48.0f ;
    }

    //Library
    GuiDrawText(X, Y, 12, GUI_MUTED, "CANDIDATE TERMS");
    Y += 16.0f ;
    if (GuiSlider(4, (Rectangle){ X, Y, W, 38.0f }, "Highest polynomial degree", &(Options->MaxDegree), 1.0, 4.0, 0)) GuiMarkDirty(App, GUI_STAGE_LIBRARY);
    Y += 44.0f ;
    if (GuiToggle((Rectangle){ X, Y, W, 24.0f }, "Include sin and cos terms", &(Options->IncludeTrig))) GuiMarkDirty(App, GUI_STAGE_LIBRARY);
    Y += 34.0f ;

    //Sparse regression
    GuiDrawText(X, Y, 12, GUI_MUTED, "SPARSE REGRESSION (STLSQ)");
    Y += 16.0f ;
    if (GuiSlider(5, (Rectangle){ X, Y, W, 38.0f }, "Threshold", &(Options->Threshold), 0.005, 1.0, 3)) GuiMarkDirty(App, GUI_STAGE_SPARSE);
    Y += 46.0f ;

    //Demo data: noise and clock jitter can be changed live (the demo is rebuilt when the mouse button is released)
    if (App->IsDemo)
    {
        GuiDrawText(X, Y, 12, GUI_MUTED, "DEMO DATA");
        Y += 16.0f ;
        char NoiseText[32] ;
        snprintf(NoiseText, sizeof(NoiseText), "%.1e", pow(10.0, (Options->DemoNoiseExp)));
        if (GuiSliderCore(9, (Rectangle){ X, Y, W, 38.0f }, "Noise (sigma)", &(Options->DemoNoiseExp), -6.0, -1.0, 1, NoiseText)) (App->NoiseDirty) = true ;
        Y += 44.0f ;
        if (GuiSlider(10, (Rectangle){ X, Y, W, 38.0f }, "Clock jitter (steps)", &(Options->DemoJitter), 0.0, 0.4, 2)) (App->NoiseDirty) = true ;
        Y += 48.0f ;
    }

    EndScissorMode();
    GuiClipActive = false ;
    (App->LeftContentHeight) = (Y - ContentTop) + 8.0f ;

    //A thin scroll bar when there is more than fits
    if (MaxScroll > 0.0f)
    {
        float ThumbHeight = View.height * View.height / (App->LeftContentHeight) ;
        float ThumbY = View.y + (View.height - ThumbHeight) * (App->LeftScroll) / MaxScroll ;
        DrawRectangleRec((Rectangle){ Box.x + Box.width - 9.0f, ThumbY, 4.0f, ThumbHeight }, GUI_FRAME);
    }

    //Buttons, pinned to the bottom
    bool NeedsReprocess = ((App->DirtyFrom) < GUI_STAGE_SPARSE) ;
    if (GuiButton((Rectangle){ X, ButtonsTop, W + 8.0f, 30.0f }, NeedsReprocess ? "Reprocess (changes pending)" : "Reprocess", App->HasData, NeedsReprocess))
    {
        GuiMarkDirty(App, GUI_STAGE_TIMING);
        GuiStartProcessing(App);
    }
    if (GuiButton((Rectangle){ X, ButtonsTop + 36.0f, W + 8.0f, 30.0f }, "Load another file (or drop it here)", true, false))
    {
        (App->AskForFile) = true ;
    }
    if (GuiButton((Rectangle){ X, ButtonsTop + 72.0f, W + 8.0f, 30.0f }, "Save screenshot (PNG)", true, false))
    {
        (App->ShotRequested) = true ;
        snprintf((App->ShotFile), sizeof(App->ShotFile), "EPnC_Shot%02d.png", (App->ShotCount)+1);
    }
    return;
}



/// @brief Draws one horizontal bar chart: one bar per candidate term, for one snapshot of a STLSQ run.
/// @param Box Where the chart is.
/// @param Title Text above the chart (like `dx/dt`).
/// @param Job Address of the finished STLSQ job.
/// @param Library Address of the term library (for the names).
/// @param Snapshot Which snapshot to show (clamped to the last one).
/// @param Threshold The threshold in use (drawn as two dashed lines).
/// @attention Blue bar: alive and big enough. Orange bar: alive but below the threshold, removed next. Grey name, no bar: already removed in an earlier round.
void GuiDrawBars (Rectangle Box, const char* Title, StlsqJob* Job, TermLibrary* Library, int Snapshot, double Threshold)
{
    size_t TermCount = (Job->TermCount) ;
    if ((size_t)Snapshot >= (Job->SnapshotCount)) Snapshot = (int)(Job->SnapshotCount) - 1 ;

    GuiDrawText(Box.x, Box.y, 16, GUI_TEXT, Title);
    float NameWidth = 64.0f ;
    float RowTop = Box.y + 26.0f ;
    float RowHeight = (Box.height - 26.0f) / (float)TermCount ;
    if (RowHeight > 26.0f) RowHeight = 26.0f ;
    Rectangle BarArea = { Box.x + NameWidth, RowTop, Box.width - NameWidth - 44.0f, RowHeight*(float)TermCount } ;
    float ZeroX = BarArea.x + 0.5f*BarArea.width ;

    //Scale: the biggest alive coefficient fills the half width, but never zoom in beyond 3 times the threshold
    double BiggestValue = 3.0*Threshold ;
    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
    {
        size_t Place = (size_t)Snapshot*TermCount + ThisTerm ;
        if (((Job->ActiveHistory)[Place]) && (fabs((Job->History)[Place]) > BiggestValue)) BiggestValue = fabs((Job->History)[Place]) ;
    }
    double PixelsPerUnit = 0.5*(double)BarArea.width / (1.1*BiggestValue) ;

    DrawLineEx((Vector2){ ZeroX, BarArea.y }, (Vector2){ ZeroX, BarArea.y + BarArea.height }, 1.0f, GUI_FRAME);
    for (int ThisSide=-1; ThisSide<=1; ThisSide+=2)
    {
        float LineX = ZeroX + (float)ThisSide*(float)(Threshold*PixelsPerUnit) ;
        for (float DashY=BarArea.y; DashY<BarArea.y+BarArea.height; DashY+=8.0f)
        {
            DrawLineEx((Vector2){ LineX, DashY }, (Vector2){ LineX, DashY + 4.0f }, 1.0f, GUI_BAD);
        }
    }

    char Name[32] ;
    char ValueText[32] ;
    for (size_t ThisTerm=0; ThisTerm<TermCount; ThisTerm++)
    {
        size_t Place = (size_t)Snapshot*TermCount + ThisTerm ;
        bool IsAlive = (Job->ActiveHistory)[Place] ;
        double Value = (Job->History)[Place] ;
        float RowY = RowTop + RowHeight*(float)ThisTerm ;

        GetTermName(&(Library->Terms[ThisTerm]), Name, sizeof(Name));
        GuiDrawText(Box.x, RowY + 0.5f*RowHeight - 7.0f, 14, IsAlive ? GUI_TEXT : GUI_FRAME, Name);

        if (IsAlive)
        {
            bool IsWeak = (fabs(Value) < Threshold) ;
            float BarLength = (float)(Value*PixelsPerUnit) ;
            Rectangle Bar = { (BarLength >= 0.0f) ? ZeroX : ZeroX + BarLength, RowY + 0.2f*RowHeight, fabsf(BarLength), 0.6f*RowHeight } ;
            if (Bar.width < 1.5f) Bar.width = 1.5f ;
            DrawRectangleRec(Bar, IsWeak ? GUI_WARM : GUI_ACCENT);

            snprintf(ValueText, sizeof(ValueText), "%.3g", Value);
            if (BarLength >= 0.0f) GuiDrawText(Bar.x + Bar.width + 4.0f, RowY + 0.5f*RowHeight - 6.0f, 12, GUI_MUTED, ValueText);
            else GuiDrawTextRight(Bar.x - 4.0f, RowY + 0.5f*RowHeight - 6.0f, 12, GUI_MUTED, ValueText);
        }
    }
    return;
}



/// @brief Draws the live view of the neural fitting in place of the normal plots: the chunk being fitted (noisy dots and the network's curve), the slopes found so far, a progress bar and the skip button.
/// @param App Address of the app (needs `NeuralActive`).
/// @param Box Where the panel is.
void GuiDrawNeuralView (GuiApp* App, Rectangle Box)
{
    NeuralJob* Job = &(App->Job) ;
    Matrix* Path = &(App->EvenPath) ;
    GuiDrawPanel(Box, "The network is fitting the data", GUI_PANEL);

    //Upper plot: the chunk being fitted
    GuiPlotBox Chunk ;
    (Chunk.Box) = (Rectangle){ Box.x + 64.0f, Box.y + 76.0f, Box.width - 64.0f - 24.0f, 0.30f*Box.height } ;
    double LowX, HighX, LowY, HighY ;
    double FirstTime = GetMatrixCell(Path, (Job->ChunkStart), 0) ;
    double LastTime = GetMatrixCell(Path, (Job->ChunkStart)+(Job->ChunkSize)-1, 0) ;
    LowX = 1e300 ;
    HighX = -1e300 ;
    for (size_t ThisRow=0; ThisRow<(Job->ChunkSize); ThisRow++)
    {
        for (size_t ThisCol=1; ThisCol<=2; ThisCol++)
        {
            double Value = GetMatrixCell(Path, (Job->ChunkStart)+ThisRow, ThisCol) ;
            if (Value < LowX) LowX = Value ;
            if (Value > HighX) HighX = Value ;
        }
    }
    LowY = LowX - 0.08*(HighX - LowX) ;
    HighY = HighX + 0.08*(HighX - LowX) ;
    (Chunk.XMin) = FirstTime ;
    (Chunk.XMax) = LastTime ;
    (Chunk.YMin) = LowY ;
    (Chunk.YMax) = HighY ;
    GuiDrawAxes(&Chunk, "time", "the piece of data now being fitted");
    GuiBeginClip(Chunk.Box);
    for (size_t ThisRow=0; ThisRow<(Job->ChunkSize); ThisRow++)
    {
        double Time = GetMatrixCell(Path, (Job->ChunkStart)+ThisRow, 0) ;
        DrawCircleV(GuiToPixel(&Chunk, Time, GetMatrixCell(Path,(Job->ChunkStart)+ThisRow,1)), 2.2f, Fade(GUI_ACCENT, 0.6f));
        DrawCircleV(GuiToPixel(&Chunk, Time, GetMatrixCell(Path,(Job->ChunkStart)+ThisRow,2)), 2.2f, Fade(GUI_WARM, 0.6f));
    }
    Vector2 BeforeX = { 0.0f, 0.0f } ;
    Vector2 BeforeY = { 0.0f, 0.0f } ;
    for (int ThisPoint=0; ThisPoint<=120; ThisPoint++)
    {
        double Input = -1.0 + 2.0*(double)ThisPoint/120.0 ;
        double Time = (Job->MiddleTime) + Input*(Job->HalfSpan) ;
        Vector2 HereX = GuiToPixel(&Chunk, Time, (Job->MeanX) + (Job->StdX)*EvaluateCurve(&(Job->NetX), Input)) ;
        Vector2 HereY = GuiToPixel(&Chunk, Time, (Job->MeanY) + (Job->StdY)*EvaluateCurve(&(Job->NetY), Input)) ;
        if (ThisPoint > 0)
        {
            DrawLineEx(BeforeX, HereX, 3.0f, GUI_ACCENT);
            DrawLineEx(BeforeY, HereY, 3.0f, GUI_WARM);
        }
        BeforeX = HereX ;
        BeforeY = HereY ;
    }
    EndScissorMode();
    GuiDrawText(Chunk.Box.x + 10.0f, Chunk.Box.y + 6.0f, 14, GUI_MUTED, "dots: noisy measurements    lines: the network's curve (x and y)");

    //Lower plot: slopes of the whole path
    GuiPlotBox Slopes ;
    (Slopes.Box) = (Rectangle){ Box.x + 64.0f, Chunk.Box.y + Chunk.Box.height + 62.0f, Box.width - 64.0f - 24.0f, 0.28f*Box.height } ;
    GuiFindRange(Path, 0, 0, &(Slopes.XMin), &(Slopes.XMax));
    double LowDx, HighDx, LowDy, HighDy ;
    GuiFindRange(&(App->FdSlopes), 0, 5, &LowDx, &HighDx);
    GuiFindRange(&(App->FdSlopes), 1, 5, &LowDy, &HighDy);
    (Slopes.YMin) = (LowDx < LowDy) ? LowDx : LowDy ;
    (Slopes.YMax) = (HighDx > HighDy) ? HighDx : HighDy ;
    GuiDrawAxes(&Slopes, "time", "slope");
    GuiBeginClip(Slopes.Box);
    for (size_t ThisRow=0; ThisRow<(Path->RowCount); ThisRow+=2)
    {
        double Time = GetMatrixCell(Path, ThisRow, 0) ;
        DrawCircleV(GuiToPixel(&Slopes, Time, GetMatrixCell(&(App->FdSlopes),ThisRow,0)), 1.6f, Fade(GUI_ACCENT, 0.3f));
        DrawCircleV(GuiToPixel(&Slopes, Time, GetMatrixCell(&(App->FdSlopes),ThisRow,1)), 1.6f, Fade(GUI_WARM, 0.3f));
    }
    EndScissorMode();
    size_t DoneRows = (Job->KeepStart) ;
    if (DoneRows > (Path->RowCount)) DoneRows = (Path->RowCount) ;
    GuiDrawSeriesRange(&Slopes, Path, 0, &(Job->Derivatives), 0, 0, DoneRows, GUI_ACCENT, 2.5f);
    GuiDrawSeriesRange(&Slopes, Path, 0, &(Job->Derivatives), 1, 0, DoneRows, GUI_WARM, 2.5f);
    GuiBeginClip(Slopes.Box);
    for (size_t ThisRow=1; ThisRow<(Job->ChunkSize); ThisRow++)
    {
        double Time0 = GetMatrixCell(Path, (Job->ChunkStart)+ThisRow-1, 0) ;
        double Time1 = GetMatrixCell(Path, (Job->ChunkStart)+ThisRow, 0) ;
        double SlopeX0 = (Job->StdX)*EvaluateCurveSlope(&(Job->NetX), (Job->Inputs)[ThisRow-1])/(Job->HalfSpan) ;
        double SlopeX1 = (Job->StdX)*EvaluateCurveSlope(&(Job->NetX), (Job->Inputs)[ThisRow])/(Job->HalfSpan) ;
        double SlopeY0 = (Job->StdY)*EvaluateCurveSlope(&(Job->NetY), (Job->Inputs)[ThisRow-1])/(Job->HalfSpan) ;
        double SlopeY1 = (Job->StdY)*EvaluateCurveSlope(&(Job->NetY), (Job->Inputs)[ThisRow])/(Job->HalfSpan) ;
        DrawLineEx(GuiToPixel(&Slopes, Time0, SlopeX0), GuiToPixel(&Slopes, Time1, SlopeX1), 1.5f, GUI_TEXT);
        DrawLineEx(GuiToPixel(&Slopes, Time0, SlopeY0), GuiToPixel(&Slopes, Time1, SlopeY1), 1.5f, GUI_TEXT);
    }
    EndScissorMode();
    GuiDrawText(Slopes.Box.x + 10.0f, Slopes.Box.y + 6.0f, 14, GUI_MUTED, "faint dots: slopes from the raw noisy dots    thick: finished chunks    thin dark: chunk being trained");

    //Progress
    double Overall = ((double)(Job->ChunkIndex) + (double)(Job->StepsDone)/(double)(Job->StepsPerChunk)) / (double)(Job->ChunkCount) ;
    float BarY = Box.y + Box.height - 56.0f ;
    Rectangle Bar = { Box.x + 14.0f, BarY, Box.width - 28.0f - 150.0f, 14.0f } ;
    DrawRectangleRec(Bar, GUI_WIDGET);
    DrawRectangleRec((Rectangle){ Bar.x, Bar.y, Bar.width*(float)Overall, Bar.height }, GUI_ACCENT);
    char Line[200] ;
    snprintf(Line, sizeof(Line), "Chunk %zu of %zu    step %zu of %zu    misfit x %.4f, y %.4f", (Job->ChunkIndex)+1, (Job->ChunkCount), (Job->StepsDone), (Job->StepsPerChunk), (Job->LossX), (Job->LossY));
    GuiDrawText(Box.x + 14.0f, BarY + 20.0f, 14, GUI_MUTED, Line);
    if (GuiButton((Rectangle){ Box.x + Box.width - 14.0f - 136.0f, BarY - 6.0f, 136.0f, 30.0f }, "Skip animation", true, false)) (App->NeuralSkip) = true ;
    return;
}



/// @brief Draws the middle panel: a tabbed plot of the data on top, the STLSQ animation below.
/// @param App Address of the app.
/// @param Box Where the panel is.
void GuiDrawMiddlePanel (GuiApp* App, Rectangle Box)
{
    if (App->NeuralActive)
    {
        GuiDrawNeuralView(App, Box);
        return;
    }
    GuiDrawPanel(Box, "Plots", GUI_PANEL);

    //Tabs
    const char* TabLabels[3] = { "Data", "Phase portrait", "Slopes" } ;
    GuiChoice((Rectangle){ Box.x + Box.width - 14.0f - 330.0f, Box.y + 8.0f, 330.0f, 26.0f }, 3, TabLabels, &(App->PlotTab), 0u);

    //Upper plot
    float PlotHeight = 0.44f * (Box.height - 100.0f) ;
    GuiPlotBox Plot ;
    (Plot.Box) = (Rectangle){ Box.x + 64.0f, Box.y + 76.0f, Box.width - 64.0f - 24.0f, PlotHeight } ;
    (Plot.XMin) = 0.0 ;
    (Plot.XMax) = 1.0 ;
    (Plot.YMin) = 0.0 ;
    (Plot.YMax) = 1.0 ;

    bool Ready = (App->HasData) && (App->StageReady[GUI_STAGE_SPARSE]) ;
    if (Ready)
    {
        Matrix* Even = &(App->EvenPath) ;
        switch (App->PlotTab)
        {
            case 1:
                GuiFindRange(Even, 1, 0, &(Plot.XMin), &(Plot.XMax));
                GuiFindRange(Even, 2, 0, &(Plot.YMin), &(Plot.YMax));
                GuiDrawAxes(&Plot, "x", "y");
                GuiDrawSeries(&Plot, Even, 1, Even, 2, GUI_ACCENT, 2.0f);
                GuiDrawSimulation(App, &Plot, 1);
                break;

            case 2:
            {
                GuiFindRange(Even, 0, 0, &(Plot.XMin), &(Plot.XMax));
                double LowX, HighX, LowY, HighY ;
                GuiFindRange(&(App->Slopes), 0, 2, &LowX, &HighX);
                GuiFindRange(&(App->Slopes), 1, 2, &LowY, &HighY);
                (Plot.YMin) = (LowX < LowY) ? LowX : LowY ;
                (Plot.YMax) = (HighX > HighY) ? HighX : HighY ;
                GuiDrawAxes(&Plot, "time", "estimated slope");
                GuiDrawSeries(&Plot, Even, 0, &(App->Slopes), 0, GUI_ACCENT, 2.0f);
                GuiDrawSeries(&Plot, Even, 0, &(App->Slopes), 1, GUI_WARM, 2.0f);
                GuiDrawText(Plot.Box.x + 10.0f, Plot.Box.y + 6.0f, 14, GUI_ACCENT, "dx/dt");
                GuiDrawText(Plot.Box.x + 70.0f, Plot.Box.y + 6.0f, 14, GUI_WARM, "dy/dt");
                break;
            }

            default:
            {
                GuiFindRange(Even, 0, 0, &(Plot.XMin), &(Plot.XMax));
                double LowX, HighX, LowY, HighY ;
                GuiFindRange(Even, 1, 0, &LowX, &HighX);
                GuiFindRange(Even, 2, 0, &LowY, &HighY);
                (Plot.YMin) = (LowX < LowY) ? LowX : LowY ;
                (Plot.YMax) = (HighX > HighY) ? HighX : HighY ;
                GuiDrawAxes(&Plot, "time", "value");
                GuiDrawSeries(&Plot, Even, 0, Even, 1, GUI_ACCENT, 2.0f);
                GuiDrawSeries(&Plot, Even, 0, Even, 2, GUI_WARM, 2.0f);

                //Red dots where the interpolation had to invent a value (at most about 300 of them so the plot stays readable)
                size_t Stride = 1 + (App->InterpolatedCount) / 300 ;
                size_t Seen = 0 ;
                GuiBeginClip(Plot.Box);
                for (size_t ThisRow=0; ThisRow<(Even->RowCount); ThisRow++)
                {
                    if (!((App->WasInterpolated)[ThisRow])) continue;
                    if ((Seen++ % Stride) != 0) continue;
                    DrawCircleV(GuiToPixel(&Plot, GetMatrixCell(Even,ThisRow,0), GetMatrixCell(Even,ThisRow,1)), 2.5f, GUI_BAD);
                }
                EndScissorMode();
                GuiDrawText(Plot.Box.x + 10.0f, Plot.Box.y + 6.0f, 14, GUI_ACCENT, "x(t)");
                GuiDrawText(Plot.Box.x + 56.0f, Plot.Box.y + 6.0f, 14, GUI_WARM, "y(t)");
                if ((App->InterpolatedCount) > 0) GuiDrawText(Plot.Box.x + 104.0f, Plot.Box.y + 6.0f, 14, GUI_BAD, "red dots: values rebuilt by interpolation");
                GuiDrawSimulation(App, &Plot, 0);
                break;
            }
        }
    }
    else
    {
        GuiDrawAxes(&Plot, "", "");
        GuiDrawTextCentered(Plot.Box.x + 0.5f*Plot.Box.width, Plot.Box.y + 0.5f*Plot.Box.height - 8.0f, 16, GUI_MUTED, "Your data will appear here");
    }

    //Lower half: STLSQ animation
    float LowerTop = Plot.Box.y + Plot.Box.height + 52.0f ;
    GuiDrawText(Box.x + 14.0f, LowerTop - 8.0f, 18, GUI_TEXT, "Sparse regression, round by round");
    if (Ready)
    {
        int MostSnapshots = (int)(App->JobDx.SnapshotCount) ;
        if ((int)(App->JobDy.SnapshotCount) > MostSnapshots) MostSnapshots = (int)(App->JobDy.SnapshotCount) ;

        //Playback controls, right aligned
        float ControlsX = Box.x + Box.width - 14.0f - 4.0f*52.0f ;
        if (GuiButton((Rectangle){ ControlsX, LowerTop - 12.0f, 48.0f, 26.0f }, "|<", true, false)) { (App->AnimRound) = 0 ; (App->Playing) = false ; }
        if (GuiButton((Rectangle){ ControlsX + 52.0f, LowerTop - 12.0f, 48.0f, 26.0f }, "<", (App->AnimRound) > 0, false)) { (App->AnimRound)-- ; (App->Playing) = false ; }
        if (GuiButton((Rectangle){ ControlsX + 104.0f, LowerTop - 12.0f, 48.0f, 26.0f }, (App->Playing) ? "||" : "Play", true, true))
        {
            if (!(App->Playing) && ((App->AnimRound) >= MostSnapshots - 1)) (App->AnimRound) = 0 ;
            (App->Playing) = !(App->Playing) ;
            (App->AnimClock) = 0.0 ;
        }
        if (GuiButton((Rectangle){ ControlsX + 156.0f, LowerTop - 12.0f, 48.0f, 26.0f }, ">", (App->AnimRound) < MostSnapshots - 1, false)) { (App->AnimRound)++ ; (App->Playing) = false ; }

        float ChartHeight = Box.y + Box.height - LowerTop - 36.0f - 24.0f ;
        float ChartWidth = 0.5f*(Box.width - 28.0f - 24.0f) ;
        Rectangle LeftChart = { Box.x + 14.0f, LowerTop + 28.0f, ChartWidth, ChartHeight } ;
        Rectangle RightChart = { Box.x + 14.0f + ChartWidth + 24.0f, LowerTop + 28.0f, ChartWidth, ChartHeight } ;
        GuiDrawBars(LeftChart, "dx/dt", &(App->JobDx), &(App->Library), (App->AnimRound), (App->Options.Threshold));
        GuiDrawBars(RightChart, "dy/dt", &(App->JobDy), &(App->Library), (App->AnimRound), (App->Options.Threshold));

        char Caption[200] ;
        int Shown = ((size_t)(App->AnimRound) < (App->JobDx.SnapshotCount)) ? (App->AnimRound) : (int)(App->JobDx.SnapshotCount) - 1 ;
        GuiDescribeSnapshot(&(App->JobDx), Shown, (App->Options.Threshold), Caption, sizeof(Caption));
        GuiDrawText(Box.x + 14.0f, Box.y + Box.height - 28.0f, 14, GUI_MUTED, Caption);
        GuiDrawTextRight(Box.x + Box.width - 14.0f, Box.y + Box.height - 28.0f, 12, GUI_BAD, "dashed red: threshold");
    }
    else
    {
        GuiDrawTextCentered(Box.x + 0.5f*Box.width, LowerTop + 90.0f, 16, GUI_MUTED, "The candidate terms and their coefficients will appear here");
    }
    return;
}



/// @brief Draws the right panel: facts about the data, timing, speed, the discovered equations and what is happening now.
/// @param App Address of the app.
/// @param Box Where the panel is.
void GuiDrawRightPanel (GuiApp* App, Rectangle Box)
{
    GuiDrawPanel(Box, "Information", GUI_SIDE_PANEL);
    float X = Box.x + 14.0f ;
    float W = Box.width - 28.0f ;
    float Y = Box.y + 52.0f ;
    char Line[256] ;

    if (!(App->HasData))
    {
        GuiDrawWrapped(X, Y, W, 14, GUI_MUTED, "No data yet. Drop a .csv file with the columns t, x, y onto the window, or try one of the demos.");
        return;
    }

    GuiDrawText(X, Y, 12, GUI_MUTED, "DATA");
    Y += 18.0f ;
    Y = GuiDrawWrapped(X, Y, W, 14, GUI_TEXT, (App->SourceName)) ;
    snprintf(Line, sizeof(Line), "%zu rows, time %.4g to %.4g", (App->RawPath.RowCount), GetMatrixCell(&(App->RawPath),0,0), GetMatrixCell(&(App->RawPath),(App->RawPath.RowCount)-1,0));
    Y = GuiDrawWrapped(X, Y, W, 14, GUI_MUTED, Line) + 8.0f ;

    GuiDrawText(X, Y, 12, GUI_MUTED, "TIMING");
    Y += 18.0f ;
    if (!(App->TimeIsUsable))
    {
        Y = GuiDrawWrapped(X, Y, W, 14, GUI_BAD, "Time is not strictly increasing, so interpolation was skipped. Sort the file by time.") ;
    }
    else
    {
        snprintf(Line, sizeof(Line), "Ideal step %.5g (smallest %.5g, largest %.5g)", (App->Timing.IdealStep), (App->Timing.SmallestStep), (App->Timing.LargestStep));
        Y = GuiDrawWrapped(X, Y, W, 14, GUI_TEXT, Line) ;
        snprintf(Line, sizeof(Line), "Largest drift %.2f steps; %zu of %zu rows drifted more than %.2f", (App->Timing.LargestDrift), (App->Timing.DriftedCount), (App->Timing.PointCount), (App->Options.DriftTolerance));
        Y = GuiDrawWrapped(X, Y, W, 14, GUI_TEXT, Line) ;
        if ((App->Options.Mode) == 0)
        {
            snprintf(Line, sizeof(Line), "%zu rows were rebuilt by %s interpolation", (App->InterpolatedCount), GetInterpolationName((InterpolationMethod)(App->Options.InterpMethod)));
            Y = GuiDrawWrapped(X, Y, W, 14, ((App->InterpolatedCount) > 0) ? GUI_BAD : GUI_GOOD, Line) ;
        }
    }
    if ((App->Options.Mode) == 1)
    {
        Y = GuiDrawWrapped(X, Y + 4.0f, W, 14, GUI_MUTED, "Filtered mode (experimental): a small neural network is fitted to the noisy points and differentiated. Time stamps are used as they are.") ;
    }
    Y += 8.0f ;

    if (App->StageReady[GUI_STAGE_SPARSE])
    {
        GuiDrawText(X, Y, 12, GUI_MUTED, "DISCOVERED EQUATIONS");
        Y += 18.0f ;
        char Equation[512] ;
        GuiFormatEquation(Equation, sizeof(Equation), "dx/dt", &(App->Library), (App->JobDx.Coefficients), 1e-9);
        Y = GuiDrawWrapped(X, Y, W, 16, GUI_ACCENT, Equation) + 4.0f ;
        GuiFormatEquation(Equation, sizeof(Equation), "dy/dt", &(App->Library), (App->JobDy.Coefficients), 1e-9);
        Y = GuiDrawWrapped(X, Y, W, 16, GUI_WARM, Equation) + 8.0f ;

        if (App->HasSupport)
        {
            GuiDrawText(X, Y, 12, GUI_MUTED, "CHECK AGAINST THE TRUTH (demo only)");
            Y += 18.0f ;
            snprintf(Line, sizeof(Line), "True terms found: %d of %d", (App->Support.FoundTrueCount), (App->Support.TrueTermCount));
            Y = GuiDrawWrapped(X, Y, W, 14, ((App->Support.FoundTrueCount) == (App->Support.TrueTermCount)) ? GUI_GOOD : GUI_BAD, Line) ;
            snprintf(Line, sizeof(Line), "Extra terms that should not be there: %d", (App->Support.ExtraTermCount));
            Y = GuiDrawWrapped(X, Y, W, 14, ((App->Support.ExtraTermCount) == 0) ? GUI_GOOD : GUI_BAD, Line) ;
            snprintf(Line, sizeof(Line), "Biggest coefficient miss: %.4g", (App->Support.BiggestMiss));
            Y = GuiDrawWrapped(X, Y, W, 14, GUI_MUTED, Line) + 8.0f ;
        }

        GuiDrawText(X, Y, 12, GUI_MUTED, "SPEED (milliseconds per stage)");
        Y += 18.0f ;
        snprintf(Line, sizeof(Line), "timing %.1f, derivatives %.1f, library %.1f, regression %.1f", (App->StageMilliseconds[0]), (App->StageMilliseconds[1]), (App->StageMilliseconds[2]), (App->StageMilliseconds[3]));
        Y = GuiDrawWrapped(X, Y, W, 14, GUI_MUTED, Line) ;
    }

    if ((App->Message[0]) != '\0')
    {
        GuiDrawWrapped(X, Box.y + Box.height - 60.0f, W, 14, GUI_BAD, (App->Message));
    }
    return;
}



/// @brief Draws the popup that asks for a file (or a demo) and reacts to the demo buttons.
/// @param App Address of the app.
/// @param Screen The whole window as a rectangle.
void GuiDrawDropPopup (GuiApp* App, Rectangle Screen)
{
    DrawRectangleRec(Screen, (Color){ 20, 22, 28, 150 });
    Rectangle Box = { Screen.x + 0.5f*Screen.width - 290.0f, Screen.y + 0.5f*Screen.height - 170.0f, 580.0f, 340.0f } ;
    DrawRectangleRec(Box, GUI_PANEL);
    DrawRectangleLinesEx(Box, 2.0f, GUI_ACCENT);

    GuiDrawTextCentered(Box.x + 0.5f*Box.width, Box.y + 22.0f, 30, GUI_TEXT, "Drop a .csv file here");
    GuiDrawTextCentered(Box.x + 0.5f*Box.width, Box.y + 66.0f, 16, GUI_MUTED, "Drag it from your file manager onto this window.");
    GuiDrawTextCentered(Box.x + 0.5f*Box.width, Box.y + 88.0f, 16, GUI_MUTED, "Columns: t, x, y (a header line is fine, extra columns are ignored).");

    GuiDrawTextCentered(Box.x + 0.5f*Box.width, Box.y + 138.0f, 16, GUI_TEXT, "or try a demo with a little noise:");
    for (int ThisSystem=0; ThisSystem<SYSTEM_COUNT; ThisSystem++)
    {
        int Column = ThisSystem % 3 ;
        int Row = ThisSystem / 3 ;
        Rectangle Button = { Box.x + 24.0f + (float)Column*184.0f, Box.y + 168.0f + (float)Row*44.0f, 172.0f, 36.0f } ;
        if (GuiButton(Button, GetSystemName((SystemID)ThisSystem), true, false)) GuiLoadDemo(App, (SystemID)ThisSystem, 1e-4);
    }

    if ((App->Message[0]) != '\0') GuiDrawTextCentered(Box.x + 0.5f*Box.width, Box.y + 262.0f, 14, GUI_BAD, (App->Message));
    if ((App->HasData) && GuiButton((Rectangle){ Box.x + Box.width - 120.0f, Box.y + Box.height - 46.0f, 100.0f, 32.0f }, "Cancel", true, false)) (App->AskForFile) = false ;
    return;
}





//====================================================================================================
//The main loop
//====================================================================================================

/// @brief Draws a slightly see-through layer over the whole window while the pipeline works, so nothing can be edited meanwhile. Shows which stage is running and a small spinner.
/// @param App Address of the app.
/// @param Screen The whole window as a rectangle (logical pixels).
void GuiDrawProcessingOverlay (GuiApp* App, Rectangle Screen)
{
    DrawRectangleRec(Screen, (Color){ 20, 22, 28, 120 });
    Rectangle Box = { Screen.x + 0.5f*Screen.width - 170.0f, Screen.y + 0.5f*Screen.height - 50.0f, 340.0f, 100.0f } ;
    DrawRectangleRec(Box, GUI_PANEL);
    DrawRectangleLinesEx(Box, 2.0f, GUI_ACCENT);

    GuiDrawText(Box.x + 84.0f, Box.y + 20.0f, 22, GUI_TEXT, "Processing...");
    char Line[96] ;
    snprintf(Line, sizeof(Line), "%s (%d of %d)", GuiStageNames[(App->StageToRun)], (App->StageToRun)+1, GUI_STAGE_COUNT);
    GuiDrawText(Box.x + 84.0f, Box.y + 56.0f, 14, GUI_MUTED, Line);

    //Spinner: 12 dots in a circle, the bright one moves around
    int Lit = (int)(GetTime() * 12.0) % 12 ;
    for (int ThisDot=0; ThisDot<12; ThisDot++)
    {
        float Angle = (float)ThisDot * 0.5235988f ;
        float Brightness = 1.0f - 0.075f*(float)((Lit - ThisDot + 12) % 12) ;
        DrawCircleV((Vector2){ Box.x + 44.0f + 18.0f*sinf(Angle), Box.y + 50.0f - 18.0f*cosf(Angle) }, 3.4f, Fade(GUI_ACCENT, Brightness));
    }
    return;
}



/// @brief Runs the whole app until the window is closed.
/// @param argc Argument count from `main`.
/// @param argv Argument values from `main`. Optional: a CSV file path, or the switches below (used for automatic tests and slide screenshots).
/// @return `0`.
/// @attention Switches (all optional): `-h` prints this list. `--demo=N` loads built-in system N (0 to 4). `--thresh=V` sets the STLSQ threshold. `--degree=N` and `--trig=0|1` set the library. `--tab=N` picks the plot tab (0 to 2). `--round=N` shows STLSQ round N and pauses. `--font=N` sets the text size. `--theme=N` picks the theme (0 to 2). `--shot=File.png` saves a screenshot after `--frames=K` frames (default 10) and quits. With fewer than 8 frames the screenshot shows the processing overlay.
/// @warning Needs a window system. On a machine without a display (like a server) use a virtual display.
int GuiRun (int argc, char** argv)
{
    GuiApp App = GuiAppCreate() ;
    const char* CsvPath = NULL ;
    int DemoNumber = -1 ;
    int FixedRound = -1 ;
    int FrameLimit = 10 ;
    const char* ShotName = NULL ;
    (App.WantedFontSize) = DEFAULT_FONT_SIZE ;
    double DemoNoise = 1e-4 ;
    bool RunFirst = true ;

    for (int ThisArg=1; ThisArg<argc; ThisArg++)
    {
        if ((strcmp(argv[ThisArg],"-h") == 0) || (strcmp(argv[ThisArg],"--help") == 0))
        {
            printf("\nEPnC_App [file.csv] [--demo=N] [--mode=N] [--noise=S] [--jitter=F] [--thresh=V] [--degree=N] [--trig=0|1] [--tab=N] [--round=N] [--font=N] [--theme=N] [--shot=File.png] [--frames=K] [--async] (and more, see -h)");
            printf("\n  file.csv     load this file at start (columns t, x, y). Otherwise drop a file onto the window.");
            printf("\n  --demo=N     load built-in system N (0 to 4)");
            printf("\n  --thresh=V   STLSQ threshold, default 0.05");
            printf("\n  --degree=N   highest polynomial degree in the library (1 to 4), default 3");
            printf("\n  --trig=0|1   include sin and cos terms, default 1");
            printf("\n  --mode=N     0 interpolated SINDy, 1 SINDy + filtered (neural)");
            printf("\n  --hidden=N --nsteps=N --chunk=N   neural filter settings (units, steps per chunk, rows per chunk)");
            printf("\n  --noise=S    noise width of the demo (default 0.0001)   --jitter=F  clock jitter of the demo, in steps");
            printf("\n  --async      with --shot: do not finish the processing before the first frame (to catch the animation)");
            printf("\n  --tab=N      plot tab: 0 data, 1 phase portrait, 2 slopes");
            printf("\n  --round=N    show STLSQ round N and pause the animation");
            printf("\n  --font=N     text size in pixels (11 to 26), default is DEFAULT_FONT_SIZE in the code");
            printf("\n  --theme=N    0 light, 1 coffee, 2 dark");
            printf("\n  --shot=File  save a screenshot after K frames and quit (for slides and tests)");
            printf("\n  --frames=K   frames to wait before the screenshot, default 10 (under 8 you catch the processing overlay)");
            printf("\n");
            return 0;
        }
        else if (strncmp(argv[ThisArg],"--demo=",7) == 0) DemoNumber = atoi(argv[ThisArg]+7) ;
        else if (strncmp(argv[ThisArg],"--thresh=",9) == 0) (App.Options.Threshold) = atof(argv[ThisArg]+9) ;
        else if (strncmp(argv[ThisArg],"--degree=",9) == 0) (App.Options.MaxDegree) = atof(argv[ThisArg]+9) ;
        else if (strncmp(argv[ThisArg],"--trig=",7) == 0) (App.Options.IncludeTrig) = (atoi(argv[ThisArg]+7) != 0) ;
        else if (strncmp(argv[ThisArg],"--tab=",6) == 0) (App.PlotTab) = atoi(argv[ThisArg]+6) ;
        else if (strncmp(argv[ThisArg],"--round=",8) == 0) FixedRound = atoi(argv[ThisArg]+8) ;
        else if (strncmp(argv[ThisArg],"--font=",7) == 0) (App.WantedFontSize) = atoi(argv[ThisArg]+7) ;
        else if (strncmp(argv[ThisArg],"--theme=",8) == 0) GuiThemeIndex = atoi(argv[ThisArg]+8) % GUI_THEME_COUNT ;
        else if (strncmp(argv[ThisArg],"--mode=",7) == 0) (App.Options.Mode) = atoi(argv[ThisArg]+7) ;
        else if (strncmp(argv[ThisArg],"--hidden=",9) == 0) (App.Options.NeuralHidden) = atof(argv[ThisArg]+9) ;
        else if (strncmp(argv[ThisArg],"--nsteps=",9) == 0) (App.Options.NeuralSteps) = atof(argv[ThisArg]+9) ;
        else if (strncmp(argv[ThisArg],"--chunk=",8) == 0) (App.Options.NeuralChunk) = atof(argv[ThisArg]+8) ;
        else if (strncmp(argv[ThisArg],"--noise=",8) == 0) DemoNoise = atof(argv[ThisArg]+8) ;
        else if (strncmp(argv[ThisArg],"--jitter=",9) == 0) (App.Options.DemoJitter) = atof(argv[ThisArg]+9) ;
        else if (strcmp(argv[ThisArg],"--async") == 0) RunFirst = false ;
        else if (strncmp(argv[ThisArg],"--shot=",7) == 0) ShotName = argv[ThisArg]+7 ;
        else if (strncmp(argv[ThisArg],"--frames=",9) == 0) FrameLimit = atoi(argv[ThisArg]+9) ;
        else CsvPath = argv[ThisArg] ;
    }

    GuiOpenWindow(1280, 760, "EPnC: equation discovery from data");
    GuiSetFontSize(App.WantedFontSize);

    //Open as big as is comfortable for this screen (not when making a screenshot: those have a fixed size)
    if (ShotName == NULL)
    {
        int Monitor = GetCurrentMonitor() ;
        int MonitorWidth = GetMonitorWidth(Monitor) ;
        int MonitorHeight = GetMonitorHeight(Monitor) ;
        int WindowWidth = (int)(0.92*(double)MonitorWidth) ;
        int WindowHeight = (int)(0.90*(double)MonitorHeight) ;
        if (WindowWidth > 1900) WindowWidth = 1900 ;
        if (WindowHeight > 1100) WindowHeight = 1100 ;
        SetWindowSize(WindowWidth, WindowHeight);
        SetWindowPosition((MonitorWidth - WindowWidth)/2, (MonitorHeight - WindowHeight)/2);
    }

    if (CsvPath != NULL) GuiLoadFile(&App, CsvPath);
    else if ((DemoNumber >= 0) && (DemoNumber < SYSTEM_COUNT)) GuiLoadDemo(&App, (SystemID)DemoNumber, DemoNoise);
    if ((ShotName != NULL) && (FrameLimit >= 8) && RunFirst && (App.HasData)) GuiRunPipeline(&App);      //For screenshots with enough frames the work is finished before the first frame. With fewer frames you can catch the processing overlay.
    if ((FixedRound >= 0) && (App.HasData))
    {
        (App.AnimRound) = FixedRound ;
        (App.Playing) = false ;
    }

    int FrameNumber = 0 ;
    while (!WindowShouldClose())
    {
        if ((App.WantedFontSize) != GuiFontSize) GuiSetFontSize(App.WantedFontSize);
        GuiBeginFrame();

        //A file was dropped onto the window
        if (IsFileDropped())
        {
            FilePathList Files = LoadDroppedFiles() ;
            if (Files.count > 0) GuiLoadFile(&App, Files.paths[0]);
            UnloadDroppedFiles(Files);
        }

        //While processing, one stage runs per frame. The overlay drawn in the frame before is still on screen while it works.
        GuiStepProcessing(&App);

        //Advance the STLSQ animation by itself (one round per second) while it is playing
        if (App.HasData && App.StageReady[GUI_STAGE_SPARSE] && (App.Playing) && !(App.Processing))
        {
            int MostSnapshots = (int)(App.SimCount) ;
            (App.AnimClock) += (double)GetFrameTime() ;
            if ((App.AnimClock) > 1.0)
            {
                (App.AnimClock) = 0.0 ;
                if ((App.AnimRound) < MostSnapshots - 1) (App.AnimRound)++ ;
                else (App.Playing) = false ;
            }
        }

        BeginDrawing();
        ClearBackground(GUI_BACKGROUND);
        rlPushMatrix();
        rlScalef((float)GuiScale, (float)GuiScale, 1.0f);       //Everything below is drawn in logical pixels

        GuiLayout Layout = GuiMakeLayout() ;
        Rectangle Whole = { 0.0f, 0.0f, GuiScreenWidth(), GuiScreenHeight() } ;
        bool ShowPopup = !(App.HasData) || (App.AskForFile) ;
        bool Locked = ShowPopup || (App.Processing) ;
        (GuiNow.Blocked) = Locked ;                 //While a popup or the overlay is open, the panels behind it are only for looking at
        GuiDrawLeftPanel(&App, Layout.Left);
        (GuiNow.Blocked) = ShowPopup || ((App.Processing) && !(App.NeuralActive)) ;     //The live neural view has its own Skip button
        GuiDrawMiddlePanel(&App, Layout.Middle);
        (GuiNow.Blocked) = Locked ;
        GuiDrawRightPanel(&App, Layout.Right);
        if (ShowPopup)
        {
            (GuiNow.Blocked) = (App.Processing) ;
            GuiDrawDropPopup(&App, Whole);
        }
        if (App.Processing)
        {
            if (App.NeuralActive)       //Only the two side panels are locked, the middle shows the networks learning
            {
                DrawRectangleRec(Layout.Left, (Color){ 20, 22, 28, 110 });
                DrawRectangleRec(Layout.Right, (Color){ 20, 22, 28, 110 });
            }
            else GuiDrawProcessingOverlay(&App, Whole);
        }

        //Demo data sliders: rebuild the demo when the mouse button is released
        if ((App.NoiseDirty) && !(GuiNow.Held) && !(App.Processing) && (App.IsDemo))
        {
            (App.NoiseDirty) = false ;
            GuiLoadDemo(&App, (App.DemoSystem), pow(10.0, (App.Options.DemoNoiseExp)));
        }

        //Slider moves of the cheap stage take effect right away. The others wait for the Reprocess button.
        if ((App.DirtyFrom) == GUI_STAGE_SPARSE && !(App.Processing)) GuiRunPipeline(&App);

        rlPopMatrix();
        if (App.ShotRequested)
        {
            rlDrawRenderBatchActive();      //Raylib draws in batches: send the last batch to the screen first, or the picture misses everything after the last scissor
            Image Shot = LoadImageFromScreen() ;
            ExportImage(Shot, (App.ShotFile));
            UnloadImage(Shot);
            (App.ShotRequested) = false ;
            (App.ShotCount)++ ;
        }
        EndDrawing();

        FrameNumber++ ;
        if ((ShotName != NULL) && (FrameNumber == FrameLimit))
        {
            (App.ShotRequested) = true ;
            snprintf((App.ShotFile), sizeof(App.ShotFile), "%s", ShotName);
        }
        else if ((ShotName != NULL) && (FrameNumber > FrameLimit)) break;
    }

    if (App.HasData)
    {
        GuiFreeResultsFrom(&App, GUI_STAGE_TIMING);
        MatrixDestroy(&(App.RawPath));
    }
    GuiCloseWindow();
    return 0;
}

#endif
