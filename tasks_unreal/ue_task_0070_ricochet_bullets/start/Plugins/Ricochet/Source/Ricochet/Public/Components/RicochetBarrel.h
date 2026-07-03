// MIT

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Components/PrimitiveComponent.h"
#include "RicochetBarrel.generated.h"

class URicochetMuzzleAttachment;

/**
* @file RicochetBarrel.h
* @brief Core firearm component that defines Calibers, Fire Modes, Fire Rate, Base Recoil, and has Barrel and Magazine Attachments
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RICOCHET_API URicochetBarrel : public UPrimitiveComponent, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URicochetBarrel();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY()
	UAbilitySystemComponent* AbilitySystemComponent;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	FGameplayTagContainer CompatibleCalibers;

	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	FGameplayTagContainer CompatibleMuzzleAttachments;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Velocity", meta = (ToolTip = "Amount of recoil applied to the barrel, only works with physics enabled"))
	TSoftObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;

	// Muzzle Attachment
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Velocity", meta = (ToolTip = "Amount of recoil applied to the barrel, only works with physics enabled"))
    TSoftClassPtr<URicochetMuzzleAttachment> MuzzleAttachment = nullptr;
    	

	float GetCalculatedRecoilModifier();

	float GetBarrelRecoilModifier();

	float GetBarrelAttachmentRecoilModifier();
};
