//Copyright 2021, Dakota Dawe, All rights reserved
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DataTypes/SKGFPSDataTypes.h"
#include "GameplayTagContainer.h"
#include "SKGCharacterAnimInstance.generated.h"


DECLARE_STATS_GROUP(TEXT("SKGAnimInstanceStatGroup"), STATGROUP_SKGAnimInstance, STATCAT_Advanced);

class ASKGFirearm;
class USKGCharacterComponent;
class UAnimSequence;

UCLASS()
class ULTIMATEFPSFRAMEWORK_API USKGCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
		
public:
	USKGCharacterAnimInstance();
	
	virtual void NativeBeginPlay() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// Default gameplay tags to be used for blending between state machines (similar to index in example)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Settings")
	FGameplayTag DefaultGameplayTag;
	// If false, the tick logic will not run on a dedicated server
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Settings")
	bool bRunOnDedicatedServer;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Settings")
	bool bUseProceduralSpine;
	// The right hand bone name of your character
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Settings")
	FName RightHandBone;
	// Invert the rotation lag of held actor (turning right makes firearm point more right instead of lagging behind)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Settings")
	bool bInvertRotationLag;
	// Bone name of the firearm collision bone on your characters skeleton
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Settings")
	FName FirearmCollisionVirtualBoneName;
	// Collision channel to be used with firearm collision (block = block firearm)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Settings")
	TEnumAsByte<ECollisionChannel> FirearmCollisionChannel;
	// Amount to blend spine bones by (lower spine bones can bend less than upper spine bones). Make sure total value equals 1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Settings", meta = (EditCondition = "bUseProceduralSpine", EditConditionHides))
	TArray<float> SpineBlendPercents;
	// Default interpolation speed for aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float AimInterpolationSpeed;
	// Default interpolation speed for the head/neck leaning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float HeadAimingInterpolationSpeed;
	// Default interpolation speed for cycling through different sights while aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float CycleSightsInterpolationSpeed;
	// Speed at which your rotation lag tries to center itself
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Procedural")
	float RotationLagResetInterpolationSpeed;
	// Speed at which your Movement lag tries to center itself
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Procedural")
	float MovementLagResetInterpolationSpeed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Procedural")
	float CurveChangeInterpSpeed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Procedural")
	float CurveChangeEqualTolerance;
	
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	USKGCharacterComponent* CharacterComponent;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float CharacterDirection;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float CharacterVelocity;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	bool bEquipped;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Firearm")
	int32 AnimationIndex;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Firearm")
	FGameplayTag AnimationGameplayTag;
	bool bIndexOrTagForced;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Firearm")
	AActor* HeldActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	UAnimSequence* LeftHandPose;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	bool bValidLeftHandPose;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector AimCurveLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FRotator AimCurveRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float AimCurveAlpha;
	float AimCurveStartTime;
	FSKGFirearmPoseCurveSettings AimCurveSettings;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector RelativeToHandLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FRotator RelativeToHandRotation;
	FTransform RelativeToHandTransform;
	FTransform FinalRelativeHand;
	bool bInterpRelativeToHand;

	bool bFirstRun;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector SightLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FRotator SightRotation;

	float SightDistance;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float AimingAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	bool bIsAiming;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float HeadAimingAlpha;
	bool bIsHeadAiming;
	bool bInterpHeadAimingAlpha;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector HeadAimingLocation;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FRotator HeadAimingRotation;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector NeckAimingLocation;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FRotator NeckAimingRotation;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FRotator SightAimingRotation;

	void SetSightTransform();
	void SetRelativeToHand();
	void InterpRelativeToHand(float DeltaSeconds);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|LeftHandIK")
	FTransform LeftHandIKTransform;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|LeftHandIK")
	float LeftHandIKAlpha;
	void SetLeftHandIK();
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|LeftHandIK")
	float LeftHandTwoBoneIKAlpha;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Actions")
	float RotationAlpha;
	
	bool bInterpAiming;
	bool bInterpCameraZoom;
	void InterpCameraZoom(float DeltaSeconds);
	float DefaultCameraFOV;

	void InterpAimingAlpha(float DeltaSeconds);
	void InterpHeadAimingAlpha(float DeltaSeconds);
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Lag")
	FVector RotationLagLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Lag")
	FRotator RotationLagRotation;
	FTransform RotationLagTransform;
	FRotator UnmodifiedRotationLag;
	FRotator OldRotation;
	void SetRotationLag(float DeltaSeconds);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Lag")
	FVector MovementLagLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Lag")
	FRotator MovementLagRotation;

	void SetMovementLag(float DeltaSeconds);
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	bool bIsLocallyControlled;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FSKGFirearmStats FirearmStats;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator Spine0Rotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator Spine1Rotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator Spine2Rotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator Spine3Rotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator Spine4Rotation;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Recoil")
	FVector RecoilLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Recoil")
	FRotator RecoilRotation;
	FTransform RecoilTransform;
	FTransform FinalRecoilTransform;
	bool bApplyForwardVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "FPSTemplate|Default")
	FVector BasePoseOffsetLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "FPSTemplate|Default")
	FRotator BasePoseOffsetRotation;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Stock")
	FVector StockLOPOffset;

	bool bInterpRecoil;
	float RecoilStartTime;
	float RecoilMultiplier;
	float ShotCounterStartTime;
	uint8 ShotCounter;

	void RecoilInterpToZero(float DeltaSeconds);
	void RecoilInterpTo(float DeltaSeconds);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator LeanRotation;
	bool bInterpLeaning;
	float CurrentGraphTime;
	float PressedGraphTime;
	float LeanStartTime;
	float GraphTimeToGoTo;
	void InterpLeaning(float DeltaSeconds);
	
	float CurveTimer;
	float VelocityMultiplier;
	void HandleMovementSway(float DeltaSeconds);
	void HandleSprinting();
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FVector AdditiveLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator AdditiveRotation;
	bool bLerpingNewGraph;
	TObjectPtr<UCurveVector> CachedMovementCurve;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FVector SwayLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator SwayRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FVector SprintPoseLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator SprintPoseRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FVector SuperSprintPoseLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator SuperSprintPoseRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float SprintAlpha;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float SuperSprintAlpha;
	bool bSprinting;

	float SwayMultiplier;

	bool bCustomizingFirearm;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FTransform WeaponCustomizingTransform;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float WeaponCustomizingAlpha;

	bool bCanAim;

	bool bInterpHeldActorPose;
	ESKGFirearmPose HeldActorPose;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float HeldActorPoseAlpha;
	FTransform HeldActorPoseToInterpTo;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	FVector HeldActorPoseLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	FRotator HeldActorPoseRotation;
	float HeldActorPoseStartTime;
	float HeldActorPoseCurrentTime;
	FSKGFirearmPoseCurveSettings HeldActorPoseGraphSettings;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	FVector FirearmCollisionLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	FRotator FirearmCollisionRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	float FirearmCollisionAlpha;
	float ShortStockBlend;
	float FirearmMovedDistance;
	bool bInCollisionPose;
	FTransform CollisionPose;

	FTransform DefaultRelativeToHand;

	void InterpHeldActorPose(float DeltaSeconds);
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator FreeLookRotation;
	FRotator FreeLookStartRotation;
	bool bFreeLook;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FRotator ShakeRotation;
	bool bInterpShakeCurve;
	void InterpShakeCurve(float DeltaSeconds);
	float ShakeCurveStartTime;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float ShakeCurveAlpha;
	bool bCanPlayShakeCurve;
	
	FRotator SpineToInterpTo;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	FVector CustomPoseLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	FRotator CustomPoseRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Poses")
	float CustomPoseAlpha;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Curve")
	FVector CustomCurveLocation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Curve")
	FRotator CustomCurveRotation;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Curve")
	float CustomCurveAlpha;
	FSKGCurveData CustomCurveData;
	float CustomCurveStartTime;
	bool bInterpCustomCurve;
	void InterpCustomCurve(float DeltaSeconds);

	// Returns true if you are the one controlling the actor of this AnimBP
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Extra")
	bool IsLocallyControlled();

	FSKGCollisionSettings CurrentCollisionSettings;
	/*FTransform FirearmGripOffset;
	FTransform RelativeMuzzle;
	float MuzzleDistanceToRoot;
	UPROPERTY()
	TArray<AActor*> CollisionIgnoredActors;*/
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Curve")
	FVector FirearmCollisionBoneLocationOffset;
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SKGFPSFramework|Curve")
	FRotator FirearmCollisionBoneRotationOffset;
	void HandleHeldActor(float DeltaSeconds);
	void HandleSpine(float DeltaSeconds);
	void HandleFirearmCollision(float DeltaSeconds);

	void SetEssentials(float DeltaSeconds);
	void HandleLeftHandIK();
	
