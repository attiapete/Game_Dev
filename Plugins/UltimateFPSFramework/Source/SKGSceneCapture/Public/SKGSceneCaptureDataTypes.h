
#pragma once

#include "SKGSceneCaptureDataTypes.generated.h"

UENUM(BlueprintType)
enum class ESKGMeasurementType : uint8
{
	Metric		UMETA(DisplayName = "Metric"),
	Imperial	UMETA(DisplayName = "Imperial")
};

UENUM(BlueprintType)
enum class ESKGRotationDirection : uint8
{
	ClockWise			UMETA(DisplayName = "ClockWise"),
	CounterClockWise	UMETA(DisplayName = "CounterClockWise")
};

UENUM(BlueprintType)
enum class ESKGElevationWindage : uint8
{
	Elevation	UMETA(DisplayName = "Elevation"),
	Windage		UMETA(DisplayName = "Windage"),
	Both		UMETA(DisplayName = "Both")
};

UENUM(BlueprintType)
enum class ESKGScopeAdjustment : uint8
{
	MRAD	UMETA(DisplayName = "MRAD"),
	MOA		UMETA(DisplayName = "MOA")
};

UENUM(BlueprintType)
enum class ESKGMOAAdjustment : uint8
{
	OneEighth	UMETA(DisplayName = "1/8"),
	OneQuarter	UMETA(DisplayName = "1/4"),
	OneHalf		UMETA(DisplayName = "1/2"),
	One			UMETA(DisplayName = "1")
};

UENUM(BlueprintType)
enum class ESKGMRADAdjustment : uint8
{
	PointOne	UMETA(DisplayName = ".1")
};

class UMaterialInterface;

USTRUCT(BlueprintType)
struct FSKGDefaultZeroSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework")
	bool bCanBeZeroed = true;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework")
	bool bStartWithDefaultZero = false;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework", meta = (EditCondition = "bStartWithDefaultZero", EditConditionHides))
	int32 StartingElevationClicks = 0;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework", meta = (EditCondition = "bStartWithDefaultZero", EditConditionHides))
	int32 StartingWindageClicks = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework", meta = (EditCondition = "bCanBeZeroed && !bStartWithDefaultZero", EditConditionHides))
	bool bStartWithRandomZero = false;
	// What is the max amount of clicks for the random elevation zero
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework", meta = (EditCondition = "bCanBeZeroed && bStartWithRandomZero && !bStartWithDefaultZero", EditConditionHides))
	uint8 RandomMaxElevationStartClicks = 25;
	// What is the max amount of clicks for the random windage zero
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework", meta = (EditCondition = "bCanBeZeroed && bStartWithRandomZero && !bStartWithDefaultZero", EditConditionHides))
	uint8 RandomMaxWindageStartClicks = 25;
};

USTRUCT(BlueprintType)
struct FSKGTurretAdjustmentSettings
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework")
	ESKGScopeAdjustment AdjustmentType = ESKGScopeAdjustment::MOA;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework", meta = (EditCondition = "AdjustmentType == ESKGScopeAdjustment::MOA", EditConditionHides))
	ESKGMOAAdjustment MOAAdjustmentAmount = ESKGMOAAdjustment::OneQuarter;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework", meta = (EditCondition = "AdjustmentType == ESKGScopeAdjustment::MRAD", EditConditionHides))
	ESKGMRADAdjustment MRADAdjustmentAmount = ESKGMRADAdjustment::PointOne;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework")
	float RedDotClickMultiplier = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework")
	int32 ElevationClicks = 0;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework")
	int32 WindageClicks = 0;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework")
	float AdjustmentAmount = 0;
	
protected:
	float MRADAdjustment = 0.057296f;// .1 MRAD = 0.0057296f
	float MOAAdjustment = 0.01667f;// 1/4 = 0.004167f
	bool bIsRedDot = false;
	float RedDotAdjustmentMultiplier = 0.5265f;
public:
	FSKGTurretAdjustmentSettings(){}
	FSKGTurretAdjustmentSettings(const FSKGTurretAdjustmentSettings& TurretAdjustmentAmount, bool RedDot = false)
	{
		AdjustmentType = TurretAdjustmentAmount.AdjustmentType;
		MOAAdjustmentAmount = TurretAdjustmentAmount.MOAAdjustmentAmount;
		MRADAdjustmentAmount = TurretAdjustmentAmount.MRADAdjustmentAmount;
		bIsRedDot = RedDot;

		if (AdjustmentType == ESKGScopeAdjustment::MOA)
		{
			switch (MOAAdjustmentAmount)
			{
			case ESKGMOAAdjustment::OneQuarter : AdjustmentAmount = MOAAdjustment * 0.25f; break;
			case ESKGMOAAdjustment::OneHalf : AdjustmentAmount = MOAAdjustment * 0.5f; break;
			case ESKGMOAAdjustment::OneEighth : AdjustmentAmount = MOAAdjustment * 0.125f; break;
			case ESKGMOAAdjustment::One : AdjustmentAmount = MOAAdjustment; break;
			}
		}
		else
		{
			switch (MRADAdjustmentAmount)
			{
			case ESKGMRADAdjustment::PointOne : AdjustmentAmount = MRADAdjustment * 0.1f; break;
			}
		}
		if (bIsRedDot)
		{
			AdjustmentAmount *= RedDotAdjustmentMultiplier * RedDotClickMultiplier;
		}
	}
};

USTRUCT(BlueprintType)
struct FSKGTurretClickEvent
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework")
	int32 OldClicks = 0;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework")
	int32 NewClicks = 0;
};

