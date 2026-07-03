// Copyright (c) Yevhenii Selivanov

#pragma once

#include "DalPrimaryDataAsset.h"

#include "GameplayTagContainer.h"

#include "BmrGameStateDataAsset.generated.h"

/**
 * A single legal transition between two game-state phase tags.
 * `From = FGameplayTag()` (an empty tag) marks a legal initial state — i.e., the first phase
 * tag the game state is allowed to enter from no prior state.
 */
USTRUCT(BlueprintType)
struct BOMBER_API FBmrGameStateTransition
{
	GENERATED_BODY()

	/** The previously-broadcasted phase tag, or an empty tag to mark a legal initial state. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "GameState"))
	FGameplayTag From;

	/** The phase tag being entered. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "GameState"))
	FGameplayTag To;
};

/**
 * The data of the game match.
 */
UCLASS()
class BOMBER_API UBmrGameStateDataAsset final : public UDalPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Returns the Game State data asset. */
	static const UBmrGameStateDataAsset& Get();

	static constexpr float GTickInterval = 0.2f;

	/** Returns general value how ofter update actors and states in the game. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE float GetTickInterval() const { return GTickInterval; }

	/** Return the summary time required to start the 'Three-two-one-GO' timer. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE int32 GetStartingCountdown() const { return StartingCountdown; }

	/** Returns the left second to the end of the game. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE int32 GetInGameCountdown() const { return InGameCountdown; }

	/** Returns the maximum ping compensation in seconds for any time-based networked actions, like bomb detonation. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE float GetMaxPingCompensationSec() const { return MaxPingCompensationSec; }

	/** Returns the empty startup map that is used only in builds at launch and designed for fast boot before transitioning to the gameplay level. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	const FORCEINLINE TSoftObjectPtr<UWorld>& GetStartupLevel() const { return StartupLevel; }

	/** Returns the main gameplay level to load. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	const FORCEINLINE TSoftObjectPtr<UWorld>& GetMainLevel() const { return MainLevel; }

	/** Returns the State Tree asset that is used to manage the overall game state. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE class UStateTree* GetGameStateTreeAsset() const { return GameStateTreeAsset; }

	/** Returns the authoritative list of allowed game-state phase transitions.
	 * If the editor-authored array is empty, falls back to the canonical project graph
	 * (Menu → GameStarting → InGame → EndGame → Menu) so source ships usable without editor work. */
	const TArray<FBmrGameStateTransition>& GetAllowedTransitions() const;

protected:
	/** The summary seconds of launching 'Three-two-one-GO' timer that is used on game starting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	int32 StartingCountdown = 3;

	/** Seconds to the end of the round. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	int32 InGameCountdown = 120;

	/** Maximum ping compensation in seconds for any time-based networked actions, like bomb detonation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	float MaxPingCompensationSec = 0.5f;

	/* Empty startup map that is used only in builds at launch.
	 * Designed for fast boot before transitioning to the gameplay level.
	 * Contains no references to content to ensure minimal load time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	TSoftObjectPtr<UWorld> StartupLevel = nullptr;

	/** The main gameplay level to load. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	TSoftObjectPtr<UWorld> MainLevel = nullptr;

	/** The State Tree asset that is used to manage the overall game state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	TObjectPtr<UStateTree> GameStateTreeAsset = nullptr;

	/** Authoritative graph of allowed phase-tag transitions. Empty = use canonical fallback in GetAllowedTransitions(). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, TitleProperty = "{From} -> {To}"))
	TArray<FBmrGameStateTransition> AllowedTransitions;
};