//Copyright 2021, Dakota Dawe, All rights reserved

#include "SKGCharacterAnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Actors/SKGFirearm.h"
#include "Components/SKGCharacterComponent.h"
#include "Interfaces/SKGFirearmInterface.h"

#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Curves/CurveVector.h"
#include "DataTypes/SKGFPSFrameworkMacros.h"
#include "GameFramework/PawnMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Components/SkeletalMeshComponent.h"

DECLARE_CYCLE_STAT(TEXT("SKGAnimNativeUpdateEntire"), STAT_SKGNativeUpdateEntire, STATGROUP_SKGAnimInstance);
DECLARE_CYCLE_STAT(TEXT("FirearmCollision"), STAT_SKGFirearmCollision, STATGROUP_SKGAnimInstance);
DECLARE_CYCLE_STAT(TEXT("SKGRecoil"), STAT_SKGRecoil, STATGROUP_SKGAnimInstance);

USKGCharacterAnimInstance::USKGCharacterAnimInstance()
{
	bRunOnDedicatedServer = true;
	
	AimInterpolationSpeed = 20.0f;
	HeadAimingInterpolationSpeed = 6.0f;
	CycleSightsInterpolationSpeed = 20.0f;
	RotationLagResetInterpolationSpeed = 20.0f;
	MovementLagResetInterpolationSpeed = 20.0f;
	CurveChangeInterpSpeed = 3.5f;
	CurveChangeEqualTolerance = 1.0f;

	FirearmCollisionVirtualBoneName = FName("VB FirearmCollisionGrip");

	bUseProceduralSpine = true;
	SpineBlendPercents = { 1.0f };
	
	bInterpRelativeToHand = false;
	bFirstRun = true;
	bIsAiming = false;
	bIsHeadAiming = false;
	AimingAlpha = 0.0f;
	LeftHandIKAlpha = 0.0f;
	LeftHandTwoBoneIKAlpha = 0.0f;
	RotationAlpha = false;
	bInterpAiming = false;
	RightHandBone = FName("hand_r");
	
	HeadAimingRotation = FRotator(45.0f, 0.0f, 0.0f);

	SightDistance = 0.0f;
	CurveTimer = 1.0f;
	bCustomizingFirearm = false;
	bCanAim = true;
	bInterpHeldActorPose = false;
	FirearmCollisionAlpha = 0.0f;
	bValidLeftHandPose = false;
	bInvertRotationLag = false;
	bSprinting = false;
	
	SwayMultiplier = 1.0f;

	bIsLocallyControlled = false;

	FreeLookRotation = FRotator(90.0f, 0.0f, 0.0f);

	bInterpRecoil = false;
	bInterpShakeCurve = false;
	bCanPlayShakeCurve = true;
	
	ShakeCurveAlpha = 0.0f;

	CustomPoseAlpha = 0.0f;

	CustomCurveStartTime = 0.0f;
	CustomCurveAlpha = 0.0f;
	bInterpCustomCurve = false;

	bEquipped = false;

	bIndexOrTagForced = false;

	bApplyForwardVector = false;
}

void USKGCharacterAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	
	if (const AActor* OwningActor = GetOwningActor())
	{
		CharacterComponent = Cast<USKGCharacterComponent>(OwningActor->GetComponentByClass(TSubclassOf<USKGCharacterComponent>()));
		if (IsValid(CharacterComponent))
		{
			bIsLocallyControlled = CharacterComponent->IsLocallyControlled();
		}
	}
}

void USKGCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	SCOPE_CYCLE_COUNTER(STAT_SKGNativeUpdateEntire);
	
	if ((!bRunOnDedicatedServer && UKismetSystemLibrary::IsDedicatedServer(GetWorld())) || !CharacterComponent || bCustomizingFirearm) { return; }

	FVector FPSVelocity = CharacterComponent->GetOwner()->GetVelocity();
	FPSVelocity.Z = 0.0f;
	CharacterVelocity = FPSVelocity.Size();

	CharacterDirection = UKismetAnimationLibrary::CalculateDirection(FPSVelocity, CharacterComponent->GetOwner()->GetActorRotation());

	HeldActor = CharacterComponent->GetHeldActor();
	HandleHeldActor(DeltaSeconds);
	HandleFirearmCollision(DeltaSeconds);
	HandleSpine(DeltaSeconds);

	if (bInterpRecoil)
	{
		RecoilInterpTo(DeltaSeconds);
	}

	AdditiveLocation = SwayLocation + RotationLagLocation;
	AdditiveRotation = SwayRotation + RotationLagRotation + MovementLagRotation;
}

