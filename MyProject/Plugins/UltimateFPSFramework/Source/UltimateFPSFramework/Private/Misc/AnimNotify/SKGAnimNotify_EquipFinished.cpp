// Copyright 2022, Dakota Dawe, All rights reserved


#include "Misc/AnimNotify/SKGAnimNotify_EquipFinished.h"

#include "SKGCharacterAnimInstance.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

USKGAnimNotify_EquipFinished::USKGAnimNotify_EquipFinished()
{
	
}

void USKGAnimNotify_EquipFinished::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (GetCharacterComponent(MeshComp))
	{
		if (USKGCharacterAnimInstance* AnimInstance = Cast<USKGCharacterAnimInstance>(MeshComp->GetAnimInstance()))
		{
			AnimInstance->SetEquipped(true);
		}
		CharacterComponent->SetCanAim(true);
	}
}

USKGCharacterComponent* USKGAnimNotify_EquipFinished::GetCharacterComponent(const USkeletalMeshComponent* MeshComp)
{
	if (CharacterComponent)
	{
		return CharacterComponent.Get();
	}

	if (MeshComp && MeshComp->GetOwner())
	{
		CharacterComponent = Cast<USKGCharacterComponent>(MeshComp->GetOwner()->GetComponentByClass(USKGCharacterComponent::StaticClass()));
		return CharacterComponent.Get();
	}
	return nullptr;
}
