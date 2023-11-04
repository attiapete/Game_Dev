// Copyright 2021, Dakota Dawe, All rights reserved


#include "Actors/FirearmParts/SKGPart.h"
#include "Actors/SKGFirearm.h"
#include "Components/SKGCharacterComponent.h"
#include "Components/ShapeComponent.h"

#include "Net/UnrealNetwork.h"


// Sets default values
ASKGPart::ASKGPart()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	FirearmCollisionChannel = ECC_GameTraceChannel2;
	
	PartStats.Weight = 0.22f;
	PartStats.ErgonomicsChangePercentage = 0.0f;
	PartStats.VerticalRecoilChangePercentage = 0.0f;
	PartStats.HorizontalRecoilChangePercentage = 0.0f;

	PartType = ESKGPartType::Other;
}

void ASKGPart::PostInitProperties()
{
	Super::PostInitProperties();
	PartData.PartClass = GetClass();
}

void ASKGPart::Destroyed()
{
	if (IsValid(GetOwningFirearm()) && OwningFirearm->Implements<USKGFirearmPartsInterface>())
	{
		if (this == ISKGFirearmPartsInterface::Execute_GetCurrentSightActor(OwningFirearm))
		{
			ISKGFirearmInterface::Execute_CycleSights(OwningFirearm.Get(), true, false);
		}
	}
	Super::Destroyed();
}

bool ASKGPart::Server_Use_Validate()
{
	return true;
}

void ASKGPart::Server_Use_Implementation()
{
}

void ASKGPart::CacheCharacterAndFirearm()
{
	GetOwningFirearm();
	ISKGFirearmAttachmentsInterface::Execute_GetOwningCharacterComponent(this);
}

FSKGFirearmPartStats ASKGPart::GetPartStats_Implementation(bool bIncludeChildren)
{
	FSKGFirearmPartStats ReturnPartStats = PartStats;
	if (bIncludeChildren)
	{
		for (const USKGAttachmentComponent* PartComponent : AttachmentComponents)
		{
			if (IsValid(PartComponent))
			{
				ASKGPart* Part = PartComponent->GetAttachment<ASKGPart>();
				if (IsValid(Part))
				{
					ReturnPartStats += ISKGFirearmAttachmentsInterface::Execute_GetPartStats(Part, bIncludeChildren);
				}
			}
		}
	}
	return ReturnPartStats;
}

AActor* ASKGPart::GetOwningActor_Implementation()
{
	if (OwningActor)
	{
		return OwningActor.Get();
	}
	AActor* PartOwner = GetOwner();
	for (uint8 i = 0; i < MAX_PartStack; ++i)
	{
		if (IsValid(PartOwner))
		{
			if (PartOwner->GetClass()->ImplementsInterface(USKGAttachmentComponent::StaticClass()))
			{
				OwningActor = PartOwner;
				return OwningActor.Get();
			}

			PartOwner = PartOwner->GetOwner();
		}
	}
	return nullptr;
}

USKGCharacterComponent* ASKGPart::GetOwningCharacterComponent_Implementation()
{
	if (OwningCharacterComponent)
	{
		return OwningCharacterComponent.Get();
	}

	if (OwningFirearm)
	{
		if (USKGCharacterComponent* CharacterComponent = OwningFirearm->GetCharacterComponent())
		{
			OwningCharacterComponent = CharacterComponent;
			return CharacterComponent;
		}
	}
	else
	{
		OwningFirearm = Cast<ASKGFirearm>(ISKGFirearmAttachmentsInterface::Execute_GetOwningActor(this));
		if (OwningFirearm)
		{
			if (USKGCharacterComponent* CharacterComponent = OwningFirearm->GetCharacterComponent())
			{
				OwningCharacterComponent = CharacterComponent;
				return CharacterComponent;
			}
		}
	}
	return nullptr;
}

ASKGFirearm* ASKGPart::GetOwningFirearm()
{
	if (OwningFirearm)
	{
		return OwningFirearm.Get();
	}

	if (ISKGFirearmAttachmentsInterface::Execute_GetOwningActor(this))
	{
		OwningFirearm = Cast<ASKGFirearm>(OwningActor);
	}
	return OwningFirearm.Get();
}

void ASKGPart::Use_Implementation()
{
	OnUse();
}