void USKGCharacterAnimInstance::HandleHeldActor(float DeltaSeconds)
{
	if (IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
	{
		SetEssentials(DeltaSeconds);
		HandleLeftHandIK();
		HandleSprinting();
	
		if (bInterpHeldActorPose)
		{
			InterpHeldActorPose(DeltaSeconds);
		}

		if (bInterpShakeCurve)
		{
			InterpShakeCurve(DeltaSeconds);
		}
		
		if (bInterpAiming)
		{
			InterpAimingAlpha(DeltaSeconds);
		}
		if (bInterpHeadAimingAlpha)
		{
			InterpHeadAimingAlpha(DeltaSeconds);
		}
		
		if (CharacterComponent->IsLocallyControlled())
		{
			if (bInterpCameraZoom)
			{
				InterpCameraZoom(DeltaSeconds);
			}

			const UPawnMovementComponent* MovementComponent = CharacterComponent->GetMovementComponent();
			if ((MovementComponent && MovementComponent->Velocity.Size()) || (!MovementLagRotation.Equals(FRotator::ZeroRotator)))
			{
				SetMovementLag(DeltaSeconds);
			}

			SetRotationLag(DeltaSeconds);
			HandleMovementSway(DeltaSeconds);
		}

		if (bInterpCustomCurve)
		{
			InterpCustomCurve(DeltaSeconds);
		}
	}
	else
	{
		bValidLeftHandPose = false;
		LeftHandIKAlpha = 0.0f;
	}
}

void USKGCharacterAnimInstance::HandleLeftHandIK()
{
	if (CharacterComponent->GetUseLeftHandIK())
	{	// Handles Left Hand IK and Pose
		const FSKGLeftHandIKData LeftHandIKData = ISKGProceduralAnimationInterface::Execute_GetLeftHandIKData(HeldActor);
		LeftHandPose = LeftHandIKData.GripAnimation;
		if (LeftHandPose)
		{
			bValidLeftHandPose = true;
		}
		else
		{
			bValidLeftHandPose = false;
		}
		
		if (CharacterComponent->GetUseLeftHandIK() && LeftHandIKData.bUseLeftHandIK)
		{
			SetLeftHandIK();
		}
		else
		{
			LeftHandIKAlpha = 0.0f;
		}
	}
}

void USKGCharacterAnimInstance::HandleSpine(float DeltaSeconds)
{
	if (bUseProceduralSpine && !bFreeLook)
	{
		if (!IsLocallyControlled())
		{
			SpineToInterpTo = CharacterComponent->GetBaseAimRotation();
			SpineToInterpTo = UKismetMathLibrary::NormalizedDeltaRotator(SpineToInterpTo, TryGetPawnOwner()->GetActorRotation());
			SpineToInterpTo.Roll = (SpineToInterpTo.Pitch * -1.0f);
			SpineToInterpTo.Pitch = 0.0f;
			SpineToInterpTo.Yaw = 0.0f;
			FRotator TempRotator = SpineToInterpTo;
			for (uint8 i = 0; i < SpineBlendPercents.Num(); ++i)
			{
				TempRotator.Roll = SpineToInterpTo.Roll * SpineBlendPercents[i];
				switch (i)
				{
				case 0: Spine0Rotation = UKismetMathLibrary::RInterpTo(Spine0Rotation, TempRotator, DeltaSeconds, 10.0f); break;
				case 1: Spine1Rotation = UKismetMathLibrary::RInterpTo(Spine1Rotation, TempRotator, DeltaSeconds, 10.0f); break;
				case 2: Spine2Rotation = UKismetMathLibrary::RInterpTo(Spine2Rotation, TempRotator, DeltaSeconds, 10.0f); break;
				case 3: Spine3Rotation = UKismetMathLibrary::RInterpTo(Spine3Rotation, TempRotator, DeltaSeconds, 10.0f); break;
				case 4: Spine4Rotation = UKismetMathLibrary::RInterpTo(Spine4Rotation, TempRotator, DeltaSeconds, 10.0f); break;
				default: checkNoEntry(); break;
				}
			}
		}
		else
		{
			SpineToInterpTo = CharacterComponent->GetControlRotation();
			SpineToInterpTo = UKismetMathLibrary::NormalizedDeltaRotator(SpineToInterpTo, TryGetPawnOwner()->GetActorRotation());
			SpineToInterpTo.Roll = (SpineToInterpTo.Pitch * -1.0f);
			SpineToInterpTo.Pitch = 0.0f;
			SpineToInterpTo.Yaw = 0.0f;
			FRotator TempRotator = SpineToInterpTo;
			for (uint8 i = 0; i < SpineBlendPercents.Num(); ++i)
			{
				TempRotator.Roll = SpineToInterpTo.Roll * SpineBlendPercents[i];
				switch (i)
				{
				case 0: Spine0Rotation = TempRotator; break;
				case 1: Spine1Rotation = TempRotator; break;
				case 2: Spine2Rotation = TempRotator; break;
				case 3: Spine3Rotation = TempRotator; break;
				case 4: Spine4Rotation = TempRotator; break;
				default: checkNoEntry(); break;
				}
			}
		}
	}

	if (bInterpLeaning)
	{
		InterpLeaning(DeltaSeconds);
	}
}

void USKGCharacterAnimInstance::HandleFirearmCollision(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_SKGFirearmCollision);
	if (IsValid(HeldActor) && IsValid(CharacterComponent) && CharacterComponent->IsUsingFirearmCollision())
	{
		if (HeldActor->GetClass()->ImplementsInterface(USKGFirearmCollisionInterface::StaticClass()))
		{
			// UPDATE THIS IN CHARACTER COMPONENT UPON AIMING ACTOR CHANGED
			FTransform FirearmGripOffset = CharacterComponent->GetInUseMesh()->GetSocketTransform(ISKGFirearmCollisionInterface::Execute_GetFirearmGripSocket(HeldActor), RTS_ParentBoneSpace);
			FirearmCollisionBoneLocationOffset = FirearmGripOffset.GetLocation();
			FirearmCollisionBoneRotationOffset = FirearmGripOffset.Rotator();
			CurrentCollisionSettings = ISKGFirearmCollisionInterface::Execute_GetCollisionSettings(HeldActor);
		
			const FTransform MuzzleTransform = ISKGFirearmCollisionInterface::Execute_GetCollisionMuzzleSocketTransform(HeldActor);
			FTransform RelativeMuzzle = MuzzleTransform.GetRelativeTransform(HeldActor->GetActorTransform());
			FSKGCollisionSettings CollisionSettings = ISKGFirearmCollisionInterface::Execute_GetCollisionSettings(HeldActor);
			RelativeMuzzle.SetLocation(RelativeMuzzle.GetLocation() + CollisionSettings.MuzzlePositionOffset);
		
			float MuzzleDistanceToRoot = RelativeMuzzle.GetLocation().Size();

			FTransform EndTransform = RelativeMuzzle *  CharacterComponent->GetInUseMesh()->GetSocketTransform(FirearmCollisionVirtualBoneName, RTS_World);
		
			FVector End = (EndTransform).GetTranslation();
			FVector Start = End + (EndTransform.Rotator().Vector() * -1.0f) * MuzzleDistanceToRoot + CollisionSettings.TraceStartOffset;
		
			FHitResult HitResult;
			// Needs refactors to be set upon changes to the given firearm
			TArray<AActor*> IgnoredActors;
			IgnoredActors.Append(ISKGFirearmCollisionInterface::Execute_GetPartsToIgnore(HeldActor));
			IgnoredActors.Add(CharacterComponent->GetOwner());
			IgnoredActors.Add(HeldActor);
			
			const bool bHit = UKismetSystemLibrary::SphereTraceSingle(GetWorld(), Start, End, CurrentCollisionSettings.TraceRadius, UEngineTypes::ConvertToTraceType(CharacterComponent->GetFirearmCollisionChannel()), false, IgnoredActors, CurrentCollisionSettings.DebugTrace, HitResult, true);

			if (bHit)
			{
#if WITH_EDITOR
				if (CurrentCollisionSettings.bPrintTraceHit)
				{
					if (AActor* HitActor = HitResult.GetActor())
					{
						FString DebugString = FString("Hit Actor: " + HitActor->GetName());
						if (UPrimitiveComponent* HitComponent = HitResult.GetComponent())
						{
							DebugString.Append(FString("    Hit Component: ") + HitComponent->GetName());
						}
						UKismetSystemLibrary::PrintString(GetWorld(), DebugString);
					}
				}
#endif
				FirearmMovedDistance = FVector::Distance(End, HitResult.Location);
				
				if (FirearmMovedDistance < CurrentCollisionSettings.DistanceBeforePush && ShortStockBlend == 0.0f)
				{
					return;
				}
				
				if (FirearmCollisionAlpha == 0.0f)
				{
					ISKGFirearmCollisionInterface::Execute_OnCollisionStart(HeldActor);
				}
				
				FirearmCollisionAlpha = 1.0f;
				
				if (HeldActorPose != ESKGFirearmPose::None) // CHECK IF IN SHORT STOCK POSE???
				{
					if (bIsAiming && FirearmMovedDistance > CurrentCollisionSettings.PushDistanceToStopAiming)
					{
						SetFirearmPose(ESKGFirearmPose::None);
						SetIsAiming(false);
					}
					else
					{
						if (!bIsAiming && CharacterComponent->IsAiming())
						{
							SetIsAiming(true);
						}
					}
					CollisionPose = FTransform();
					const FVector Pose = FVector (0.0f, -FirearmMovedDistance, 0.0f);

					FTransform CurrentTransform = FTransform(FirearmCollisionRotation, FirearmCollisionLocation);
					const FTransform InterpToTransform = FTransform(FRotator::ZeroRotator, Pose);
					CurrentTransform = UKismetMathLibrary::TInterpTo(CurrentTransform, InterpToTransform, DeltaSeconds, CurrentCollisionSettings.InterpolationSpeed);

					FirearmCollisionLocation = CurrentTransform.GetLocation();
					FirearmCollisionRotation = CurrentTransform.Rotator();
				}
				else
				{
					SetFirearmPose(ESKGFirearmPose::None);
					const FTransform ShortStockPose = ISKGFirearmCollisionInterface::Execute_GetCollisionShortStockPose(HeldActor);
					float MaxPush = -ShortStockPose.GetLocation().Y;
					ShortStockBlend = UKismetMathLibrary::NormalizeToRange(FirearmMovedDistance, 0.0f, MaxPush);

					if (FirearmMovedDistance > CurrentCollisionSettings.PushDistanceToStopAiming)
					{
						if (bIsAiming)
						{
							SetIsAiming(false);
						}
					}
					else
					{
						if (!bIsAiming && CharacterComponent->IsAiming())
						{
							SetIsAiming(true);
						}
					}
				
					if (ShortStockBlend < CurrentCollisionSettings.ShortStockMaxDistance)
					{
						const FTransform ShortStockLerpped = UKismetMathLibrary::TLerp(FTransform(), ShortStockPose, ShortStockBlend);
						CollisionPose = UKismetMathLibrary::TInterpTo(CollisionPose, ShortStockLerpped, DeltaSeconds, CurrentCollisionSettings.InterpolationSpeed);
					}
					else
					{
						CollisionPose = ISKGFirearmCollisionInterface::Execute_GetCollisionPose(HeldActor);
						const FTransform CurrentPoseTransform = FTransform(FirearmCollisionRotation, FirearmCollisionLocation);
						CollisionPose = UKismetMathLibrary::TInterpTo(CurrentPoseTransform, CollisionPose, DeltaSeconds, CurrentCollisionSettings.InterpolationSpeed);

						if (CollisionPose.GetLocation().Y < FirearmMovedDistance)
						{
							FVector Correct = CollisionPose.GetLocation();
							Correct.Y = -FirearmMovedDistance - (CollisionPose.GetLocation().Y / CurrentCollisionSettings.CollisionPosePushMultiplier);

							if (Correct.Y < CurrentCollisionSettings.MaxCollisionPush)
							{
								Correct.Y = CurrentCollisionSettings.MaxCollisionPush;
							}
							CollisionPose.SetLocation(Correct);
						}
					}

					FirearmCollisionLocation = CollisionPose.GetLocation();
					FirearmCollisionRotation = CollisionPose.Rotator();
				}
			}
			else if (FirearmCollisionAlpha == 1.0f)
			{
				ISKGFirearmCollisionInterface::Execute_OnCollisionEnd(HeldActor);
				CollisionPose = FTransform();
				FirearmCollisionAlpha = 0.0f;
				ShortStockBlend = 0.0f;
				FirearmMovedDistance = 0.0f;
				if (CharacterComponent->IsAiming())
				{
					SetIsAiming(true);
				}
				else
				{
					SetFirearmPose(CharacterComponent->GetFirearmPose());
				}
			}
			else
			{
				if (!CharacterComponent->IsAiming())
				{
					SetFirearmPose(CharacterComponent->GetFirearmPose());
				}
			}
		}
	}
}

