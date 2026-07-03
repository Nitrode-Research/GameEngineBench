// Fill out your copyright notice in the Description page of Project Settings.


#include "Performance/RogueSignificanceComponent.h"
#include "ActionRoguelike.h"
#include "NiagaraComponent.h"
#include "RogueSignificanceInterface.h"
#include "ParticleHelper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RogueSignificanceComponent)

/* Allows us to force significance on all classes to quickly compare the performance differences as if the system was disabled */
static float GForcedSignificance = -1;
static FAutoConsoleVariableRef CVarSignificanceManager_ForceSignificance(
	TEXT("SigMan.ForceSignificance"),
	GForcedSignificance,
	TEXT("Force significance on all managed objects. -1 is default, 0-4 is hidden, lowest, medium, highest.\n"),
	ECVF_Cheat
	);


URogueSignificanceComponent::URogueSignificanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void URogueSignificanceComponent::BeginPlay()
{
	Super::BeginPlay();
}


void URogueSignificanceComponent::RegisterWithManager()
{
}


void URogueSignificanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


float URogueSignificanceComponent::CalcSignificance(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint) const
{
	return 0.0f;
}


void URogueSignificanceComponent::PostSignificanceUpdate(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
}


float URogueSignificanceComponent::GetSignificanceByDistance(float DistanceSqrd) const
{
	return 0.0f;
}


void URogueSignificanceComponent::UpdateParticleSignificance(float NewSignificance)
{
	// Niagara Particle Systems
	// @todo: we don't need to call into niagara, the EffectType significance handler can do this for us...
	/*TArray<UNiagaraComponent*> NiagaraSystems;
	GetOwner()->GetComponents<UNiagaraComponent>(NiagaraSystems);

	for (UNiagaraComponent* Comp : NiagaraSystems)
	{
		// Niagara uses 'int32 index' to set significance, you should map this with the input "float NewSignificance" (eg. not something between 0.0-1.0 as it gets rounded)

		Comp->SetSystemSignificanceIndex(NewSignificance);
	}*/
}
