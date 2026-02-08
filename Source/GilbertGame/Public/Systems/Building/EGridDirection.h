// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EGridDirection.generated.h"

UENUM(BlueprintType)
enum class EGridDirection : uint8
{
    None     UMETA(DisplayName = "None"),
    PosX     UMETA(DisplayName = "PosX"),
    NegX    UMETA(DisplayName = "NegX"),
    PosY  UMETA(DisplayName = "PosY"),
    NegY    UMETA(DisplayName = "NegY")
};