void USKGCharacterAnimInstance::SetEssentials(float DeltaSeconds)
{
	BasePoseOffsetLocation = ISKGProceduralAnimationInterface::Execute_GetBasePoseOffset(HeldActor).GetLocation();
	BasePoseOffsetRotation = ISKGProceduralAnimationInterface::Execute_GetBasePoseOffset(HeldActor).Rotator();

	//if (bInterpRelativeToHand)
	{
		SetRelativeToHand();
		InterpRelativeToHand(DeltaSeconds);
	}
}

void USKGCharacterAnimInstance::InterpLeaning(float DeltaSeconds)
{
	const FSKGLeanCurves LeanGraphs = CharacterComponent->GetLeanSettings().LeanCurves;
	if (LeanGraphs.IsValid())
	{
		const UCurveFloat* GraphToUse = LeanGraphs.LeanCurve;
		bool bEndingLean = false;
		if (!CharacterComponent->GetLeanSettings().bIncrementalLeaning && LeanGraphs.LeanEndCurve)
		{
			if ((GraphTimeToGoTo <= 0.0f && CurrentGraphTime < GraphTimeToGoTo) || (GraphTimeToGoTo >= 0.0f && CurrentGraphTime > GraphTimeToGoTo))
			{
				GraphToUse = LeanGraphs.LeanEndCurve;
				bEndingLean = true;
			}
		}
		
		float InterpSpeed = CharacterComponent->GetLeanSpeed(bEndingLean);
		if (HeldActor && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
		{
			InterpSpeed *= ISKGProceduralAnimationInterface::Execute_GetLeaningSpeedMultiplier(HeldActor);
		}
		CurrentGraphTime = UKismetMathLibrary::FInterpTo_Constant(CurrentGraphTime, GraphTimeToGoTo, DeltaSeconds, InterpSpeed);
		
		float GraphValue = GraphToUse->GetFloatValue(CurrentGraphTime);
		GraphValue = (CurrentGraphTime < 0.0f ? -GraphValue : GraphValue);
		
		GraphValue = UKismetMathLibrary::FInterpTo_Constant(LeanRotation.Pitch, GraphValue, DeltaSeconds, CharacterComponent->GetLeanSettings().LeaningSmoothInterpSpeed);
		
		LeanRotation.Pitch = GraphValue;
		
		if (CurrentGraphTime == GraphTimeToGoTo)
		{
			bInterpLeaning = false;
		}
	}
}

void USKGCharacterAnimInstance::SetLeaning(float TargetGraphTime)
{
	PressedGraphTime = CurrentGraphTime;
	GraphTimeToGoTo = TargetGraphTime;
	LeanStartTime = GetWorld()->GetTimeSeconds();
	bInterpLeaning = true;
}

void USKGCharacterAnimInstance::SetFreeLook(bool FreeLook)
{
	if (IsValid(CharacterComponent))
	{
		bFreeLook = FreeLook;
		if (bFreeLook)
		{
			FreeLookStartRotation = CharacterComponent->GetControlRotation();
		}
		else
		{
			CharacterComponent->OnFreeLookEnd.Broadcast(FreeLookStartRotation);
			FreeLookStartRotation = FRotator::ZeroRotator;
		}
	}
}

void USKGCharacterAnimInstance::InterpRelativeToHand(float DeltaSeconds)
{
	// Change InterpSpeed to weight of firearm
	if (!bIndexOrTagForced)
	{
		AnimationIndex = ISKGProceduralAnimationInterface::Execute_GetAnimationIndex(HeldActor);
		AnimationGameplayTag = ISKGProceduralAnimationInterface::Execute_GetAnimationGameplayTag(HeldActor);
	}
	
	float InterpSpeed = AimInterpolationSpeed;
	float Multiplier = ISKGProceduralAnimationInterface::Execute_GetAimInterpolationMultiplier(HeldActor);
	Multiplier = UKismetMathLibrary::NormalizeToRange(Multiplier, -40.0f, 150.0f);
	Multiplier = FMath::Clamp(Multiplier, 0.0f, 1.0f);
	InterpSpeed *= Multiplier;

	// Change InterpSpeed to be modified by firearm in hand
	RelativeToHandTransform = UKismetMathLibrary::TInterpTo(RelativeToHandTransform, FinalRelativeHand, DeltaSeconds, InterpSpeed);
	RelativeToHandLocation = RelativeToHandTransform.GetLocation();
	RelativeToHandRotation = RelativeToHandTransform.Rotator();
	
	float HandToSightDistance = FinalRelativeHand.GetLocation().X;
	float HandToSightOffset = 0.0f;
	
	bool bUseFixedCameraDistance = false;
	FSKGAimCameraSettings CameraSettings;
	ISKGProceduralAnimationInterface::Execute_GetCameraSettings(HeldActor, CameraSettings);
	if (CharacterComponent->IsLocallyControlled() && !CharacterComponent->IsInThirdPerson())
	{
		bUseFixedCameraDistance = CameraSettings.bUseFixedCameraDistance;
		if (bUseFixedCameraDistance)
		{
			HandToSightOffset = -CameraSettings.CameraDistance;
		}
		else
		{
			HandToSightOffset += CameraSettings.CameraDistance;
		}
	}
	else
	{
		bUseFixedCameraDistance = CameraSettings.bThirdPersonUseFixedCameraDistance;
		if (bUseFixedCameraDistance)
		{
			HandToSightOffset = -CameraSettings.ThirdPersonCameraDistance;
		}
		else
		{
			HandToSightOffset += CameraSettings.ThirdPersonCameraDistance;
		}
	}

	if (!bUseFixedCameraDistance && HeldActor->GetClass()->ImplementsInterface(USKGFirearmPartsInterface::StaticClass()))
	{
		HandToSightOffset -= ISKGFirearmPartsInterface::Execute_GetStockLengthOfPull(HeldActor) / 2.0f;
	}
	
	const float MaxOffset = ISKGProceduralAnimationInterface::Execute_GetMaxSightDistanceOffset(HeldActor);
	if (HandToSightOffset > MaxOffset)
	{
		HandToSightOffset = MaxOffset;
	}
	
	if (bUseFixedCameraDistance)
	{
		HandToSightDistance = HandToSightOffset;
	}
	else
	{
		HandToSightDistance -= HandToSightOffset;
	}
	
	SightDistance = UKismetMathLibrary::FInterpTo(SightDistance, HandToSightDistance * -1.0f, DeltaSeconds, InterpSpeed);
	SetSightTransform();

	if (RelativeToHandTransform.Equals(FinalRelativeHand))
	{
		if (SprintAlpha == 0.0f)
		{
			if (HeldActor->Implements<USKGFirearmPartsInterface>() && ISKGFirearmPartsInterface::Execute_GetCurrentSightActor(HeldActor) != HeldActor)
			{
				const float Distance = FVector::Dist(FinalRelativeHand.GetLocation(), DefaultRelativeToHand.GetLocation());
				if (Distance != 0.0f)
				{
					bInterpRelativeToHand = false;
				}
			}
			else
			{
				bInterpRelativeToHand = false;
			}
		}
		SightDistance = HandToSightDistance * -1.0f;
		SetSightTransform();
	}
	
	if (!IsValid(HeldActor))
	{
		AnimationIndex = 0;
		AnimationGameplayTag = DefaultGameplayTag;
	}
}

void USKGCharacterAnimInstance::SetSightTransform()
{
	FTransform CameraTransform = CharacterComponent->GetInUseMesh()->GetSocketTransform(CharacterComponent->GetCameraSocket(), RTS_ParentBoneSpace);

	FRotator NewRot = FRotator::ZeroRotator;
	NewRot.Roll += -90.0f;
	NewRot.Yaw += 90.0f;

	CameraTransform.SetRotation(NewRot.Quaternion());
	
	FVector CameraVector = CameraTransform.GetLocation();
	const float AdditiveDistance = SightDistance + 10.0f;
	
	switch (CharacterComponent->GetRightHandAxis())
	{
	case EAxis::Type::Y : CameraVector.Y += AdditiveDistance; break;
	case EAxis::Type::Z : CameraVector.Z += AdditiveDistance; break;
	case EAxis::Type::X : CameraVector.X += AdditiveDistance; break;
	case EAxis::Type::None : break;
	}

	if (HeldActor->GetClass()->ImplementsInterface(USKGFirearmPartsInterface::StaticClass()))
	{
		const float LengthOfPull = ISKGFirearmPartsInterface::Execute_GetStockLengthOfPull(HeldActor);
		StockLOPOffset.Y = -LengthOfPull;
		if (!CharacterComponent->IsLocallyControlled() || CharacterComponent->IsInThirdPerson())
		{
			FVector StockOffset = ISKGFirearmPartsInterface::Execute_GetAimStockOffset(HeldActor) * -1.0f;
			StockOffset.Y -= LengthOfPull;
			CameraVector.X += StockOffset.Z * -1.0f;
			CameraVector.Z += StockOffset.X;
			CameraVector.Y += StockOffset.Y;
		}
	}

	CameraTransform.SetLocation(CameraVector);
	
	SightLocation = CameraTransform.GetLocation();
	SightRotation = CameraTransform.Rotator();
}

void USKGCharacterAnimInstance::SetRelativeToHand()
{
	if (IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
	{
		const FTransform HeadAimTransform = ISKGProceduralAnimationInterface::Execute_GetHeadAimTransform(HeldActor);
		HeadAimingLocation = HeadAimTransform.GetLocation();
		HeadAimingRotation = HeadAimTransform.Rotator();
		const FTransform NeckAimTransform = ISKGProceduralAnimationInterface::Execute_GetNeckAimTransform(HeldActor);
		NeckAimingLocation = NeckAimTransform.GetLocation();
		NeckAimingRotation = NeckAimTransform.Rotator();
		SightAimingRotation = NeckAimingRotation.GetInverse() + HeadAimingRotation.GetInverse();

		const FTransform DefaultTransform = ISKGProceduralAnimationInterface::Execute_GetDefaultAimSocketTransform(HeldActor);
		FTransform SightSocketTransform = ISKGProceduralAnimationInterface::Execute_GetAimSocketTransform(HeldActor);

		if (SprintAlpha > 0.0f || SuperSprintAlpha > 0.0f || HeldActorPose != ESKGFirearmPose::None || FirearmCollisionAlpha > 0.0f)
		{
			SightSocketTransform = DefaultTransform;
		}
		else if (ISKGProceduralAnimationInterface::Execute_GetUseBasePoseCorrection(HeldActor))
		{
			SightSocketTransform = UKismetMathLibrary::TLerp(DefaultTransform, SightSocketTransform, AimingAlpha);
		}
		
		const FTransform Hand_RTransform = CharacterComponent->GetInUseMesh()->GetSocketTransform(RightHandBone);
		FinalRelativeHand = UKismetMathLibrary::MakeRelativeTransform(SightSocketTransform, Hand_RTransform);
	
		DefaultRelativeToHand = UKismetMathLibrary::MakeRelativeTransform(DefaultTransform, Hand_RTransform);
	}
}

void USKGCharacterAnimInstance::SetLeftHandIK()
{
	const FTransform LeftHandIK = ISKGProceduralAnimationInterface::Execute_GetLeftHandIKData(HeldActor).Transform;
	if (LeftHandIK.GetLocation().Equals(FVector::ZeroVector))
	{
		LeftHandIKAlpha = 0.0f; return;
	}
	LeftHandIKAlpha = 1.0f;
	
	FVector OutPosition;
	FRotator OutRotation;

	CharacterComponent->GetInUseMesh()->TransformToBoneSpace(RightHandBone, LeftHandIK.GetLocation(), LeftHandIK.Rotator(), OutPosition, OutRotation);

	LeftHandIKTransform.SetLocation(OutPosition);
	LeftHandIKTransform.SetRotation(OutRotation.Quaternion());
	LeftHandIKTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
}

void USKGCharacterAnimInstance::InterpHeldActorPose(float DeltaSeconds)
{
	if (HeldActorPoseGraphSettings.bUseLegacySystem)
	{
		FTransform ToInterpFrom = FTransform(HeldActorPoseRotation, HeldActorPoseLocation);
		ToInterpFrom = UKismetMathLibrary::TInterpTo(ToInterpFrom, HeldActorPoseToInterpTo, DeltaSeconds, ISKGProceduralAnimationInterface::Execute_GetHighLowPortPoseInterpolationSpeed(HeldActor));
		HeldActorPoseLocation = ToInterpFrom.GetLocation();
		HeldActorPoseRotation = ToInterpFrom.Rotator();
		
		if (HeldActorPoseLocation.Equals(HeldActorPoseToInterpTo.GetLocation()) && HeldActorPoseRotation.Equals(HeldActorPoseToInterpTo.Rotator()))
		{
			CharacterComponent->OnHeldActorPoseFinished.Broadcast();
			bInterpHeldActorPose = false;
		}
	}
	else
	{
		HeldActorPoseCurrentTime = (GetWorld()->GetTimeSeconds() - HeldActorPoseStartTime) * HeldActorPoseGraphSettings.GetActiveCurveSetting().CurveSpeedMultiplier;
	
		const FTransform PoseTransform = HeldActorPoseGraphSettings.GetTransform(HeldActorPoseCurrentTime, CharacterComponent->IsLocallyControlled());

		HeldActorPoseLocation = PoseTransform.GetLocation();
		HeldActorPoseRotation = PoseTransform.Rotator();
		
		if (HeldActorPoseCurrentTime >= HeldActorPoseGraphSettings.GetActiveCurveSetting().CurveLength)
		{
			CharacterComponent->OnHeldActorPoseFinished.Broadcast();
			HeldActorPoseCurrentTime = 0.0f;
			bInterpHeldActorPose = false;
		}
	}

	if (!IsValid(HeldActor))
	{
		bInterpHeldActorPose = false;
	}
}

void USKGCharacterAnimInstance::SetFirearmPose(ESKGFirearmPose Pose)
{
	if (IsValid(HeldActor) && HeldActorPose != Pose)
	{
		HeldActorPoseStartTime = GetWorld()->GetTimeSeconds();
		
		HeldActorPose = Pose;

		if (HeldActorPose != ESKGFirearmPose::None)
		{
			switch (HeldActorPose)
			{
			case ESKGFirearmPose::High: HeldActorPoseToInterpTo = ISKGProceduralAnimationInterface::Execute_GetHighPortPose(HeldActor); HeldActorPoseGraphSettings = ISKGProceduralAnimationInterface::Execute_GetHighPortCurveSettings(HeldActor); break;
			case ESKGFirearmPose::Low: HeldActorPoseToInterpTo = ISKGProceduralAnimationInterface::Execute_GetLowPortPose(HeldActor); HeldActorPoseGraphSettings = ISKGProceduralAnimationInterface::Execute_GetLowPortCurveSettings(HeldActor); break;
			case ESKGFirearmPose::ShortStock: HeldActorPoseToInterpTo = ISKGProceduralAnimationInterface::Execute_GetShortStockPose(HeldActor); HeldActorPoseGraphSettings = ISKGProceduralAnimationInterface::Execute_GetShortStockCurveSettings(HeldActor); break;
			case ESKGFirearmPose::OppositeShoulder: HeldActorPoseToInterpTo = ISKGProceduralAnimationInterface::Execute_GetOppositeShoulderPose(HeldActor); HeldActorPoseGraphSettings = ISKGProceduralAnimationInterface::Execute_GetOppositeShoulderCurveSettings(HeldActor); break;
			case ESKGFirearmPose::BlindFireLeft: HeldActorPoseToInterpTo = ISKGProceduralAnimationInterface::Execute_GetBlindFireLeftPose(HeldActor); HeldActorPoseGraphSettings = ISKGProceduralAnimationInterface::Execute_GetBlindFireLeftCurveSettings(HeldActor); break;
			case ESKGFirearmPose::BlindFireUp: HeldActorPoseToInterpTo = ISKGProceduralAnimationInterface::Execute_GetBlindFireUpPose(HeldActor); HeldActorPoseGraphSettings = ISKGProceduralAnimationInterface::Execute_GetBlindFireUpCurveSettings(HeldActor); break;
			default: HeldActorPoseToInterpTo = FTransform(); HeldActorPoseGraphSettings.bGoIntoPose = false;
			}
			HeldActorPoseGraphSettings.bGoIntoPose = true;
			SetIsAiming(false);
		}
		else
		{
			HeldActorPoseToInterpTo = FTransform();
			HeldActorPoseGraphSettings.bGoIntoPose = false;
			if (HeldActorPoseGraphSettings.bUseLegacySystem && ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).PerformShakeAfterPortPose)
			{
				PlayFirearmShakeCurve(true);
			}
		}

		if (!CharacterComponent->IsLocallyControlled())
		{
			RelativeToHandTransform = DefaultRelativeToHand;
			RelativeToHandLocation = RelativeToHandTransform.GetLocation();
			RelativeToHandRotation = RelativeToHandTransform.Rotator();
		}
		HeldActorPoseAlpha = 1.0f;
		bInterpHeldActorPose = true;
	}
}

void USKGCharacterAnimInstance::InterpShakeCurve(float DeltaSeconds)
{
	if (const UCurveVector* Curve = ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).ShakeCurve)
	{
		const float CurrentTime = (GetWorld()->GetTimeSeconds() - ShakeCurveStartTime) * ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).ShakeCurveSpeed;
		FVector Shake = Curve->GetVectorValue(CurrentTime);
		Shake *= ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).ShakeCurveMultplier;
		ShakeRotation = FRotator(Shake.X, Shake.Y, Shake.Z);

		if (CurrentTime > ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).ShakeCurveDuration)
		{
			bInterpShakeCurve = false;
			ShakeCurveAlpha = 0.0f;
		}
	}
	else
	{
		ShakeCurveAlpha = 0.0f;
		bInterpShakeCurve = false;
	}

	if (!IsValid(HeldActor))
	{
		ShakeCurveAlpha = 0.0f;
		bInterpShakeCurve = false;
	}
}

