// Copyright 2021, Dakota Dawe, All rights reserved

#include "Actors/FirearmParts//SKGSight.h"
#include "Actors/SKGFirearm.h"
#include "Camera/CameraComponent.h"
#include "Actors/FirearmParts//SKGMagnifier.h"
#include "Components/SKGSceneCaptureOptic.h"
#include "SKGSceneCaptureDataTypes.h"
#include "Components/SKGCharacterComponent.h"
#include "DataTypes/SKGFPSFrameworkMacros.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/World.h"
#define OpticLocation FName("OpticLocation")
#define DistanceFromOptic FName("DistanceFromOptic")
#define OpticRadius FName("OpticRadius")

ASKGSight::ASKGSight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	PartStats.Weight = 0.1f;
	PartStats.ErgonomicsChangePercentage = -2.0f;
	CameraSettings.CameraFOVZoom = 10.0f;
	CameraSettings.CameraFOVZoomSpeed = 10.0f;
	CameraSettings.CameraDistance = 0.0f;
	CameraSettings.bUseFixedCameraDistance = false;

	bIsAimable = true;
	AimSockets = { "S_Aim" };
	AimSocket = FName("S_Aim");

	DefaultSwayMultiplier = 1.0f;

	PartType = ESKGPartType::Sight;

	DotZero = FSKGSightZero();

	ReticleIndex = 0;
	ReticleBrightnessIndex = 0;

	AnimationIndex = -1;
	AimInterpolationMultiplier = 1.0f;
	RotationLagInterpolationMultiplier = 50.0f;
	GripSocket = FName("cc_FirearmGrip");
	/*HeadAimTransform =
	NeckAimTransform*/

	CurrentDotElevation = 0.0f;
	CurrentDotWindage = 0.0f;
	StartingDotElevation = 0.0f;
	StartingDotWindage = 0.0f;
}

// Called when the game starts or when spawned
void ASKGSight::BeginPlay()
{
	Super::BeginPlay();
	if (AimSockets.Num())
	{
		AimSocket = AimSockets[0];
	}
	
	ReticleBrightnessIndex = ReticleSettings.ReticleBrightness.ReticleDefaultBrightnessIndex;
	if (ReticleBrightnessIndex > ReticleSettings.ReticleBrightness.ReticleBrightnessSettings.Num())
	{
		ReticleBrightnessIndex = ReticleSettings.ReticleBrightness.ReticleBrightnessSettings.Num();
		ReticleSettings.ReticleBrightness.ReticleDefaultBrightnessIndex = ReticleBrightnessIndex;
	}

	for (FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
	{
		if (Reticle.RenderTargetMaterial.Material)
		{
			Reticle.RenderTargetMaterial.DynamicMaterial = UMaterialInstanceDynamic::Create(Reticle.RenderTargetMaterial.Material, this);
			Reticle.RenderTargetMaterial.DynamicMaterial->GetScalarParameterValue(FName("ReticleSize"), Reticle.ReticleSize);
		}
	}
	
	if (!IsValid(SceneCapture))
	{
		SceneCapture = FindComponentByClass<USKGSceneCaptureOptic>();
		if (SceneCapture)
		{
			bHasRenderTarget = true;
			SceneCapture->SetHiddenInGame(false);
			SceneCapture->SetReticleSettings(ReticleSettings);
			SceneCapture->SetOwningMesh(AttachmentMesh.Get());
			SceneCapture->SetTurretAdjustment(SightAdjustmentSettings);
			Execute_DisableRenderTarget(this, true);
		}
		else
		{
			SightAdjustmentSettings = FSKGTurretAdjustmentSettings(SightAdjustmentSettings, true);
		}
	}

	Execute_SetReticleBrightness(this, ReticleBrightnessIndex);
	Execute_SetReticle(this, ReticleIndex);

	if (DefaultZeroSettings.bStartWithDefaultZero)
	{
		SetDefaultZero();
	}
	else
	{
		if (DefaultZeroSettings.bCanBeZeroed && DefaultZeroSettings.bStartWithRandomZero)
		{
			SetRandomZero();
		}
	}

	if (MPC)
	{
		MPCInstance = GetWorld()->GetParameterCollectionInstance(MPC);
		if (MPCInstance)
		{
			CacheCharacterAndFirearm();
			if (OwningCharacterComponent && OwningCharacterComponent->IsLocallyControlled())
			{
				//SetActorTickEnabled(true);
			}
		}
	}

	if (bIsAimable && AttachmentMesh.IsValid() && !AttachmentMesh->DoesSocketExist(AimSocket))
	{
		SKG_PRINT(TEXT("Attachment Mesh or Socket %s Does NOT Exist On Actor: %s"), *AimSocket.ToString(), *GetName());
	}
}

void ASKGSight::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGSight, AimSocketIndex, Params);
}

