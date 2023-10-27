//Copyright 2021, Dakota Dawe, All rights reserved

#include "Actors/SKGFirearm.h"
#include "SKGCharacterAnimInstance.h"
#include "Misc/BlueprintFunctionsLibraries/SKGFPSStatics.h"
#include "Actors/FirearmParts/SKGSight.h"
#include "Actors/FirearmParts/SKGHandguard.h"
#include "Actors/FirearmParts/SKGForwardGrip.h"
#include "Actors/FirearmParts/SKGPart.h"
#include "Components/SKGCharacterComponent.h"
#include "Components/SKGFirearmStabilizerComponent.h"
#include "Components/SKGAttachmentComponent.h"
#include "Interfaces/SKGStockInterface.h"
#include "DataTypes/SKGFPSFrameworkMacros.h"

#include "NiagaraSystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Camera/CameraComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

// Sets default values
ASKGFirearm::ASKGFirearm(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PoseCollision = ECollisionChannel::ECC_GameTraceChannel2;
	
	FirearmAnimationIndex = 0;
	PointAimIndex = -1;

	FirearmGripSocket = FName("cc_FirearmGrip");
	GripSocketOffset = FTransform();
	MuzzleSocket = FName("S_Muzzle");
	AimSocket = FName("S_Aim");

	DefaultLengthOfPull = 0.0f;

	CameraSettings.CameraFOVZoom = 10.0f;
	CameraSettings.CameraFOVZoomSpeed = 10.0f;
	CameraSettings.CameraDistance = 0.0f;
	CameraSettings.bUseFixedCameraDistance = false;

	MaxSightDistanceOffset = 0.0f;
	
	LeaningSpeedMultiplier = 1.4f;

	PointAimADSInterpolationMultiplier = 1.2f;
	bUnAimMultiplierSameAsADS = false;
	UnAimInterpolationMultiplier = 1.0f;
	AimShoulderStockOffset = FVector(0.0f, 12.0f, 0.0f);
	OppositeShoulderAimShoulderStockOffset = FVector(5.0f, 5.0f, 0.0f);
	HeadAimTransform = FTransform(FRotator(-10.0f, 0.0f, 0.0f), FVector::ZeroVector, FVector::ZeroVector);
	NeckAimTransform = FTransform(FRotator(20.0f, 0.0f, 0.0f), FVector(-2.0f, 0.0f, 0.0f), FVector::ZeroVector);
	OppositeShoulderHeadAimTransform = FTransform(FRotator(10.0f, 0.0f, 0.0f), FVector::ZeroVector, FVector::ZeroVector);
	OppositeShoulderNeckAimTransform = FTransform(FRotator(-30.0f, 0.0f, 0.0f), FVector(2.0f, 0.0f, 0.0f), FVector::ZeroVector);
	bUseBasePoseCorrection = false;

	bCanCycleSights = true;
	
	FireModeIndex = 0;

	TimeSinceLastShot = 0.0f;

	CurrentFirearmPose = ESKGFirearmPose::None;
	FirstPersonShortStockPose = FTransform();
	ThirdPersonShortStockPose = FTransform();
	FirstPersonBasePoseOffset = FTransform();
	ThirdPersonBasePoseOffset = FTransform();
	FirstPersonSprintPose.SetLocation(FVector(-5.0f, 0.0f, -10.0f));
	FirstPersonSprintPose.SetRotation(FRotator(-45.0f, 0.0f, 20.0f).Quaternion());
	FirstPersonSuperSprintPose.SetLocation(FVector());
	FirstPersonSuperSprintPose.SetRotation(FRotator().Quaternion());
	FirstPersonHighPortPose.SetLocation(FVector(-10.0f, 0.0f, -10.0f));
	FirstPersonHighPortPose.SetRotation(FRotator(80.0f, 45.0f, 0.0f).Quaternion());
	FirstPersonLowPortPose.SetLocation(FVector(-10.0f, 0.0f, -10.0f));
	FirstPersonLowPortPose.SetRotation(FRotator(80.0f, -45.0f, 0.0f).Quaternion());

	ThirdPersonSprintPose.SetLocation(FVector(-5.0f, 0.0f, -10.0f));
	ThirdPersonSprintPose.SetRotation(FRotator(-45.0f, 0.0f, 20.0f).Quaternion());
	ThirdPersonSuperSprintPose.SetLocation(FVector());
	ThirdPersonSuperSprintPose.SetRotation(FRotator().Quaternion());
	ThirdPersonHighPortPose.SetLocation(FVector(-10.0f, 0.0f, -3.0f));
	ThirdPersonHighPortPose.SetRotation(FRotator(80.0f, 45.0f, 0.0f).Quaternion());
	ThirdPersonLowPortPose.SetLocation(FVector(0.0f, 8.0f, 8.0f));
	ThirdPersonLowPortPose.SetRotation(FRotator(80.0f, -45.0f, 0.0f).Quaternion());

	AimCurveSettings.bUseLegacySystem = false;
	
	DefaultSwayMultiplier = 2.0f;
	SwayMultiplier = DefaultSwayMultiplier;

	RotationLagInterpolationMultiplier = 1.0f;
	
	CharacterComponent = nullptr;
}

