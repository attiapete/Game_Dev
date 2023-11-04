//Copyright 2021, Dakota Dawe, All rights reserved

#include "Projectiles/SKGProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "SKGProjectileWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/WindDirectionalSource.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"

// Sets default values
ASKGProjectile::ASKGProjectile()
{
	// Create our collision component (a sphere) along with its default values
 	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	RootComponent = CollisionComponent;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Add a mesh to the projectile to allow for an effect such as a tracer being a simple shaped mesh
	// With a bright emissive material to give the tracer effect
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	Mesh->SetupAttachment(CollisionComponent);

	// This sets our drag curve. This curve controls how our bullets drag gets affected.
	DragCurve = CreateDefaultSubobject<UCurveFloat>("DragCurveFloat");

	// Create and setup our projectile movement component. Because we are using a hybrid system (line
	// Traces for hit detection) we disable some things such as bouncing.
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->SetUpdatedComponent(CollisionComponent);
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	// Disable auto activation in order to set custom velocity specified by the user in blueprint.
	ProjectileMovementComponent->bAutoActivate = false;

	// After 10 seconds the bullet is destroyed
	InitialLifeSpan = 10.0f;

	// Set tick to true as the tick function controls our simulation for drag/wind ect.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// VelocityFPS is our velocity in feet per second
	VelocityFPS = 3200.0f;
	// Set our inital velocity to 0
	ProjectileMovementComponent->InitialSpeed = 0.0f;
	ProjectileMovementComponent->MaxSpeed = VelocityFPS * 100.f;

#if WITH_EDITOR
	// To visually see your hits for debug purposes we have a debug box that we draw when the bullet
	// Impacts with an object.
	bDrawProjectilePath = false;
	DebugPathSize = 2.0f;
	bDrawDebugSphereOnImpact = false;
	DebugSphereSize = 10.0f;
#endif
	
	// Dictates whether or not the bullet is affected by wind.
	AffectedByWind = true;

	// Bullet weight to allow for doing calculations with bullet weight and velocity for damage
	// In your own system.
	BulletWeightGrains = 55;

	bReplicates = false;

	CollisionChannel = ECollisionChannel::ECC_Visibility;
	MaxRicochets = 4.0f;
}

// Called when the game starts or when spawned
void ASKGProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Setup our velocity to be in cm/s and set our last location to the spawn location
	VelocityFPS *= 30.48f;

	LastPosition = GetActorLocation();

	// Find and set our bullet to utilize a wind source in the world to be effected by wind
	if (const USKGProjectileWorldSubsystem* Subsystem = GetWorld()->GetSubsystem<USKGProjectileWorldSubsystem>())
	{
		WindSource = Subsystem->GetWindSource();
		if (WindSource)
		{
			if (const UWindDirectionalSourceComponent* WindComponent = WindSource->GetComponent())
			{
				float Weight = 0.1f;
				WindComponent->GetWindParameters(GetActorLocation(), WindData, Weight);
			}
		}
	}

	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());
	Params.bReturnPhysicalMaterial = true;
}

float ASKGProjectile::CalculateDrag() const
{
	if (DragCurve)
	{	// Get the information from our graph based on our projectiles velocity
		const float ProjectileVelocity = ProjectileMovementComponent->Velocity.Size();
		const float DragGraphValue = DragCurve->GetFloatValue(GetGameTimeSinceCreation());

		float Drag = -0.5f * DragGraphValue * 1.225f * 0.0000571f * (ProjectileVelocity * 0.01f) * (ProjectileVelocity * 0.01f);
		Drag /= 3.56394f;
		Drag = Drag * GetWorld()->DeltaTimeSeconds * -100.0f / ProjectileVelocity;
		Drag = 1.0f - Drag;
		// Return our calculated drag
		return Drag;
	}
	return 1.0f;
}

float ASKGProjectile::CalculateHitThickness(const FHitResult& HitResult, FVector& PenetratedLocation)
{
	FHitResult TraceOutHitResult;
	FCollisionQueryParams TraceOutParams;
	TraceOutParams.AddIgnoredActor(this);
	TraceOutParams.AddIgnoredActor(HitResult.GetActor());
		
	const FVector EndLocation = HitResult.Location + GetActorForwardVector() * 10000.0f;
	FVector NewStart = EndLocation;
	if (GetWorld()->LineTraceSingleByChannel(TraceOutHitResult, HitResult.Location, EndLocation, CollisionChannel, TraceOutParams))
	{
		NewStart = TraceOutHitResult.Location;
	}
		
	FHitResult TraceBackHitResult;
	FCollisionQueryParams TraceBackParams;
	TraceBackParams.AddIgnoredActor(this);
	TraceBackParams.AddIgnoredActor(TraceOutHitResult.GetActor());
	GetWorld()->LineTraceSingleByChannel(TraceBackHitResult, NewStart, HitResult.Location, CollisionChannel, TraceBackParams);
	//DrawDebugLine(GetWorld(), TraceBackHitResult.Location, HitResult.Location, FColor::Green, true, -1, 0, 1.5f);
	//DrawDebugLine(GetWorld(), NewStart, HitResult.Location, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), NewStart, 10.0f, 12, FColor::Red, true);
	//DrawDebugSphere(GetWorld(), HitResult.Location, 10.0f, 12, FColor::Green, true);
	PenetratedLocation = TraceBackHitResult.Location;
	return FVector::Dist(TraceBackHitResult.Location, HitResult.Location);
}

