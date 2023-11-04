// Copyright 2022, Dakota Dawe, All rights reserved


#include "Misc/AnimNotify/SKGAnimNotify_ReloadComplete.h"

#include "Interfaces/SKGFirearmInterface.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"

USKGAnimNotify_ReloadComplete::USKGAnimNotify_ReloadComplete()
{
	
}

void USKGAnimNotify_ReloadComplete::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (IsValid(MeshComp) && MeshComp->GetOwner() && MeshComp->GetOwner()->GetClass()->ImplementsInterface(USKGFirearmInterface::StaticClass()))
	{
		ISKGFirearmInterface::Execute_OnReloadComplete(MeshComp->GetOwner());
	}
}
