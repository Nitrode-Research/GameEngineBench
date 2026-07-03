#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "OculusXRAnchorComponent.generated.h"

USTRUCT(BlueprintType)
struct OCULUSXRANCHORS_API FOculusXRUUID
{
	GENERATED_BODY()

	UPROPERTY()
	FString Value;

	FOculusXRUUID()
		: Value(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens))
	{
	}

	explicit FOculusXRUUID(const FString& InValue)
		: Value(InValue)
	{
	}

	FString ToString() const
	{
		return Value;
	}

	bool operator==(const FOculusXRUUID& Other) const
	{
		return Value == Other.Value;
	}
};

UCLASS(ClassGroup=(OculusXR), meta=(BlueprintSpawnableComponent))
class OCULUSXRANCHORS_API UOculusXRAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UOculusXRAnchorComponent();

	const FOculusXRUUID& GetUUID() const
	{
		return UUID;
	}

	void SetUUID(const FOculusXRUUID& InUUID)
	{
		UUID = InUUID;
	}

private:
	UPROPERTY()
	FOculusXRUUID UUID;
};