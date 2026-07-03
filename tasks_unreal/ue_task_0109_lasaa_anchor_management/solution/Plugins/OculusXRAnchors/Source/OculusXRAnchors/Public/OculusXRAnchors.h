#pragma once

#include "CoreMinimal.h"
#include "OculusXRAnchorComponent.h"

namespace EOculusXRAnchorResult
{
	enum Type
	{
		Success,
		Failure
	};
}

enum class EOculusXRSpaceStorageLocation : uint8
{
	Local,
	Cloud
};

struct FOculusXRSpaceQueryResult
{
	FOculusXRUUID UUID;
	FTransform Transform;
};

DECLARE_DELEGATE_TwoParams(FOculusXRSpatialAnchorCreateDelegate, EOculusXRAnchorResult::Type, UOculusXRAnchorComponent*);
DECLARE_DELEGATE_TwoParams(FOculusXRAnchorSaveDelegate, EOculusXRAnchorResult::Type, UOculusXRAnchorComponent*);
DECLARE_DELEGATE_TwoParams(FOculusXRAnchorEraseDelegate, EOculusXRAnchorResult::Type, FOculusXRUUID);
DECLARE_DELEGATE_TwoParams(FOculusXRAnchorQueryDelegate, EOculusXRAnchorResult::Type, const TArray<FOculusXRSpaceQueryResult>&);

namespace OculusXRAnchors
{
	class OCULUSXRANCHORS_API FOculusXRAnchors
	{
	public:
		static void CreateSpatialAnchor(const FTransform& Transform, AActor* Owner, FOculusXRSpatialAnchorCreateDelegate Delegate, EOculusXRAnchorResult::Type& OutResult);
		static void SaveAnchor(UOculusXRAnchorComponent* AnchorComponent, EOculusXRSpaceStorageLocation Location, FOculusXRAnchorSaveDelegate Delegate, EOculusXRAnchorResult::Type& OutResult);
		static void EraseAnchor(UOculusXRAnchorComponent* AnchorComponent, FOculusXRAnchorEraseDelegate Delegate, EOculusXRAnchorResult::Type& OutResult);
		static void QueryAnchors(const TArray<FOculusXRUUID>& UUIDs, EOculusXRSpaceStorageLocation Location, FOculusXRAnchorQueryDelegate Delegate, EOculusXRAnchorResult::Type& OutResult);
	};
}