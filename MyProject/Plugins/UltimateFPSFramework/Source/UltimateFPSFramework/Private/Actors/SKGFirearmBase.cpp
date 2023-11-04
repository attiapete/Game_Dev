//Copyright 2023, Dakota Dawe, All rights reserved


#include "Actors/SKGFirearmBase.h"
#include "Actors/SKGFirearm.h"
#include "Actors/FirearmParts/SKGMuzzle.h"
#include "Components/SKGAttachmentManager.h"
#include "DataTypes/SKGFPSFrameworkMacros.h"
#include "Interfaces/SKGAimInterface.h"
#include "Interfaces/SKGFirearmAttachmentsInterface.h"
#include "Interfaces/SKGRenderTargetInterface.h"
#include "Misc/SKGFPSFrameworkDeveloperSettings.h"
#include "Misc/BlueprintFunctionsLibraries/SKGFPSStatics.h"

#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/World.h"

ASKGFirearmBase::ASKGFirearmBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	NetUpdateFrequency = 6.0f;

	AttachmentManager = CreateDefaultSubobject<USKGAttachmentManager>(TEXT("FPSTemplateAttachmentManager"));

	bShouldSpawnDefaultsFromPreset = true;
	
	DefaultFirearmStats.Weight = 7.0f;
	DefaultFirearmStats.Ergonomics = 50.0f;
	DefaultFirearmStats.VerticalRecoilMultiplier = 1.0f;
	DefaultFirearmStats.HorizontalRecoilMultiplier = 1.0f;
}

void ASKGFirearmBase::BeginPlay()
{
	Super::BeginPlay();
}

void ASKGFirearmBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (const USKGFPSFrameworkDeveloperSettings* Settings = GetDefault<USKGFPSFrameworkDeveloperSettings>())
	{
		TArray<UActorComponent*> ActorComponents = GetComponentsByTag(UMeshComponent::StaticClass(), Settings->FirearmMeshTag);
		if (ActorComponents.Num())
		{
			FirearmMesh = Cast<UMeshComponent>(ActorComponents[0]);
		}
		else
		{
			const FString Message = FString::Printf(TEXT("%s: FirearmMesh Does NOT have the ComponentTag set in the project settings added, Set this"), *GetName());
			UKismetSystemLibrary::PrintString(GetWorld(), Message, true, false, FLinearColor::Blue, 10.0f);
			UE_LOG(LogTemp, Error, TEXT("%s"), *Message);
		}
	}
	else
	{
		const FString Message = FString::Printf(TEXT("%s: FirearmMesh Does NOT have the SKGFirearm ComponentTag added, Set this"), *GetName());
		UKismetSystemLibrary::PrintString(GetWorld(), Message, true, false, FLinearColor::Blue, 10.0f);
		UE_LOG(LogTemp, Error, TEXT("%s"), *Message);
	}
}

void ASKGFirearmBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGFirearmBase, CachedComponents, Params);
	
	FDoRepLifetimeParams ParamsCondition;
	ParamsCondition.bIsPushBased = true;
	ParamsCondition.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASKGFirearmBase, FirearmStats, ParamsCondition);
}

FHitResult ASKGFirearmBase::MuzzleTrace(float Distance, ECollisionChannel CollisionChannel, bool& bMadeBlockingHit, bool bDrawDebugLine)
{
	FTransform MuzzleTransform = FSKGProjectileTransform::GetTransformFromProjectile(Execute_GetMuzzleSocketTransform(this));
	FVector End = MuzzleTransform.GetLocation() + MuzzleTransform.Rotator().Vector() * Distance;
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	
	bMadeBlockingHit = GetWorld()->LineTraceSingleByChannel(HitResult, MuzzleTransform.GetLocation(), End, CollisionChannel, Params);
	if (bDrawDebugLine)
	{
		DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), End, FColor::Red, false, 3.0f, 0, 2.0f);
	}
	return HitResult;
}

