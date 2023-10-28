// Copyright 2021, Dakota Dawe, All rights reserved


#include "Components/SKGCharacterComponent.h"
#include "SKGCharacterAnimInstance.h"
#include "Actors/SKGFirearm.h"
#include "Misc/BlueprintFunctionsLibraries/SKGFPSStatics.h"
#include "Interfaces/SKGAimInterface.h"
#include "Interfaces/SKGInfraredInterface.h"
#include "Components/SKGCharacterMovementComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "Interfaces/SKGRenderTargetInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

DECLARE_CYCLE_STAT(TEXT("SKGTick"), STAT_SKGTick, STATGROUP_SKGCharacterComponent);

USKGCharacterComponent::USKGCharacterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.0f / 30.0f; // tick firearm collision at 30fps
	SetIsReplicatedByDefault(true);
	
	FirearmCollisionChannel = ECC_GameTraceChannel2;

	bAutoSetOwnerOnEquipHeldActor = true;
	bAutoAttachOnEquipHeldActor = true;
	
	bIsInitialized = false;
	bLocallyControlled = false;
	
	MovementComponentSprintSpeed = 350.0f;
	MovementComponentSuperSprintSpeed = 425.0f;
	bForceIntoSprintPose = false;
	MaxLookUpAngle = 80.0f;
	MaxLookDownAngle = 80.0f;
	DefaultCameraFOV = 90.0f;
	
	CameraSocket = FName("Camera");
	CameraSocketParentBone = FName("head");
	RightHandAxis = EAxis::Type::Y;
	
	MaxFirearmAttachmentAttempts = 5;
	FirearmReAttachmentAttemptInterval = 0.5f;
	AttachmentAttempt = 0;

	bUseLeftHandIK = true;
	bUseLeftHandTwoBoneIK = true;
	bFreeLook = false;

	MaxLookLeftRight = 50.0f;
	MaxLookUpDown = 35.0f;

	LookUpDownOffset = 0.0f;
	
	ControlYaw = 0.0f;

	bIsThirdPersonDefault = false;
	bInThirdPerson = bIsThirdPersonDefault;

	bCanAim = true;

	SprintType = ESKGSprintType::None;

	bNightVisionOn = false;
}

void USKGCharacterComponent::BeginPlay()
{
	Super::BeginPlay();

	SetupPawnComponents();
}

void USKGCharacterComponent::PostInitProperties()
{
	Super::PostInitProperties();
	Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
	OnRep_Flags();
}

void USKGCharacterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	//DOREPLIFETIME_WITH_PARAMS_FAST(UFPSTemplate_CharacterComponent, CurrentFirearm);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGCharacterComponent, HeldActor, Params);

	FDoRepLifetimeParams ParamsOwnerOnly;
	ParamsOwnerOnly.bIsPushBased = true;
	ParamsOwnerOnly.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGCharacterComponent, Flags, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGCharacterComponent, LeanSettings, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGCharacterComponent, SprintType, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGCharacterComponent, FirearmPose, ParamsOwnerOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(USKGCharacterComponent, ControlYaw, ParamsOwnerOnly);
}

void USKGCharacterComponent::OnRep_Flags()
{
	bIsAiming = (Flags & (1 << 0)) ? 1 : 0;
	bUseLeftHandIK = (Flags & (1 << 1)) ? 1 : 0;
	bFreeLook = (Flags & (1 << 2)) ? 1 : 0;
	bUseLeftHandTwoBoneIK = (Flags & (1 << 3)) ? 1 : 0;
	if (AnimationInstance)
	{
		AnimationInstance->SetIsAiming(bIsAiming);
		AnimationInstance->SetHeadAiming(bIsAiming);
		AnimationInstance->EnableLeftHandIK(bUseLeftHandIK);
		AnimationInstance->EnableLeftHandTwoBoneIK(bUseLeftHandTwoBoneIK);
	}

	SetFreeLook(bFreeLook);
}

void USKGCharacterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SCOPE_CYCLE_COUNTER(STAT_SKGTick);
	if (HasAuthority())
	{
		ControlYaw = GetControlRotation().Yaw;
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, ControlYaw, this);
	}
	
	HandleInfraredMaterialCollection();
}

