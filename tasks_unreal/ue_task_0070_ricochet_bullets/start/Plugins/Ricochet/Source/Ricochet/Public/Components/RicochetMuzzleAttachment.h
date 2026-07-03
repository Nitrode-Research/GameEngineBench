// Copyright (C) 2025 Uriel Ballinas, VOIDWARE Prohibited. All rights reserved.
// This software is licensed under the MIT License (LICENSE.md).

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Components/ActorComponent.h"
#include "RicochetMuzzleAttachment.generated.h"


/**
* @file RicochetMuzzleAttachment.h
* @brief Barrel Attachment class that affects Recoil, Accuracy, Flash, Sound and Weight.
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RICOCHET_API URicochetMuzzleAttachment : public UActorComponent, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URicochetMuzzleAttachment();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual float GetMuzzleAttachmentWeight();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	

	// Health
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Velocity", meta = (ToolTip = "Amount of recoil applied to the barrel, only works with physics enabled"))
	TSoftObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;

protected:
	// Ability System Component for the bullet (optional, or can be tied to instigator)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
