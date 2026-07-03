#pragma once

#include "CoreMinimal.h"
#include "Math/UnrealMath.h"
#include "DataConfig/Extra/DcExtraCommon.h"
#include "DcExtraTestFixtures.generated.h"

USTRUCT()
struct DATACONFIGEXTRA_API FDcExtraTestSimpleStruct1
{
	GENERATED_BODY()

	UPROPERTY() FName NameField;
	UPROPERTY(meta = (DcSkip)) int IntFieldWithDefault = 253;
};

USTRUCT()
struct DATACONFIGEXTRA_API FDcExtraTestSimpleStruct2
{
	GENERATED_BODY()

	UPROPERTY() FString StrField;
};


DECLARE_DELEGATE(FDcExtraTestSingularAction);

USTRUCT()
struct DATACONFIGEXTRA_API FDcExtraTestDestructDelegateContainer
{
	GENERATED_BODY()

	FDcExtraTestSingularAction DestructAction;

	~FDcExtraTestDestructDelegateContainer()
	{
		DestructAction.ExecuteIfBound();
	}
};

USTRUCT()
struct DATACONFIGEXTRA_API FDcExtraTestCopyDelegateContainer
{
	GENERATED_BODY()

	UPROPERTY() FString PropValue;
	FString NonPropValue;

	FDcExtraTestSingularAction CopyAction;

	FDcExtraTestCopyDelegateContainer() = default;

	FDcExtraTestCopyDelegateContainer& operator=(const FDcExtraTestCopyDelegateContainer& InOther)
	{
		if (this != &InOther)
		{
			InOther.CopyAction.ExecuteIfBound();
			PropValue = InOther.PropValue;
			NonPropValue = InOther.NonPropValue;
		}

		return *this;
	}
};

USTRUCT(BlueprintType)
struct DATACONFIGEXTRA_API FDcStructShapeBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) FName ShapeName;
};

USTRUCT(BlueprintType)
struct DATACONFIGEXTRA_API FDcStructShapeRectangle : public FDcStructShapeBase
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcStructShapeRectangle)

	UPROPERTY(EditAnywhere) float Height;
	UPROPERTY(EditAnywhere) float Width;
};

USTRUCT(BlueprintType)
struct DATACONFIGEXTRA_API FDcStructShapeCircle : public FDcStructShapeBase
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcStructShapeCircle)

	UPROPERTY(EditAnywhere) float Radius;
};


UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class DATACONFIGEXTRA_API UDcBaseShape : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere) FName ShapeName;
};

UCLASS()
class DATACONFIGEXTRA_API UDcShapeBox : public UDcBaseShape
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere) float Height;
	UPROPERTY(EditAnywhere) float Width;
};

UCLASS()
class DATACONFIGEXTRA_API UDcShapeSquare : public UDcBaseShape
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere) float Radius;
};


UENUM()
enum class EDcExtraTestEnum1
{
	Alpha,
	Beta,
	Gamma,
};

USTRUCT()
struct DATACONFIGEXTRA_API FDcExtraSimpleStruct
{
	GENERATED_BODY()

	UPROPERTY() FString Name;
	UPROPERTY() int Id = 0;
	UPROPERTY() EDcExtraTestEnum1 Type = EDcExtraTestEnum1::Alpha;
};

USTRUCT()
struct DATACONFIGEXTRA_API FDcExtraCoreTypesStruct
{
	GENERATED_BODY()
	DCEXTRA_ZEROINIT_CONSTRUCTOR(FDcExtraCoreTypesStruct)

	UPROPERTY() FGuid GuidField1;
	UPROPERTY() FGuid GuidField2;

	UPROPERTY() FVector2D Vec2Field1;
	UPROPERTY() FVector2D Vec2Field2;

	UPROPERTY() FVector VecField1;
	UPROPERTY() FVector VecField2;

	UPROPERTY() FPlane PlaneField1;
	UPROPERTY() FPlane PlaneField2;

	UPROPERTY()	FMatrix MatrixField1;
	UPROPERTY()	FMatrix MatrixField2;

	UPROPERTY() FRotator RotatorField1;
	UPROPERTY() FRotator RotatorField2;

	UPROPERTY() FQuat QuatField1;
	UPROPERTY() FQuat QuatField2;

	UPROPERTY() FTransform TransformField1;
	UPROPERTY() FTransform TransformField2;

	UPROPERTY() FColor ColorField1;
	UPROPERTY() FColor ColorField2;

	UPROPERTY() FLinearColor LinearColorField1;
	UPROPERTY() FLinearColor LinearColorField2;

	UPROPERTY() FIntPoint IntPointField1;
	UPROPERTY() FIntPoint IntPointField2;

	UPROPERTY() FIntVector IntVectorField1;
	UPROPERTY() FIntVector IntVectorField2;

	UPROPERTY() FDateTime DateTimeField1;
	UPROPERTY() FDateTime DateTimeField2;

	UPROPERTY() FTimespan TimespanField1;
	UPROPERTY() FTimespan TimespanField2;

	void MakeFixture();
};


