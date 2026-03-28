#include "GilbertShadersViewExtension.h"

#include "GilbertPainterlyShader.h"

#include "Logging/LogMacros.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScreenPass.h"
#include "GlobalShader.h"
#include "SceneRenderTargetParameters.h"



DEFINE_LOG_CATEGORY_STATIC(LogGilbertShadersViewExt, Log, All);

#define GILBERT_TEST_PASS EPostProcessingPass::AfterDOF


FGilbertShadersViewExtension::FGilbertShadersViewExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
    UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: ViewExtension constructed"));
}

void FGilbertShadersViewExtension::SubscribeToPostProcessingPass(
    EPostProcessingPass Pass,
    const FSceneView& View,
    FAfterPassCallbackDelegateArray& InOutPassCallbacks,
    bool bIsPassEnabled)
{
    if (Pass == GILBERT_TEST_PASS)
    {
        UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: Subscribing to test post-process pass"));

        InOutPassCallbacks.Add(
            FAfterPassCallbackDelegate::CreateRaw(
                this,
                &FGilbertShadersViewExtension::PostProcessPass_RenderThread));
    }
}

FScreenPassTexture FGilbertShadersViewExtension::PostProcessPass_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs)
{
    UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: PostProcessPass_RenderThread hit"));

    const FScreenPassTexture SceneColor(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

    if (!SceneColor.IsValid())
    {
        UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: SceneColor invalid"));
        return SceneColor;
    }

    UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: SceneColor valid, adding painterly pass"));
    return AddGilbertPainterlyPass(GraphBuilder, View, Inputs, SceneColor);
}

FScreenPassTexture FGilbertShadersViewExtension::AddGilbertPainterlyPass(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs,
    const FScreenPassTexture& InputSceneColor)
{
    FScreenPassRenderTarget Output = FScreenPassRenderTarget::CreateFromInput(
        GraphBuilder,
        InputSceneColor,
        View.GetOverwriteLoadAction(),
        TEXT("GilbertShaders.Output"));

    const FScreenPassTextureViewport InputViewport(InputSceneColor);


    FGilbertPainterlyPS::FParameters* PassParameters =
        GraphBuilder.AllocParameters<FGilbertPainterlyPS::FParameters>();

    PassParameters->InputViewport = GetScreenPassTextureViewportParameters(InputViewport);
    PassParameters->View = View.ViewUniformBuffer;
    PassParameters->SceneTextures = CreateSceneTextureShaderParameters(
        GraphBuilder,
        View,
        ESceneTextureSetupMode::All);

    PassParameters->SceneColorTexture = InputSceneColor.Texture;
    PassParameters->SceneColorSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();

    PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

    PassParameters->EdgeStrength = 4.0f;
    PassParameters->EdgeThreshold = 0.08f;
    PassParameters->EdgeSoftness = 0.04f;

    PassParameters->NormalEdgeStrength = 1.0f;
    PassParameters->NormalEdgeThreshold = 0.75f;
    PassParameters->NormalEdgeSoftness = 0.12f;



    const FScreenPassTextureViewport OutputViewport(Output);

    TShaderMapRef<FScreenPassVS> VertexShader(GetGlobalShaderMap(View.GetFeatureLevel()));
    TShaderMapRef<FGilbertPainterlyPS> PixelShader(GetGlobalShaderMap(View.GetFeatureLevel()));

    AddDrawScreenPass(
        GraphBuilder,
        RDG_EVENT_NAME("GilbertShaders_Painterly"),
        View,
        OutputViewport,
        InputViewport,
        VertexShader,
        PixelShader,
        PassParameters);

    return MoveTemp(Output);
}