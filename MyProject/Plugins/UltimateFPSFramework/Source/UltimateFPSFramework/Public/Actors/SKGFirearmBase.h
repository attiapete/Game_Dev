//Copyright 2023, Dakota Dawe, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "Animation/SkeletalMeshActor.h"
#include "DataTypes/SKGFPSDataTypes.h"
#include "Interfaces/SKGAttachmentInterface.h"
#include "Interfaces/SKGFirearmPartsInterface.h"
#include "GameFramework/Actor.h"
//#include "Grippables/GrippableActor.h" AGrippableActor FOR USE WITH VR EXPANSION PLUGIN

#include "SKGFirearmBase.generated.h"

#define DEFAULT_STATS_MULTIPLIER (FirearmStats.Ergonomics * (10.0f / (FirearmStats.Weight * 1.5f)))

UCLASS()
class ULTIMATEFPSFRAMEWORK_API ASKGFirearmBase : public AActor, public ISKGAttachmentInterface, public ISKGFirearmPartsInterface
{
	GENERATED_BODY()
public:
	ASKGFirearmBase(const FObjectInitializer& ObjectInitializer);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework")
	UMeshComponent* FirearmMesh;
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework")
	USKGAttachmentManager* AttachmentManager;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|Default")
	FSKGFirearmStats DefaultFirearmStats;
	// Random niagara systems for this muzzle, to be used for shooting effects like muzzle flash
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SKGFPSFramework|Default")
	TArray<UNiagaraSystem*> FireNiagaraSystems;
	// Default aim socket such as a mesh with iron sights
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Sockets")
	FName AimSocket;
	// Muzzle socket for the firearm in cases of not using attachments for barrel/muzzle devices
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SKGFPSFramework|Sockets")
	FName MuzzleSocket;

	bool bShouldSpawnDefaultsFromPreset;
	
	UPROPERTY()
	TArray<AActor*> CachedParts;
	UPROPERTY(ReplicatedUsing = OnRep_CachedComponent)
	FSKGFirearmCachedParts CachedComponents;
	UFUNCTION()
	virtual void OnRep_CachedComponent() {}
	UPROPERTY(Replicated)
	FSKGFirearmStats FirearmStats;

	bool bHasRunBeginPlay;
	UPROPERTY()
	TArray<USKGAttachmentComponent*> AttachmentComponentsToConstruct;

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
	void HandleCachingAttachment(USKGAttachmentComponent* AttachmentComponent);
	void HandleUpdateFirearmStats(USKGAttachmentComponent* AttachmentComponent);
public:
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Firearm")
	void DestroyAllParts();

	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Firearm")
	FHitResult MuzzleTrace(float Distance, ECollisionChannel CollisionChannel, bool& bMadeBlockingHit, bool bDrawDebugLine);
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Firearm")
	TArray<FHitResult> MuzzleTraceMulti(float Distance, ECollisionChannel CollisionChannel, bool& bMadeBlockingHit, bool bDrawDebugLine);

	// MOA = Minute of angle. 1 MOA = 1 inch of shift at 100 yards
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Projectile")
	virtual FSKGProjectileTransform GetMuzzleProjectileSocketTransform(float RangeMeters, float MOA = 1.0f);
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Projectile")
	TArray<FSKGProjectileTransform> GetMultipleMuzzleProjectileSocketTransforms(float RangeMeters, float InchSpreadAt25Yards = 40.0f, uint8 ShotCount = 4);
	
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Default", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsSuppressed();
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Default")
	UNiagaraSystem* GetFireNiagaraSystem();

	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Part")
	ASKGMuzzle* GetMuzzleDevice();
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Part")
	AActor* GetMuzzleActor();
	
	// ATTACHMENT INTERFACE
	virtual UMeshComponent* GetMesh_Implementation() override { return FirearmMesh; }
	virtual USkeletalMeshComponent* GetMeshSkeletal_Implementation() override { return (USkeletalMeshComponent*)FirearmMesh; }
	virtual UStaticMeshComponent* GetMeshStatic_Implementation() override { return (UStaticMeshComponent*)FirearmMesh; }
	virtual USKGAttachmentManager* GetAttachmentManager_Implementation() const override { return AttachmentManager; }
	virtual TArray<USKGAttachmentComponent*> GetAttachmentComponents_Implementation() override;
	virtual TArray<USKGAttachmentComponent*> GetAllAttachmentComponents_Implementation(bool bReCache) override;
	virtual void SetLoadDefaultAttachments_Implementation(bool bLoadedByPreset) override { bShouldSpawnDefaultsFromPreset = bLoadedByPreset; }
	virtual bool GetShouldLoadDefaultAttachments_Implementation() override { return bShouldSpawnDefaultsFromPreset; }
	virtual void OnAttachmentUpdated_Implementation() override;
	// END OF ATTACHMENT INTERFACE
	
	// FIREARM PARTS INTERFACE
	virtual TArray<AActor*> GetCachedParts_Implementation() override { return CachedParts; }
	virtual FSKGFirearmCachedParts GetCachedComponents_Implementation() override { return CachedComponents; }
	virtual FTransform GetMuzzleSocketTransform_Implementation() override;
	// END OF FIREARM PARTS INTERFACE
};
