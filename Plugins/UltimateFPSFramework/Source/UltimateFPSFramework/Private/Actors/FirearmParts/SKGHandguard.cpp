// Copyright 2021, Dakota Dawe, All rights reserved


#include "Actors/FirearmParts/SKGHandguard.h"
#include "Actors/SKGFirearm.h"
#include "Actors/FirearmParts/SKGForwardGrip.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ASKGHandguard::ASKGHandguard()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PartStats.Weight = 1.0f;
	PartStats.ErgonomicsChangePercentage = 5.0f;

	PartType = ESKGPartType::Handguard;

	bBeCheckedForOverlap = false;
}

// Called when the game starts or when spawned
void ASKGHandguard::BeginPlay()
{
	Super::BeginPlay();
}

FTransform ASKGHandguard::GetGripTransform() const
{
	return AttachmentMesh.IsValid() ? AttachmentMesh->GetSocketTransform(LeftHandIKData.HandGripSocket) : FTransform();
}

const FSKGLeftHandIKData& ASKGHandguard::GetLeftHandIKData()
{
	LeftHandIKData.Transform = GetGripTransform();
	return LeftHandIKData;
}
