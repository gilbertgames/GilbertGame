#include "BuildingActorDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyHandle.h"

// Include your actor header
#include "Systems/Building/BuildingActor.h"

TSharedRef<IDetailCustomization> FBuildingActorDetails::MakeInstance()
{
    return MakeShared<FBuildingActorDetails>();
}

void FBuildingActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // --- 1) Hide categories you don't want ---
    //DetailBuilder.HideCategory(TEXT("StaticMesh"));
    //DetailBuilder.HideCategory(TEXT("Materials"));
    //DetailBuilder.HideCategory(TEXT("Tags"));
    DetailBuilder.HideCategory(TEXT("Collision"));
    DetailBuilder.HideCategory(TEXT("Mesh Painting"));
    DetailBuilder.HideCategory(TEXT("Lighting"));
    DetailBuilder.HideCategory(TEXT("Material Cache"));
    DetailBuilder.HideCategory(TEXT("Rendering"));
    DetailBuilder.HideCategory(TEXT("HLOD"));
    DetailBuilder.HideCategory(TEXT("VirtualTexture"));
    DetailBuilder.HideCategory(TEXT("Cooking"));
    DetailBuilder.HideCategory(TEXT("Navigation"));
    DetailBuilder.HideCategory(TEXT("Replication"));
    DetailBuilder.HideCategory(TEXT("Networking"));
    DetailBuilder.HideCategory(TEXT("Physics"));
    DetailBuilder.HideCategory(TEXT("Input"));
    DetailBuilder.HideCategory(TEXT("Actor"));
    DetailBuilder.HideCategory(TEXT("LOD"));
    DetailBuilder.HideCategory(TEXT("RayTracing"));
    DetailBuilder.HideCategory(TEXT("TextureStreaming"));
    DetailBuilder.HideCategory(TEXT("MaterialParameters"));
    DetailBuilder.HideCategory(TEXT("Mobile"));
    DetailBuilder.HideCategory(TEXT("AssetUserData"));
    DetailBuilder.HideCategory(TEXT("WorldPartition"));
    DetailBuilder.HideCategory(TEXT("LevelInstance"));
    DetailBuilder.HideCategory(TEXT("DataLayers"));


    // Add more as needed

    // --- 2) Create a "high priority" category at the top ---
    IDetailCategoryBuilder& TopCat = DetailBuilder.EditCategory(
        TEXT("Building"),
        FText::FromString(TEXT("Building")),
        ECategoryPriority::Important
    );

    // Example: move properties into the top category
    // (these names must match your UPROPERTY names)
    const TSharedPtr<IPropertyHandle> bMirrored  = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ABuildingActor, bMirrored));

    if (bMirrored && bMirrored->IsValidHandle())  TopCat.AddProperty(bMirrored);

    // --- 3) Optionally hide the original properties from their original category ---
    // If they were in "Default" / another category, they might still show there.
    // One approach: hide that whole original category, or ensure those UPROPERTY Category=... is consistent.
}
