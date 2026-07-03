#include "OculusXRAnchors.h"
#include "GameFramework/Actor.h"

void OculusXRAnchors::FOculusXRAnchors::CreateSpatialAnchor(const FTransform& Transform, AActor* Owner, FOculusXRSpatialAnchorCreateDelegate Delegate, EOculusXRAnchorResult::Type& OutResult)
{
	OutResult = EOculusXRAnchorResult::Failure;
	UOculusXRAnchorComponent* Component = nullptr;
	if (Owner)
	{
		Component = NewObject<UOculusXRAnchorComponent>(Owner);
		if (Component)
		{
			Component->SetWorldTransform(Transform);
			Component->RegisterComponent();
			Owner->AddInstanceComponent(Component);
			OutResult = EOculusXRAnchorResult::Success;
		}
	}
	Delegate.ExecuteIfBound(OutResult, Component);
}

void OculusXRAnchors::FOculusXRAnchors::SaveAnchor(UOculusXRAnchorComponent* AnchorComponent, EOculusXRSpaceStorageLocation, FOculusXRAnchorSaveDelegate Delegate, EOculusXRAnchorResult::Type& OutResult)
{
	OutResult = AnchorComponent ? EOculusXRAnchorResult::Success : EOculusXRAnchorResult::Failure;
	Delegate.ExecuteIfBound(OutResult, AnchorComponent);
}

void OculusXRAnchors::FOculusXRAnchors::EraseAnchor(UOculusXRAnchorComponent* AnchorComponent, FOculusXRAnchorEraseDelegate Delegate, EOculusXRAnchorResult::Type& OutResult)
{
	OutResult = AnchorComponent ? EOculusXRAnchorResult::Success : EOculusXRAnchorResult::Failure;
	const FOculusXRUUID UUID = AnchorComponent ? AnchorComponent->GetUUID() : FOculusXRUUID();
	Delegate.ExecuteIfBound(OutResult, UUID);
	if (AnchorComponent)
	{
		AnchorComponent->DestroyComponent();
	}
}

void OculusXRAnchors::FOculusXRAnchors::QueryAnchors(const TArray<FOculusXRUUID>& UUIDs, EOculusXRSpaceStorageLocation, FOculusXRAnchorQueryDelegate Delegate, EOculusXRAnchorResult::Type& OutResult)
{
	TArray<FOculusXRSpaceQueryResult> Results;
	for (const FOculusXRUUID& UUID : UUIDs)
	{
		FOculusXRSpaceQueryResult Result;
		Result.UUID = UUID;
		Result.Transform = FTransform::Identity;
		Results.Add(Result);
	}
	OutResult = EOculusXRAnchorResult::Success;
	Delegate.ExecuteIfBound(OutResult, Results);
}