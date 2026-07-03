#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZedPawn.generated.h"

UCLASS()
class HORDETEMPLATEV2NATIVE_API AZedPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AZedPawn();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zed AI")
	FName PatrolTag;

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE bool GetIsDead() const { return IsDead; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zed AI")
	float Health = 100.f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Zed AI")
	bool IsDead = false;
};
