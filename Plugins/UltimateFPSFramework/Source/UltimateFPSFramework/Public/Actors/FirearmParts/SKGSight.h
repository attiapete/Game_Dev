// Copyright 2021, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SKGSceneCaptureOptic.h"
#include "Actors/FirearmParts/SKGPart.h"
#include "Interfaces/SKGAimInterface.h"
#include "Interfaces/SKGProceduralAnimationInterface.h"
#include "Interfaces/SKGSightInterface.h"
#include "Interfaces/SKGRenderTargetInterface.h"
#include "GameplayTagContainer.h"
#include "SKGSight.generated.h"

class UMaterialInstance;
class ASKGMagnifier;
class USKGSceneCaptureOptic;

UCLASS()
class ULTIMATEFPSFRAMEWORK_API ASKGSight : public ASKGPart, public ISKGProceduralAnimationInterface,
	public ISKGAimInterface, public ISKGRenderTargetInterface, public ISKGSightInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASKGSight();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Default")
	FSKGReticleSettings ReticleSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	FSKGAimCameraSettings CameraSettings;
	// Used to blend between different state machines in the anim graph using an integer
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Animation")
	int32 AnimationIndex;
	// Used to blend between different state machines in the anim graph using gameplay tags
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	FGameplayTag AnimationGameplayTag;
	// How much faster to ADS with this optic
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Animation")
	float AimInterpolationMultiplier;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	float RotationLagInterpolationMultiplier;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Animation")
	FName GripSocket;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Animation")
	FSKGCurveAndShakeSettings CurveAndShakeSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|Animation")
	float DefaultSwayMultiplier;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FTransform HeadAimTransform;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aiming")
	FTransform NeckAimTransform;
	// Whether or not this optic can have its zero adjusted (point of impact shifts)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Zero")
	FSKGDefaultZeroSettings DefaultZeroSettings;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Aim")
	bool bIsAimable;
	// Socket that is used for aiming such as S_Aim that is on optics and the lightlaser mesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Aim")
	TArray<FName> AimSockets;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Zero")
	FSKGTurretAdjustmentSettings SightAdjustmentSettings;

	FName AimSocket;
	UPROPERTY(ReplicatedUsing = OnRep_AimSocketIndex)
	uint8 AimSocketIndex;
	UFUNCTION()
	void OnRep_AimSocketIndex();
	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_SetAimSocketIndex(uint8 Index);
	
	uint8 ReticleIndex;
	uint8 ReticleBrightnessIndex;

	bool bHasRenderTarget;
	
	float CurrentDotElevation, CurrentDotWindage, StartingDotElevation, StartingDotWindage;
	FSKGSightZero DotZero;
	FName ReticleZeroName = FName("ReticleZero");
	FSKGTurretClickEvent ElevationClicksChanged;
	FSKGTurretClickEvent WindageClicksChanged;

	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Default")
	TObjectPtr<ASKGMagnifier> Magnifier;

	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Default")
	UMaterialParameterCollection* MPC;
	TObjectPtr<UMaterialParameterCollectionInstance> MPCInstance;
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetDefaultZero();
	void SetRandomZero();
	// Leave as UPROPERTY due to garbage collection
	UPROPERTY()
	USKGSceneCaptureOptic* SceneCapture;

	virtual void PointOfImpactUp(bool bUp, uint8 Clicks = 1);
	virtual void PointOfImpactLeft(bool bLeft, uint8 Clicks = 1);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SKGFPSFramework|Zero")
	void OnElevationChanged(const FSKGTurretClickEvent& ElevationClicks);
	UFUNCTION(BlueprintImplementableEvent, Category = "SKGFPSFramework|Zero")
	void OnWindageChanged(const FSKGTurretClickEvent& WindageClicks);
	UFUNCTION(BlueprintImplementableEvent, Category = "SKGFPSFramework|Zero")
	void OnReturnToZeroElevation();
	UFUNCTION(BlueprintImplementableEvent, Category = "SKGFPSFramework|Zero")
	void OnReturnToZeroWindage();

	virtual float GetMagnification() const;
