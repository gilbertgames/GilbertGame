#include "GilbertShadersViewExtension.h"

#include "GilbertPainterlyShader.h"

#include "Logging/LogMacros.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScreenPass.h"
#include "GlobalShader.h"

DEFINE_LOG_CATEGORY_STATIC(LogGilbertShadersViewExt, Log, All);

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
    if (Pass == EPostProcessingPass::Tonemap)
    {
        UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: Subscribing to Tonemap pass"));

        InOutPassCallbacks.Add(
            FAfterPassCallbackDelegate::CreateRaw(
                this,
                &FGilbertShadersViewExtension::AfterTonemap_RenderThread));
    }
}

FScreenPassTexture FGilbertShadersViewExtension::AfterTonemap_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessMaterialInputs& Inputs)
{
    UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: AfterTonemap_RenderThread hit"));

    const FScreenPassTexture SceneColor(Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

    if (!SceneColor.IsValid())
    {
        UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: SceneColor invalid"));
        return SceneColor;
    }

    UE_LOG(LogGilbertShadersViewExt, Warning, TEXT("GilbertShaders: SceneColor valid, adding painterly pass"));
    return AddGilbertPainterlyPass(GraphBuilder, View, SceneColor);
}

FScreenPassTexture FGilbertShadersViewExtension::AddGilbertPainterlyPass(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FScreenPassTexture& InputSceneColor)
{
    FScreenPassRenderTarget Output = FScreenPassRenderTarget::CreateFromInput(
        GraphBuilder,
        InputSceneColor,
        View.GetOverwriteLoadAction(),
        TEXT("GilbertShaders.Output"));

    FGilbertPainterlyPS::FParameters* PassParameters =
        GraphBuilder.AllocParameters<FGilbertPainterlyPS::FParameters>();

    PassParameters->InputTexture = InputSceneColor.Texture;
    PassParameters->InputSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
    PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

    const FScreenPassTextureViewport InputViewport(InputSceneColor);
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