void USKGCharacterAnimInstance::PlayFirearmShakeCurve(bool ManuallyPlay)
{
	if (IsValid(HeldActor) && bCanPlayShakeCurve)
	{
		bCanPlayShakeCurve = ManuallyPlay;
		ShakeRotation = FRotator::ZeroRotator;
		bInterpShakeCurve = true;
		ShakeCurveAlpha = 1.0f;
		ShakeCurveStartTime = GetWorld()->GetTimeSeconds();
	}
}

void USKGCharacterAnimInstance::InterpCustomCurve(float DeltaSeconds)
{
	if (IsValid(CustomCurveData.RotationCurve) || IsValid(CustomCurveData.LocationCurve))
	{
		const float CurrentTime = (GetWorld()->GetTimeSeconds() - CustomCurveStartTime) * CustomCurveData.CurveSpeed;
		if (CustomCurveData.LocationCurve)
		{
			CustomCurveLocation = CustomCurveData.LocationCurve->GetVectorValue(CurrentTime);
		}
		
		if (CustomCurveData.RotationCurve)
		{
			const FVector Shake = CustomCurveData.RotationCurve->GetVectorValue(CurrentTime);
			CustomCurveRotation = FRotator(Shake.X, Shake.Y, Shake.Z);
		}
		
		if (CurrentTime > CustomCurveData.CurveDuration)
		{
			bInterpCustomCurve = false;
			CustomCurveAlpha = 0.0f;
		}
	}
	else
	{
		bInterpCustomCurve = false;
		CustomCurveAlpha = 0.0f;
	}
}

