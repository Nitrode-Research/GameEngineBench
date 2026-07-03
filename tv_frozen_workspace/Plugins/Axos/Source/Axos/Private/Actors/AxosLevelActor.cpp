// MIT

#include "Actors/AxosLevelActor.h"

#include "Components/AxosGameStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameState.h"

// Sets default values
AAxosLevelActor::AAxosLevelActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	GameState =nullptr;
	ReplicatedYaw = 0.0f;

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("MyArrow"));
	ArrowComponent->SetupAttachment(RootComponent); 
	ArrowComponent->SetArrowColor(FLinearColor::Red);
}

void AAxosLevelActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAxosLevelActor, ReplicatedYaw);
}

// Called when the game starts or when spawned
void AAxosLevelActor::BeginPlay()
{
	Super::BeginPlay();

	if (AGameState* GS = GetWorld()->GetGameState<AGameState>())
	{
		GameState = GS;
		// Bind the GameState's RepNotify function for the override
		//GameState.Get()->OnRep_AuthoritativeYaw.AddDynamic(this, &AAxosLevelActor::OnGameStateOverrideUpdated);
		if (const TSoftObjectPtr<UAxosGameStateComponent> AxosGameStateComponent = IAxosInterface::Execute_GetAxosGameStateComponent(GS))
		{
			ReplicatedYaw = AxosGameStateComponent->GetNorthYaw();
			SetActorRotation(FRotator(0.0f, ReplicatedYaw, 0.0f));
			AxosGameStateComponent->OnNorthYawChanged.AddDynamic(this, &AAxosLevelActor::OnGameStateChange);
		}
	}

	if (DaySequenceActor)
	{
		DaySequenceActor->SetActorRotation(FRotator(0, ReplicatedYaw, 0));
	}
	
}

// Called every frame
void AAxosLevelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// On all clients and server
	if (bHasGameStateOverride)
	{
		// Use the authoritative override yaw from the GameState
		SetActorRotation(FRotator(0, GameStateOverrideYaw, 0));
	}
	else if (!HasAuthority())
	{
		// Client smooths its rotation towards the server's replicated yaw
		FRotator CurrentRot = GetActorRotation();
		FRotator TargetRot(0, ReplicatedYaw, 0);
		SetActorRotation(FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 10.0f));
	}

}

void AAxosLevelActor::OnGameStateChange(float OldValue, float NewValue)
{
	ReplicatedYaw = NewValue;
	SetActorRotation(FRotator(0, NewValue, 0));
}

void AAxosLevelActor::Server_SetYaw_Implementation(float NewYaw)
{
	// The server receives the client request
	if (!bHasGameStateOverride)
	{
		ReplicatedYaw = NewYaw;
		SetActorRotation(FRotator(0, NewYaw, 0));
	}
}

void AAxosLevelActor::OnGameStateOverrideUpdated()
{
	// if (GameState.Get())
	// {
	// 	// The GameState has an override active, use it
	// 	bHasGameStateOverride = GameState.Get()->GetAuthoritativeYawOverride().IsNearlyZero() == false;
	// 	GameStateOverrideYaw = GameState.Get()->GetAuthoritativeYawOverride().Yaw;
// 
	// 	if (bHasGameStateOverride)
	// 	{
	// 		// Apply the override immediately
	// 		SetActorRotation(FRotator(0, GameStateOverrideYaw, 0));
	// 	}
	// }
}
