// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddEffects.h"
#include "ModularFeatures_InternalFuncs.h"
#include "Engine/GameInstance.h"
#include "Components/GameFrameworkComponentManager.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddEffects)
#endif

void UGameFeatureAction_AddEffects::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddEffects::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddEffects::ResetExtension()
{
	ActiveExtensions.Empty();
	Super::ResetExtension();
}

void UGameFeatureAction_AddEffects::AddToWorld(const FWorldContext& WorldContext)
{
}

void UGameFeatureAction_AddEffects::HandleActorExtension(AActor* Owner, const FName EventName)
{
}

void UGameFeatureAction_AddEffects::AddEffects(AActor* TargetActor, const FEffectStackedData& Effect)
{
}

void UGameFeatureAction_AddEffects::RemoveEffects(AActor* TargetActor)
{
	ActiveExtensions.Remove(TargetActor);
}
