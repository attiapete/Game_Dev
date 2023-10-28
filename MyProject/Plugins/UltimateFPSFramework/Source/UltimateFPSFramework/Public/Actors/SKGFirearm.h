//Copyright 2021, Dakota Dawe, All rights reserved
#pragma once

#include "CoreMinimal.h"
#include "SKGFirearmBase.h"
#include "DataTypes/SKGFPSDataTypes.h"
#include "Animation/SkeletalMeshActor.h"
#include "Actors/FirearmParts/SKGLightLaser.h"
#include "Interfaces/SKGAimInterface.h"
#include "Interfaces/SKGFirearmInterface.h"
#include "Interfaces/SKGProceduralAnimationInterface.h"
#include "Interfaces/SKGFirearmCollisionInterface.h"
#include "Interfaces/SKGFirearmPartsInterface.h"
#include "Interfaces/SKGSightInterface.h"
#include "Components/SKGFirearmStabilizerComponent.h"
#include "SKGFirearm.generated.h"

class USKGCharacterComponent;
class ASKGMuzzle;
class USKGAttachmentComponent;
class UAnimationAsset;
class UAnimSequence;
class UAnimMontage;
class UCurveVector;
class UNiagaraSystem;
class USKGAttachmentManager;

UCLASS()
class ULTIMATEFPSFRAMEWORK_API ASKGFirearm : public ASKGFirearmBase, public ISKGAimInterface, public ISKGFirearmInterface,
public ISKGProceduralAnimationInterface, public ISKGFirearmCollisionInterface, public ISKGSightInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASKGFirearm(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CharacterComponent)
	USKGCharacterComponent* CharacterComponent;
	UFUNCTION()
	virtual void OnRep_CharacterComponent();
	
	// Used to blend between different state machines in the anim graph using an integer
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	int32 FirearmAnimationIndex;
	// Used to blend between different state machines in the anim graph using gameplay tags
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FGameplayTag FirearmAnimationGameplayTag;
	// Array of fire modes that you can cycle between. This is replicated but not implemented into anything by default aside from example m4
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	TArray<ESKGFirearmFireMode> FireModes;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FSKGAimCameraSettings CameraSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float MaxSightDistanceOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FSKGLeanCurves LeanCurves;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Default")
	TEnumAsByte<ECollisionChannel> PoseCollision;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	float DefaultLengthOfPull;

	// Socket for the firearm to attach to on your character
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Sockets")
	FName FirearmGripSocket;
	// The offset for the grip so you can control how the firearm is positioned in your hand on a per firearm basis
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Sockets")
	FTransform GripSocketOffset;
	// Where to aim when point aiming, uses sockets on the firearm
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Sockets")
	TArray<FName> PointAimSockets;
	
	// Where the left hand will go if using left hand IK
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|LeftHandIK")
	FSKGLeftHandIKData LeftHandIKData;

	// How much faster should you point aim compared to normal aiming down sights
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	float PointAimADSInterpolationMultiplier;
	// Make un aiming the same speed as aiming down sights
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	bool bUnAimMultiplierSameAsADS;
	// How much faster should you un aim compared to normal aiming down sights
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming", meta = (EditCondition = "!bUnAimMultiplierSameAsADS", EditConditionHides))
	float UnAimInterpolationMultiplier;
	// When aiming for remote clients (you looking at someone else), adjust where you want the firearm to go (Example: To the right shoulder)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector AimShoulderStockOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FVector OppositeShoulderAimShoulderStockOffset;
	// When aiming for remote clients (you looking at someone else), how much should the head lean
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FTransform HeadAimTransform;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FTransform NeckAimTransform;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FTransform OppositeShoulderHeadAimTransform;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FTransform OppositeShoulderNeckAimTransform;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	bool bUseBasePoseCorrection;
	
	UPROPERTY(ReplicatedUsing = OnRep_FireMode, BlueprintReadOnly, Category = "SKGFPSFramework|Firearm")
	ESKGFirearmFireMode FireMode;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Firearm")
	int32 FireModeIndex;
	FTimerHandle TFAHandle;
	uint8 BurstFireCount;
	UFUNCTION()
	void OnRep_FireMode();

	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGCollisionSettings CollisionSettings;

