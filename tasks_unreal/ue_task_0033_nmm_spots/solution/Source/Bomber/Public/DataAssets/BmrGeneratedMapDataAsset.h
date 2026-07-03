// Copyright (c) Yevhenii Selivanov

#pragma once

#include "DataAssets/BmrLevelActorDataAsset.h"

// Bomber
#include "Structures/BmrGeneratedMapSettings.h"

#include "BmrGeneratedMapDataAsset.generated.h"

/**
 * Contains all data that describe all levels.
 */
UCLASS(Blueprintable, BlueprintType)
class BOMBER_API UBmrGeneratedMapDataAsset final : public UBmrBaseDataAsset
{
	GENERATED_BODY()

public:
	/** Returns the generated map data asset. */
	static const UBmrGeneratedMapDataAsset& Get();

	/** Returns settings for runtime generation of the level map.
	 * Prefer ABmrGeneratedMap::GetGenerationSetting() instead, as defaults might be overridden by Class Defaults of the Generated Map itself. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	const FORCEINLINE FBmrGeneratedMapSettings& GetGenerationSettings() const { return GenerationSettings; }

	/** Returns asset that contains scalable collision. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE TSubclassOf<class AActor> GetCollisionsAssetClass() const { return CollisionsAsset; }

	/** Returns height offset to spawn actors above the level to avoid collision issues on spawn. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "[Bomber]")
	FORCEINLINE float GetActorsHeightOffset() const { return ActorsHeightOffset; }

protected:
	/** Contains settings for runtime generation of the level map.
	 * These settings might be overridden by Class Defaults of the Generated Map itself. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	FBmrGeneratedMapSettings GenerationSettings;

	/** Asset that contains scalable collision. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	TSubclassOf<class AActor> CollisionsAsset = nullptr;

	/** Height offset to spawn actors above the level to avoid collision issues on spawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, ShowOnlyInnerProperties))
	float ActorsHeightOffset = 100.f;
};