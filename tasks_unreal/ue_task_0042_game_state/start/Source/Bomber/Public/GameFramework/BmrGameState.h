// Copyright (c) Yevhenii Selivanov

#pragma once

#include "GameFramework/GameState.h"

// UE
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayTagContainer.h"

#include "BmrGameState.generated.h"

/**
 * Own implementation of managing the game's global state.
 */
UCLASS()
class BOMBER_API ABmrGameState final : public AGameStateBase,
                                       public IAbilitySystemInterface,
                                       public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	ABmrGameState();

	/** Returns the current game state, it will crash if can't be obtained, should be used only when the game is running. */
	static ABmrGameState& Get();

	/*********************************************************************************************
	 * Game State Tree
	 ********************************************************************************************* */
public:
	/** Returns the State Tree Component. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE class UStateTreeComponent* GetGameStateTreeComponent() const { return GameStateTreeComponent; }

	/** Returns true if the game state State Tree can be started. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	bool CanStartGameStateTree() const;

	/** Starts the State Tree. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[Bomber]")
	void StartGameStateTree();

	/** Stops the State Tree. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "[Bomber]")
	void StopGameStateTree();

protected:
	/** The State Tree component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "[Bomber]", meta = (BlueprintProtected))
	TObjectPtr<class UStateTreeComponent> GameStateTreeComponent = nullptr;

	/** Binds the game-state change listener. */
	UFUNCTION(BlueprintCallable, Category = "[Bomber]", meta = (BlueprintProtected))
	void BindOnGameStateTagChanged();

	/** Broadcasts the game-state-changed event. */
	UFUNCTION(BlueprintCallable, Category = "[Bomber]", meta = (BlueprintProtected))
	void BroadcastGameStateChanged();

	/** Validates a phase transition against the allowed graph. */
	bool IsTransitionAllowed(const FGameplayTag& PriorTag, const FGameplayTag& NewTag) const;

	/** Last broadcasted phase tag; used by transition validation. */
	UPROPERTY(Transient)
	FGameplayTag LastBroadcastedPhaseTag;

	/*********************************************************************************************
	 * Overrides
	 ********************************************************************************************* */
public:
	/** Returns the Ability System Component for this actor. */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAbilitySystemComponent& GetAbilitySystemComponentChecked() const;

	/** Returns the gameplay tags owned by this actor via IGameplayTagAssetInterface. */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Handles the local-pawn-ready gameplay event. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "[Bomber]", meta = (BlueprintProtected))
	void OnLocalPawnReady(const struct FGameplayEventData& Payload);
};