void ASKGFirearm::OnRep_CharacterComponent()
{
	//CycleSights();
	/*if (IsValid(this) && AttachedToSocket != NAME_None)
	{
		AttachToSocket(AttachedToSocket);
	}*/
}

// Called when the game starts or when spawned
void ASKGFirearm::BeginPlay()
{
	Super::BeginPlay();
	
	if (const AActor* OwningActor = GetOwner())
	{
		CharacterComponent = Cast<USKGCharacterComponent>(OwningActor->GetComponentByClass(USKGCharacterComponent::StaticClass()));
	}

	if (HasAuthority())
	{
		HandleSightComponents();
	}
	
	FixPoseTransforms(FirstPersonCollisionPose, ThirdPersonCollisionPose);
	FixPoseTransforms(FirstPersonBasePoseOffset, ThirdPersonBasePoseOffset);
	FixPoseTransforms(FirstPersonHighPortPose, ThirdPersonHighPortPose);
	FixPoseTransforms(FirstPersonLowPortPose, ThirdPersonLowPortPose);
	FixPoseTransforms(FirstPersonShortStockPose, ThirdPersonShortStockPose);
	FixPoseTransforms(FirstPersonOppositeShoulderPose, ThirdPersonOppositeShoulderPose);
	FixPoseTransforms(FirstPersonBlindFireLeftPose, ThirdPersonBlindFireLeftPose);
	FixPoseTransforms(FirstPersonBlindFireUpPose, ThirdPersonBlindFireUpPose);
	FixPoseTransforms(FirstPersonSprintPose, ThirdPersonSprintPose);
	FixPoseTransforms(FirstPersonSuperSprintPose, ThirdPersonSuperSprintPose);
}

void ASKGFirearm::PostInitProperties()
{
	Super::PostInitProperties();
	SwayMultiplier = DefaultSwayMultiplier;

	DefaultCurveAndShakeSettings = CurveAndShakeSettings;
	
	FirearmStats = DefaultFirearmStats;
	if (FireModes.Num())
	{
		FireMode = FireModes[0];
	}
}

void ASKGFirearm::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	bool bValidMesh = false;
	if (FirearmMesh)
	{
		bValidMesh = true;
		FirearmMesh->SetCollisionResponseToChannel(PoseCollision, ECR_Ignore);
		FirearmMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		FirearmMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
		FirearmMesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	}
	
	ensureMsgf(bValidMesh, TEXT("Firearm: %s Has an INVALID mesh component"), *GetName());
}

void ASKGFirearm::OnRep_Owner()
{
	if (const AActor* OwningActor = GetOwner())
	{
		CharacterComponent = Cast<USKGCharacterComponent>(OwningActor->GetComponentByClass(USKGCharacterComponent::StaticClass()));
		MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, CharacterComponent, this);
	}
	RefreshCurrentSight();
}

void ASKGFirearm::OnRep_AttachmentReplication()
{
	if (GetCharacterComponent())
	{
		if (USceneComponent* AttachParentComponent = CharacterComponent->GetInUseMesh())
		{
			RootComponent->SetRelativeLocation_Direct(GetAttachmentReplication().LocationOffset);
			RootComponent->SetRelativeRotation_Direct(GetAttachmentReplication().RotationOffset);
			RootComponent->SetRelativeScale3D_Direct(GetAttachmentReplication().RelativeScale3D);

			// If we're already attached to the correct Parent and Socket, then the update must be position only.
			// AttachToComponent would early out in this case.
			// Note, we ignore the special case for simulated bodies in AttachToComponent as AttachmentReplication shouldn't get updated
			// if the body is simulated (see AActor::GatherMovement).
			const bool bAlreadyAttached = (AttachParentComponent == RootComponent->GetAttachParent() && GetAttachmentReplication().AttachSocket == RootComponent->GetAttachSocketName() && AttachParentComponent->GetAttachChildren().Contains(RootComponent));
			if (bAlreadyAttached)
			{
				// Note, this doesn't match AttachToComponent, but we're assuming it's safe to skip physics (see comment above).
				RootComponent->UpdateComponentToWorld(EUpdateTransformFlags::SkipPhysicsUpdate, ETeleportType::None);
			}
			else
			{
				RootComponent->AttachToComponent(AttachParentComponent, FAttachmentTransformRules::KeepRelativeTransform, GetAttachmentReplication().AttachSocket);
			}
		}
		else
		{
			Super::OnRep_AttachmentReplication();
		}
	}
	else
	{
		Super::OnRep_AttachmentReplication();
	}
}

void ASKGFirearm::OnRep_CachedComponent()
{
	Super::OnRep_CachedComponent();
	/*if (GetCharacterComponent() && CharacterComponent->GetAnimationInstance())
	{
		CharacterComponent->GetAnimationInstance()->UpdateFirearmCollisionData();
	}*/
}

FPrimaryAssetId ASKGFirearm::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, AssetName);
}

