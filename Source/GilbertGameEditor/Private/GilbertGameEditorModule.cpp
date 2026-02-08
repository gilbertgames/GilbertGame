#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#include "BuildingActorDetails.h"
#include "Systems/Building/BuildingActor.h"

class FGilbertGameEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FPropertyEditorModule& PropertyEditorModule =
            FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyEditorModule.RegisterCustomClassLayout(
            ABuildingActor::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateStatic(&FBuildingActorDetails::MakeInstance)
        );

        PropertyEditorModule.NotifyCustomizationModuleChanged();
    }

    virtual void ShutdownModule() override
    {
        if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
        {
            FPropertyEditorModule& PropertyEditorModule =
                FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

            PropertyEditorModule.UnregisterCustomClassLayout(ABuildingActor::StaticClass()->GetFName());
            PropertyEditorModule.NotifyCustomizationModuleChanged();
        }
    }
};

IMPLEMENT_MODULE(FGilbertGameEditorModule, GilbertGameEditor)
