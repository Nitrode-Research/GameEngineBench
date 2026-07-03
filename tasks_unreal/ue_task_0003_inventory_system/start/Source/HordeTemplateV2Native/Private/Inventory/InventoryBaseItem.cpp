#include "InventoryBaseItem.h"
#include "Net/UnrealNetwork.h"

AInventoryBaseItem::AInventoryBaseItem()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetReplicates(true);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root Component"));

	WorldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldMesh"));
	WorldMesh->SetupAttachment(RootComponent);
}

void AInventoryBaseItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInventoryBaseItem, ItemInfo);
	DOREPLIFETIME(AInventoryBaseItem, ItemID);
}

void AInventoryBaseItem::BeginPlay()
{
	Super::BeginPlay();
}

void AInventoryBaseItem::Interact_Implementation(AActor* InteractingOwner) {}

FInteractionInfo AInventoryBaseItem::GetInteractionInfo_Implementation() { return FInteractionInfo{}; }

void AInventoryBaseItem::PopInfo() {}

void AInventoryBaseItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}