void USKGCharacterAnimInstance::PlayCustomCurve(FSKGCurveData INCurveData)
{
	if (IsValid(INCurveData.RotationCurve) || IsValid(INCurveData.LocationCurve))
	{
		CustomCurveAlpha = 1.0f;
		CustomCurveData = INCurveData;
		CustomCurveStartTime = GetWorld()->GetTimeSeconds();
		bInterpCustomCurve = true;
	}
}

void USKGCharacterAnimInstance::InterpCameraZoom(float DeltaSeconds)
{
	if (IsValid(CharacterComponent) && CharacterComponent->GetCameraComponent())
	{
		float CurrentFOV = CharacterComponent->GetCameraComponent()->FieldOfView;
		float TargetFOV = CharacterComponent->GetDefaultFOV();
		float InterpSpeed = 10.0f;
		if (bIsAiming)
		{
			FSKGAimCameraSettings CameraSettings;
			ISKGProceduralAnimationInterface::Execute_GetCameraSettings(HeldActor, CameraSettings);
			TargetFOV -= CameraSettings.CameraFOVZoom;
			InterpSpeed = CameraSettings.CameraFOVZoomSpeed;
		}
		CurrentFOV = UKismetMathLibrary::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, InterpSpeed);
		CharacterComponent->SetCameraFOV(CurrentFOV);
		if (CurrentFOV == TargetFOV)
		{
			bInterpCameraZoom = false;
		}
	}
}