void ASKGFirearm::FixPoseTransforms(FTransform& FirstPerson, FTransform& ThirdPerson)
{
	FirstPerson = USKGFPSStatics::FixTransform(FirstPerson);
	ThirdPerson = USKGFPSStatics::FixTransform(ThirdPerson);
}

void ASKGFirearm::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGFirearm, CharacterComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGFirearm, CurrentSightComponent, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGFirearm, PointAimIndex, Params);

	FDoRepLifetimeParams ParamsCondition;
	ParamsCondition.bIsPushBased = true;
	ParamsCondition.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGFirearm, FireMode, ParamsCondition);
}

float ASKGFirearm::GetStockLengthOfPull_Implementation()
{
	if (IsValid(CachedComponents.Stock))
	{
		const AActor* Stock = CachedComponents.Stock->GetAttachment();
		if (IsValid(Stock) && Stock->GetClass()->ImplementsInterface(USKGAttachmentInterface::StaticClass()) && Stock->GetClass()->ImplementsInterface(USKGStockInterface::StaticClass()))
		{
			if (Stock->GetClass()->ImplementsInterface(USKGAttachmentInterface::StaticClass()))
			{
				if (!ISKGStockInterface::Execute_GetIsStockFolded(Stock))
				{
					return ISKGAttachmentInterface::Execute_GetAttachmentOffset(Stock) - ISKGStockInterface::Execute_GetStockLengthOfPull(Stock);
				}
			}
		}
	}
	return DefaultLengthOfPull;
}

void ASKGFirearm::SetFirearmPoseMultiplier(FSKGFirearmPoseCurveSettings& FirearmPose, ESKGFirearmPoseType PoseType, float Multiplier)
{
	switch (PoseType)
	{
	case ESKGFirearmPoseType::ToPose:
		{
			FirearmPose.PoseCurveSetting.CurveSpeedMultiplier = Multiplier;
			break;
		}
	case ESKGFirearmPoseType::ReturnPose:
		{
			FirearmPose.ReturnCurveSetting.CurveSpeedMultiplier = Multiplier;
			break;
		}
	case ESKGFirearmPoseType::Both:
		{
			FirearmPose.PoseCurveSetting.CurveSpeedMultiplier = Multiplier;
			FirearmPose.ReturnCurveSetting.CurveSpeedMultiplier = Multiplier;
			break;
		}
	}
}

FSKGProjectileTransform ASKGFirearm::GetMuzzleProjectileSocketTransform(float RangeMeters, float MOA)
{
	RangeMeters *= 100.0f;
	if (RangeMeters > 10000)
	{
		RangeMeters = 10000;
	}
	else if (RangeMeters < 2500)
	{
		RangeMeters = 2500;
	}

	const FTransform SightTransform = Execute_GetAimSocketTransform(this);
	FTransform MuzzleTransform = Execute_GetMuzzleSocketTransform(this);
	FRotator MuzzleRotation = USKGFPSStatics::GetEstimatedMuzzleToScopeZero(MuzzleTransform, SightTransform, RangeMeters);	
	MuzzleRotation = USKGFPSStatics::SetMuzzleMOA(MuzzleRotation, MOA);

	MuzzleTransform.SetRotation(MuzzleRotation.Quaternion());
	return MuzzleTransform;
}

void ASKGFirearm::SetFirearmPoseCurveSpeedMultiplier(ESKGFirearmPose Pose, ESKGFirearmPoseType PoseType, float NewMultiplier)
{
	switch (Pose)
	{
	case ESKGFirearmPose::High:
		{
			SetFirearmPoseMultiplier(HighPortCurveSettings, PoseType, NewMultiplier);
			break;
		}
	case ESKGFirearmPose::Low:
		{
			SetFirearmPoseMultiplier(LowPortCurveSettings, PoseType, NewMultiplier);
			break;
		}
	case ESKGFirearmPose::ShortStock:
		{
			SetFirearmPoseMultiplier(ShortStockCurveSettings, PoseType, NewMultiplier);
			break;
		}
	}
}

float ASKGFirearm::GetAimInterpolationMultiplier_Implementation()
{
	float SightAimMultiplier = 1.0f;
	if (PointAimIndex > INDEX_NONE)
	{
		SightAimMultiplier = PointAimADSInterpolationMultiplier;
	}
	else
	{
		ASKGSight* Sight = Execute_GetCurrentSight(this);
		if (IsValid(Sight) && Sight->Implements<USKGAimInterface>())
		{
			SightAimMultiplier = ISKGProceduralAnimationInterface::Execute_GetAimInterpolationMultiplier(Sight);
		}
	}
	return (DEFAULT_STATS_MULTIPLIER * PoseSettings.AimInterpolationMultiplier) * SightAimMultiplier;
}

float ASKGFirearm::GetUnAimInterpolationMultiplier_Implementation()
{
	if (bUnAimMultiplierSameAsADS)
	{
		return ISKGProceduralAnimationInterface::Execute_GetAimInterpolationMultiplier(this);
	}
	return (DEFAULT_STATS_MULTIPLIER * PoseSettings.AimInterpolationMultiplier) * UnAimInterpolationMultiplier;
}

