//Copyright 2021, Dakota Dawe, All rights reserved
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneManagement.h"
#include "CollisionQueryParams.h"
#include "SKGProjectile.generated.h"

class AWindDirectionalSource;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UCurveFloat;

UCLASS()
class SKGPROJECTILE_API ASKGProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASKGProjectile();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SKGFPSFramework")
	UStaticMeshComponent* Mesh;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework")
	USphereComponent* CollisionComponent;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework")
	UProjectileMovementComponent* ProjectileMovementComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "SKGFPSFramework|Debug")
	bool bDrawProjectilePath;
	UPROPERTY(EditAnywhere, Category = "SKGFPSFramework|Debug")
	float DebugPathSize;
	UPROPERTY(EditAnywhere, Category = "SKGFPSFramework|Debug")
	bool bDrawDebugSphereOnImpact;
	UPROPERTY(EditAnywhere, Category = "SKGFPSFramework|Debug")
	float DebugSphereSize;
#endif
	
	// The drag curve used for this projectile (air resistance)
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Physics")
	UCurveFloat* DragCurve;
	// Whether or not this bullet is affected by wind
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Physics")
	bool AffectedByWind;
	UPROPERTY(BlueprintReadOnly, Category = "SKGFPSFramework|Physics")
	AWindDirectionalSource* WindSource;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Physics")
	uint8 MaxRicochets;
	// Store our wind data to allow for wind to push projectile on tick
	FWindData WindData;
	uint8 CurrentRicochets;

	FCollisionQueryParams Params;
	FVector LastPosition;

	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Default")
	float VelocityFPS;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Default")
	uint16 BulletWeightGrains;
	UPROPERTY(EditDefaultsOnly, Category = "SKGFPSFramework|Default")
	TEnumAsByte<ECollisionChannel> CollisionChannel;

	virtual void BeginPlay() override;
	float CalculateDrag() const;
	float CalculateHitThickness(const FHitResult& HitResult, FVector& PenetratedLocation);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SKGFPSFramework|Events")
	void OnProjectileImpact(const FHitResult& HitResult, const float HitObjectThickness, const float Angle, const FVector& PenetrationLocation);

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Default")
	UProjectileMovementComponent* GetProjectileMovement() const {return ProjectileMovementComponent;}
	// Get the wind source that is affecting our projectile
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Default")
	AWindDirectionalSource* GetWindSource() const {return WindSource;}

	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Impact")
	void PerformRicochet(const FHitResult& HitResult, float VelocityMultiplier = 1.0f);
	// If velocity after penetration is nearly 0, projectile is destroyed
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Impact")
	void SetAtPenetratedLocation(const FVector& PenetratedLocation, float VelocityMultiplier = 1.0f);

	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Ammo")
	float CalculateImpactForce() const;
	UFUNCTION(BlueprintPure, Category = "SKGFPSFramework|Ammo")
	float GetProjectilePower() const { return FMath::Sqrt(VelocityFPS * BulletWeightGrains); }
	UFUNCTION(BlueprintCallable, Category = "SKGFPSFramework|Ammo")
	void ActivateProjectile(float VelocityMultiplier);
};