void USKGCharacterComponent::SetupPawnComponents()
{
	if (GetOwner())
	{
		TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
		GetOwner()->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
		USkeletalMeshComponent* FoundFirstPersonMesh = nullptr;
		USkeletalMeshComponent* FoundThirdPersonMesh = nullptr;
		
		for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
		{
			if (MeshComponent)
			{
				if (MeshComponent->GetFName().IsEqual(ThirdPersonMeshComponentName))
				{
					FoundThirdPersonMesh = MeshComponent;
					if (bUseSingleMesh)
					{
						FoundFirstPersonMesh = MeshComponent;
						break;
					}
				}
				else if (MeshComponent->GetFName().IsEqual(FirstPersonMeshComponentName))
				{
					FoundFirstPersonMesh = MeshComponent;
					if (bUseSingleMesh)
					{
						FoundThirdPersonMesh = MeshComponent;
						break;
					}
				}
			}
		}

		TArray<UCameraComponent*> CameraComponents;
		GetOwner()->GetComponents<UCameraComponent>(CameraComponents);
		UCameraComponent* FoundCameraComponent = nullptr;
		for (UCameraComponent* CameraComponent : CameraComponents)
		{
			if (CameraComponent && CameraComponent->GetFName().IsEqual(CameraComponentName))
			{
				FoundCameraComponent = CameraComponent;
				break;
			}
		}
		
		Init(FoundCameraComponent, FoundFirstPersonMesh, FoundThirdPersonMesh);
	}
}

void USKGCharacterComponent::HandleInfraredMaterialCollection()
{
	if (InfraredMaterialSettings.bUseInfraredMaterialSettings)
	{
		if (bNightVisionOn)
		{
			const float ElapsedTime = GetWorld()->GetTimeSeconds() - InfraredMaterialSettings.TimeElapsed;
			if (ElapsedTime > InfraredMaterialSettings.InfraredStrobeIntervalSeconds)
			{
				InfraredMaterialSettings.TimeElapsed = GetWorld()->GetTimeSeconds();
				if (InfraredMaterialSettings.InfraredMPC)
				{
					float InfraredStrobeOnValue;
					InfraredMaterialSettings.InfraredMPC->GetScalarParameterValue(FName("InfraredStrobeOn"), InfraredStrobeOnValue);
				
					if (InfraredStrobeOnValue)
					{
						InfraredMaterialSettings.bInfraredOn = false;
						InfraredMaterialSettings.InfraredMPC->SetScalarParameterValue(FName("InfraredStrobeOn"), 0.0f);
					}
					else
					{
						InfraredMaterialSettings.bInfraredOn = true;
						InfraredMaterialSettings.InfraredMPC->SetScalarParameterValue(FName("InfraredStrobeOn"), 1.0f);
					}
				}
			}
		}
		else if (InfraredMaterialSettings.bInfraredOn)
		{
			InfraredMaterialSettings.bInfraredOn = false;
			InfraredMaterialSettings.InfraredMPC->SetScalarParameterValue(FName("InfraredStrobeOn"), 0.0f);
		}
	}
}

void USKGCharacterComponent::AddInfraredDevice(AActor* InfraredDevice)
{
	if (IsValid(InfraredDevice) && InfraredDevice->Implements<USKGInfraredInterface>())
	{
		InfraredInfraredDevices.Add(InfraredDevice);
	}
}

void USKGCharacterComponent::SetNightVisionOn(bool bOn)
{
	bNightVisionOn = bOn;
	
	if (InfraredMaterialSettings.InfraredMPC)
	{
		InfraredMaterialSettings.InfraredMPC->SetScalarParameterValue(FName("NightVisionOn"), (float)bNightVisionOn);
	}
	
	for (uint8 i = 0; i < InfraredInfraredDevices.Num(); ++i)
	{
		if (InfraredInfraredDevices[i])
		{
			ISKGInfraredInterface::Execute_HandleIRDevice(InfraredInfraredDevices[i]);
		}
		else
		{
			InfraredInfraredDevices.RemoveAt(i, 1, false);
		}
	}
	
	InfraredInfraredDevices.Shrink();
}