#pragma region PoseCurves
	ESKGFirearmPose CurrentFirearmPose;
	// If you dont know what short stocking is, google it
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonCollisionShortStockPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonCollisionPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonBasePoseOffset;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonBasePoseOffset;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGFirearmPoseCurveSettings HighPortCurveSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGFirearmPoseCurveSettings LowPortCurveSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGFirearmPoseCurveSettings ShortStockCurveSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGFirearmPoseCurveSettings OppositeShoulderCurveSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGFirearmPoseCurveSettings BlindFireLeftCurveSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FSKGFirearmPoseCurveSettings BlindFireUpCurveSettings;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonHighPortPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonLowPortPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonShortStockPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonOppositeShoulderPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonBlindFireLeftPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonBlindFireUpPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonSprintPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform FirstPersonSuperSprintPose;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonCollisionShortStockPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonCollisionPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonHighPortPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonLowPortPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonShortStockPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonOppositeShoulderPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonBlindFireLeftPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonBlindFireUpPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonSprintPose;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Poses")
	FTransform ThirdPersonSuperSprintPose;
#pragma endregion PoseCurves

	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Animation")
	FSKGFirearmPoseCurveSettings AimCurveSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Animation")
	FSKGFirearmPoseCurveSettings OppositeShoulderAimCurveSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	float LeaningSpeedMultiplier;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	FSKGSwayMultipliers SwayMultipliers;
	// Less = slower return to center (more perceived lag)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	float RotationLagInterpolationMultiplier;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	FSKGPoseSettings PoseSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	FSKGCurveAndShakeSettings CurveAndShakeSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	FSKGRecoilData RecoilData;

	FSKGCurveAndShakeSettings DefaultCurveAndShakeSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|Animation")
	float DefaultSwayMultiplier;
	float SwayMultiplier;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentSightComponent)
	USKGAttachmentComponent* CurrentSightComponent;
	UFUNCTION()
	void OnRep_CurrentSightComponent();
	int32 SightComponentIndex;
	
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSightComponent)
	int32 PointAimIndex;
	
	TObjectPtr<USKGFirearmStabilizerComponent> StabilizerComponent;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostInitProperties() override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_Owner() override;
	virtual void OnRep_AttachmentReplication() override;

	virtual void OnRep_CachedComponent() override;
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|AssetID")
	FPrimaryAssetType AssetType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|AssetID")
	FName AssetName;
	
	virtual void FixPoseTransforms(FTransform& FirstPerson, FTransform& ThirdPerson);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CycleSights(USKGAttachmentComponent* SightComponent);

	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void PerformProceduralRecoil(float Multiplier = 1.0f, bool bPlayCameraShake = true, bool bModifyControlRotation = false);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetFireMode(ESKGFirearmFireMode NewFireMode);

	//UFPSTemplateAttachmentComponent* GetSightComponent();

	void HandleSightComponents();
	
	float TimeSinceLastShot;

	bool bCanCycleSights;
	
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SetPointAiming(int32 Index);
	void SetFirearmPoseMultiplier(FSKGFirearmPoseCurveSettings& FirearmPose, ESKGFirearmPoseType PoseType, float Multiplier);