TArray<FHitResult> ASKGFirearmBase::MuzzleTraceMulti(float Distance, ECollisionChannel CollisionChannel, bool& bMadeBlockingHit, bool bDrawDebugLine)
{
	const FTransform MuzzleTransform = FSKGProjectileTransform::GetTransformFromProjectile(Execute_GetMuzzleSocketTransform(this));
	const FVector End = MuzzleTransform.GetLocation() + MuzzleTransform.Rotator().Vector() * Distance;
	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	
	bMadeBlockingHit = GetWorld()->LineTraceMultiByChannel(HitResults, MuzzleTransform.GetLocation(), End, CollisionChannel, Params);
	if (bDrawDebugLine)
	{
		DrawDebugLine(GetWorld(), MuzzleTransform.GetLocation(), End, FColor::Red, false, 3.0f, 0, 2.0f);
	}
	return HitResults;
}

TArray<FSKGProjectileTransform> ASKGFirearmBase::GetMultipleMuzzleProjectileSocketTransforms(float RangeMeters, float InchSpreadAt25Yards,	uint8 ShotCount)
{
	TArray<FSKGProjectileTransform> ProjectileTransforms;
	ProjectileTransforms.Reserve(ShotCount);	InchSpreadAt25Yards *= 4.0f;
	for (uint8 i = 0; i < ShotCount; ++i)
	{
		ProjectileTransforms.Add(GetMuzzleProjectileSocketTransform(RangeMeters, InchSpreadAt25Yards));
	}
	return ProjectileTransforms;
}

FSKGProjectileTransform ASKGFirearmBase::GetMuzzleProjectileSocketTransform(float RangeMeters, float MOA)
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

	FTransform SightTransform;
	if (CachedComponents.Sights.Num() && CachedComponents.Sights[0])
	{
		if (AActor* Sight = CachedComponents.Sights[0]->GetAttachment())
		{
			SightTransform = ISKGProceduralAnimationInterface::Execute_GetAimSocketTransform(Sight);
		}
	}
	else if (FirearmMesh && FirearmMesh->DoesSocketExist(AimSocket))
	{
		SightTransform = FirearmMesh->GetSocketTransform(AimSocket);
	}
	else
	{
		SightTransform = Execute_GetMuzzleSocketTransform(this);
		FVector SightLocation = SightTransform.GetLocation();
		SightLocation.Z += 5.0f;
		SightTransform.SetLocation(SightLocation);
	}
	
	FTransform MuzzleTransform = Execute_GetMuzzleSocketTransform(this);
	FRotator MuzzleRotation = USKGFPSStatics::GetEstimatedMuzzleToScopeZero(MuzzleTransform, SightTransform, RangeMeters);	
	MuzzleRotation = USKGFPSStatics::SetMuzzleMOA(MuzzleRotation, MOA);

	MuzzleTransform.SetRotation(MuzzleRotation.Quaternion());
	return MuzzleTransform;
}

bool ASKGFirearmBase::IsSuppressed()
{
	const ASKGMuzzle* Muzzle = GetMuzzleDevice();
	if (IsValid(Muzzle) && Muzzle->IsSuppressor())
	{
		return true;
	}
	return false;
}

UNiagaraSystem* ASKGFirearmBase::GetFireNiagaraSystem()
{
	AActor* MuzzleActor = GetMuzzleActor();
	if (IsValid(MuzzleActor) && MuzzleActor->GetClass()->ImplementsInterface(USKGMuzzleInterface::StaticClass()))
	{
		if (UNiagaraSystem* NiagaraSystem = ISKGMuzzleInterface::Execute_GetFireNiagaraSystem(MuzzleActor))
		{
			return NiagaraSystem;
		}
	}
	
	const int32 RandomIndex = USKGFPSStatics::GetRandomIndexForArray(FireNiagaraSystems.Num());
	if (RandomIndex != INDEX_NONE)
	{
		return FireNiagaraSystems[RandomIndex];
	}
	return nullptr;
}

ASKGMuzzle* ASKGFirearmBase::GetMuzzleDevice()
{
	return Cast<ASKGMuzzle>(GetMuzzleActor());
}

AActor* ASKGFirearmBase::GetMuzzleActor()
{
	USKGAttachmentComponent* AttachmentComponent = CachedComponents.Muzzle;
	if (AttachmentComponent)
	{
		AActor* Muzzle = AttachmentComponent->GetAttachment();
		if (IsValid(Muzzle))
		{
			return Muzzle;
		}
	}
	
	AttachmentComponent = CachedComponents.Barrel;
	if (AttachmentComponent)
	{
		AActor* Barrel = AttachmentComponent->GetAttachment();
		if (IsValid(Barrel))
		{
			return Barrel;
		}
	}
	return this;
}

