#include "HordeTrader.h"
#include "Components/SceneComponent.h"
#include "ConstructorHelpers.h"

AHordeTrader::AHordeTrader()
{
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	TraderMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TraderMesh"));
	TraderMeshComponent->SetupAttachment(RootComponent);
	const ConstructorHelpers::FObjectFinder<USkeletalMesh> TraderMeshAsset(TEXT("SkeletalMesh'/Game/HordeTemplateBP/Assets/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin'"));
	if (TraderMeshAsset.Succeeded())
	{
		TraderMeshComponent->SetSkeletalMesh(TraderMeshAsset.Object);
		TraderMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		TraderMeshComponent->SetCollisionProfileName(TEXT("TraderCollision"));
	}

	TraderTextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Trader Header Text"));
	TraderTextComponent->SetupAttachment(TraderMeshComponent);
	TraderTextComponent->SetRelativeLocation(FVector(-29.f, 0.f, 180.f));
	TraderTextComponent->SetRelativeRotation(FRotator(0.f, 90.f, 0.f).Quaternion());
	TraderTextComponent->SetText(FText::FromString("Trader"));
}

void AHordeTrader::PlayWelcome_Implementation() {}
bool AHordeTrader::PlayWelcome_Validate() { return true; }

void AHordeTrader::Interact_Implementation(AActor* InteractingOwner) {}

FInteractionInfo AHordeTrader::GetInteractionInfo_Implementation() { return FInteractionInfo{}; }
