// MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsCharacter.h"
#include "AlsxtCharacter.h"
#include "NativeGameplayTags.h"
#include "Settings/AlsxtGestureSettings.h"
#include "AlsxtGestureComponent.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSXT_API UAlsxtGestureComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAlsxtGestureComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Als Character", Meta = (AllowPrivateAccess))
	AAlsxtCharacter* Character{ Cast<AAlsxtCharacter>(GetOwner()) };

	AAlsCharacter* AlsCharacter{ Cast<AAlsCharacter>(GetOwner()) };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (AllowPrivateAccess))
	TObjectPtr<UAlsxtGestureSettings> GestureSettings;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Settings")
	bool CanGesture();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Replicated, Category = "State", Meta = (AllowPrivateAccess))
	UAnimMontage* CurrentGestureMontage;
	
	// Desired Gesture

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Gesture", Meta = (AutoCreateRefTerm = "NewGestureTag"))
	void AddDesiredGesture(const FGameplayTag& Gesture, const FGameplayTag& GestureHand);

	UFUNCTION(BlueprintCallable, Category = "ALS|Gesture")
	void ExitGesture(bool Immediate);

private:
	UFUNCTION(Server, Reliable)
	void ServerAddDesiredGesture(const FGameplayTag& Gesture, const FGameplayTag& GestureHand);

	// Emote

private:
	void AddGesture(const FGameplayTag& Gesture, const FGameplayTag& GestureHand);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Gesture")
	void OnGesture(const FGameplayTag& Gesture, const FGameplayTag& GestureHand);
};