public:
	virtual FSKGProjectileTransform GetMuzzleProjectileSocketTransform(float RangeMeters, float MOA) override;
	
	/**
	 * Will return a rotation for your projectile to shoot from from the muzzle to
	 * whatever the center of your screen is pointing at. Recoil and sway still apply.
	 * bAimingOverride will use GetMuzzleProjectileSocketTransform when aiming at a zeroed range of RangeMetersOverride.
	 */
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Projectile")
	FSKGProjectileTransform GetProjectileSocketTransformToCenter(const float MaxDistanceToTest, float MOA = 1.0f, bool bAimingOverride = false, float RangeMetersOverride = 100.0f);
	
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void EnableSightCycling() { bCanCycleSights = true; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void DisableSightCycling() { bCanCycleSights = false; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void RefreshCurrentSight();
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void SetSwayMultiplier(float NewMultiplier) { NewMultiplier >= 0.0f ? SwayMultiplier = NewMultiplier : SwayMultiplier = DefaultSwayMultiplier; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void ResetSwayMultiplier() { SwayMultiplier = DefaultSwayMultiplier; }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Animation")
	void SetFirearmPoseCurveSpeedMultiplier(ESKGFirearmPose Pose, ESKGFirearmPoseType PoseType, float NewMultiplier = 20.0f);
	
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Default")
	int32 GetFirearmIndex() const { return FirearmAnimationIndex; }
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Default")
	USKGCharacterComponent* GetCharacterComponent();
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Default")
	ESKGFirearmFireMode GetFireMode() const { return FireMode; }
	

	virtual bool IsPointAiming_Implementation() override { return PointAimIndex > INDEX_NONE; }
	
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Sight")
	void DisableAllRenderTargets(bool Disable);

	void SetSight_Implementation(USKGAttachmentComponent* SightComponent);
	//virtual void PartsChanged_Implementation() override;

#pragma region PoseFunctions
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetBasePoseOffset(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetShortStockPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetSprintPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetSuperSprintPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetHighPortPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetLowPortPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetOppositeShoulderPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetBlindFireleftPose(const FTransform& Transform, bool bFirstPerson);
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Pose")
	void SetBlindFireUpPose(const FTransform& Transform, bool bFirstPerson);
#pragma endregion PoseFunctions
	
	const FName& GetAimSocket() const { return AimSocket; }

	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Stabilizer")
	USKGFirearmStabilizerComponent* GetStabilizerComponent();
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Stabilizer")
	bool IsStabilized() const;

	// ATTACHMENT INTERFACE
	virtual void OnAttachmentUpdated_Implementation() override;
	// END OF ATTACHMENT INTERFACE
	
	// FIREARM INTERFACE
	virtual FSKGFirearmStats GetFirearmStats_Implementation() override { return FirearmStats; }
	virtual FSKGRecoilData GetRecoilData_Implementation() override { return RecoilData; }
	virtual void SetCharacterComponent_Implementation(USKGCharacterComponent* INCharacterComponent) override { CharacterComponent = INCharacterComponent; }
	virtual void SetupStabilizerComponent_Implementation() override;
	virtual void Equip_Implementation() override { Execute_SetupStabilizerComponent(this); }
	virtual void CycleFireMode_Implementation(bool bReverse = false) override;
	virtual void SetFireModeIndex_Implementation(int32 Index) override;
	virtual void CycleSights_Implementation(bool bDownArray = true, bool bStopAtEndOfArray = false) override;
	virtual void CyclePointAim_Implementation(bool bDownArray, bool bStopAtEndOfArray) override;
	virtual void SetPointAimIndex_Implementation(int32 Index) override;
	// END OF FIREARM INTERFACE

	// PROCEDURAL ANIMATION INTERFACE
	virtual FSKGLeftHandIKData GetLeftHandIKData_Implementation() override;
	virtual FTransform GetGripSocketOffset_Implementation() override { return GripSocketOffset; }
	virtual FTransform GetBasePoseOffset_Implementation() override;
	
	virtual FTransform GetHighPortPose_Implementation() override;
	virtual FTransform GetLowPortPose_Implementation() override;
	virtual FTransform GetShortStockPose_Implementation() override;
	virtual FTransform GetOppositeShoulderPose_Implementation() override;
	virtual FTransform GetBlindFireLeftPose_Implementation() override;
	virtual FTransform GetBlindFireUpPose_Implementation() override;
	
	virtual FSKGFirearmPoseCurveSettings GetHighPortCurveSettings_Implementation() override { return HighPortCurveSettings; }
	virtual FSKGFirearmPoseCurveSettings GetLowPortCurveSettings_Implementation() override { return LowPortCurveSettings; }
	virtual FSKGFirearmPoseCurveSettings GetShortStockCurveSettings_Implementation() override { return ShortStockCurveSettings; }
	virtual FSKGFirearmPoseCurveSettings GetOppositeShoulderCurveSettings_Implementation() override { return OppositeShoulderCurveSettings; }
	virtual FSKGFirearmPoseCurveSettings GetBlindFireLeftCurveSettings_Implementation() override { return BlindFireLeftCurveSettings; }
	virtual FSKGFirearmPoseCurveSettings GetBlindFireUpCurveSettings_Implementation() override { return BlindFireUpCurveSettings; }
	virtual void StoreCurrentPose_Implementation(ESKGFirearmPose CurrentPose) override { CurrentFirearmPose = CurrentPose; }

	virtual FSKGFirearmPoseCurveSettings GetAimCurveSettings_Implementation() override { return CurrentFirearmPose == ESKGFirearmPose::OppositeShoulder ? OppositeShoulderAimCurveSettings : AimCurveSettings; }
	
	virtual float GetHighLowPortPoseInterpolationSpeed_Implementation() override { return PoseSettings.HighLowPortPoseInterpolationSpeed; }
	virtual FSKGCurveAndShakeSettings GetCurveAndShakeSettings_Implementation() override { return CurveAndShakeSettings; }
	virtual void SetCurveAndShakeSettings_Implementation(const FSKGCurveAndShakeSettings& NewCurveAndShakeSettings) override { CurveAndShakeSettings = NewCurveAndShakeSettings; }
	virtual void RevertToDefaultCurveAndShakeSettings_Implementation() override { CurveAndShakeSettings = DefaultCurveAndShakeSettings; }
	virtual float GetSwayMultiplier_Implementation() override { return SwayMultiplier; }
	virtual FTransform GetSprintPose_Implementation() override;
	virtual FTransform GetSuperSprintPose_Implementation() override;
	virtual FSKGSwayMultipliers GetSwayMultipliers_Implementation() override { return SwayMultipliers; }
	virtual FTransform GetAimSocketTransform_Implementation() override;
	virtual FTransform GetDefaultAimSocketTransform_Implementation() override;
	virtual FTransform GetHeadAimTransform_Implementation() const override { return CurrentFirearmPose == ESKGFirearmPose::OppositeShoulder ? OppositeShoulderHeadAimTransform : HeadAimTransform; }
	virtual FTransform GetNeckAimTransform_Implementation() const override { return CurrentFirearmPose == ESKGFirearmPose::OppositeShoulder ? OppositeShoulderNeckAimTransform : NeckAimTransform; }
	virtual void GetCameraSettings_Implementation(FSKGAimCameraSettings& OutCameraSettings) override;
	virtual float GetMaxSightDistanceOffset_Implementation() override { return MaxSightDistanceOffset; }
	virtual int32 GetAnimationIndex_Implementation() const override { return FirearmAnimationIndex; }
	virtual FGameplayTag GetAnimationGameplayTag_Implementation() const override { return FirearmAnimationGameplayTag; }
	virtual float GetAimInterpolationMultiplier_Implementation() override;
	virtual float GetUnAimInterpolationMultiplier_Implementation() override;
	virtual float GetRotationLagInterpolationMultiplier_Implementation() override;
	virtual bool GetUseBasePoseCorrection_Implementation() override { return bUseBasePoseCorrection; }
	virtual float GetLeaningSpeedMultiplier_Implementation() override { return LeaningSpeedMultiplier; }
	virtual FSKGLeanCurves GetLeanCurves_Implementation() override { return LeanCurves; }
	virtual FName GetGripSocketName_Implementation() const override { return Execute_GetFirearmGripSocket(this); }
	// END PROCEDURAL ANIMATION INTERFACE

	// FIREARM COLLISION INTERFACE
	virtual FName GetFirearmGripSocket_Implementation() const override { return FirearmGripSocket; }
	virtual FSKGCollisionSettings GetCollisionSettings_Implementation() override { return CollisionSettings; }
	virtual FTransform GetCollisionShortStockPose_Implementation() override;
	virtual FTransform GetCollisionPose_Implementation() override;
	virtual FTransform GetCollisionMuzzleSocketTransform_Implementation() override { return Execute_GetMuzzleSocketTransform(this); }
	virtual TArray<AActor*> GetPartsToIgnore_Implementation() override { return Execute_GetCachedParts(this); }
	// END OF FIREARM COLLISION INTERFACE

	// FIREARM PARTS INTERFACE
	virtual float GetStockLengthOfPull_Implementation() override;
	virtual FVector GetAimStockOffset_Implementation() override { return CurrentFirearmPose == ESKGFirearmPose::OppositeShoulder ? OppositeShoulderAimShoulderStockOffset : AimShoulderStockOffset; }
	virtual void ActivateCurrentSight_Implementation(bool bActivate) override;
	virtual ASKGSight* GetCurrentSight_Implementation() override;
	virtual AActor* GetCurrentSightActor_Implementation() override;
	// END OF FIREARM PARTS INTERFACE

	// SIGHT INTERFACE
	virtual float GetCurrentMagnification_Implementation() const override;
	// END OF SIGHT INTERFACE
};