void USKGCharacterComponent::Init(UCameraComponent* CameraComponent, USkeletalMeshComponent* FirstPersonMesh, USkeletalMeshComponent* ThirdPersonMesh)
{
	FPCameraComponent = CameraComponent;
	FPMesh = FirstPersonMesh;
	TPMesh = ThirdPersonMesh;

	if (FPCameraComponent && FPMesh && TPMesh)
	{
		LeanSettings = DefaultLeanSettings;

		OwningPawn = GetOwner<APawn>();

		if (OwningPawn)
		{
			if (OwningPawn->IsLocallyControlled() && !Cast<AAIController>(GetOwningController()))
			{
				AnimationInstance = Cast<USKGCharacterAnimInstance>(FPMesh->GetAnimInstance());
				bLocallyControlled = true;
			}
			else
			{
				AnimationInstance = Cast<USKGCharacterAnimInstance>(TPMesh->GetAnimInstance());
			}
		}

		if (AnimationInstance)
		{
			AnimationInstance->SetCharacterComponent(this);
			OnRep_Flags();
		}

		if (FPCameraComponent)
		{
			UpdateDefaultCameraFOV(FPCameraComponent->FieldOfView);
			if (IsLocallyControlled())
			{
				GetInUseMesh()->SetOwnerNoSee(false);
				if (const APlayerController* PC = GetOwner<APawn>()->GetController<APlayerController>())
				{
					if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
					{
						CameraManager->ViewPitchMax = MaxLookUpAngle;
						CameraManager->ViewPitchMin = -MaxLookDownAngle;
					}
				}
			}
		}

		bInThirdPerson = bIsThirdPersonDefault;

		if (InfraredMaterialSettings.InfraredMaterialParameterCollection)
		{
			InfraredMaterialSettings.InfraredMPC = GetWorld()->GetParameterCollectionInstance(InfraredMaterialSettings.InfraredMaterialParameterCollection);
		}
	
		bIsInitialized = true;
	}
	else
	{
		if (!FPCameraComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("Character Component: CameraComponent invalid. Check name in details panel matches component name"));
		}
		if (!FPMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Character Component: First Person Mesh invalid. Check name in details panel matches component name or bUseSingleMesh is true (if applicable"));
		}
		if (!TPMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Character Component: Third Person Mesh invalid. Check name in details panel matches component name or bUseSingleMesh is true (if applicable"));
		}
	}
}

bool USKGCharacterComponent::Server_SetFirearmPose_Validate(ESKGFirearmPose NewFirearmPose)
{
	return true;
}

void USKGCharacterComponent::Server_SetFirearmPose_Implementation(ESKGFirearmPose NewFirearmPose)
{
	FirearmPose = NewFirearmPose;
	MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, FirearmPose, this);
	OnRep_FirearmPose();
}

void USKGCharacterComponent::StopFirearmPose()
{
	bHighPort = false;
	bLowPort = false;
	FirearmPose = ESKGFirearmPose::None;
	OnRep_FirearmPose();
	if (!HasAuthority())
	{
		Server_SetFirearmPose(FirearmPose);
	}
	else
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, FirearmPose, this);
	}
}

void USKGCharacterComponent::SetFreeLook(bool FreeLook)
{
	if (OwningPawn && AnimationInstance)
	{
		bFreeLook = FreeLook;
		Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
		AnimationInstance->SetFreeLook(bFreeLook);
		OwningPawn->bUseControllerRotationYaw = !bFreeLook;
		if (bFreeLook)
		{
			LookUpDownOffset = OwningPawn->GetControlRotation().Pitch;
		}
		if (HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, Flags, this);
		}
	}
}

bool USKGCharacterComponent::ValidTurn(float AxisValue)
{
	if (bFreeLook && OwningPawn)
	{
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(OwningPawn->GetActorRotation(), GetControlRotation());
		if (Delta.Yaw > MaxLookLeftRight && AxisValue < 0.0f)
		{
			return false;
		}
		if (Delta.Yaw < -MaxLookLeftRight && AxisValue > 0.0f)
		{
			return false;
		}
	}
	return true;
}

bool USKGCharacterComponent::ValidLookUp(float AxisValue)
{
	if (bFreeLook && OwningPawn)
	{
		const FRotator ControlRotation = GetControlRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(OwningPawn->GetActorRotation(), ControlRotation);
		
		if (Delta.Pitch > MaxLookUpDown && AxisValue > 0.0f)
		{
			return false;
		}
		if (Delta.Pitch < -MaxLookUpDown && AxisValue < 0.0f)
		{
			return false;
		}
	}
	return true;
}

void USKGCharacterComponent::SetControlRotation(FRotator NewControlRotation)
{
	if (OwningPawn && OwningPawn->Controller)
	{
		OwningPawn->Controller->SetControlRotation(NewControlRotation);
	}
}

ASKGSight* USKGCharacterComponent::GetCurrentSight() const
{
	if (IsValid(HeldActor) && HeldActor->Implements<USKGFirearmPartsInterface>())
	{
		return ISKGFirearmPartsInterface::Execute_GetCurrentSight(HeldActor);
	}
	return nullptr;
}

void USKGCharacterComponent::SetCameraFOV(const float NewFOV) const
{
	GetCameraComponent()->SetFieldOfView(NewFOV);
	OnCameraFOVChanged.Broadcast(NewFOV);
}

