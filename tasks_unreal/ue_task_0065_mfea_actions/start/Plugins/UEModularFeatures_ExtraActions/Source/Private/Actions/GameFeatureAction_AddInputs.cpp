// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddInputs.h"
#include "ModularFeatures_InternalFuncs.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddInputs)
#endif

void UGameFeatureAction_AddInputs::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddInputs::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddInputs::ResetExtension()
{
	ActiveExtensions.Empty();
	AbilityActions.Empty();
	Super::ResetExtension();
}

void UGameFeatureAction_AddInputs::AddToWorld(const FWorldContext& WorldContext)
{
}

void UGameFeatureAction_AddInputs::HandleActorExtension(AActor* Owner, const FName EventName)
{
}

void UGameFeatureAction_AddInputs::AddActorInputs(AActor* TargetActor)
{
}

void UGameFeatureAction_AddInputs::RemoveActorInputs(AActor* TargetActor)
{
	ActiveExtensions.Remove(TargetActor);
}

void UGameFeatureAction_AddInputs::SetupActionBindings(AActor* TargetActor, UObject* FunctionOwner, UEnhancedInputComponent* InputComponent)
{
}

UEnhancedInputLocalPlayerSubsystem* UGameFeatureAction_AddInputs::GetEnhancedInputComponentFromPawn(APawn* TargetPawn)
{
	// Get the Player Controller associated to this pawn
	if (const APlayerController* const PlayerController = TargetPawn->GetController<APlayerController>())
	{
		// Check if the controller is local. We don't want to add inputs to a non-local player
		if (!PlayerController->IsLocalController())
		{
			return nullptr;
		}

		// Get the local player associated to the controller
		if (const ULocalPlayer* const LocalPlayer = PlayerController->GetLocalPlayer())
		{
			// Get the enhanced input subsystem of the local player and check if its valid
			UEnhancedInputLocalPlayerSubsystem* const Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (!IsValid(Subsystem))
			{
				UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: LocalPlayer %s has no EnhancedInputLocalPlayerSubsystem."),
				       *FString(__FUNCTION__), *LocalPlayer->GetName());
				return nullptr;
			}

			return Subsystem;
		}
	}

	return nullptr;
}

FGameplayAbilitySpec UGameFeatureAction_AddInputs::GetAbilitySpecInformationFromBindingData(
	AActor* TargetActor, const FAbilityInputBindingData& AbilityBindingData, UEnum* InputIDEnum)
{
	// Get the ability input ID from the enumeration by its value name
	const int32 InputID = ModularFeaturesHelper::GetInputIDByName(AbilityBindingData.InputIDValueName, InputIDEnum);

	// Create the spec, used as param of ability binding
	FGameplayAbilitySpec NewAbilitySpec;

	// If the user wants to find a active ability spec, we'll try to get the ability system component of the target actor and get the spec using the specified ability class
	if (AbilityBindingData.bFindAbilitySpec)
	{
		if (const UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentInActor(TargetActor))
		{
			// We're not using the InputID to search for existing spec because more than 1 abilities can have the same Input Id
			AbilitySystemComponent->FindAbilitySpecFromClass(TSubclassOf<UGameplayAbility>(AbilityBindingData.AbilityClass.LoadSynchronous()));
		}
		else
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."),
			       *FString(__FUNCTION__), *TargetActor->GetName());
		}
	}
	// If we are not using an existing spec, we'll create a basic spec just to pass some parameters to the ability binding
	else
	{
		// Only add the class if it's valid
		if (!AbilityBindingData.AbilityClass.IsNull())
		{
			NewAbilitySpec.Ability = Cast<UGameplayAbility>(AbilityBindingData.AbilityClass.LoadSynchronous()->GetDefaultObject());
		}

		// Only append tags if the container is not empty
		if (!AbilityBindingData.AbilityTags.IsEmpty())
		{
			NewAbilitySpec.Ability->AbilityTags.AppendTags(AbilityBindingData.AbilityTags);
		}

		NewAbilitySpec.InputID = InputID;
	}

	return NewAbilitySpec;
}
