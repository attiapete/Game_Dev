// Copyright 2021, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Actors/FirearmParts/SKGPart.h"
#include "SKGHandguard.generated.h"

class ASKGForwardGrip;
class ASKGFirearm;
class AFPSTemplateFirearm_Sight;
class UAnimSequence;

UCLASS()
class ULTIMATEFPSFRAMEWORK_API ASKGHandguard : public ASKGPart
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASKGHandguard();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|LeftHandIK")
	FSKGLeftHandIKData LeftHandIKData;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	FTransform GetGripTransform() const;

	const FSKGLeftHandIKData& GetLeftHandIKData();
};