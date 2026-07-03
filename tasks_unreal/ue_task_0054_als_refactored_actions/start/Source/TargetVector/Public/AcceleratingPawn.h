// MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AcceleratingPawn.generated.h"

UCLASS()
class TARGETVECTOR_API AAcceleratingPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AAcceleratingPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
		UStaticMeshComponent* meshComponent;

	float accelerationValue;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	bool hasAStaticMesh();

	float acceleration();

	void accelerate();

	float currentSpeed();

};
