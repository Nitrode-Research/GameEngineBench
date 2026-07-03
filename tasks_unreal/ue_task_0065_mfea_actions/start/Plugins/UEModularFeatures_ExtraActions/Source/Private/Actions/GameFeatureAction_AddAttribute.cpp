// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddAttribute.h"
#include "ModularFeatures_InternalFuncs.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/GameInstance.h"
#include "Runtime/Launch/Resources/Version.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddAttribute)
#endif

void UGameFeatureAction_AddAttribute::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddAttribute::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddAttribute::ResetExtension()
{
	ActiveExtensions.Empty();
	Super::ResetExtension();
}

void UGameFeatureAction_AddAttribute::AddToWorld(const FWorldContext& WorldContext)
{
}

void UGameFeatureAction_AddAttribute::HandleActorExtension(AActor* Owner, const FName EventName)
{
}

void UGameFeatureAction_AddAttribute::AddAttribute(AActor* TargetActor)
{
}

void UGameFeatureAction_AddAttribute::RemoveAttribute(AActor* TargetActor)
{
	ActiveExtensions.Remove(TargetActor);
}