void USKGCharacterAnimInstance::InterpAimingAlpha(float DeltaSeconds)
{
	float InterpSpeed = AimInterpolationSpeed;
	float Multiplier;
	if (bIsAiming)
	{
		Multiplier = ISKGProceduralAnimationInterface::Execute_GetAimInterpolationMultiplier(HeldActor);
	}
	else
	{
		Multiplier = ISKGProceduralAnimationInterface::Execute_GetUnAimInterpolationMultiplier(HeldActor);
	}
	
	Multiplier = UKismetMathLibrary::NormalizeToRange(Multiplier, -40.0f, 150.0f);
	InterpSpeed *= Multiplier;

	AimingAlpha = UKismetMathLibrary::FInterpTo(AimingAlpha, bIsAiming - RotationAlpha, DeltaSeconds, InterpSpeed);
	if (!AimCurveSettings.bUseLegacySystem && AimCurveSettings.IsValid())
	{
		AimCurveAlpha = bIsAiming ? 1.0f - AimingAlpha : AimingAlpha;
		const float AimCurveTime = GetWorld()->GetTimeSeconds() - AimCurveStartTime;
		const FTransform AimCurves = AimCurveSettings.GetTransformMultiplied(AimCurveTime, true, 4.0f);
		AimCurveLocation = AimCurves.GetLocation();
		AimCurveRotation = AimCurves.Rotator();
	}
	
	if ((bIsAiming && AimingAlpha >= 1.0f) || (!bIsAiming && AimingAlpha <= 0.0f))
	{
		AimCurveAlpha = 0.0f;
		bInterpAiming = false;
	}
}

void USKGCharacterAnimInstance::InterpHeadAimingAlpha(float DeltaSeconds)
{
	HeadAimingAlpha = UKismetMathLibrary::FInterpTo(HeadAimingAlpha, bIsHeadAiming, DeltaSeconds, HeadAimingInterpolationSpeed);
	if (HeadAimingAlpha >= 1.0f || HeadAimingAlpha <= 0.0f)
	{
		bInterpHeadAimingAlpha = false;
	}
}

void USKGCharacterAnimInstance::SetRotationLag(float DeltaSeconds)
{
	float InterpSpeed = RotationLagResetInterpolationSpeed;
	float Multiplier = ISKGProceduralAnimationInterface::Execute_GetRotationLagInterpolationMultiplier(HeldActor);
	Multiplier = UKismetMathLibrary::NormalizeToRange(Multiplier, -40.0f, 150.0f);
	InterpSpeed *= Multiplier;
	// Temp Workaround for consistency
	InterpSpeed *= 1.5f;
	const float Delta = (1.0f - (DeltaSeconds * 35.0f));
	
	const FRotator CurrentRotation = CharacterComponent->GetControlRotation();
	const FQuat Difference = (CurrentRotation - OldRotation).Quaternion() * Delta;

	FRotator Rotation = UKismetMathLibrary::RInterpTo(UnmodifiedRotationLag, Difference.Rotator(), DeltaSeconds, InterpSpeed);
	UnmodifiedRotationLag = Rotation;

	float FirearmWeightMultiplier = 1.0f;
	if (HeldActor->Implements<USKGFirearmInterface>())
	{
		// Modify on HELD weapon weight	
		FirearmWeightMultiplier = UKismetMathLibrary::NormalizeToRange(ISKGFirearmInterface::Execute_GetFirearmStats(HeldActor).Weight, 0.0f, 25.0f);
		FirearmWeightMultiplier = FirearmWeightMultiplier * (0.3f / Multiplier);
	}
	
	Rotation *= 3.0f;
	Rotation *= FirearmWeightMultiplier;

	float InvertMultiplier = -1.0f;
	Rotation.Roll = Rotation.Pitch;
	Rotation.Pitch = 0.0f;
	if (bInvertRotationLag)
	{
		InvertMultiplier = 1.0f;
	}
	Rotation.Yaw = FMath::Clamp(Rotation.Yaw, -7.0f, 7.0f) * InvertMultiplier;	
	Rotation.Roll = FMath::Clamp(Rotation.Roll, -3.0f, 3.0f) * -InvertMultiplier;

	RotationLagRotation = Rotation;
	RotationLagLocation = FVector(Rotation.Yaw / 4.0f, 0.0f, Rotation.Roll / 1.5);
	RotationLagTransform.SetRotation(Rotation.Quaternion());
	RotationLagTransform.SetLocation(FVector(Rotation.Yaw / 4.0f, 0.0f, Rotation.Roll / 1.5));

	OldRotation = CurrentRotation;
}