USTRUCT(BlueprintType)
struct FSKGSightZoomSettings
{
	GENERATED_BODY()
	// Smoothly zoom in/out
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|ZoomSettings")
	bool bSmoothZoom = false;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|ZoomSettings", meta = (EditCondition = "bSmoothZoom", EditConditionHides))
	float SmoothZoomSmoothness = 0.02f;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|ZoomSettings", meta = (EditCondition = "bSmoothZoom", EditConditionHides))
	float SmoothZoomSpeed = 25.0f;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|ZoomSettings")
	bool bFreeZoom = true;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|ZoomSettings", meta = (EditCondition = "bFreeZoom", EditConditionHides))
	float ZoomIncrementAmount = 1.0f;
};

USTRUCT(BlueprintType)
struct FSKGSightMagnification
{
	GENERATED_BODY()
	// Array of magnifications your sight can go through
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|MagnificationSettings")
	TArray<float> Magnifications = { 1.0f };
	// If true, sight will act as first focal plane optic and scale the reticle accordingly
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|MagnificationSettings")
	bool bIsFirstFocalPlane = true;
	// Amount to shrink the eyebox by as you zoom in to simulate real life
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|MagnificationSettings")
	float EyeboxShrinkOnZoomAmount = 30.0f;
	// Useful for scaling the reticle individually to the optic and not just on the material
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|MagnificationSettings")
	float DecreaseReticleScaleAmount = 8.0f;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|MagnificationSettings")
	FSKGSightZoomSettings ZoomSettings;

	uint8 MagnificationIndex = 0;
	float CurrentMagnification = 1.0f;
};

USTRUCT(BlueprintType)
struct FSKGSceneCaptureOptimization
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization")
	bool bOverrideCaptureEveryFrame = false;
	// The refresh rate of the Scene Capture Component (60 = 60 times per second)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "!bOverrideCaptureEveryFrame", EditConditionHides))
	float RefreshRate = 60.0f;
};

USTRUCT(BlueprintType)
struct FSKGSightOptimization
{
	GENERATED_BODY()
	// Use this instead of setting Capture Every Frame
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization")
	bool bOverrideCaptureEveryFrame = false;
	// The refresh rate of the optic (60 = 60 times per second)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "!bOverrideCaptureEveryFrame", EditConditionHides))
	float RefreshRate = 60.0f;
	// Disable the scene capture component when not aiming down sights
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "!bOverrideCaptureEveryFrame", EditConditionHides))
	bool bDisableWhenNotAiming = true;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "bDisableWhenNotAiming && !bOverrideCaptureEveryFrame", EditConditionHides))
	float StopCaptureDelay = 0.0f;
	// Continue running scene capture component at a set refresh rate (5 = 5 times per second)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "!bDisableWhenNotAiming && !bOverrideCaptureEveryFrame", EditConditionHides))
	float NotAimingRefreshRate = 5.0f;
	// When not aiming clear the scope with a color
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "bDisableWhenNotAiming", EditConditionHides))
	bool bClearScopeWithColor = true;
	// Color to clear the scope with
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "bDisableWhenNotAiming && bClearScopeWithColor", EditConditionHides))
	FLinearColor ClearedColor = FLinearColor::Black;
	// When not aiming clear the scope with a material
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "bDisableWhenNotAiming && !bClearScopeWithColor", EditConditionHides))
	bool bClearScopeWithMaterial = false;
	// Material to clear the scope with
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Optimization", meta = (EditCondition = "bDisableWhenNotAiming && !bClearScopeWithColor && bClearScopeWithMaterial", EditConditionHides))
	UMaterialInterface* ClearedScopeMaterial = nullptr;
	
	TWeakObjectPtr<UMaterialInterface> OriginalScopeMaterial = nullptr;
};

USTRUCT(BlueprintType)
struct FSKGRenderTargetSize
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework")
	int32 Width = 512;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework")
	int32 Height = 512;
};

USTRUCT(BlueprintType)
struct FSKGSightZero
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework")
	float Elevation = 0.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework")
	float Windage = 0.0f;
};

USTRUCT(BlueprintType)
struct FSKGReticleBrightness
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|ReticleBrightness")
	TArray<float> ReticleBrightnessSettings = { 1.0f };
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|ReticleBrightness")
	int32 ReticleDefaultBrightnessIndex = 0;
};

class UMaterialInstance;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FSKGRenderTargetMaterial
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|ReticleMaterial")
	UMaterialInstance* Material = nullptr;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|ReticleMaterial")
	UMaterialInstanceDynamic* DynamicMaterial = nullptr;
};

USTRUCT(BlueprintType)
struct FSKGReticleMaterial
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|ReticleMaterial")
	FSKGRenderTargetMaterial RenderTargetMaterial;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|ReticleMaterial")
	float ReticleSize = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|ReticleMaterial")
	float StartingEyeboxRange = -2000.0f;
};

USTRUCT(BlueprintType)
struct FSKGReticleSettings
{
	GENERATED_BODY()
	// Set this to the material index (element) that the reticle will be on
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|ReticleSettings")
	int32 ReticleMaterialIndex = 1;
	// Ignore for now, this is for a future feature
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|ReticleSettings")
	float Radius = 1.0f;
	// Reticle materials you wish to use and cycle through (such as red and green dots)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|ReticleSettings")
	TArray<FSKGReticleMaterial> ReticleMaterials;
	// Reticle Brightness settings to make your reticle brighter/darker
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|ReticleSettings")
	FSKGReticleBrightness ReticleBrightness;
};

USTRUCT(BlueprintType)
struct FSKGManualSceneCaptureSetup
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|SceneCapture")
	int32 MaterialIndex = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|SceneCapture")
	TArray<FSKGRenderTargetMaterial> RenderTargetMaterials;
};