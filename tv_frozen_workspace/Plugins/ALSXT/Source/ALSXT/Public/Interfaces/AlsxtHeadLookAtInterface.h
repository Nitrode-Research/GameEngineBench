#pragma once

#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "Utility/AlsxtStructs.h"
#include "State/AlsxtHeadLookAtState.h"
#include "AlsxtHeadLookAtInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtHeadLookAtInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtHeadLookAtInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  bool CanHeadLookAt() const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Head Look At Interface")
  bool CanBeHeadLookedAt() const;

  UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Head Look At Interface")
  void IsInFrontOf(bool& IsInFrontOf, FVector LookAtActorLocation) const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  FAlsxtHeadLookAtActor GetHeadLookAtActor(FVector FromLocation) const;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Head Look At Interface")
  FVector GetHeadLookAtLocation();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Head Look At Interface")
  FTransform GetHeadLookAtTransform();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Head Look At Interface")
  FAlsxtHeadLookAtSettings GetLookAtSettings();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  void AddHeadLookAtEntry(FAlsxtHeadLookAtEntry HeadLookAtEntry);

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  void RemoveHeadLookAtEntry(FAlsxtHeadLookAtEntry HeadLookAtEntry);

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  void ResetHeadLookAt();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  FAlsxtHeadLookAtEntry GetBestHeadLookAtEntry() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  FAlsxtHeadLookAtEntry GetNewBestGazeHeadLookAtEntry() const;

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  void BeginHeadLookAt(FAlsxtHeadLookAtEntry HeadLookAtEntry);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Head Look At Interface")
  void OnBeginHeadLookAt();

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Head Look At Interface")
  void EndHeadLookAt();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Head Look At Interface")
  UPARAM(meta = (Categories = "Als.Facial Expression")) FGameplayTagContainer GetFacialReactions();
};