void USKGCharacterAnimInstance::SetMovementLag(float DeltaSeconds)
{
	const FVector FPSVelocity = CharacterComponent->GetMovementComponent()->Velocity;
	float RightSpeed = FVector::DotProduct(FPSVelocity, CharacterComponent->GetActorRightVector());
	float VerticalSpeed = FPSVelocity.Z;
	RightSpeed = UKismetMathLibrary::NormalizeToRange(RightSpeed, 0.0f, 75.0f);
	VerticalSpeed = UKismetMathLibrary::NormalizeToRange(VerticalSpeed, 0.0f, 75.0f);

	const FSKGSwayMultipliers SwayMultipliers = ISKGProceduralAnimationInterface::Execute_GetSwayMultipliers(HeldActor);
	
	FRotator NewRot = MovementLagRotation;
	NewRot.Pitch = UKismetMathLibrary::FInterpTo(NewRot.Pitch, RightSpeed * SwayMultipliers.MovementRollMultiplier, DeltaSeconds, 10.0f);
	NewRot.Yaw = UKismetMathLibrary::FInterpTo(NewRot.Yaw, (RightSpeed * SwayMultipliers.MovementYawMultiplier) * -1.0f, DeltaSeconds, 10.0f);
	NewRot.Roll = UKismetMathLibrary::FInterpTo(NewRot.Roll, VerticalSpeed * SwayMultipliers.MovementPitchMultiplier, DeltaSeconds, 10.0f);
	
	MovementLagRotation = NewRot;
}

void USKGCharacterAnimInstance::HandleMovementSway(float DeltaSeconds)
{
	if (UCurveVector* Curve = ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).MovementSwayCurve)
	{
		if (!bLerpingNewGraph)
		{
			bLerpingNewGraph = Curve != CachedMovementCurve;
		}
		CachedMovementCurve = Curve;
		
		const float OldVelocityMultiplier = VelocityMultiplier;
		VelocityMultiplier = UKismetMathLibrary::NormalizeToRange(CharacterVelocity, 0.0f, CharacterComponent->GetMovementComponentSprintSpeed());
		if (VelocityMultiplier < OldVelocityMultiplier)
		{
			VelocityMultiplier = UKismetMathLibrary::FInterpTo(OldVelocityMultiplier, VelocityMultiplier, DeltaSeconds, 3.2f);
		}
		if (VelocityMultiplier < 0.25f)
		{
			VelocityMultiplier = 0.25f;
		}
		float Multiplier = 1.1f;
		
		if (ISKGProceduralAnimationInterface::Execute_GetCurveAndShakeSettings(HeldActor).ControlMovementSwayByStats && HeldActor->Implements<USKGFirearmInterface>())
		{
			const FSKGFirearmStats Stats = ISKGFirearmInterface::Execute_GetFirearmStats(HeldActor);
			Multiplier = Stats.Weight * (Stats.Weight * 3.0f / (Stats.Ergonomics * 1.5f)) + 1.0f;
			Multiplier = FMath::Clamp(Multiplier, 0.5f, 1.0f);
		}

		SwayMultiplier = UKismetMathLibrary::FInterpTo(SwayMultiplier, ISKGProceduralAnimationInterface::Execute_GetSwayMultiplier(HeldActor), DeltaSeconds, 2.0f);
		
		CurveTimer += (DeltaSeconds * VelocityMultiplier);
		FVector Graph = Curve->GetVectorValue(CurveTimer);
		Graph *= VelocityMultiplier * Multiplier;
		const FRotator Rotation = FRotator(Graph.Y, Graph.X, Graph.Z);
		
		if (bLerpingNewGraph)
		{
			SwayLocation = UKismetMathLibrary::VInterpTo(SwayLocation, Graph * SwayMultiplier, DeltaSeconds, CurveChangeInterpSpeed);
			SwayRotation = UKismetMathLibrary::RInterpTo(SwayRotation, Rotation * SwayMultiplier, DeltaSeconds, CurveChangeInterpSpeed);
			if (SwayLocation.Equals(Graph * SwayMultiplier, CurveChangeEqualTolerance))
			{
				bLerpingNewGraph = false;
			}
		}
		else
		{
			SwayLocation = Graph * SwayMultiplier;
			SwayRotation = Rotation * SwayMultiplier;	
		}
	}
}

void USKGCharacterAnimInstance::HandleSprinting()
{
	const bool bForcedIntoSprint = CharacterComponent->GetForceIntoSprintPose();
	if (CharacterComponent->GetIsSprinting() && (CharacterVelocity > CharacterComponent->GetMovementComponentSprintSpeed() / 2.0f || bForcedIntoSprint))
	{
		if (HeldActorPose != ESKGFirearmPose::None)
		{
			CharacterComponent->StopFirearmPose();
		}
		SprintAlpha = 1.0f;
		if (bSprinting != true)
		{
			bInterpRelativeToHand = true;
			bSprinting = true;
			SetIsAiming(false);
		}

		const FTransform SprintTransform = ISKGProceduralAnimationInterface::Execute_GetSprintPose(HeldActor);
		SprintPoseLocation = SprintTransform.GetLocation();
		SprintPoseRotation = SprintTransform.Rotator();
		
		const FTransform SuperSprintTransform = ISKGProceduralAnimationInterface::Execute_GetSuperSprintPose(HeldActor);
		SuperSprintPoseLocation = SuperSprintTransform.GetLocation();
		SuperSprintPoseRotation = SuperSprintTransform.Rotator();
		
		if (bForcedIntoSprint)
		{
			if (CharacterComponent->GetSprintType() == ESKGSprintType::SuperSprint)
			{
				SuperSprintAlpha = 1.0f;
				SprintAlpha = 0.0f;
			}
			else
			{
				SuperSprintAlpha = 0.0f;
				SprintAlpha = 1.0f;
			}
		}
		else if (CharacterComponent->CanSuperSprint())
		{
			if (CharacterVelocity > CharacterComponent->GetMovementComponentSprintSpeed() + 25.0f)
			{
				SprintAlpha = 0.0f;
				SuperSprintAlpha = 1.0f;
			}
			else if (CharacterComponent->GetSprintType() == ESKGSprintType::SuperSprint)
			{
				SprintAlpha = 0.5f;
				SuperSprintAlpha = 0.0f;
			}
			else
			{
				SuperSprintAlpha = 0.0f;
				SprintAlpha = 1.0f;
			}
		}
	}
	else if (bSprinting != false)
	{
		bSprinting = false;
		if (CharacterComponent->IsAiming())
		{
			SetIsAiming(true);
		}
		SprintAlpha = 0.0f;
		SuperSprintAlpha = 0.0f;
	}
}

bool USKGCharacterAnimInstance::IsLocallyControlled()
{
	if (IsValid(CharacterComponent))
	{
		bIsLocallyControlled = CharacterComponent->IsLocallyControlled();
	}
	return bIsLocallyControlled;
}