float ASKGFirearm::GetRotationLagInterpolationMultiplier_Implementation()
{
	return DEFAULT_STATS_MULTIPLIER * RotationLagInterpolationMultiplier;
}

void ASKGFirearm::GetCameraSettings_Implementation(FSKGAimCameraSettings& OutCameraSettings)
{
	if (PointAimIndex > INDEX_NONE)
	{
		OutCameraSettings = CameraSettings;
		return;
	}
	ASKGSight* CurrentSight = Execute_GetCurrentSight(this);
	if (IsValid(CurrentSight))
	{
		if (CurrentSight->Implements<USKGAimInterface>())
		{
			Execute_GetCameraSettings(CurrentSight, OutCameraSettings);
			return;
		}
	}
	OutCameraSettings = CameraSettings;
}

float ASKGFirearm::GetCurrentMagnification_Implementation() const
{
	if (IsValid(CurrentSightComponent))
	{
		const AActor* CurrentSight = CurrentSightComponent->GetAttachment();
		if (IsValid(CurrentSight) && CurrentSight->GetClass()->ImplementsInterface(USKGSightInterface::StaticClass()))
		{
			return ISKGSightInterface::Execute_GetCurrentMagnification(CurrentSight);
		}
	}
	return 1.0f;
}

FTransform ASKGFirearm::GetAimSocketTransform_Implementation()
{
	if (PointAimIndex > INDEX_NONE && PointAimIndex < PointAimSockets.Num() && FirearmMesh)
	{
		if (FirearmMesh->DoesSocketExist(PointAimSockets[PointAimIndex]))
		{
			return FirearmMesh->GetSocketTransform(PointAimSockets[PointAimIndex]);
		}
		SKG_PRINT(TEXT("Point Aim Index: %d of socket name: %s On Actor: %s does NOT exist"), PointAimIndex, *PointAimSockets[PointAimIndex].ToString(), *GetName());
	}
	
	if (CurrentSightComponent)
	{
		AActor* CurrentAimingDevice = CurrentSightComponent->GetAttachment();
		if (IsValid(CurrentAimingDevice) && CurrentAimingDevice->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
		{
			return ISKGProceduralAnimationInterface::Execute_GetAimSocketTransform(CurrentAimingDevice);
		}
	}

	if (FirearmMesh && FirearmMesh->DoesSocketExist(AimSocket))
	{
		return FirearmMesh->GetSocketTransform(AimSocket);
	}
	return FTransform();
}

FTransform ASKGFirearm::GetDefaultAimSocketTransform_Implementation()
{
	if (FirearmMesh && FirearmMesh->DoesSocketExist(AimSocket))
	{
		return FirearmMesh->GetSocketTransform(AimSocket);
	}

	const FString Message = FString::Printf(TEXT("Socket: %s Does NOT exist on FirearmMesh"), *AimSocket.ToString());
	UKismetSystemLibrary::PrintString(GetWorld(), Message);
	return Execute_GetAimSocketTransform(this);
}

void ASKGFirearm::HandleSightComponents()
{
	if (!IsValid(CurrentSightComponent) || (IsValid(CurrentSightComponent) && IsValid(CurrentSightComponent->GetAttachment()) && (IsValid(CurrentSightComponent) && !IsValid(CurrentSightComponent->GetAttachment()))))
	{
		Execute_CycleSights(this, true, false);
	}
}

USKGCharacterComponent* ASKGFirearm::GetCharacterComponent()
{
	if (!CharacterComponent)
	{
		if (IsValid(GetOwner()))
		{
			UActorComponent* Component = GetOwner()->GetComponentByClass(USKGCharacterComponent::StaticClass());
			CharacterComponent = Cast<USKGCharacterComponent>(Component);
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, CharacterComponent, this);
			}
			OnRep_CharacterComponent();
		}
	}
	return CharacterComponent;
}

bool ASKGFirearm::Server_SetPointAiming_Validate(int32 Index)
{
	return true;
}

void ASKGFirearm::Server_SetPointAiming_Implementation(int32 Index)
{
	Execute_SetPointAimIndex(this, Index);
}

void ASKGFirearm::SetPointAimIndex_Implementation(int32 Index)
{
	if (Index < PointAimSockets.Num())
	{
		PointAimIndex = Index;
		if (GetCharacterComponent() && CharacterComponent->IsAiming())
		{
			if (PointAimIndex > INDEX_NONE)
			{
				Execute_ActivateCurrentSight(this, false);
			}
			else
			{
				Execute_ActivateCurrentSight(this, true);
			}
		}
		//OnRep_PointAimIndex();
	}
	else
	{
		PointAimIndex = INDEX_NONE;
	}
	
	if (!HasAuthority())
	{
		Server_SetPointAiming(PointAimIndex);
	}
	else
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, PointAimIndex, this);
	}
}

