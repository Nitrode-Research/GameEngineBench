// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/RogueGameplayInterface.h"
#include "GameFramework/Actor.h"
#include "RogueMonsterCorpse.generated.h"


class URogueMonsterData;

UCLASS()
class ACTIONROGUELIKE_API ARogueMonsterCorpse : public AActor, public IRogueGameplayInterface
{
	GENERATED_BODY()

public:

	void SetCorpseProperties(USkeletalMeshComponent* ReferenceMeshComp, URogueMonsterData* MonsterData);

	USkeletalMeshComponent* GetMesh() const
	{
		return MeshComp;
	}

	virtual bool AddImpulseAtLocationCustom(FVector Impulse, FVector Location, FName BoneName = NAME_None) override;

protected:

	/* Allow remapping bones to others in the skeleton to better handle impulses */
	UPROPERTY(EditDefaultsOnly, Category=Physics) // @todo: dont expose to BP
	TMap<FName, FName> ImpulseBoneRemapping;

	//UPROPERTY(EditAnywhere, Category="Corpse")
	FName PoseSnapshotName = "InitialPose";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Components)
	TObjectPtr<USkeletalMeshComponent> MeshComp;

public:

	ARogueMonsterCorpse();

};
