#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ScreenPass.h"
#include "SceneTexturesConfig.h"

class FGilbertPainterlyPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FGilbertPainterlyPS);
    SHADER_USE_PARAMETER_STRUCT(FGilbertPainterlyPS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)

        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_STRUCT_INCLUDE(FSceneTextureShaderParameters, SceneTextures)

        SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, InputViewport)

        SHADER_PARAMETER(float, EdgeStrength)
        SHADER_PARAMETER(float, EdgeThreshold)
        SHADER_PARAMETER(float, EdgeSoftness)

        SHADER_PARAMETER(float, NormalEdgeStrength)
        SHADER_PARAMETER(float, NormalEdgeThreshold)
        SHADER_PARAMETER(float, NormalEdgeSoftness)


        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
};