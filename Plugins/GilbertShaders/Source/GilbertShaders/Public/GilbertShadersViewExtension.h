#pragma once

#include "SceneViewExtension.h"

class FGilbertShadersViewExtension : public FSceneViewExtensionBase
{
public:
    FGilbertShadersViewExtension(const FAutoRegister& AutoRegister);

    virtual void SubscribeToPostProcessingPass(
        EPostProcessingPass Pass,
        const FSceneView& View,
        FAfterPassCallbackDelegateArray& InOutPassCallbacks,
        bool bIsPassEnabled) override;

    FScreenPassTexture PostProcessPass_RenderThread(
        FRDGBuilder& GraphBuilder,
        const FSceneView& View,
        const FPostProcessMaterialInputs& Inputs);

private:
    FScreenPassTexture AddGilbertPainterlyPass(
        FRDGBuilder& GraphBuilder,
        const FSceneView& View,
        const FPostProcessMaterialInputs& Inputs,
        const FScreenPassTexture& InputSceneColor);
};