void ASKGSight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (MPCInstance && OwningCharacterComponent && OwningCharacterComponent->GetCurrentSight() == this)
	{
		MPCInstance->SetVectorParameterValue(OpticLocation, GetAimSocketTransform().GetLocation());
		const float DistanceToOptic = FVector::Distance(GetAimSocketTransform().GetLocation(), OwningCharacterComponent->GetCameraComponent()->GetComponentLocation());
		MPCInstance->SetScalarParameterValue(DistanceFromOptic, DistanceToOptic);
		MPCInstance->SetScalarParameterValue(OpticRadius, ReticleSettings.Radius);
		//UE_LOG(LogTemp, Warning, TEXT("DistFromOptic: %f   Optic: %s"), DistanceToOptic, *GetName());
	}
}

void ASKGSight::SetDefaultZero()
{
	const ESKGRotationDirection ElevationDirection = DefaultZeroSettings.StartingElevationClicks > 0 ? ESKGRotationDirection::CounterClockWise : ESKGRotationDirection::ClockWise;
	ISKGSightInterface::Execute_MovePointOfImpact(this, ESKGElevationWindage::Elevation, ElevationDirection, DefaultZeroSettings.StartingElevationClicks);

	const ESKGRotationDirection WindageDirection = DefaultZeroSettings.StartingWindageClicks > 0 ? ESKGRotationDirection::CounterClockWise : ESKGRotationDirection::ClockWise;
	ISKGSightInterface::Execute_MovePointOfImpact(this, ESKGElevationWindage::Windage, WindageDirection, DefaultZeroSettings.StartingWindageClicks);
}

void ASKGSight::SetRandomZero()
{
	const uint8 ElevationStart = FMath::RandRange(0, DefaultZeroSettings.RandomMaxElevationStartClicks);
	const uint8 WindageStart = FMath::RandRange(0, DefaultZeroSettings.RandomMaxWindageStartClicks);
	if (FMath::RandBool())
	{
		ISKGSightInterface::Execute_MovePointOfImpact(this, ESKGElevationWindage::Elevation, ESKGRotationDirection::CounterClockWise, ElevationStart);
	}
	else
	{
		ISKGSightInterface::Execute_MovePointOfImpact(this, ESKGElevationWindage::Elevation, ESKGRotationDirection::ClockWise, ElevationStart);
	}
	if (FMath::RandBool())
	{
		ISKGSightInterface::Execute_MovePointOfImpact(this, ESKGElevationWindage::Windage, ESKGRotationDirection::ClockWise, WindageStart);
	}
	else
	{
		ISKGSightInterface::Execute_MovePointOfImpact(this, ESKGElevationWindage::Windage, ESKGRotationDirection::CounterClockWise, WindageStart);
	}
}

