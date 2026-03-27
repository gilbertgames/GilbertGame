#include "GilbertShadersModule.h"

#include "GilbertShadersViewExtension.h"

#include "Misc/CoreDelegates.h"
#include "Interfaces/IPluginManager.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include "SceneViewExtension.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogGilbertShaders, Log, All);

void FGilbertShadersModule::StartupModule()
{
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("GilbertShaders"));
    if (Plugin.IsValid())
    {
        const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
        AddShaderSourceDirectoryMapping(TEXT("/GilbertShaders"), ShaderDirectory);

        UE_LOG(LogGilbertShaders, Warning, TEXT("GilbertShaders: Shader directory mapped: %s"), *ShaderDirectory);
    }
    else
    {
        UE_LOG(LogGilbertShaders, Error, TEXT("GilbertShaders: Failed to find plugin."));
    }

    if (GIsEditor || IsRunningGame())
    {
        PostEngineInitHandle = FCoreDelegates::OnPostEngineInit.AddRaw(this, &FGilbertShadersModule::CreateViewExtension);
        UE_LOG(LogGilbertShaders, Warning, TEXT("GilbertShaders: Registered OnPostEngineInit callback for view extension creation."));
    }
}

void FGilbertShadersModule::CreateViewExtension()
{
    if (!ViewExtension.IsValid())
    {
        ViewExtension = FSceneViewExtensions::NewExtension<FGilbertShadersViewExtension>();

        if (ViewExtension.IsValid())
        {
            UE_LOG(LogGilbertShaders, Warning, TEXT("GilbertShaders: View extension created successfully."));
        }
        else
        {
            UE_LOG(LogGilbertShaders, Error, TEXT("GilbertShaders: Failed to create view extension."));
        }
    }
}

void FGilbertShadersModule::ShutdownModule()
{
    UE_LOG(LogGilbertShaders, Warning, TEXT("GilbertShaders: ShutdownModule"));

    if (PostEngineInitHandle.IsValid())
    {
        FCoreDelegates::OnPostEngineInit.Remove(PostEngineInitHandle);
        PostEngineInitHandle.Reset();
    }

    ViewExtension.Reset();
}

IMPLEMENT_MODULE(FGilbertShadersModule, GilbertShaders)