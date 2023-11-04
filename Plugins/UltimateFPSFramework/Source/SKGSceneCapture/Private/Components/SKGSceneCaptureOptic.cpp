// Copyright 2022, Dakota Dawe, All rights reserved


#include "Components/SKGSceneCaptureOptic.h"

#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

constexpr float FOVDivider = 15.0f;

USKGSceneCaptureOptic::USKGSceneCaptureOptic(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
	bUseAttachParentBound = true;
	
	bHiddenInGame = false;
	bCaptureEveryFrame = false;
	bCaptureOnMovement = false;
	bAlwaysPersistRenderingState = true;
	PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CaptureSource = ESceneCaptureSource::SCS_SceneColorHDRNoAlpha;
	
	ShowFlags.DynamicShadows = true;
	ShowFlags.AmbientOcclusion = false;
	ShowFlags.AmbientCubemap = false;
	ShowFlags.DistanceFieldAO = false;
	ShowFlags.LightFunctions = false;
	ShowFlags.LightShafts = false;
	ShowFlags.ReflectionEnvironment = true;
	ShowFlags.ScreenSpaceReflections = false;
	ShowFlags.TexturedLightProfiles = false;
	ShowFlags.VolumetricFog = true;
	ShowFlags.MotionBlur = 0;

	PostProcessSettings.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;
	PostProcessSettings.bOverride_DynamicGlobalIlluminationMethod = 1;
	PostProcessSettings.ReflectionMethod = EReflectionMethod::Lumen;
	PostProcessSettings.bOverride_ReflectionMethod = 1;

	ReticleIndex = 0;
	ReticleBrightnessIndex = 0;

	RenderTargetMaterialVarName = FName("RenderTarget");

	bZoomingIn = false;
}

void USKGSceneCaptureOptic::BeginPlay()
{
	Super::BeginPlay();
	
	SetComponentTickEnabled(false);
	bCaptureEveryFrame = false;
	bCaptureOnMovement = false;
	if (!Optimization.bOverrideCaptureEveryFrame)
	{
		SetComponentTickInterval(1.0f / Optimization.RefreshRate);
	}
	SetupRenderTarget();
}

void USKGSceneCaptureOptic::PostInitProperties()
{
	Super::PostInitProperties();
	SetTurretAdjustment(TurretAdjustmentSettings);
}

void USKGSceneCaptureOptic::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USKGSceneCaptureOptic::SetupRenderTarget()
{
	if (!RenderTarget.IsValid())
	{
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), RenderTargetSize.Width, RenderTargetSize.Height, ETextureRenderTargetFormat::RTF_RGBA16f);
		TextureTarget = RenderTarget.Get();
		StartingSceneCaptureRot = GetRelativeRotation();
	}
}

void USKGSceneCaptureOptic::StartCapture()
{
	GetWorld()->GetTimerManager().ClearTimer(TDelayedStopCaptureHandle);
	if (!OwningMesh.IsValid())
	{
		SetupRenderTarget();
	}

	if (OwningMesh.IsValid() && Optimization.bClearScopeWithMaterial && ReticleIndex < ReticleSettings.ReticleMaterials.Num() && ReticleSettings.ReticleMaterials[ReticleIndex].RenderTargetMaterial.DynamicMaterial)
	{
		OwningMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, ReticleSettings.ReticleMaterials[ReticleIndex].RenderTargetMaterial.DynamicMaterial);
	}
	if (!Optimization.bOverrideCaptureEveryFrame && !bCaptureEveryFrame)
	{
		CaptureScene();
		SetComponentTickInterval(1.0f / Optimization.RefreshRate);
	}
	
	bCaptureEveryFrame = true;
	bCaptureOnMovement = true;
	SetComponentTickEnabled(true);
}

