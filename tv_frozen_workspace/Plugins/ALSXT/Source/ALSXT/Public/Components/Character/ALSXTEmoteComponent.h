// MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsCharacter.h"
#include "AlsxtCharacter.h"
#include "Settings/AlsxtEmoteSettings.h"
#include "NativeGameplayTags.h"
#include "AlsxtEmoteComponent.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSXT_API UAlsxtEmoteComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAlsxtEmoteComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (AllowPrivateAccess))
	TObjectPtr<UAlsxtEmoteSettings> EmoteSettings;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Movement System")
	bool CanEmote();

	// Desired Emote

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Meta = (AllowPrivateAccess))
	FGameplayTag Emote{ FGameplayTag::EmptyTag };

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Emote", Meta = (AutoCreateRefTerm = "NewEmoteTag"))
	void AddDesiredEmote(const FGameplayTag& NewEmote);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddDesiredEmote(const FGameplayTag& NewEmote);

	UFUNCTION(BlueprintCallable, Category = "ALS|Movement System")
	FGameplayTag GetEmote() const;

	// UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character", Meta = (AutoCreateRefTerm = "NewFreelookState"))
	// FALSXTFreelookState ProcessNewFreelookState(const FALSXTFreelookState& NewFreelookState);

private:
	UFUNCTION(Server, Reliable)
		void ServerAddDesiredEmote(const FGameplayTag& NewEmote);

	// Emote

private:
	void AddEmote(const FGameplayTag& NewEmote);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Emote")
	void OnEmote(const FGameplayTag& NewEmote);
};
