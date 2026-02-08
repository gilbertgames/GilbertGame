// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EGridSlot.generated.h"

UENUM(BlueprintType)
enum class EGridSlot : uint8
{
    None     UMETA(DisplayName = "None"),
    Wall     UMETA(DisplayName = "Wall"),
    Floor    UMETA(DisplayName = "Floor"),
    Ceiling  UMETA(DisplayName = "Ceiling"),
	Roof    UMETA(DisplayName = "Roof")
};
