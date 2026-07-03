// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddAbilities.h"
#include "ModularFeatures_InternalFuncs.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "InputAction.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddAbilities)
#endif

void UGameFeatureAction_AddAbilities::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddAbilities::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddAbilities::ResetExtension()
{
	ActiveExtensions.Empty();
	Super::ResetExtension();
}

void UGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext)
{
}

void UGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Owner, const FName EventName)
{
}

void UGameFeatureAction_AddAbilities::AddActorAbilities(AActor* TargetActor, const FAbilityMapping& Ability)
{
}

void UGameFeatureAction_AddAbilities::RemoveActorAbilities(AActor* TargetActor)
{
	ActiveExtensions.Remove(TargetActor);
}