void USKGSceneCaptureOptic::StopCapture(bool bForce)
{
	if (!Optimization.bOverrideCaptureEveryFrame && !Optimization.bDisableWhenNotAiming)
	{
		SetComponentTickInterval(1.0f / Optimization.NotAimingRefreshRate);
	}
	else if (OwningMesh.IsValid())
	{
		if (Optimization.bClearScopeWithColor)
		{
			UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), TextureTarget, Optimization.ClearedColor);
		}
		else if (Optimization.bClearScopeWithMaterial && Optimization.ClearedScopeMaterial)
		{
			OwningMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, Optimization.ClearedScopeMaterial);
		}

		if (!bForce && Optimization.StopCaptureDelay > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(TDelayedStopCaptureHandle, this, &USKGSceneCaptureOptic::DelayStopCapture, Optimization.StopCaptureDelay, false);
		}
		else
		{
			SetComponentTickEnabled(false);
			bCaptureEveryFrame = false;
			bCaptureOnMovement = false;
		}
	}
}

void USKGSceneCaptureOptic::SetOwningMesh(UMeshComponent* Mesh)
{
	OwningMesh = Mesh;
	if (MagnificationSettings.Magnifications.Num())
	{
		MagnificationSettings.CurrentMagnification = MagnificationSettings.Magnifications[0];
	}
	
	for (FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
	{
		if (Reticle.RenderTargetMaterial.Material)
		{
			Reticle.RenderTargetMaterial.DynamicMaterial = UMaterialInstanceDynamic::Create(Reticle.RenderTargetMaterial.Material, this);
			if (IsValid(Reticle.RenderTargetMaterial.DynamicMaterial))
			{
				Reticle.RenderTargetMaterial.DynamicMaterial->GetScalarParameterValue(FName("ReticleSize"), Reticle.ReticleSize);
				Reticle.RenderTargetMaterial.DynamicMaterial->SetTextureParameterValue(RenderTargetMaterialVarName, RenderTarget.Get());
				float EyeboxRange;
				Reticle.RenderTargetMaterial.DynamicMaterial->GetScalarParameterValue(FName("EyeboxRange"), EyeboxRange);
				Reticle.StartingEyeboxRange = EyeboxRange;
				if (MagnificationSettings.bIsFirstFocalPlane)
				{
					Reticle.RenderTargetMaterial.DynamicMaterial->SetScalarParameterValue(FName("ReticleSize"), (MagnificationSettings.CurrentMagnification / MagnificationSettings.DecreaseReticleScaleAmount) * Reticle.ReticleSize);
				}
			}
		}
	}
	
	SetReticle(ReticleIndex);
		
	FOVAngle = FOVDivider / MagnificationSettings.CurrentMagnification;
	PreviousMagnification = MagnificationSettings.CurrentMagnification;
	SmoothMagnification = MagnificationSettings.CurrentMagnification;
}

void USKGSceneCaptureOptic::HandleReticleSmoothZoom()
{
	if (!MagnificationSettings.ZoomSettings.bFreeZoom)
	{
		MagnificationToInterpTo = MagnificationSettings.CurrentMagnification;
	}
	
	SmoothMagnification = FMath::FInterpConstantTo(SmoothMagnification, MagnificationToInterpTo, GetWorld()->DeltaTimeSeconds, MagnificationSettings.ZoomSettings.SmoothZoomSpeed);
	HandleZoom(SmoothMagnification);
	
	if ((SmoothMagnification >= MagnificationToInterpTo && bZoomingIn) || (SmoothMagnification <= MagnificationToInterpTo && !bZoomingIn))
	{
		ZoomInAmount = 0;
		GetWorld()->GetTimerManager().ClearTimer(TZoomHandle);
	}
}

void USKGSceneCaptureOptic::HandleZoom(float Magnification)
{
	for (const FSKGReticleMaterial& Reticle : ReticleSettings.ReticleMaterials)
	{
		if (IsValid(Reticle.RenderTargetMaterial.DynamicMaterial))
		{
			float EyeboxRange;
			Reticle.RenderTargetMaterial.DynamicMaterial->GetScalarParameterValue(FName("EyeboxRange"), EyeboxRange);
			if (EyeboxRange)
			{
				EyeboxRange = Reticle.StartingEyeboxRange - (MagnificationSettings.EyeboxShrinkOnZoomAmount * Magnification);
				Reticle.RenderTargetMaterial.DynamicMaterial->SetScalarParameterValue(FName("EyeboxRange"), EyeboxRange);
			}
			if (MagnificationSettings.bIsFirstFocalPlane)
			{
				Reticle.RenderTargetMaterial.DynamicMaterial->SetScalarParameterValue(FName("ReticleSize"), (Magnification / MagnificationSettings.DecreaseReticleScaleAmount) * Reticle.ReticleSize);
			}
		}
	}
		
	FOVAngle = FOVDivider / Magnification;
}

void USKGSceneCaptureOptic::Zoom()
{
	if (bZoomingIn)
	{
		if (MagnificationSettings.MagnificationIndex + 1 < MagnificationSettings.Magnifications.Num())
		{
			++MagnificationSettings.MagnificationIndex;
		}
	}
	else
	{
		if (MagnificationSettings.MagnificationIndex - 1 >= 0)
		{
			--MagnificationSettings.MagnificationIndex;
		}
	}

	MagnificationSettings.MagnificationIndex = FMath::Clamp((int)MagnificationSettings.MagnificationIndex, 0, MagnificationSettings.Magnifications.Num() - 1);
	MagnificationSettings.CurrentMagnification = MagnificationSettings.Magnifications[MagnificationSettings.MagnificationIndex];
	
	if (MagnificationSettings.ZoomSettings.bSmoothZoom)
	{
		if (MagnificationSettings.ZoomSettings.bFreeZoom)
		{
			ZoomInAmount += bZoomingIn ? MagnificationSettings.ZoomSettings.ZoomIncrementAmount : -MagnificationSettings.ZoomSettings.ZoomIncrementAmount;
			MagnificationToInterpTo = SmoothMagnification + ZoomInAmount;
			MagnificationToInterpTo = FMath::Clamp(MagnificationToInterpTo, GetMinMagnification(), GetMaxMagnification());
		}

		PreviousMagnification = SmoothMagnification;
		GetWorld()->GetTimerManager().SetTimer(TZoomHandle, this, &USKGSceneCaptureOptic::HandleReticleSmoothZoom, MagnificationSettings.ZoomSettings.SmoothZoomSmoothness, true);
	}
	else
	{
		HandleZoom(MagnificationSettings.CurrentMagnification);
	}
}

void USKGSceneCaptureOptic::ZoomIn()
{
	if (!bZoomingIn)
	{
		ZoomInAmount = 0;
	}
	bZoomingIn = true;
	Zoom();
}

void USKGSceneCaptureOptic::ZoomOut()
{
	if (bZoomingIn)
	{
		ZoomInAmount = 0;
	}
	bZoomingIn = false;
	Zoom();
}

float USKGSceneCaptureOptic::GetMaxMagnification()
{
	return MagnificationSettings.Magnifications[MagnificationSettings.Magnifications.Num() - 1];
}

float USKGSceneCaptureOptic::GetMinMagnification()
{
	return MagnificationSettings.Magnifications[0];
}

void USKGSceneCaptureOptic::DelayStopCapture()
{
	StopCapture(true);
}

float USKGSceneCaptureOptic::GetMagnification() const
{
	return MagnificationSettings.ZoomSettings.bSmoothZoom ? SmoothMagnification : MagnificationSettings.CurrentMagnification;
}

float USKGSceneCaptureOptic::GetMagnificationSensitivity() const
{
	return 1.0f / (MagnificationSettings.ZoomSettings.bSmoothZoom ? SmoothMagnification : MagnificationSettings.CurrentMagnification);
}

void USKGSceneCaptureOptic::ReturnToZeroElevation()
{
	SightZero.Elevation = 0.0f;
	FRotator CurrentRot = GetRelativeRotation();
	CurrentRot.Pitch = StartingSceneCaptureRot.Pitch;
	SetRelativeRotation(CurrentRot);
}

void USKGSceneCaptureOptic::ReturnToZeroWindage()
{
	SightZero.Windage = 0.0f;
	FRotator CurrentRot = GetRelativeRotation();
	CurrentRot.Yaw = StartingSceneCaptureRot.Yaw;
	SetRelativeRotation(CurrentRot);
}

void USKGSceneCaptureOptic::SetNewZeroElevation()
{
	SightZero.Elevation = 0.0f;
	StartingSceneCaptureRot.Pitch = GetRelativeRotation().Pitch;
}

void USKGSceneCaptureOptic::SetNewZeroWindage()
{
	SightZero.Windage = 0.0f;
	StartingSceneCaptureRot.Yaw = GetRelativeRotation().Yaw;
}

FSKGTurretClickEvent USKGSceneCaptureOptic::PointOfImpactUp(bool bUp, uint8 Clicks)
{
	if (Clicks > 0)
	{
		ElevationClicksChanged.OldClicks = TurretAdjustmentSettings.ElevationClicks;
		FRotator CurrentRot = GetRelativeRotation();
		if (bUp)
		{
			TurretAdjustmentSettings.ElevationClicks += Clicks;
			CurrentRot.Pitch -= TurretAdjustmentSettings.AdjustmentAmount * Clicks;
		}
		else
		{
			TurretAdjustmentSettings.ElevationClicks -= Clicks;
			CurrentRot.Pitch += TurretAdjustmentSettings.AdjustmentAmount * Clicks;
		}
		SetRelativeRotation(CurrentRot);
		ElevationClicksChanged.NewClicks = TurretAdjustmentSettings.ElevationClicks;
		OnElevationChanged.Broadcast(ElevationClicksChanged);
	}
	return ElevationClicksChanged;
}

FSKGTurretClickEvent USKGSceneCaptureOptic::PointOfImpactLeft(bool bLeft, uint8 Clicks)
{
	if (Clicks > 0)
	{
		WindageClicksChanged.OldClicks = TurretAdjustmentSettings.WindageClicks;
		FRotator CurrentRot = GetRelativeRotation();
		if (bLeft)
		{
			TurretAdjustmentSettings.WindageClicks -= Clicks;
			CurrentRot.Yaw += TurretAdjustmentSettings.AdjustmentAmount * Clicks;
		}
		else
		{
			TurretAdjustmentSettings.WindageClicks += Clicks;
			CurrentRot.Yaw -= TurretAdjustmentSettings.AdjustmentAmount * Clicks;
		}
		SetRelativeRotation(CurrentRot);
		WindageClicksChanged.NewClicks = TurretAdjustmentSettings.WindageClicks;
		OnWindageChanged.Broadcast(WindageClicksChanged);
	}
	return WindageClicksChanged;
}

void USKGSceneCaptureOptic::CycleReticle()
{
	if (ReticleSettings.ReticleMaterials.Num())
	{
		if (++ReticleIndex >= ReticleSettings.ReticleMaterials.Num())
		{
			ReticleIndex = 0;
		}
		SetReticle(ReticleIndex);
	}
}

void USKGSceneCaptureOptic::SetReticle(uint8 Index)
{
	if (OwningMesh.IsValid() && Index < ReticleSettings.ReticleMaterials.Num() && ReticleSettings.ReticleMaterials[Index].RenderTargetMaterial.DynamicMaterial)
	{
		ReticleIndex = Index;
		OwningMesh->SetMaterial(ReticleSettings.ReticleMaterialIndex, ReticleSettings.ReticleMaterials[ReticleIndex].RenderTargetMaterial.DynamicMaterial);
	}
}

void USKGSceneCaptureOptic::SetReticleBrightness(uint8 Index)
{
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

void USKGSceneCaptureOptic::SetReticleSettings(const FSKGReticleSettings& INReticleSettings)
{
	ReticleSettings = INReticleSettings;
}

void USKGSceneCaptureOptic::SetupManually(FSKGManualSceneCaptureSetup& ManualSettings)
{
	if (OwningMesh.IsValid() && ManualSettings.RenderTargetMaterials.Num())
	{
		for (FSKGRenderTargetMaterial& Material : ManualSettings.RenderTargetMaterials)
		{
			if (Material.Material)
			{
				Material.DynamicMaterial = UMaterialInstanceDynamic::Create(Material.Material, this);
				if (IsValid(Material.DynamicMaterial))
				{
					Material.DynamicMaterial->SetTextureParameterValue(RenderTargetMaterialVarName, RenderTarget.Get());
				}
			}
		}
		OwningMesh->SetMaterial(ManualSettings.MaterialIndex, ManualSettings.RenderTargetMaterials[0].DynamicMaterial);
	}
}

void USKGSceneCaptureOptic::SetTurretAdjustment(const FSKGTurretAdjustmentSettings& TurretAdjustment)
{
	TurretAdjustmentSettings = FSKGTurretAdjustmentSettings(TurretAdjustment);
}