void USKGCharacterComponent::OnRep_FirearmPose() const
{
	if (AnimationInstance)
	{
		if (IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
		{
			ISKGProceduralAnimationInterface::Execute_StoreCurrentPose(HeldActor, FirearmPose);
		}
		AnimationInstance->SetFirearmPose(FirearmPose);
	}
}

void USKGCharacterComponent::AttachItem(AActor* Actor, const FName SocketName)
{
	if (IsValid(Actor))
	{
		if (USkeletalMeshComponent* AttachToMesh = GetInUseMesh())
		{
			if (AttachToMesh->DoesSocketExist(SocketName))
			{
				Actor->AttachToComponent(AttachToMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
				if (Actor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
				{
					const FTransform GripSocketOffset = ISKGProceduralAnimationInterface::Execute_GetGripSocketOffset(Actor);
					Actor->SetActorRelativeTransform(GripSocketOffset);
				}
				
				GetAnimationInstance()->CycledSights();
				
				if (Actor->Implements<USKGFirearmInterface>())
				{
					ISKGFirearmInterface::Execute_SetCharacterComponent(Actor, this);
				}
			}
			else
			{
				#if WITH_EDITOR
				const FString ErrorString = FString::Printf(TEXT("Socket: %s does not exist on mesh: %s"), *SocketName.ToString(), *AttachToMesh->GetName());
				USKGFPSStatics::PrintError(this, ErrorString);
				#endif
			}
			AttachmentAttempt = 0;
		}
		else
		{
			if (++AttachmentAttempt <= MaxFirearmAttachmentAttempts)
			{
				FTimerHandle TTempHandle;
				FTimerDelegate TimerDelegate;
				TimerDelegate.BindUFunction(this, FName("AttachItem"), Actor, SocketName);
				GetWorld()->GetTimerManager().SetTimer(TTempHandle, TimerDelegate, FirearmReAttachmentAttemptInterval, false);
			}
		}
	}
}

bool USKGCharacterComponent::Server_ClearCurrentHeldActor_Validate()
{
	return true;
}

void USKGCharacterComponent::Server_ClearCurrentHeldActor_Implementation()
{
	ClearCurrentHeldActor();
}

void USKGCharacterComponent::ClearCurrentHeldActor()
{
	AActor* OldAimingActor = HeldActor;
	HeldActor = nullptr;
	HeldActor = nullptr;
	OnRep_HeldActor(OldAimingActor);
	if (!HasAuthority())
	{
		Server_ClearCurrentHeldActor();
	}
	else
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, HeldActor, this);
	}
}

void USKGCharacterComponent::SetFirearmPose(ESKGFirearmPose Pose, bool bSkipNone, bool bGoToNoneIfEqual)
{
	bool bPoseChanged = false;
	if (FirearmPose != Pose)
	{
		if (FirearmPose == ESKGFirearmPose::None || bSkipNone)
		{
			FirearmPose = Pose;
		}
		else
		{
			FirearmPose = ESKGFirearmPose::None;
		}
		bPoseChanged = true;
	}
	else if (bGoToNoneIfEqual)
	{
		FirearmPose = ESKGFirearmPose::None;
		bPoseChanged = true;
	}

	if (bPoseChanged)
	{
		OnRep_FirearmPose();
		if (!HasAuthority())
		{
			Server_SetFirearmPose(FirearmPose);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, FirearmPose, this);
		}
	}
}

void USKGCharacterComponent::EquipHeldActor(AActor* INHeldActor)
{
	AActor* OldHeldActor = HeldActor;
	if (IsValid(HeldActor) && HeldActor->Implements<USKGFirearmInterface>() && !IsValid(INHeldActor))
	{
		ISKGFirearmInterface::Execute_SetCharacterComponent(HeldActor, nullptr);
	}
	if (bAutoSetOwnerOnEquipHeldActor && HasAuthority() && GetOwner() && INHeldActor)
	{
		INHeldActor->SetOwner(GetOwner());
	}
	HeldActor = INHeldActor;
	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, HeldActor, this);
	}
	OnRep_HeldActor(OldHeldActor);
}

void USKGCharacterComponent::OnRep_HeldActor(AActor* OldActor)
{
	if (IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
	{
		if (HeldActor->Implements<USKGFirearmInterface>())
		{
			ISKGFirearmInterface::Execute_SetCharacterComponent(HeldActor, this);
			ISKGFirearmInterface::Execute_Equip(HeldActor);
		}
		if (bAutoAttachOnEquipHeldActor)
		{
			AttachItem(HeldActor, ISKGProceduralAnimationInterface::Execute_GetGripSocketName(HeldActor));
		}
		if (LeanSettings.bAllowOverrideFromAimingActor)
		{
			LeanSettings.LeanCurves.HandleNewLeanCurveSettings(ISKGProceduralAnimationInterface::Execute_GetLeanCurves(HeldActor));
		}
	}
	else
	{
		HeldActor = nullptr;
		
		if (LeanSettings.bAllowOverrideFromAimingActor)
		{
			LeanSettings.LeanCurves.HandleNewLeanCurveSettings(DefaultLeanSettings.LeanCurves);
		}
	}

	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, HeldActor, this);
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, LeanSettings, this);
	}

	OnHeldActorChanged.Broadcast(OldActor, HeldActor);
}

