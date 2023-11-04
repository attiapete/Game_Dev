// Copyright 2022, Dakota Dawe, All rights reserved


#include "Misc/AnimNotify/SKGAnimNotify_Footstep.h"
#include "Misc/BlueprintFunctionsLibraries/SKGFPSStatics.h"
#include "Misc/SKGPhysicalMaterial.h"

#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "Components/SkeletalMeshComponent.h"

USKGAnimNotify_Footstep::USKGAnimNotify_Footstep()
{
	FootName = FName("foot_r");
	TraceChannel = ECollisionChannel::ECC_Visibility;
	TraceStartHeight = 20.0f;
	TraceEndHeight = 40.0f;
	bIgnoreZAxis = true;
}

void USKGAnimNotify_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetWorld() && MeshComp->GetOwner() && MeshComp->DoesSocketExist(FootName))
	{
		const FVector FootLocation = MeshComp->GetSocketLocation(FootName);
		const FVector Start = FVector(FootLocation.X, FootLocation.Y, FootLocation.Z + TraceStartHeight);
		const FVector End = FVector(FootLocation.X, FootLocation.Y, FootLocation.Z - TraceEndHeight);

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.bReturnPhysicalMaterial = true;
		QueryParams.AddIgnoredActor(MeshComp->GetOwner());
		
		if (MeshComp->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, TraceChannel, QueryParams))
		{
			HitResult.Location = MeshComp->GetOwner()->GetActorLocation();
			float SoundMultiplierAmount = 1.0f;
			if (SoundMultiplier)
			{
				float Speed = 0.0f;
				if (bIgnoreZAxis)
				{
					FVector Velocity = MeshComp->GetComponentVelocity();
					Velocity.Z = 0.0f;
					Speed = Velocity.Size();
				}
				else
				{
					Speed = MeshComp->GetComponentVelocity().Size();
				}
				SoundMultiplierAmount = SoundMultiplier->GetFloatValue(Speed);
			}
			
			if (USKGPhysicalMaterial* PhysMat = Cast<USKGPhysicalMaterial>(HitResult.PhysMaterial))
			{
				PhysMat->FootstepImpactEffect.SoundSettings.VolumeMultiplier = SoundMultiplierAmount;
				USKGFPSStatics::SpawnImpactEffect(HitResult, PhysMat->FootstepImpactEffect);
			}
			else
			{
				DefaultSound.SoundSettings.VolumeMultiplier = SoundMultiplierAmount;
				USKGFPSStatics::SpawnImpactEffect(HitResult, DefaultSound);
			}
		}
	}
}
