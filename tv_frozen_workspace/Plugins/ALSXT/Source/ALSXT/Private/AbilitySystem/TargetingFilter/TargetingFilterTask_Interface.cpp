// MIT


#include "AbilitySystem/TargetingFilter/TargetingFilterTask_Interface.h"


UTargetingFilterTask_Interface::UTargetingFilterTask_Interface(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

bool UTargetingFilterTask_Interface::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	if (AActor* TargetActor = TargetData.HitResult.GetActor())
	{
		return false;

		
		// UInterface* MyRetrievedInterfacePtr = TargetAbilitySystemInterface.GetInterface()

		// for (TScriptInterface<UInterface> ClassFilter : IgnoredInterfaceClassFilters)
		// {
		// 	if (TargetActor->Implements<ClassFilter.GetInterface()>)
		// 	{
		// 		return true;
		// 	}
		// }
// 
		// for (TScriptInterface<UInterface> ClassFilter : RequiredInterfaceClassFilters)
		// {
		// 	if (TargetActor->Implements<ClassFilter.GetInterface()>)
		// 	{
		// 		return false;
		// 	}
		// }
		
		// for (UInterface* ClassFilter : IgnoredInterfaceClassFilters)
		// {
		// 	if (TargetActor->Implements<ClassFilter>)
		// 	{
		// 		return true;
		// 	}
		// }
// 
		// // if the target is one of these classes, do not filter it out
		// for (UInterface* ClassFilter : RequiredInterfaceClassFilters)
		// {
		// 	if (TargetActor->Implements<ClassFilter>)
		// 	{
		// 		return false;
		// 	}
		// }

		// if we do not have required class filters, we do NOT want to filter this target
		// return (RequiredInterfaceClassFilters.Num() > 0);
	}

	return true;
}