public:	
	// Aim Interface defaults
	virtual void SetMagnifier_Implementation(ASKGMagnifier* INMagnifier) override { Magnifier = INMagnifier; }
	virtual ASKGMagnifier* GetMagnifier_Implementation() override { return Magnifier; }
	virtual void GetCameraSettings_Implementation(FSKGAimCameraSettings& OutCameraSettings) override;
	virtual bool IsAimable_Implementation() override;
	virtual void EnableAiming_Implementation() override { bIsAimable = true; }
	virtual void DisableAiming_Implementation() override { bIsAimable = false; }
	// END OF AIM INTERFACE
	
	// PROCEDURAL ANIMATION INTERFACE
	virtual FTransform GetDefaultAimSocketTransform_Implementation() override { return Execute_GetAimSocketTransform(this); }
	virtual FTransform GetAimSocketTransform_Implementation() override;
	virtual int32 GetAnimationIndex_Implementation() const override { return AnimationIndex; }
	virtual FGameplayTag GetAnimationGameplayTag_Implementation() const override { return AnimationGameplayTag; }
	virtual float GetAimInterpolationMultiplier_Implementation() override { return AimInterpolationMultiplier; }
	virtual float GetRotationLagInterpolationMultiplier_Implementation() override { return RotationLagInterpolationMultiplier; }
	virtual FTransform GetHeadAimTransform_Implementation() const override { return HeadAimTransform; }
	virtual FTransform GetNeckAimTransform_Implementation() const override { return NeckAimTransform; }
	virtual FName GetGripSocketName_Implementation() const override { return GripSocket; }
	virtual FSKGCurveAndShakeSettings GetCurveAndShakeSettings_Implementation() override { return CurveAndShakeSettings; }
	virtual float GetSwayMultiplier_Implementation() override { return DefaultSwayMultiplier; }
	// END OF PROCEDUAL ANIMATION INTERFACE

	// START OF RENDER TARGET INTERFACE
	virtual bool HasRenderTarget_Implementation() const override { return bHasRenderTarget; }
	virtual void DisableRenderTarget_Implementation(bool Disable) override;
	virtual bool IsRenderTargetCapturing_Implementation() override;
	// END OF RENDER TARGET INTERFACE
	
	// START OF SIGHT INTERFACE
	virtual void ZoomOptic_Implementation(bool bZoom) override;
	virtual float GetCurrentMagnification_Implementation() const override { return GetMagnification(); }
	
	virtual void CycleReticle_Implementation() override;
	virtual void SetReticle_Implementation(uint8 Index) override;
	virtual void IncreaseBrightness_Implementation() override;
	virtual void DecreaseBrightness_Implementation() override;
	virtual void SetReticleBrightness_Implementation(uint8 Index) override;
	virtual void ReturnToZeroElevation_Implementation() override;
	virtual void ReturnToZeroWindage_Implementation() override;
	virtual void SetNewZeroElevation_Implementation() override;
	virtual void SetNewZeroWindage_Implementation() override;
	virtual void MovePointOfImpact_Implementation(ESKGElevationWindage Turret, ESKGRotationDirection Direction, uint8 Clicks = 1) override;
	virtual FSKGSightZero GetSightZero_Implementation() const override;

	virtual bool SetAimSocketIndex_Implementation(const uint8 Index) override;
	virtual void CycleAimSocket_Implementation(bool bDownArray, bool bStopAtEndOfArray) override;
	// END OF SIGHT INTERFACE

	// Firearm Parts defaults
	virtual ASKGSight* GetCurrentSight_Implementation() override { return this; }
	virtual void ActivateCurrentSight_Implementation(bool bActivate) override { DisableRenderTarget_Implementation(!bActivate); }
	// END OF FIREARM PARTS INTERFACE
};