public:
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Aiming")
	void SetIsAiming(bool IsAiming);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Aiming")
	void SetHeadAiming(bool IsAiming);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Aiming")
	void CycledSights();
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Aiming")
	void PlayFirearmShakeCurve(bool ManuallyPlay = false);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Aiming")
	void PlayCustomCurve(FSKGCurveData INCurveData);
	
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void EnterCustomPose(const FTransform& Pose);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void ClearCustomPose();
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void SetAnimationIndex(int32 NewAnimationIndex, bool bForceIndex) { AnimationIndex = NewAnimationIndex; bIndexOrTagForced = bForceIndex; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void SetAnimationGameplayTag(FGameplayTag GameplayTag, bool bForceTag) { AnimationGameplayTag = GameplayTag; bIndexOrTagForced = bForceTag; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void ClearForcedIndexAndTag() { bIndexOrTagForced = false; }
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Animation")
	FTransform GetRecoilTransform() const { return FinalRecoilTransform; }
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Animation")
	FTransform GetSwayTransform() const { return FTransform(SwayRotation, SwayLocation); }
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Animation")
	FTransform GetRotationLagTransform() const { return RotationLagTransform; }
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Animation")
	FTransform GetMovementLagTransform() const { return FTransform(MovementLagRotation, MovementLagLocation); }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void SetLeaning(float TargetGraphTime);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void SetFirearmPose(ESKGFirearmPose Pose);
	
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	void SetEquipped(bool Equipped) { bEquipped = Equipped; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	bool GetEquipped() const { return bEquipped; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	void SetIsReloading(bool IsReloading, float BlendAlpha = 0.35f);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	void SetCanAim(bool CanAim);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	void PerformRecoil(float Multiplier = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	void EnableLeftHandIK(bool Enable) { Enable ? LeftHandIKAlpha = 1.0f : LeftHandIKAlpha = 0.0f; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Actions")
	void EnableLeftHandTwoBoneIK(bool Enable) { Enable ? LeftHandTwoBoneIKAlpha = 1.0f : LeftHandTwoBoneIKAlpha = 0.0f; }

	void SetCharacterComponent(USKGCharacterComponent* INCharacterComponent) { CharacterComponent = INCharacterComponent;}

	void SetFreeLook(bool FreeLook);
};
