// Copyright 2022, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SKGProjectileWorldSubsystem.generated.h"

class AWindDirectionalSource;

UCLASS()
class SKGPROJECTILE_API USKGProjectileWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	USKGProjectileWorldSubsystem();

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void FindAndSetWindSource();

	TObjectPtr<AWindDirectionalSource> WindSource;

public:
	AWindDirectionalSource* GetWindSource() const { return WindSource; }
};
