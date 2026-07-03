#pragma once

#include "PlayerSpectatingInterface.generated.h"

UINTERFACE(Blueprintable)
class UPlayerSpectatingInterface : public UInterface {
	GENERATED_BODY()
};

class TARGETVECTORCODE_API IPlayerSpectatingInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Spectating")
  bool IsPlayerSpectating();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Spectating")
  void StartSpectatingCooldown();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Spectating")
  void StopSpectatingCooldown();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Spectating")
  float GetSpectatingCooldownLength();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Spectating")
  float GetSpectatingCooldownValue();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Player State|Spectating")
  void OnSpectatingCooldownComplete();

};