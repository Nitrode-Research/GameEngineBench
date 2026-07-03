// MIT


#include "Components/RicochetBarrel.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
URicochetBarrel::URicochetBarrel()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Create the Ability System Component sub-object.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
 
	// Set Replication Mode to Mixed for NPCs.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
}


// Called when the game starts
void URicochetBarrel::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void URicochetBarrel::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void URicochetBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;
	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CompatibleCalibers, Parameters)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, CompatibleMuzzleAttachments, Parameters)
}


UAbilitySystemComponent* URicochetBarrel::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