// Called every frame
void ASKGProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	ProjectileMovementComponent->Velocity *= CalculateDrag();

	// If the bullet is allowed to be affected by wind, apply the corresponding force.
	if (AffectedByWind)
	{
		ProjectileMovementComponent->AddForce(WindData.Direction * (WindData.Speed / 4.2f));
	}

#if WITH_EDITOR
	bool bDrewPathTrace = false;
	uint8 R = 255;
	uint8 G = 255;
	if (bDrawProjectilePath)
	{
		float Normalized = UKismetMathLibrary::NormalizeToRange(ProjectileMovementComponent->Velocity.Size(), 0.0f, VelocityFPS);
		Normalized = FMath::Clamp(Normalized, 0.0f, 1.0f);
		R = static_cast<uint8>(Normalized * 255);
		G = static_cast<uint8>((Normalized * 255) / 2);
	}
#endif
	
	FHitResult HitResult;
	
	// Perform the line trace starting at the projectiles last tick position and ending at its current position.
	if (GetWorld()->LineTraceSingleByChannel(HitResult, LastPosition, GetActorLocation(), CollisionChannel, Params, FCollisionResponseParams::DefaultResponseParam))
	{
		if (const USkeletalMeshComponent* HitSkeletalMesh = Cast<USkeletalMeshComponent>(HitResult.GetComponent()))
		{
			Params.AddIgnoredActor(HitResult.GetActor());	
		}
		FVector PenetratedLocation;
		const float HitThickness = CalculateHitThickness(HitResult, PenetratedLocation);
		const double ImpactAngle = 180.0 - UKismetMathLibrary::DegAcos(FVector::DotProduct(GetActorForwardVector(), HitResult.ImpactNormal));
		OnProjectileImpact(HitResult, HitThickness, ImpactAngle, PenetratedLocation);

	#if WITH_EDITOR
		if (bDrawDebugSphereOnImpact)
		{
			DrawDebugSphere(GetWorld(), HitResult.Location, DebugSphereSize, 12.0f, FColor::Red, true);
		}
		if (bDrawProjectilePath)
		{
			bDrewPathTrace = true;
			DrawDebugLine(GetWorld(), LastPosition, HitResult.Location, FColor(R, G, 255), true, -1, 0, DebugPathSize);
		}
	#endif
	}

#if WITH_EDITOR
	if (!bDrewPathTrace && bDrawProjectilePath)
	{
		DrawDebugLine(GetWorld(), LastPosition, GetActorLocation(), FColor(R, G, 255), true, -1, 0, DebugPathSize);
	}
#endif
	
	LastPosition = GetActorLocation();
}

void ASKGProjectile::PerformRicochet(const FHitResult& HitResult, float VelocityMultiplier)
{
	++CurrentRicochets;
	if (CurrentRicochets < MaxRicochets)
	{
		const FVector LookAtNormal = UKismetMathLibrary::FindLookAtRotation(HitResult.TraceStart, HitResult.ImpactPoint).Vector();
		const FVector NewDirection = UKismetMathLibrary::GetReflectionVector(LookAtNormal, HitResult.Normal);
		SetActorLocation(HitResult.Location + NewDirection * 1.0f);
		ProjectileMovementComponent->Velocity = (ProjectileMovementComponent->Velocity.Size() * NewDirection) * VelocityMultiplier;
		if (ProjectileMovementComponent->Velocity.Equals(FVector::ZeroVector, 0.01f))
		{
			Destroy();
		}
	}
	else
	{
		Destroy();
	}
}

void ASKGProjectile::SetAtPenetratedLocation(const FVector& PenetratedLocation, float VelocityMultiplier)
{
	SetActorLocation(PenetratedLocation + GetActorForwardVector() * 1.0f);
	ProjectileMovementComponent->Velocity *= VelocityMultiplier;
	if (ProjectileMovementComponent->Velocity.Equals(FVector::ZeroVector, 0.01f))
	{
		Destroy();
	}
}

float ASKGProjectile::CalculateImpactForce() const
{
	return UKismetMathLibrary::NormalizeToRange(FMath::Sqrt(GetVelocity().Size() * BulletWeightGrains), 0.0f, 5000.0f);
}

void ASKGProjectile::ActivateProjectile(float VelocityMultiplier)
{
	ProjectileMovementComponent->Velocity = GetActorForwardVector() * (VelocityFPS * VelocityMultiplier);
	ProjectileMovementComponent->Activate();
}
