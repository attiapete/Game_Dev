// Copyright 2021, Dakota Dawe, All rights reserved


#include "Actors/FirearmParts/SKGForwardGrip.h"
#include "Actors/FirearmParts/SKGHandguard.h"
#include "Actors/SKGFirearm.h"

ASKGForwardGrip::ASKGForwardGrip()
{
	PartStats.Weight = 0.06f;
	PartStats.ErgonomicsChangePercentage = 10.0f;
	PartStats.VerticalRecoilChangePercentage = -8.0f;
	PartStats.HorizontalRecoilChangePercentage = -8.0f;
	
	PartType = ESKGPartType::ForwardGrip;
}

void ASKGForwardGrip::BeginPlay()
{
	Super::BeginPlay();
	
}

FTransform ASKGForwardGrip::GetGripTransform() const
{
	return AttachmentMesh.IsValid() ? AttachmentMesh->GetSocketTransform(LeftHandIKData.HandGripSocket) : FTransform();
}

const FSKGLeftHandIKData& ASKGForwardGrip::GetLeftHandIKData()
{
	LeftHandIKData.Transform = GetGripTransform();
	return LeftHandIKData;
}
