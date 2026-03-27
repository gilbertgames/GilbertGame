#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ScreenPass.h"

class FGilbertFullscreenVS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FGilbertFullscreenVS);
    SHADER_USE_PARAMETER_STRUCT(FGilbertFullscreenVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FGilbertFullscreenPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FGilbertFullscreenPS);
    SHADER_USE_PARAMETER_STRUCT(FGilbertFullscreenPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};