USkeletalMeshComponent* USKGCharacterComponent::GetInUseMesh() const
{
	if (OwningPawn)
	{
		if (OwningPawn->IsLocallyControlled())
		{
			return FPMesh;
		}
		return TPMesh;
	}

	return TPMesh;
}

UPawnMovementComponent* USKGCharacterComponent::GetMovementComponent() const
{
	if (OwningPawn)
	{
		return OwningPawn->GetMovementComponent();
	}
	return nullptr;
}

template <class T>
T* USKGCharacterComponent::GetMovementComponent() const
{
	if (OwningPawn)
	{
		return Cast<T>(OwningPawn->GetMovementComponent());
	}
	return nullptr;
}

FRotator USKGCharacterComponent::GetBaseAimRotation() const
{
	if (OwningPawn)
	{
		FRotator Rotation = OwningPawn->GetBaseAimRotation();
		Rotation.Yaw = ControlYaw;
		return Rotation;
	}
	return FRotator::ZeroRotator;
}

FRotator USKGCharacterComponent::GetControlRotation() const
{
	if (OwningPawn)
	{
		return OwningPawn->GetControlRotation();
	}
	return FRotator::ZeroRotator;
}

void USKGCharacterComponent::AddControlRotation(const FRotator& Rotation)
{
	if (OwningPawn)
	{
		OwningPawn->AddControllerPitchInput(Rotation.Pitch);
		OwningPawn->AddControllerYawInput(Rotation.Yaw);
	}
}

AController* USKGCharacterComponent::GetOwningController() const
{
	if (OwningPawn)
	{
		return OwningPawn->GetController();
	}
	return nullptr;
}

const FName& USKGCharacterComponent::GetCameraSocket() const
{
	if (bUseParentSocketForAiming)
	{
		return CameraSocketParentBone;
	}
	return CameraSocket;
}

bool USKGCharacterComponent::Server_SetSprinting_Validate(ESKGSprintType NewSprintType)
{
	return true;
}

void USKGCharacterComponent::Server_SetSprinting_Implementation(ESKGSprintType NewSprintType)
{
	SprintType = NewSprintType;
	MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, SprintType, this);
}

void USKGCharacterComponent::SetSprinting(ESKGSprintType NewSprintType)
{
	SprintType = NewSprintType;
	if (!bAllowSuperSprint && SprintType == ESKGSprintType::SuperSprint)
	{
		SprintType = ESKGSprintType::Sprint;
	}
	if (USKGCharacterMovementComponent* MovementComponent = GetMovementComponent<USKGCharacterMovementComponent>())
	{
		MovementComponent->SetSprinting(SprintType);
	}
	if (!HasAuthority())
	{
		Server_SetSprinting(SprintType);
	}
	else
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, SprintType, this);
	}
}

FVector USKGCharacterComponent::GetActorForwardVector() const
{
	if (OwningPawn)
	{
		return OwningPawn->GetActorForwardVector();
	}
	return FVector::ZeroVector;
}

FVector USKGCharacterComponent::GetActorRightVector() const
{
	if (OwningPawn)
	{
		return OwningPawn->GetActorRightVector();
	}
	return FVector::ZeroVector;
}

void USKGCharacterComponent::RagdollCharacter()
{
	USKGFPSStatics::Ragdoll(FPMesh);
	USKGFPSStatics::Ragdoll(TPMesh);
}

void USKGCharacterComponent::RagdollCharacterWithForce(const FVector ImpactLocation, const float ImpactForce)
{
	USKGFPSStatics::RagdollWithImpact(FPMesh, ImpactLocation, ImpactForce);
	USKGFPSStatics::RagdollWithImpact(TPMesh, ImpactLocation, ImpactForce);
}

void USKGCharacterComponent::SetThirdPersonView(bool bThirdPerson)
{
	bInThirdPerson = bThirdPerson;
	OnRep_FirearmPose();
}

void USKGCharacterComponent::OnRep_LeanSettings()
{
	if (AnimationInstance)
	{
		AnimationInstance->SetLeaning(LeanSettings.ReplicatedLeanCurveTime);
	}
}

void USKGCharacterComponent::SetLeaning(bool bIncremental)
{
	if (GetAnimationInstance())
	{
		if (!bIncremental)
		{
			switch (LeanSettings.CurrentLean)
			{
			case ESKGLeaning::None : LeanSettings.CurrentLeanCurveTime = 0.0f; break;
			case ESKGLeaning::Left : LeanSettings.CurrentLeanCurveTime = -LeanSettings.CurrentLeanCurveTime; break;
			case ESKGLeaning::Right : break;
			}
			LeanSettings.ReplicatedLeanCurveTime = LeanSettings.CurrentLeanCurveTime;
		}
		else
		{
			LeanSettings.ReplicatedLeanCurveTime = LeanSettings.CurrentIncrementalLean;
		}
		
		AnimationInstance->SetLeaning(LeanSettings.ReplicatedLeanCurveTime);
		if (!HasAuthority())
		{
			Server_SetLean(LeanSettings.ReplicatedLeanCurveTime, bIncremental);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, LeanSettings, this);
		}
	}
}