void USKGCharacterAnimInstance::SetIsAiming(bool IsAiming)
{
	if (!(FirearmMovedDistance > CurrentCollisionSettings.PushDistanceToStopAiming) || !IsAiming)
	{
		if (IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGProceduralAnimationInterface::StaticClass()))
		{
			AimCurveSettings = ISKGProceduralAnimationInterface::Execute_GetAimCurveSettings(HeldActor);
		}
		if ((IsAiming && !bCanAim) || bInCollisionPose || bSprinting)
		{
			bIsAiming = false;
			bInterpAiming = true;
			bInterpCameraZoom = true;
			AimCurveStartTime = GetWorld()->GetTimeSeconds();
			return;
		}

		if (bIsAiming != IsAiming)
		{
			SetFirearmPose(ESKGFirearmPose::None);
			bIsAiming = IsAiming;
			bInterpAiming = true;
			bInterpCameraZoom = true;
			AimCurveSettings.bGoIntoPose = bIsAiming;
			AimCurveStartTime = GetWorld()->GetTimeSeconds();
		}
	}
}

void USKGCharacterAnimInstance::SetHeadAiming(bool IsAiming)
{
	if (IsAiming && (FirearmMovedDistance > CurrentCollisionSettings.PushDistanceToStopAiming))
	{
		return;
	}
	if (bIsHeadAiming != IsAiming)
	{
		bIsHeadAiming = IsAiming;
		bInterpHeadAimingAlpha = true;
	}
}

void USKGCharacterAnimInstance::SetIsReloading(bool IsReloading, float BlendAlpha)
{
	if (IsReloading)
	{
		RotationAlpha = BlendAlpha;
		LeftHandIKAlpha = 0.0f;
		bInterpAiming = true;
	}
	else
	{
		RotationAlpha = 0.0f;
		LeftHandIKAlpha = 1.0f;
		bInterpAiming = true;
	}
}

void USKGCharacterAnimInstance::SetCanAim(bool CanAim)
{
	bCanAim = CanAim;
}

void USKGCharacterAnimInstance::CycledSights()
{
	bInterpRelativeToHand = true;
	bInterpCameraZoom = true;
}

void USKGCharacterAnimInstance::EnterCustomPose(const FTransform& Pose)
{
	CustomPoseLocation = Pose.GetLocation();
	CustomPoseRotation = Pose.Rotator();
	CustomPoseAlpha = 1.0f;
}

void USKGCharacterAnimInstance::ClearCustomPose()
{
	CustomPoseAlpha = 0.0f;
}

void USKGCharacterAnimInstance::RecoilInterpToZero(float DeltaSeconds)
{
	FinalRecoilTransform = UKismetMathLibrary::TInterpTo(FinalRecoilTransform, FTransform(), DeltaSeconds, 8.0f); // def = 6
	RecoilLocation = FinalRecoilTransform.GetLocation();
	RecoilRotation = FinalRecoilTransform.Rotator();
	if (RecoilLocation.Equals(FVector::ZeroVector, 0.1f) && RecoilRotation.Equals(FRotator::ZeroRotator, 0.1f))
	{
		bInterpRecoil = false;
	}
}

void USKGCharacterAnimInstance::RecoilInterpTo(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_SKGRecoil);
	TRACE_CPUPROFILER_EVENT_SCOPE(SKGRecoilInterpTo)
	if (IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGFirearmInterface::StaticClass()))
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds() - RecoilStartTime;
		const FSKGRecoilData RecoilData = ISKGFirearmInterface::Execute_GetRecoilData(HeldActor);
		const float VerticalRecoilMultiplier = ISKGFirearmInterface::Execute_GetFirearmStats(HeldActor).VerticalRecoilMultiplier * RecoilMultiplier;
		const float HorizontalRecoilMultiplier = ISKGFirearmInterface::Execute_GetFirearmStats(HeldActor).HorizontalRecoilMultiplier * RecoilMultiplier;
	
		if (RecoilData.RecoilLocationCurve)
		{
			const float Randomess = FMath::RandRange(RecoilData.RecoilLocationRandomness.Min, RecoilData.RecoilLocationRandomness.Max);
			RecoilLocation += ((RecoilData.RecoilLocationCurve->GetVectorValue(CurrentTime) * Randomess) * RecoilMultiplier) * (100.0f * DeltaSeconds);
			FinalRecoilTransform.SetLocation(RecoilLocation);
		}
		if (RecoilData.RecoilRotationCurve)
		{
			const FVector CurveData = RecoilData.RecoilRotationCurve->GetVectorValue(CurrentTime);
			const float PitchRandom = (CurveData.X * FMath::RandRange(RecoilData.RecoilPitchRandomness.Min, RecoilData.RecoilPitchRandomness.Max)) * VerticalRecoilMultiplier;
			const float YawRandom = (CurveData.Y * FMath::RandRange(RecoilData.RecoilYawRandomness.Min, RecoilData.RecoilYawRandomness.Max)) * HorizontalRecoilMultiplier;
			const float RollRandom = (CurveData.Z * FMath::RandRange(RecoilData.RecoilRollRandomness.Min, RecoilData.RecoilRollRandomness.Max)) * HorizontalRecoilMultiplier;
		
			RecoilRotation += FRotator(RollRandom, PitchRandom, YawRandom) * (100.0f * DeltaSeconds);
			FinalRecoilTransform.SetRotation(RecoilRotation.Quaternion());
		}
		if (RecoilData.bUseControlRotation && RecoilData.ControlRotationCurve && CharacterComponent && CharacterComponent->IsLocallyControlled())
		{
			const FVector CurveData = RecoilData.ControlRotationCurve->GetVectorValue(CurrentTime);
			const float PitchRandom = ((CurveData.X * FMath::RandRange(RecoilData.ControlRotationPitchRandomness.Min, RecoilData.ControlRotationPitchRandomness.Max)) * VerticalRecoilMultiplier) * RecoilData.ControlRotationScale;
			const float YawRandom = ((CurveData.Y * FMath::RandRange(RecoilData.ControlRotationYawRandomness.Min, RecoilData.ControlRotationYawRandomness.Max)) * HorizontalRecoilMultiplier) * RecoilData.ControlRotationScale;
		
			CharacterComponent->AddControlRotation(FRotator(PitchRandom, YawRandom, 0.0f) * (100.0f * DeltaSeconds));
		}
		/*if (bApplyForwardVector && IsValid(HeldActor) && HeldActor->GetClass()->ImplementsInterface(USKGFirearmPartsInterface::StaticClass()))
		{
			bApplyForwardVector = false;
			FTransform MuzzleTransform = ISKGFirearmPartsInterface::Execute_GetMuzzleSocketTransform(HeldActor);
			MuzzleTransform = MuzzleTransform.GetRelativeTransform(CharacterComponent->GetOwner()->GetActorTransform());
			FVector MuzzleForward = MuzzleTransform.GetRotation().GetForwardVector();
			
			RecoilLocation *= MuzzleForward;
			FinalRecoilTransform.SetLocation(RecoilLocation);
			FVector End = MuzzleTransform.GetLocation() + RecoilLocation * 300.0f;
			DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), End, FColor::Red, false, 5.0f, 0, 2.0f);
		}*/
		RecoilInterpToZero(DeltaSeconds);
	}
}

void USKGCharacterAnimInstance::PerformRecoil(float Multiplier)
{
	if (HeldActor)
	{
		RecoilStartTime = GetWorld()->GetTimeSeconds();
		RecoilMultiplier = Multiplier;

		if (ShotCounter == 0)
		{
			ShotCounterStartTime = RecoilStartTime;
		}
		++ShotCounter;
		
		bApplyForwardVector = true;
		bInterpRecoil = true;
	}
}