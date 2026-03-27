#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ScreenPass.h"

class FGilbertPainterlyVS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FGilbertPainterlyVS);
    SHADER_USE_PARAMETER_STRUCT(FGilbertPainterlyVS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
    END_SHADER_PARAMETER_STRUCT()
};

class FGilbertPainterlyPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FGilbertPainterlyPS);
    SHADER_USE_PARAMETER_STRUCT(FGilbertPainterlyPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};