void ASKGFirearm::CyclePointAim_Implementation(bool bDownArray, bool bStopAtEndOfArray)
{
	if (bDownArray)
	{
		if (++PointAimIndex > PointAimSockets.Num() - 1)
		{
			if (!bStopAtEndOfArray)
			{
				PointAimIndex = 0;
			}
			else
			{
				PointAimIndex = PointAimSockets.Num() - 1;
			}
		}
	}
	else
	{
		if (--PointAimIndex < 0)
		{
			if (!bStopAtEndOfArray)
			{
				PointAimIndex = PointAimSockets.Num() - 1;
			}
			else
			{
				PointAimIndex = 0;
			}
		}
	}
	Execute_SetPointAimIndex(this, PointAimIndex);
}

void ASKGFirearm::DisableAllRenderTargets(bool Disable)
{
	for (const USKGAttachmentComponent* PartComponent : CachedComponents.RenderTargets)
	{
		if (PartComponent != CurrentSightComponent)
		{
			AActor* Part = PartComponent->GetAttachment();
			if (IsValid(Part) && Part->GetClass()->ImplementsInterface(USKGRenderTargetInterface::StaticClass()))
			{
				if (!Disable)
				{
					ISKGRenderTargetInterface::Execute_DisableRenderTarget(Part, true);
					continue;
				}

				ISKGRenderTargetInterface::Execute_DisableRenderTarget(Part, Disable);
			}
		}
	}
	//RefreshCurrentSight();
}

FTransform ASKGFirearm::GetSprintPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonSprintPose; // THIRD PERSON HERE
		}
	}
	return FirstPersonSprintPose;
}

FTransform ASKGFirearm::GetSuperSprintPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonSuperSprintPose; // THIRD PERSON HERE
		}
	}
	return FirstPersonSuperSprintPose;
}

FTransform ASKGFirearm::GetCollisionShortStockPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonCollisionShortStockPose;
		}
	}
	return FirstPersonCollisionShortStockPose;
}

FTransform ASKGFirearm::GetCollisionPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonCollisionPose;
		}
	}
	return FirstPersonCollisionPose;
}

FTransform ASKGFirearm::GetBasePoseOffset_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonBasePoseOffset;
		}
	}
	return FirstPersonBasePoseOffset;
}

FTransform ASKGFirearm::GetHighPortPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonHighPortPose;
		}
	}
	return FirstPersonHighPortPose;
}

FTransform ASKGFirearm::GetLowPortPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonLowPortPose;
		}
	}
	return FirstPersonLowPortPose;
}

FTransform ASKGFirearm::GetShortStockPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonShortStockPose;
		}
	}
	return FirstPersonShortStockPose;
}

FTransform ASKGFirearm::GetOppositeShoulderPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonOppositeShoulderPose;
		}
	}
	return FirstPersonOppositeShoulderPose;
}

FTransform ASKGFirearm::GetBlindFireLeftPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonBlindFireLeftPose;
		}
	}
	return FirstPersonBlindFireLeftPose;
}

FTransform ASKGFirearm::GetBlindFireUpPose_Implementation()
{
	if (GetCharacterComponent())
	{
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			return ThirdPersonBlindFireUpPose;
		}
	}
	return FirstPersonBlindFireUpPose;
}

void ASKGFirearm::SetBasePoseOffset(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonBasePoseOffset = FixedTransform;
	}
	else
	{
		ThirdPersonBasePoseOffset = FixedTransform;
	}
}

void ASKGFirearm::SetShortStockPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonShortStockPose = FixedTransform;
	}
	else
	{
		ThirdPersonShortStockPose = FixedTransform;
	}
}

void ASKGFirearm::SetHighPortPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonHighPortPose = FixedTransform;
	}
	else
	{
		ThirdPersonHighPortPose = FixedTransform;
	}
}

void ASKGFirearm::SetLowPortPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonLowPortPose = FixedTransform;
	}
	else
	{
		ThirdPersonLowPortPose = FixedTransform;
	}
}

void ASKGFirearm::SetOppositeShoulderPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonOppositeShoulderPose = FixedTransform;
	}
	else
	{
		ThirdPersonOppositeShoulderPose = FixedTransform;
	}
}

void ASKGFirearm::SetBlindFireleftPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonBlindFireLeftPose = FixedTransform;
	}
	else
	{
		ThirdPersonBlindFireLeftPose = FixedTransform;
	}
}

void ASKGFirearm::SetBlindFireUpPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonBlindFireUpPose = FixedTransform;
	}
	else
	{
		ThirdPersonBlindFireUpPose = FixedTransform;
	}
}

void ASKGFirearm::SetSprintPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonSprintPose = FixedTransform;
	}
	else
	{
		ThirdPersonSprintPose = FixedTransform;
	}
}

void ASKGFirearm::SetSuperSprintPose(const FTransform& Transform, bool bFirstPerson)
{
	const FTransform FixedTransform = USKGFPSStatics::FixTransform(Transform);
	if (bFirstPerson)
	{
		FirstPersonSuperSprintPose = FixedTransform;
	}
	else
	{
		ThirdPersonSuperSprintPose = FixedTransform;
	}
}

