// MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsxtCharacter.h"
#include "Settings/AlsxtAcrobaticActionSettings.h"
#include "Interfaces/AlsxtCharacterInterface.h"
#include "AlsxtAcrobaticActionComponent.generated.h"


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSXT_API UAlsxtAcrobaticActionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAlsxtAcrobaticActionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", Meta = (AllowPrivateAccess))
	TObjectPtr<UAlsxtAcrobaticActionSettings> Settings;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", Meta = (AllowPrivateAccess))
	FAlsxtGeneralAcrobaticActionSettings GeneralAcrobaticActionSettings;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Settings")
	bool CanPerformAcrobaticAction();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void TryAcrobaticAction();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void DetermineAcrobaticActionType(FGameplayTag& AcrobaticActionType);

	void BeginFlip();

	UFUNCTION(Server, Reliable)
	void ServerBeginFlip();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBeginFlip();

	void BeginWallJump();

	void BeginWallRun();

	FTimerHandle WallRunTimer;

	void UpdateWallRun();

	void EndWallRun();
};
