// Copyright 2021, Dakota Dawe, All rights reserved


#include "Actors/FirearmParts/SKGMagnifier.h"
#include "Actors/SKGFirearm.h"
#include "Components/SKGCharacterComponent.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "TimerManager.h"
#include "Engine/World.h"

ASKGMagnifier::ASKGMagnifier()
{
	PrimaryActorTick.bCanEverTick = false;
	PartType = ESKGPartType::Magnifier;

	bFlippedOut = false;
	bFullyFlipped = true;
}

void ASKGMagnifier::OnRep_FlippedOut()
{
	OnUse();
}

void ASKGMagnifier::SetFullyFlipped(bool bIsFullyFlippedOut)
{
	bFullyFlipped = bIsFullyFlippedOut;
	if (bFlippedOut)
	{
		Execute_DisableRenderTarget(this, true);
	}
}

void ASKGMagnifier::BeginPlay()
{
	Super::BeginPlay();
	
	if (AttachmentMesh.IsValid())
	{
		FTimerHandle TempHandle;
		GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &ASKGMagnifier::SetupMagnifier, 0.2f, false);
	}
}

void ASKGMagnifier::OnRep_Owner()
{
	if (AttachmentMesh.IsValid())
	{
		FTimerHandle TempHandle;
		GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &ASKGMagnifier::SetupMagnifier, 0.2f, false);
	}
}

void ASKGMagnifier::SetupMagnifier()
{
	if (AttachmentMesh.IsValid())
	{
		FVector Start = Execute_GetAimSocketTransform(this).GetLocation();
		FVector End = Start + Execute_GetAimSocketTransform(this).Rotator().Vector() * 20.0f;
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		if (ISKGFirearmAttachmentsInterface::Execute_GetOwningActor(this))
		{
			QueryParams.AddIgnoredActor(OwningActor.Get());
		}
		
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			if (ASKGSight* HitSight = Cast<ASKGSight>(HitResult.GetActor()))
			{
				ISKGAimInterface::Execute_SetMagnifier(HitSight, this);
				SightInfront = HitSight;
			}
		}
	}
}

void ASKGMagnifier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGMagnifier, bFlippedOut, Params);
}

bool ASKGMagnifier::Server_Flip_Validate(bool bFlip)
{
	return true;
}

void ASKGMagnifier::Server_Flip_Implementation(bool bFlip)
{
	bFlippedOut = bFlip;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASKGMagnifier, bFlippedOut, this);
	OnUse();
}

void ASKGMagnifier::CycleMagnifier()
{
	bFullyFlipped = false;
	bFlippedOut = !bFlippedOut;
	if (!HasAuthority())
	{
		Server_Flip(bFlippedOut);
	}
	if (!bFlippedOut)
	{
		Execute_DisableRenderTarget(this, false);
	}
	OnRep_FlippedOut();
}

void ASKGMagnifier::Use_Implementation()
{
	bFlippedOut = !bFlippedOut;
	bFullyFlipped = false;
	
	if (bFlippedOut)
	{
		Execute_DisableRenderTarget(this, true);
	}
	else
	{
		if (ISKGFirearmAttachmentsInterface::Execute_GetOwningCharacterComponent(this) && OwningCharacterComponent->IsAiming())
		{
			if (GetOwningFirearm() && !ISKGAimInterface::Execute_IsPointAiming(OwningFirearm.Get()) && ISKGFirearmPartsInterface::Execute_GetCurrentSight(OwningFirearm.Get()) == SightInfront)
			{
				Execute_DisableRenderTarget(this, false);
			}
		}
	}
	
	OnUse();
}