FSKGLeftHandIKData ASKGFirearm::GetLeftHandIKData_Implementation()
{
	if (const USKGAttachmentComponent* AttachmentComponent = CachedComponents.ForwardGrip)
	{
		ASKGForwardGrip* ForwardGrip = AttachmentComponent->GetAttachment<ASKGForwardGrip>();
		if (IsValid(ForwardGrip))
		{
			const FSKGLeftHandIKData ForwardGripLeftHandIKData = ForwardGrip->GetLeftHandIKData();
			if (ForwardGripLeftHandIKData.bUseLeftHandIK)
			{
				return ForwardGripLeftHandIKData;
			}
		}
	}

	if (const USKGAttachmentComponent* AttachmentComponent = CachedComponents.Handguard)
	{
		ASKGHandguard* Handguard = AttachmentComponent->GetAttachment<ASKGHandguard>();
		if (IsValid(Handguard))
		{
			const FSKGLeftHandIKData HandguardLeftHandIKData = Handguard->GetLeftHandIKData();
			if (HandguardLeftHandIKData.bUseLeftHandIK)
			{
				return HandguardLeftHandIKData;
			}
		}
	}
	
	if (FirearmMesh)
	{
		LeftHandIKData.Transform = FirearmMesh->GetSocketTransform(LeftHandIKData.HandGripSocket);
	}
	return LeftHandIKData;
}

void ASKGFirearm::SetupStabilizerComponent_Implementation()
{
	if (!StabilizerComponent)
	{
		StabilizerComponent = Cast<USKGFirearmStabilizerComponent>(GetComponentByClass(USKGFirearmStabilizerComponent::StaticClass()));
	}
	else
	{
		StabilizerComponent->CacheEssentials();
	}
}

USKGFirearmStabilizerComponent* ASKGFirearm::GetStabilizerComponent()
{
	Execute_SetupStabilizerComponent(this);
	return StabilizerComponent.Get();
}

bool ASKGFirearm::IsStabilized() const
{
	if (StabilizerComponent)
	{
		return StabilizerComponent->IsStabilized();
	}
	return false;
}

void ASKGFirearm::OnAttachmentUpdated_Implementation()
{
	Super::OnAttachmentUpdated_Implementation();
	RefreshCurrentSight();
	HandleSightComponents();
}

void ASKGFirearm::OnRep_FireMode()
{
}

bool ASKGFirearm::Server_SetFireMode_Validate(ESKGFirearmFireMode NewFireMode)
{
	return true;
}

void ASKGFirearm::Server_SetFireMode_Implementation(ESKGFirearmFireMode NewFireMode)
{
	FireMode = NewFireMode;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, FireMode, this);
	OnRep_FireMode();
}

void ASKGFirearm::CycleFireMode_Implementation(bool bReverse)
{
	if (FireModes.Num() > 1)
	{
		if (bReverse)
		{
			if (--FireModeIndex < 0)
			{
				FireModeIndex = FireModes.Num() - 1;
			}
		}
		else
		{
			if (++FireModeIndex > FireModes.Num() - 1)
			{
				FireModeIndex = 0;
			}
		}
		FireMode = FireModes[FireModeIndex];
		//OnFireModeChanged();
		if (!HasAuthority())
		{
			Server_SetFireMode(FireMode);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, FireMode, this);
		}
	}
}

void ASKGFirearm::SetFireModeIndex_Implementation(int32 Index)
{
	if (Index > -1 && Index < FireModes.Num())
	{
		FireModeIndex = Index;
		FireMode = FireModes[FireModeIndex];
		//OnFireModeChanged();
		if (!HasAuthority())
		{
			Server_SetFireMode(FireMode);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, FireMode, this);
		}
	}
}

void ASKGFirearm::PerformProceduralRecoil(float Multiplier, bool PlayCameraShake, bool bModifyControlRotation)
{
	if (GetCharacterComponent() && CharacterComponent->GetAnimationInstance())
	{
		RecoilData.bUseControlRotation = bModifyControlRotation;
		CharacterComponent->GetAnimationInstance()->PerformRecoil(Multiplier);
		if (PlayCameraShake && CurveAndShakeSettings.FireCameraShake)
		{
			CharacterComponent->PlayCameraShake(CurveAndShakeSettings.FireCameraShake, Multiplier * 1.2f);
		}
	}
}