FSKGLeanSettings USKGCharacterComponent::GetLeanSettings()
{
	if (LeanSettings.bAllowOverrideFromAimingActor && IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
	{
		LeanSettings.LeanCurves.HandleNewLeanCurveSettings(ISKGProceduralAnimationInterface::Execute_GetLeanCurves(HeldActor));
	}
	return LeanSettings;
}

ESKGLeaning USKGCharacterComponent::GetCurrentLeanDirection()
{
	if (LeanSettings.CurrentLeanCurveTime > 0.0f)
	{
		return ESKGLeaning::Right;
	}
	if (LeanSettings.CurrentLeanCurveTime < 0.0f)
	{
		return ESKGLeaning::Left;
	}
	return ESKGLeaning::None;
}

void USKGCharacterComponent::LeanLeft()
{
	bLeanLeftDown = true;
	LeanSettings.CurrentIncrementalLean = 0.0f;
	LeanSettings.bIncrementalLeaning = false;
	float LeanCurveTime = LeanSettings.LeanCurveTimeLeft;
	if (LeanSettings.CurrentLean == ESKGLeaning::Right)
	{
		LeanSettings.CurrentLean = ESKGLeaning::None;
	}
	else
	{
		LeanSettings.CurrentLean = ESKGLeaning::Left;
		LeanCurveTime = LeanSettings.LeanCurveTimeLeft;
	}

	LeanSettings.CurrentLeanCurveTime = LeanCurveTime;
	SetLeaning(false);
}

void USKGCharacterComponent::LeanRight()
{
	bLeanRightDown = true;
	LeanSettings.CurrentIncrementalLean = 0.0f;
	LeanSettings.bIncrementalLeaning = false;
	float LeanCurveTime = LeanSettings.LeanCurveTimeLeft;
	if (LeanSettings.CurrentLean == ESKGLeaning::Left)
	{
		LeanSettings.CurrentLean = ESKGLeaning::None;
	}
	else
	{
		LeanSettings.CurrentLean = ESKGLeaning::Right;
		LeanCurveTime = LeanSettings.LeanCurveTimeRight;
	}
	
	LeanSettings.CurrentLeanCurveTime = LeanCurveTime;
	SetLeaning(false);
}

void USKGCharacterComponent::StopLeaning()
{
	bLeanLeftDown = false;
	bLeanRightDown = false;
	LeanSettings.bIncrementalLeaning = false;
	LeanSettings.CurrentLean = ESKGLeaning::None;
	
	LeanSettings.CurrentLeanCurveTime = 0.0f;
	SetLeaning(false);
}

void USKGCharacterComponent::StopLeanLeft()
{
	bLeanLeftDown = false;
	LeanSettings.bIncrementalLeaning = false;
	float LeanCurveTime = LeanSettings.LeanCurveTimeLeft;
	if (bLeanRightDown)
	{
		LeanSettings.CurrentLean = ESKGLeaning::Right;
		LeanCurveTime = LeanSettings.LeanCurveTimeRight;
	}
	else
	{
		LeanSettings.CurrentLean = ESKGLeaning::None;
	}
	
	LeanSettings.CurrentLeanCurveTime = LeanCurveTime;
	SetLeaning(false);
}

void USKGCharacterComponent::StopLeanRight()
{
	bLeanRightDown = false;
	LeanSettings.bIncrementalLeaning = false;
	float LeanCurveTime = LeanSettings.LeanCurveTimeRight;
	if (bLeanLeftDown)
	{
		LeanSettings.CurrentLean = ESKGLeaning::Left;
		LeanCurveTime = LeanSettings.LeanCurveTimeLeft;
	}
	else
	{
		LeanSettings.CurrentLean = ESKGLeaning::None;
	}

	LeanSettings.CurrentLeanCurveTime = LeanCurveTime;
	SetLeaning(false);
}

void USKGCharacterComponent::IncrementalLean(ESKGLeaning LeanDirection)
{
	const float PreviousAngle = LeanSettings.CurrentIncrementalLean;
	LeanSettings.bIncrementalLeaning = true;
	switch (LeanDirection)
	{
	case ESKGLeaning::Left: LeanSettings.CurrentIncrementalLean -= LeanSettings.IncrementalLeanCurveTimeAmount;  break;
	case ESKGLeaning::Right: LeanSettings.CurrentIncrementalLean += LeanSettings.IncrementalLeanCurveTimeAmount; break;
	case ESKGLeaning::None: ResetLeanAngle();
	}
	
	if (LeanSettings.CurrentIncrementalLean < 0.0f)
	{
		LeanSettings.CurrentIncrementalLean = FMath::ClampAngle(LeanSettings.CurrentIncrementalLean, -LeanSettings.IncrementalLeanMaxLeft, 0.0f);
	}
	else
	{
		LeanSettings.CurrentIncrementalLean = FMath::ClampAngle(LeanSettings.CurrentIncrementalLean, 0.0f, LeanSettings.IncrementalLeanMaxRight);
	}

	if (PreviousAngle < 0.0f)
	{
		if (LeanSettings.CurrentIncrementalLean > 0.0f)
		{
			LeanSettings.CurrentIncrementalLean = 0.0f;
		}
	}
	else if (PreviousAngle > 0.0f)
	{
		if (LeanSettings.CurrentIncrementalLean < 0.0f)
		{
			LeanSettings.CurrentIncrementalLean = 0.0f;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("LeanSettings.CurrentIncrementalLean: %f"), LeanSettings.CurrentIncrementalLean);
	
	if (LeanSettings.CurrentIncrementalLean > LeanSettings.CurrentLeanCurveTime && LeanSettings.CurrentLeanCurveTime != 0.0f)
	{
		LeanSettings.CurrentIncrementalLean = LeanSettings.CurrentLeanCurveTime;
	}
	else if (LeanSettings.CurrentIncrementalLean < -LeanSettings.CurrentLeanCurveTime && LeanSettings.CurrentLeanCurveTime != 0.0f)
	{
		LeanSettings.CurrentIncrementalLean = -LeanSettings.CurrentLeanCurveTime;
	}

	LeanSettings.ReplicatedLeanCurveTime = LeanSettings.CurrentIncrementalLean;
	SetLeaning(true);
}

void USKGCharacterComponent::ResetLeanAngle()
{
	LeanSettings.CurrentIncrementalLean = 0.0f;
	LeanSettings.ReplicatedLeanCurveTime = 0.0f;
	LeanSettings.bIncrementalLeaning = false;
	if (GetAnimationInstance())
	{
		AnimationInstance->SetLeaning(LeanSettings.CurrentIncrementalLean);
		if (!HasAuthority())
		{
			Server_SetLean(LeanSettings.CurrentIncrementalLean, false);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, LeanSettings, this);
		}
	}
}

float USKGCharacterComponent::GetLeanSpeed(bool bGetEndLeanSpeed) const
{
	if (LeanSettings.bIncrementalLeaning)
	{
		return LeanSettings.DefaultIncrementalLeanSpeed;
	}

	return bGetEndLeanSpeed ? LeanSettings.LeanCurves.DefaultEndLeanSpeed : LeanSettings.LeanCurves.DefaultLeanSpeed;
}

void USKGCharacterComponent::SetMaxLookUpAngle(const float Angle)
{
	if (Angle >= 0.0f)
	{
		if (const APlayerController* PC = GetOwner<APawn>()->GetController<APlayerController>())
		{
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				CameraManager->ViewPitchMax = Angle;
			}
		}
	}
}

