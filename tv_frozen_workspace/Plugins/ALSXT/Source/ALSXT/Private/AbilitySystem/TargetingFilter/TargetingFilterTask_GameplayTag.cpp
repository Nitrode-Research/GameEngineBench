// MIT


#include "AbilitySystem/TargetingFilter/TargetingFilterTask_GameplayTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"


UTargetingFilterTask_GameplayTag::UTargetingFilterTask_GameplayTag(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool UTargetingFilterTask_GameplayTag::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	if (AActor* TargetActor = TargetData.HitResult.GetActor())
	{
		
		if (TargetActor->Implements<UAbilitySystemInterface>())
		{
			return false;
		}

		UAbilitySystemComponent* TargetASC {nullptr};
		TScriptInterface<IAbilitySystemInterface> TargetAbilitySystemInterface;

		if (TargetActor->Implements<UAbilitySystemInterface>())
		{
			TargetAbilitySystemInterface = TScriptInterface<IAbilitySystemInterface>(TargetActor);
			IAbilitySystemInterface* MyRetrievedInterfacePtr = TargetAbilitySystemInterface.GetInterface();
			TargetASC = MyRetrievedInterfacePtr->GetAbilitySystemComponent();
		}
		
		// IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
		// IAbilitySystemInterface* MyRetrievedInterfacePtr = TargetAbilitySystemInterface.GetInterface();
		// TargetASC = MyRetrievedInterfacePtr->GetAbilitySystemComponent();
		// 
		// if (AbilitySystemInterface)
		// {
		// 	TargetASC = AbilitySystemInterface->GetAbilitySystemComponent();
		// }

		// if the target is one of these classes, filter it out
		for (FGameplayTagContainer GameplayFilter : IgnoredGameplayTagFilters)
		{
			if (TargetASC->HasAnyMatchingGameplayTags(GameplayFilter))
			{
				return true;
			}
		}

		// if the target is one of these classes, do not filter it out
		for (FGameplayTagContainer GameplayFilter : RequiredGameplayTagFilters)
		{
			if (TargetASC->HasAnyMatchingGameplayTags(GameplayFilter))
			{
				return false;
			}
		}

		// if we do not have required class filters, we do NOT want to filter this target
		return (RequiredGameplayTagFilters.Num() > 0);
	}

	return true;
}

