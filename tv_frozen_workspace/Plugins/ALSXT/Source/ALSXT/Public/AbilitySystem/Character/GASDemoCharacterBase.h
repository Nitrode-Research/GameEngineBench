// MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Data/AlsxtAbilitySystemData.h"
#include "GASDemoCharacterBase.generated.h"

class UGASDemoCharacterMovementComponent;
class UAlsxtAbilitySystemComponent;
// Character Base is a template class that contains all shared logic for "Player Characters" and "Non Player Characters".
// This Class is Abstract and should not be used directly! (Not-Blueprintable)
UCLASS(Abstract, NotBlueprintable)
class ALSXT_API AGASDemoCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGASDemoCharacterBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "MovementComponent")
	UGASDemoCharacterMovementComponent* GetGASDemoCharacterMovementComponent() const;
	
	// Implement the IAbilitySystemInterface. (This is used to find the Ability System Component.)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Ability System")
	UAlsxtAbilitySystemComponent* GetALSXTAbilitySystemComponent() const;
	
	void InitializeAbilitySystem();

	// This event is fired after Ability System Component initialization is finished.
	UFUNCTION(BlueprintNativeEvent)
	void PostInitializeAbilitySystem();

	UFUNCTION(BlueprintPure)
	const FAlsxtAbilitySystemInitializationData& GetAbilitySystemInitializationData() const
	{
		return AbilitySystemInitializationData;
	}
protected:

	// Data used to initialize the Ability System Component. (Can be found in "AbilitySystemData.h")
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Ability System", Meta = (ShowOnlyInnerProperties))
	FAlsxtAbilitySystemInitializationData AbilitySystemInitializationData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAlsxtAbilitySystemComponent> AbilitySystemComponent;

	void MovementSpeedMultiplierChanged(const FOnAttributeChangeData& OnAttributeChangeData);
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
};