void USKGCharacterComponent::SetMaxLookDownAngle(const float Angle)
{
	if (Angle >= 0.0f)
	{
		if (const APlayerController* PC = GetOwner<APawn>()->GetController<APlayerController>())
		{
			if (APlayerCameraManager* CameraManager = PC->PlayerCameraManager)
			{
				CameraManager->ViewPitchMin = -Angle;
			}
		}
	}
}

bool USKGCharacterComponent::Server_SetLean_Validate(float CurveTime, bool bIncremental)
{
	return true;
}

void USKGCharacterComponent::Server_SetLean_Implementation(float CurveTime, bool bIncremental)
{
	LeanSettings.ReplicatedLeanCurveTime = CurveTime;
	LeanSettings.bIncrementalLeaning = bIncremental;
	MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, LeanSettings, this);
	OnRep_LeanSettings();
}

bool USKGCharacterComponent::Server_SetUseLeftHandIK_Validate(bool bUse)
{
	return true;
}

void USKGCharacterComponent::Server_SetUseLeftHandIK_Implementation(bool bUse)
{
	SetUseLeftHandIK(bUse);
}

void USKGCharacterComponent::SetUseLeftHandIK(bool bUse)
{
	if (bUseLeftHandIK != bUse)
	{
		bUseLeftHandIK = bUse;
		Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
		OnRep_Flags();
		if (!HasAuthority())
		{
			Server_SetUseLeftHandIK(bUseLeftHandIK);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, Flags, this);
		}
	}
}

