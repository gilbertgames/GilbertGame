#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGilbertShadersViewExtension;

class FGilbertShadersModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void CreateViewExtension();

private:
    TSharedPtr<FGilbertShadersViewExtension, ESPMode::ThreadSafe> ViewExtension;
    FDelegateHandle PostEngineInitHandle;
};