// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Actors/AxosLevelActor.h"
#include "Interfaces/AxosInterface.h"
#include "AxosGameStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNorthWayChanged, float, OldValue, float, NewValue);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AXOS_API UAxosGameStateComponent : public UActorComponent, public IAxosInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAxosGameStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FOnNorthWayChanged OnNorthYawChanged;

	UPROPERTY(ReplicatedUsing=OnRep_UseOverrideYaw, EditAnywhere, BlueprintReadWrite, Category = "Control")
	bool UseOverrideYaw;

	UPROPERTY(ReplicatedUsing=OnRep_NorthYaw, EditAnywhere, BlueprintReadWrite, Category = "Control")
	float NorthYaw;

	UFUNCTION()
	void OnRep_UseOverrideYaw();

	UFUNCTION()
	void OnRep_NorthYaw();

	UFUNCTION(BlueprintCallable)
	float GetNorthYaw();
	
	UFUNCTION(BlueprintCallable)
	void SetNorthYaw(const float NewNorthYaw);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	TSoftObjectPtr<AAxosLevelActor> AxosLevelActor;
	
};
