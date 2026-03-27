#include "GilbertFullscreenShader.h"

IMPLEMENT_GLOBAL_SHADER(FGilbertFullscreenVS, "/GilbertShaders/Private/GilbertFullscreen.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FGilbertFullscreenPS, "/GilbertShaders/Private/GilbertFullscreen.usf", "MainPS", SF_Pixel);