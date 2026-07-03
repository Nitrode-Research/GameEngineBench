#pragma once

#include "NativeGameplayTags.h"
#include "AlsxtAIInterface.generated.h"

UINTERFACE(Blueprintable)
class UAlsxtAIInterface : public UInterface {
	GENERATED_BODY()
};

class ALSXT_API IAlsxtAIInterface {
	GENERATED_BODY()

public:

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALSXTAI Interface")
  void SetLockMovement(bool NewLocked);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALSXTAI Interface")
  bool CanAISeeThreatPoint(FVector& ThreatPoint);

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALSXTAI Interface")
  bool ShouldAIAnticipateImpact();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALSXTAI Interface")
  bool ShouldAIBlock();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALSXTAI Interface")
  bool ShouldAIEnterCombatReadyMode();

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALSXTAI Interface")
  bool ShouldAIAvoid();
};