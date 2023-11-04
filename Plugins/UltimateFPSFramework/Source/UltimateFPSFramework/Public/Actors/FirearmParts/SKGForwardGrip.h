// Copyright 2021, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Actors/FirearmParts/SKGPart.h"
#include "SKGForwardGrip.generated.h"

class UAnimSequence;

UCLASS()
class ULTIMATEFPSFRAMEWORK_API ASKGForwardGrip : public ASKGPart
{
	GENERATED_BODY()
public:
	ASKGForwardGrip();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|LeftHandIK")
	FSKGLeftHandIKData LeftHandIKData;

	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Animation")
	FTransform GetGripTransform() const;

	const FSKGLeftHandIKData& GetLeftHandIKData();
};