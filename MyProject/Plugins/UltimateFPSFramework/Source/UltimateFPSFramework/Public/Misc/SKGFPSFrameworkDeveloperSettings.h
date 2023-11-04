// Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SKGFPSFrameworkDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, DefaultConfig, meta = (DisplayName = "SKG FPSFramework Editor Settings"))
class ULTIMATEFPSFRAMEWORK_API USKGFPSFrameworkDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	USKGFPSFrameworkDeveloperSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "ComponentTags")
	FName FirearmMeshTag = "SKGFirearm";
};