FSKGProjectileTransform ASKGFirearm::GetProjectileSocketTransformToCenter(const float MaxDistanceToTest, float MOA, bool bAimingOverride, float RangeMetersOverride)
{
	FSKGProjectileTransform MuzzleProjectileTransform = GetMuzzleProjectileSocketTransform(100, MOA);
	if (GetCharacterComponent() && CharacterComponent->GetAnimationInstance())
	{
		if (bAimingOverride && CharacterComponent->IsAiming())
		{
			return GetMuzzleProjectileSocketTransform(RangeMetersOverride, MOA);
		}
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		
		if (AAIController* AIController = Cast<AAIController>(CharacterComponent->GetOwningController()))
		{
			Start = CharacterComponent->GetInUseMesh()->GetSocketLocation(FName(CharacterComponent->GetCameraSocket()));
			End = Start + AIController->GetControlRotation().Vector() * MaxDistanceToTest;
		}
		else
		{
			Start = CharacterComponent->GetCameraComponent()->GetComponentLocation();
			End = Start + CharacterComponent->GetCameraComponent()->GetForwardVector() * MaxDistanceToTest;
		}
		
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(GetOwner());
		QueryParams.AddIgnoredActors(Execute_GetCachedParts(this));
		
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(MuzzleProjectileTransform.Location, HitResult.Location);
			MuzzleProjectileTransform.Rotation = LookAtRotation;
			FRotator AddedRotator = CharacterComponent->GetAnimationInstance()->GetRecoilTransform().Rotator();
			float Temp = AddedRotator.Pitch;
			AddedRotator.Pitch = -AddedRotator.Yaw;
			AddedRotator.Yaw = Temp;
			MuzzleProjectileTransform.Rotation += AddedRotator;

			AddedRotator = CharacterComponent->GetAnimationInstance()->GetSwayTransform().Rotator();
			Temp = AddedRotator.Pitch;
			AddedRotator.Pitch = -AddedRotator.Yaw;
			AddedRotator.Yaw = Temp;
			MuzzleProjectileTransform.Rotation += AddedRotator;

			AddedRotator = CharacterComponent->GetAnimationInstance()->GetRotationLagTransform().Rotator();
			Temp = AddedRotator.Pitch;
			AddedRotator.Pitch = -AddedRotator.Yaw;
			AddedRotator.Yaw = Temp;
			MuzzleProjectileTransform.Rotation += AddedRotator;

			AddedRotator = CharacterComponent->GetAnimationInstance()->GetMovementLagTransform().Rotator();
			Temp = AddedRotator.Pitch;
			AddedRotator.Pitch = -AddedRotator.Yaw;
			AddedRotator.Yaw = Temp;
			MuzzleProjectileTransform.Rotation += AddedRotator;

			MuzzleProjectileTransform.Rotation = USKGFPSStatics::SetMuzzleMOA(MuzzleProjectileTransform.Rotation, MOA);
		}
	}
	
	return MuzzleProjectileTransform;
}

void ASKGFirearm::OnRep_CurrentSightComponent()
{
	if (GetCharacterComponent() && CharacterComponent->GetAnimationInstance())
	{
		if (!CharacterComponent->IsLocallyControlled())
		{
			CharacterComponent->GetAnimationInstance()->CycledSights();
		}
	}
}

bool ASKGFirearm::Server_CycleSights_Validate(USKGAttachmentComponent* SightComponent)
{
	return true;
}

void ASKGFirearm::Server_CycleSights_Implementation(USKGAttachmentComponent* SightComponent)
{
	if (SightComponent)
	{
		CurrentSightComponent = SightComponent;
		MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, CurrentSightComponent, this);
		OnRep_CurrentSightComponent();
	}
}

void ASKGFirearm::ActivateCurrentSight_Implementation(bool bActivate)
{
	if (IsValid(CurrentSightComponent))
	{
		AActor* CurrentSight = CurrentSightComponent->GetAttachment();
		if (IsValid(CurrentSight) && CurrentSight->GetClass()->ImplementsInterface(USKGRenderTargetInterface::StaticClass()))
		{
			if (bActivate && PointAimIndex > INDEX_NONE)
			{
				ISKGRenderTargetInterface::Execute_DisableRenderTarget(CurrentSight, true);
				return;
			}
			ISKGRenderTargetInterface::Execute_DisableRenderTarget(CurrentSight, !bActivate);
		}
	}
}

