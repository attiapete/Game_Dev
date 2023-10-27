// Copyright 2021, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "DataTypes/SKGFPSDataTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "SKGPhysicalMaterial.generated.h"

class USoundBase;

UCLASS(Blueprintable, BlueprintType)
class ULTIMATEFPSFRAMEWORK_API USKGPhysicalMaterial : public UPhysicalMaterial, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
public:
	USKGPhysicalMaterial();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Default", meta = (Categories = "PhysMat"))
	FGameplayTag GameplayTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FSKGImpactEffects ProjectileImpactEffects;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
	FSKGImpactEffects FootstepImpactEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	FSKGImpactEffects GrenadeImpactEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Other")
	FSKGImpactEffects ImpactEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Other")
	USoundBase* EmptyCaseImpactSound;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTag.GetSingleTagContainer(); }
};