bool USKGCharacterComponent::Server_SetUseLeftHandTwoBoneIK_Validate(bool bUse)
{
	return true;
}

void USKGCharacterComponent::Server_SetUseLeftHandTwoBoneIK_Implementation(bool bUse)
{
	SetUseLeftHandTwoBoneIK(bUse);
}

void USKGCharacterComponent::SetUseLeftHandTwoBoneIK(bool bUse)
{
	if (bUseLeftHandTwoBoneIK != bUse)
	{
		bUseLeftHandTwoBoneIK = bUse;
		Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
		OnRep_Flags();
		if (!HasAuthority())
		{
			Server_SetUseLeftHandTwoBoneIK(bUseLeftHandTwoBoneIK);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, Flags, this);
		}
	}
}

void USKGCharacterComponent::UpdateDefaultCameraFOV(const float FOV, const bool bChangeCameraToNewFOV)
{
	if (IsValid(FPCameraComponent) && FOV > 0.0f)
	{
		if (bChangeCameraToNewFOV)
		{
			FPCameraComponent->SetFieldOfView(FOV);
		}
		DefaultCameraFOV = FOV;
	}
}

void USKGCharacterComponent::PlayCameraShake(TSubclassOf<UCameraShakeBase> CameraShake, float Scale) const
{
	if (OwningPawn && CameraShake)
	{
		if (APlayerController* PC = OwningPawn->GetController<APlayerController>())
		{
			PC->ClientStartCameraShake(CameraShake, Scale);
		}
	}
}

float USKGCharacterComponent::GetMagnificationSensitivity() const
{
	if (bIsAiming && IsValid(HeldActor))
	{
		if (HeldActor->Implements<USKGSightInterface>())
		{
			return 1.0f / ISKGSightInterface::Execute_GetCurrentMagnification(HeldActor);
		}
	}
	return 1.0f;
}

float USKGCharacterComponent::GetMagnificationSensitivityStartValue(float StartAtMagnification) const
{
	if (bIsAiming)
	{
		if (HeldActor->Implements<USKGSightInterface>())
		{
			const float Magnification = ISKGSightInterface::Execute_GetCurrentMagnification(HeldActor);
			if (Magnification > StartAtMagnification)
			{
				return 1.0f / Magnification;
			}
		}
	}
	return 1.0f;
}

void USKGCharacterComponent::StartAiming()
{
	if (!IsValid(HeldActor))
	{
		return;
	}
	
	bIsAiming = true;
	Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
	if (HeldActor->Implements<USKGAimInterface>())
	{
		if (HeldActor->Implements<USKGFirearmPartsInterface>())
		{
			ISKGFirearmPartsInterface::Execute_ActivateCurrentSight(HeldActor, true);
		}
		else if (HeldActor->Implements<USKGRenderTargetInterface>())
		{
			ISKGRenderTargetInterface::Execute_DisableRenderTarget(HeldActor, false);
		}
	}
	if (AnimationInstance)
	{
		AnimationInstance->SetIsAiming(bIsAiming);
		AnimationInstance->SetHeadAiming(bIsAiming);
	}
	if (!HasAuthority())
	{
		Server_SetAiming(bIsAiming);
	}
	else
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, Flags, this);
	}
}

void USKGCharacterComponent::StopAiming()
{
	if (IsValid(HeldActor))
	{
		bIsAiming = false;
		Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
		if (HeldActor->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
		{
			if (HeldActor->Implements<USKGFirearmPartsInterface>())
			{
				ISKGFirearmPartsInterface::Execute_ActivateCurrentSight(HeldActor, false);
			}
			else if (HeldActor->Implements<USKGRenderTargetInterface>())
			{
				ISKGRenderTargetInterface::Execute_DisableRenderTarget(HeldActor, true);
			}
		}
		if (AnimationInstance)
		{
			AnimationInstance->SetIsAiming(bIsAiming);
			AnimationInstance->SetHeadAiming(bIsAiming);
		}
		if (!HasAuthority())
		{
			Server_SetAiming(bIsAiming);
		}
		else
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, Flags, this);
		}
	}
}

bool USKGCharacterComponent::Server_SetAiming_Validate(bool IsAiming)
{
	return true;
}

void USKGCharacterComponent::Server_SetAiming_Implementation(bool IsAiming)
{
	bIsAiming = IsAiming;
	Flags = (bIsAiming << 0)|(bUseLeftHandIK << 1)|(bFreeLook << 2)|(bUseLeftHandTwoBoneIK << 3);
	MARK_PROPERTY_DIRTY_FROM_NAME(USKGCharacterComponent, Flags, this);
	OnRep_Flags();
}
