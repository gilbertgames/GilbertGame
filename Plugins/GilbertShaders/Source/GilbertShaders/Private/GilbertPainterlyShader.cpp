#include "GilbertPainterlyShader.h"

IMPLEMENT_GLOBAL_SHADER(FGilbertPainterlyVS, "/GilbertShaders/Private/GilbertPainterly.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FGilbertPainterlyPS, "/GilbertShaders/Private/GilbertPainterly.usf", "MainPS", SF_Pixel);