void ASKGSight::SetReticleBrightness_Implementation(uint8 Index)
{
	if (IsValid(SceneCapture))
	{
		SceneCapture->SetReticleBrightness(Index);
		return;
	}
	if (Index < ReticleSettings.ReticleBrightness.ReticleBrightnessSettings.Num())
	{
		for (const FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
		{
			if (Reticle.RenderTargetMaterial.DynamicMaterial)
			{
				Reticle.RenderTargetMaterial.DynamicMaterial->SetScalarParameterValue(FName("ReticleBrightness"), ReticleSettings.ReticleBrightness.ReticleBrightnessSettings[Index]);
			}
		}
	}
}

void ASKGSight::ReturnToZeroElevation_Implementation()
{
	if (DefaultZeroSettings.bCanBeZeroed)
	{
		if (IsValid(SceneCapture))
		{
			SceneCapture->ReturnToZeroElevation();
		}
		else
		{
			DotZero.Elevation = 0.0f;
			CurrentDotElevation = StartingDotElevation;
			for (const FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
			{
				if (Reticle.RenderTargetMaterial.DynamicMaterial)
				{
					Reticle.RenderTargetMaterial.DynamicMaterial->SetVectorParameterValue(ReticleZeroName, FLinearColor(CurrentDotWindage, CurrentDotElevation, 0.0f));
				}
			}
		}
		OnReturnToZeroElevation();
	}
}

void ASKGSight::ReturnToZeroWindage_Implementation()
{
	if (DefaultZeroSettings.bCanBeZeroed)
	{
		if (IsValid(SceneCapture))
		{
			SceneCapture->ReturnToZeroWindage();
		}
		else
		{
			DotZero.Windage = 0.0f;
			CurrentDotWindage = StartingDotWindage;
			for (const FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
			{
				if (Reticle.RenderTargetMaterial.DynamicMaterial)
				{
					Reticle.RenderTargetMaterial.DynamicMaterial->SetVectorParameterValue(ReticleZeroName, FLinearColor(CurrentDotWindage, CurrentDotElevation, 0.0f));
				}
			}
		}
		OnReturnToZeroWindage();
	}
}

void ASKGSight::SetNewZeroElevation_Implementation()
{
	if (DefaultZeroSettings.bCanBeZeroed)
	{
		if (IsValid(SceneCapture))
		{
			SceneCapture->SetNewZeroElevation();
		}
		else
		{
			DotZero.Elevation = 0.0f;
			StartingDotElevation = CurrentDotElevation;
		}
		OnReturnToZeroElevation();
	}
}

void ASKGSight::SetNewZeroWindage_Implementation()
{
	if (DefaultZeroSettings.bCanBeZeroed)
	{
		if (IsValid(SceneCapture))
		{
			SceneCapture->SetNewZeroWindage();
		}
		else
		{
			DotZero.Windage = 0.0f;
			StartingDotWindage = CurrentDotWindage;
		}
		OnReturnToZeroWindage();
	}
}

void ASKGSight::PointOfImpactUp(bool bUp, uint8 Clicks)
{
	if (DefaultZeroSettings.bCanBeZeroed)
	{
		if (IsValid(SceneCapture))
		{
			ElevationClicksChanged = SceneCapture->PointOfImpactUp(bUp, Clicks);
		}
		else
		{
			ElevationClicksChanged.OldClicks = SightAdjustmentSettings.ElevationClicks;
			if (bUp)
			{
				SightAdjustmentSettings.ElevationClicks += Clicks;
				CurrentDotElevation -= SightAdjustmentSettings.AdjustmentAmount * Clicks;
			}
			else
			{
				SightAdjustmentSettings.ElevationClicks -= Clicks;
				CurrentDotElevation += SightAdjustmentSettings.AdjustmentAmount * Clicks;
			}
			ElevationClicksChanged.NewClicks = SightAdjustmentSettings.ElevationClicks;
			
			for (const FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
			{
				if (Reticle.RenderTargetMaterial.DynamicMaterial)
				{
					Reticle.RenderTargetMaterial.DynamicMaterial->SetVectorParameterValue(ReticleZeroName, FLinearColor(CurrentDotWindage, CurrentDotElevation, 0.0f));
				}
			}
		}
		OnElevationChanged(ElevationClicksChanged);
	}
}

void ASKGSight::PointOfImpactLeft(bool bLeft, uint8 Clicks)
{
	if (DefaultZeroSettings.bCanBeZeroed)
	{
		if (IsValid(SceneCapture))
		{
			WindageClicksChanged = SceneCapture->PointOfImpactLeft(bLeft, Clicks);
		}
		else
		{
			WindageClicksChanged.OldClicks = SightAdjustmentSettings.WindageClicks;
			if (bLeft)
			{
				SightAdjustmentSettings.WindageClicks += Clicks;
				CurrentDotWindage -= SightAdjustmentSettings.AdjustmentAmount * Clicks;
			}
			else
			{
				SightAdjustmentSettings.WindageClicks -= Clicks;
				CurrentDotWindage += SightAdjustmentSettings.AdjustmentAmount * Clicks;
			}
			WindageClicksChanged.NewClicks = SightAdjustmentSettings.WindageClicks;

			for (const FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
			{
				if (Reticle.RenderTargetMaterial.DynamicMaterial)
				{
					Reticle.RenderTargetMaterial.DynamicMaterial->SetVectorParameterValue(ReticleZeroName, FLinearColor(CurrentDotWindage, CurrentDotElevation, 0.0f));
				}
			}
		}
		OnWindageChanged(WindageClicksChanged);
	}
}

void ASKGSight::MovePointOfImpact_Implementation(ESKGElevationWindage Turret, ESKGRotationDirection Direction, uint8 Clicks)
{
	switch (Turret)
	{
	case ESKGElevationWindage::Elevation :
		{
			if (Direction == ESKGRotationDirection::CounterClockWise)
			{
				PointOfImpactUp(true, Clicks);
			}
			else
			{
				PointOfImpactUp(false, Clicks);
			}
			break;
		}
	case ESKGElevationWindage::Windage :
		{
			if (Direction == ESKGRotationDirection::CounterClockWise)
			{
				PointOfImpactLeft(true, Clicks);
			}
			else
			{
				PointOfImpactLeft(false, Clicks);
			}
			break;
		}
	case ESKGElevationWindage::Both :
		{
			if (Direction == ESKGRotationDirection::CounterClockWise)
			{
				PointOfImpactUp(true, Clicks);
				PointOfImpactLeft(true, Clicks);
			}
			else
			{
				PointOfImpactUp(false, Clicks);
				PointOfImpactLeft(false, Clicks);
			}
		}
	}
}

FSKGSightZero ASKGSight::GetSightZero_Implementation() const
{
	if (IsValid(SceneCapture))
	{
		return SceneCapture->GetSightZero();
	}
	return DotZero;
}

bool ASKGSight::SetAimSocketIndex_Implementation(const uint8 Index)
{
	if (Index < AimSockets.Num())
	{
		AimSocket = AimSockets[Index];
		return true;
	}
	return false;
}


void ASKGSight::OnRep_AimSocketIndex()
{
	ISKGSightInterface::Execute_SetAimSocketIndex(this, AimSocketIndex);
}

void ASKGSight::Server_SetAimSocketIndex_Implementation(uint8 Index)
{
	AimSocketIndex = Index;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASKGSight, AimSocketIndex, this);
	OnRep_AimSocketIndex();
}

bool ASKGSight::Server_SetAimSocketIndex_Validate(uint8 Index)
{
	return Index < AimSockets.Num() ? true : false;
}

void ASKGSight::CycleAimSocket_Implementation(bool bDownArray, bool bStopAtEndOfArray)
{
	if (AimSockets.Num())
	{
		bool bChangedSocket = false;
		if (bDownArray)
		{
			if (AimSocketIndex + 1 < AimSockets.Num())
			{
				++AimSocketIndex;
				bChangedSocket = true;
			}
			else if (!bStopAtEndOfArray)
			{
				AimSocketIndex = 0;
				bChangedSocket = true;
			}
		}
		else
		{
			if (AimSocketIndex - 1 > -1)
			{
				--AimSocketIndex;
				bChangedSocket = true;
			}
			else if (!bStopAtEndOfArray)
			{
				AimSocketIndex = AimSockets.Num() - 1;
				bChangedSocket = true;
			}
		}

		if (bChangedSocket && AttachmentMesh.IsValid() && AttachmentMesh->DoesSocketExist(AimSockets[AimSocketIndex]))
		{
			AimSocket = AimSockets[AimSocketIndex];
			if (!HasAuthority())
			{
				Server_SetAimSocketIndex(AimSocketIndex);
			}
			else
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(ASKGSight, AimSocketIndex, this);
			}
		}
	}
}

FTransform ASKGSight::GetAimSocketTransform_Implementation()
{
	if (IsValid(Magnifier) && Magnifier->IsFullyFlipped() && !Magnifier->IsFlippedOut() && Magnifier->GetSightInfront() == this)
	{
		return Execute_GetAimSocketTransform(Magnifier);
	}
	return AttachmentMesh.IsValid() ? AttachmentMesh->GetSocketTransform(AimSocket) : FTransform();
}

bool ASKGSight::IsAimable_Implementation()
{
	if (bIsAimable)
	{
		if (AttachmentMesh.IsValid() && AttachmentMesh->DoesSocketExist(AimSocket))
		{
			return true;
		}
	}
	return false;
}

void ASKGSight::GetCameraSettings_Implementation(FSKGAimCameraSettings& OutCameraSettings)
{
	if (IsValid(Magnifier) && !Magnifier->IsFlippedOut() && Magnifier->GetSightInfront() == this)
	{
		if (Magnifier->Implements<USKGAimInterface>())
		{
			Execute_GetCameraSettings(Magnifier, OutCameraSettings);
			return;
		}
	}
	OutCameraSettings = CameraSettings;
}

void ASKGSight::ZoomOptic_Implementation(bool bZoom)
{
	if (IsValid(SceneCapture))
	{
		if (bZoom)
		{
			SceneCapture->ZoomIn();
		}
		else
		{
			SceneCapture->ZoomOut();
		}
	}
}

void ASKGSight::DisableRenderTarget_Implementation(bool Disable)
{
	if (IsValid(Magnifier) && Magnifier->GetClass()->ImplementsInterface(USKGRenderTargetInterface::StaticClass()) && !Magnifier->IsFlippedOut() && Magnifier->GetSightInfront() == this)
	{
		ISKGRenderTargetInterface::Execute_DisableRenderTarget(Magnifier, Disable);
		return;
	}
	if (IsValid(SceneCapture))
	{
		if (!Disable)
		{
			SceneCapture->StartCapture();
		}
		else
		{
			SceneCapture->StopCapture();
		}
	}
}

bool ASKGSight::IsRenderTargetCapturing_Implementation()
{
	if (SceneCapture)
	{
		return SceneCapture->IsComponentTickEnabled();
	}
	return false;
}

float ASKGSight::GetMagnification() const
{
	if (Magnifier && !Magnifier->IsFlippedOut() && Magnifier->GetSightInfront() == this)
	{
		return Magnifier->GetMagnification();
	}
	if (IsValid(SceneCapture))
	{
		return SceneCapture->GetMagnification();
	}
	return 1.0f;
}

void ASKGSight::CycleReticle_Implementation()
{
	if (IsValid(SceneCapture))
	{
		SceneCapture->CycleReticle();
		return;
	}
	if (ReticleSettings.ReticleMaterials.Num())
	{
		if (++ReticleIndex >= ReticleSettings.ReticleMaterials.Num())
		{
			ReticleIndex = 0;
		}
		Execute_SetReticle(this, ReticleIndex);
	}
}

void ASKGSight::SetReticle_Implementation(uint8 Index)
{
	if (IsValid(SceneCapture))
	{
		SceneCapture->SetReticle(Index);
		return;
	}
	if (AttachmentMesh.IsValid() && Index < ReticleSettings.ReticleMaterials.Num() && ReticleSettings.ReticleMaterials[Index].RenderTargetMaterial.DynamicMaterial)
	{
		ReticleIndex = Index;
		AttachmentMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, ReticleSettings.ReticleMaterials[ReticleIndex].RenderTargetMaterial.DynamicMaterial);
	}
}

void ASKGSight::IncreaseBrightness_Implementation()
{
	if (++ReticleBrightnessIndex < ReticleSettings.ReticleBrightness.ReticleBrightnessSettings.Num())
	{
		Execute_SetReticleBrightness(this, ReticleBrightnessIndex);
	}
	else
	{
		ReticleBrightnessIndex = ReticleSettings.ReticleBrightness.ReticleBrightnessSettings.Num() - 1;
	}
}

void ASKGSight::DecreaseBrightness_Implementation()
{
	if (ReticleBrightnessIndex - 1 >= 0)
	{
		--ReticleBrightnessIndex;
		Execute_SetReticleBrightness(this, ReticleBrightnessIndex);
	}
	else
	{
		ReticleBrightnessIndex = 0;
	}
}