void ASKGFirearm::CycleSights_Implementation(bool bDownArray, bool bStopAtEndOfArray)
{
	if (!bCanCycleSights || PointAimIndex > INDEX_NONE)
	{
		return;
	}
	const USKGAttachmentComponent* CurrentComponent = CurrentSightComponent;
	USKGAttachmentComponent* NewSightComponent = nullptr;
	
	bool bFoundValidSight = false;
	TArray<USKGAttachmentComponent*> PartComponents = CachedComponents.Sights;

	if (GetAllAttachmentComponents_Implementation(false).Num())
	{
		if (bDownArray)
		{
			for (uint8 i = SightComponentIndex; i < PartComponents.Num(); ++i)
			{
				if (USKGAttachmentComponent* SightComponent = PartComponents[i])
				{
					if (CurrentSightComponent != SightComponent)
					{
						AActor* AimingPart = SightComponent->GetAttachment();
						if (IsValid(AimingPart) && AimingPart->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
						{
							if (ISKGAimInterface::Execute_IsAimable(AimingPart))
							{
								NewSightComponent = SightComponent;
								bFoundValidSight = true;
								SightComponentIndex = i;
								break;
							}
						}
					}
				}
			}
			if (!bFoundValidSight && !bStopAtEndOfArray)
			{
				for (uint8 i = 0; i < PartComponents.Num(); ++i)
				{
					if (USKGAttachmentComponent* SightComponent = PartComponents[i])
					{
						if (CurrentSightComponent != SightComponent)
						{
							AActor* AimingPart = SightComponent->GetAttachment();
							if (IsValid(AimingPart) && AimingPart->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
							{
								if (ISKGAimInterface::Execute_IsAimable(AimingPart))
								{
									NewSightComponent = SightComponent;
									bFoundValidSight = true;
									SightComponentIndex = i;
									break;
								}
							}
						}
					}
				}
			}
		}
		else if (PartComponents.Num())
		{
			for (int16 i = SightComponentIndex - 1; i > -1; --i)
			{
				if (USKGAttachmentComponent* SightComponent = PartComponents[i])
				{
					if (CurrentSightComponent != SightComponent)
					{
						AActor* AimingPart = SightComponent->GetAttachment();
						if (IsValid(AimingPart) && AimingPart->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
						{
							if (ISKGAimInterface::Execute_IsAimable(AimingPart))
							{
								NewSightComponent = SightComponent;
								bFoundValidSight = true;
								SightComponentIndex = i;
								break;
							}
						}
					}
				}
			}
			if (!bFoundValidSight && !bStopAtEndOfArray)
			{
				for (uint8 i = PartComponents.Num() - 1; i > -1; --i)
				{
					if (USKGAttachmentComponent* SightComponent = PartComponents[i])
					{
						if (CurrentSightComponent != SightComponent)
						{
							AActor* AimingPart = SightComponent->GetAttachment();
							if (IsValid(AimingPart) && AimingPart->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
							{
								if (ISKGAimInterface::Execute_IsAimable(AimingPart))
								{
									NewSightComponent = SightComponent;
									bFoundValidSight = true;
									SightComponentIndex = i;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	if (NewSightComponent)
	{
		Execute_ActivateCurrentSight(this, false);
		CurrentSightComponent = NewSightComponent;
	}
	
	if (bFoundValidSight)
	{
		if (GetCharacterComponent() && CharacterComponent->IsLocallyControlled())
		{
			if (!HasAuthority())
			{
				Server_CycleSights(CurrentSightComponent);
			}
			else
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, CurrentSightComponent, this);
			}
		}
	}
	
	if (GetCharacterComponent())
	{
		if (USKGCharacterAnimInstance* AnimInstance = CharacterComponent->GetAnimationInstance())
		{
			AnimInstance->CycledSights();
		}
	}

	if (GetCharacterComponent())
	{
		Execute_ActivateCurrentSight(this, GetCharacterComponent()->IsAiming());
	}
}

void ASKGFirearm::RefreshCurrentSight()
{
	if (!bCanCycleSights)
	{
		return;
	}
	
	if (CurrentSightComponent)
	{
		AActor* Sight = CurrentSightComponent->GetAttachment();
		if (IsValid(Sight) && Sight->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
		{
				if (ISKGAimInterface::Execute_IsAimable(Sight))
				{
					if (!HasAuthority() && GetLocalRole() == ROLE_AutonomousProxy)
					{
						Server_CycleSights(CurrentSightComponent);
					}
					if (GetCharacterComponent())
					{
						if (USKGCharacterAnimInstance* AnimInstance = CharacterComponent->GetAnimationInstance())
						{
							AnimInstance->CycledSights();
						}
					}
				}
		}
		else
		{
			Execute_CycleSights(this, true, false);
		}
	}
	else
	{
		Execute_CycleSights(this, true, false);
	}

	if (GetCharacterComponent())
	{
		Execute_ActivateCurrentSight(this, GetCharacterComponent()->IsAiming());
	}
}

void ASKGFirearm::SetSight_Implementation(USKGAttachmentComponent* SightComponent)
{
	const TArray<USKGAttachmentComponent*> PartComponents = CachedComponents.Sights;
	if (IsValid(SightComponent) && PartComponents.Contains(SightComponent))
	{
		CurrentSightComponent = SightComponent;
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearm, CurrentSightComponent, this);
		}
		RefreshCurrentSight();
	}
}

#pragma region Attachment
/*bool AFPSTemplateFirearm::Server_AttachToSocket_Validate(const FName& Socket)
{
	return true;
}

void AFPSTemplateFirearm::Server_AttachToSocket_Implementation(const FName& Socket)
{
	AttachToSocket(Socket);
}

void AFPSTemplateFirearm::OnRep_AttachedToSocket()
{
	if (AttachedToSocket == NAME_None)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		return;
	}
	if (GetCharacterComponent())
	{
		CharacterComponent->AttachItem(this, AttachedToSocket);
		OnAttachedToSocket(AttachedToSocket);
	}
}

void AFPSTemplateFirearm::AttachToSocket(const FName Socket)
{
	AttachedToSocket = Socket;
	OnRep_AttachedToSocket();
	if (!HasAuthority())
	{
		Server_AttachToSocket(Socket);
	}
}*/
#pragma endregion Attachment

ASKGSight* ASKGFirearm::GetCurrentSight_Implementation()
{
	if (CurrentSightComponent)
	{
		return CurrentSightComponent->GetAttachment<ASKGSight>();
	}
	return nullptr;
}

AActor* ASKGFirearm::GetCurrentSightActor_Implementation()
{
	if (CurrentSightComponent)
	{
		return CurrentSightComponent->GetAttachment();
	}
	return nullptr;
}