void ASKGFirearmBase::DestroyAllParts()
{
	AttachmentManager->DestroyAllAttachments();
}

void ASKGFirearmBase::HandleCachingAttachment(USKGAttachmentComponent* AttachmentComponent)
{
	if (IsValid(AttachmentComponent))
	{
		AActor* Part = AttachmentComponent->GetAttachment();
		if (IsValid(Part) && Part->GetClass()->ImplementsInterface(USKGFirearmAttachmentsInterface::StaticClass()))
		{
			bool bAimable = false;
			bool bHasRenderTarget = false;
			if (Part->GetClass()->ImplementsInterface(USKGAimInterface::StaticClass()))
			{
				bAimable = ISKGAimInterface::Execute_IsAimable(Part);
			}
			if (Part->GetClass()->ImplementsInterface(USKGRenderTargetInterface::StaticClass()))
			{
				bHasRenderTarget = ISKGRenderTargetInterface::Execute_HasRenderTarget(Part);
			}
			CachedComponents.AddAttachment(AttachmentComponent, ISKGFirearmAttachmentsInterface::Execute_GetPartType(Part), bAimable, bHasRenderTarget);
			if (HasAuthority())
			{
				MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearmBase, CachedComponents, this);
			}
		}
	}
}

void ASKGFirearmBase::HandleUpdateFirearmStats(USKGAttachmentComponent* AttachmentComponent)
{
	if (HasAuthority() && IsValid(AttachmentComponent))
	{
		AActor* Part = AttachmentComponent->GetAttachment();
		if (IsValid(Part) && Part->GetClass()->ImplementsInterface(USKGFirearmAttachmentsInterface::StaticClass()))
		{
			const FSKGFirearmPartStats PartStats = ISKGFirearmAttachmentsInterface::Execute_GetPartStats(Part, false);
			FSKGFirearmStats::UpdateStats(FirearmStats, DefaultFirearmStats, PartStats);
			MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearmBase, FirearmStats, this);
		}
	}
}

TArray<USKGAttachmentComponent*> ASKGFirearmBase::GetAttachmentComponents_Implementation()
{
	return AttachmentManager->GetAttachmentComponents();
}

TArray<USKGAttachmentComponent*> ASKGFirearmBase::GetAllAttachmentComponents_Implementation(bool bReCache)
{
	if (IsValid(AttachmentManager))
	{
		return AttachmentManager->GetAllAttachmentComponents(false);
	}
	SKG_PRINT(TEXT("%s: has an INVALID attachment manager. Check and make sure your blueprint is not currupted"), *GetName());
	return TArray<USKGAttachmentComponent*>();
}

void ASKGFirearmBase::OnAttachmentUpdated_Implementation()
{
	CachedComponents.Empty();
	CachedParts.Empty();
	CachedParts.Reserve(AttachmentManager->GetAllAttachmentComponents(false).Num());
	if (HasAuthority())
	{
		FirearmStats = DefaultFirearmStats;
	}
	for (USKGAttachmentComponent* AttachmentComponent : AttachmentManager->GetAllAttachmentComponents(false))
	{
		if (IsValid(AttachmentComponent))
		{
			HandleCachingAttachment(AttachmentComponent);
			HandleUpdateFirearmStats(AttachmentComponent);
			if (AActor* Attachment = AttachmentComponent->GetAttachment())
			{
				CachedParts.Add(AttachmentComponent->GetAttachment());
			}
		}
	}
	CachedParts.Shrink();

	if (HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ASKGFirearmBase, CachedComponents, this);
	}
}

FTransform ASKGFirearmBase::GetMuzzleSocketTransform_Implementation()
{
	AActor* MuzzleActor = GetMuzzleActor();
	if (IsValid(MuzzleActor) && MuzzleActor->GetClass()->ImplementsInterface(USKGMuzzleInterface::StaticClass()))
	{
		return ISKGMuzzleInterface::Execute_GetMuzzleSocketTransform(MuzzleActor);
	}
	if (FirearmMesh)
	{
		return FirearmMesh->GetSocketTransform(MuzzleSocket);
	}
	return FTransform();
}