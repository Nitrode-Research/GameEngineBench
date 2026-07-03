#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "SpudState.h"
#include "SpudPropertySerializationTestTypes.h"

int32 USpudPropertyNestedBase::CallbackSequence = 0;

void USpudPropertyNestedBase::SpudPreStore_Implementation(const USpudState*)
{
	PreStoreOrder = ++CallbackSequence;
}

void USpudPropertyNestedBase::SpudPostStore_Implementation(const USpudState*)
{
	PostStoreOrder = ++CallbackSequence;
}

void USpudPropertyNestedBase::SpudPreRestore_Implementation(const USpudState*)
{
	PreRestoreOrder = ++CallbackSequence;
}

void USpudPropertyNestedBase::SpudPostRestore_Implementation(const USpudState*)
{
	PostRestoreOrder = ++CallbackSequence;
}

namespace SpudPropertySerializationTest
{
	static constexpr TCHAR ObjectId[] = TEXT("PropertySerializationRoot");

	static USpudPropertyRootObject* NewRootObject(const TCHAR* Name)
	{
		return NewObject<USpudPropertyRootObject>(GetTransientPackage(), Name);
	}

	static USpudState* SaveAndReload(USpudPropertyRootObject* Source)
	{
		USpudState* SavedState = NewObject<USpudState>();
		SavedState->StoreGlobalObject(Source, ObjectId);

		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		SavedState->SaveToArchive(Writer);

		USpudState* LoadedState = NewObject<USpudState>();
		FMemoryReader Reader(Bytes, true);
		LoadedState->LoadFromArchive(Reader, true);
		return LoadedState;
	}

	static void PopulateSource(USpudPropertyRootObject* Source)
	{
		Source->SavedBool = true;
		Source->SavedInt = 7401;
		Source->SavedDouble = 74.25;
		Source->SavedString = TEXT("Saved string value");
		Source->SavedName = TEXT("SavedNameValue");
		Source->SavedText = FText::FromString(TEXT("Saved text value"));
		Source->SavedEnum = ESpudPropertyTestEnum::Beta;
		Source->SavedVector = FVector(1.0, 2.0, 3.0);
		Source->SavedIntArray = {4, 7, 74};
		Source->SavedClass = USpudPropertyNestedDerived::StaticClass();
		Source->SavedSoftObject = TSoftObjectPtr<UObject>(USpudPropertyNestedDerived::StaticClass());
		Source->SavedStruct.SavedInt = 1234;
		Source->SavedStruct.SavedVector = FVector(10.0, 20.0, 30.0);
		Source->SavedStruct.RuntimeOnlyInt = 9999;
		Source->SavedMap.Add(TEXT("alpha"), 10);
		Source->SavedMap.Add(TEXT("beta"), 20);

		FSpudPropertyInnerStruct FirstStruct;
		FirstStruct.SavedInt = 101;
		FirstStruct.SavedVector = FVector(101.0, 0.0, 0.0);
		FirstStruct.RuntimeOnlyInt = 10001;
		Source->SavedStructArray.Add(FirstStruct);

		FSpudPropertyInnerStruct SecondStruct;
		SecondStruct.SavedInt = 202;
		SecondStruct.SavedVector = FVector(0.0, 202.0, 0.0);
		SecondStruct.RuntimeOnlyInt = 20002;
		Source->SavedStructArray.Add(SecondStruct);

		USpudPropertyNestedDerived* Nested = NewObject<USpudPropertyNestedDerived>(Source);
		Nested->SavedNestedInt = 9876;
		Nested->RuntimeOnlyNestedInt = 8765;
		Nested->SavedDerivedString = TEXT("Derived nested value");
		Source->NestedObject = Nested;
		Source->NullNestedObject = nullptr;
		Source->RuntimeOnlyInt = 5555;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpudPropertySerializationSlowPathFallbackTest,
	"SPUD.PersistentRuntimeRestore.slow_path_restores_custom_structs_and_fallback_containers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpudPropertySerializationSlowPathFallbackTest::RunTest(const FString&)
{
	USpudPropertyRootObject* Source = SpudPropertySerializationTest::NewRootObject(TEXT("PropertySourceC"));
	SpudPropertySerializationTest::PopulateSource(Source);

	USpudState* LoadedState = SpudPropertySerializationTest::SaveAndReload(Source);
	LoadedState->bTestRequireSlowPath = true;

	USpudPropertyRootObject* Target = SpudPropertySerializationTest::NewRootObject(TEXT("PropertyTargetC"));
	LoadedState->RestoreGlobalObject(Target, SpudPropertySerializationTest::ObjectId);

	TestEqual(TEXT("Custom struct SaveGame int should restore through nested metadata"), Target->SavedStruct.SavedInt, 1234);
	TestEqual(TEXT("Custom struct SaveGame vector should restore through nested metadata"), Target->SavedStruct.SavedVector, FVector(10.0, 20.0, 30.0));
	TestEqual(TEXT("Custom struct runtime-only member should not restore"), Target->SavedStruct.RuntimeOnlyInt, 0);
	TestEqual(TEXT("Fallback map should restore key count"), Target->SavedMap.Num(), 2);
	TestEqual(TEXT("Fallback map alpha value should restore"), Target->SavedMap.FindRef(TEXT("alpha")), 10);
	TestEqual(TEXT("Fallback map beta value should restore"), Target->SavedMap.FindRef(TEXT("beta")), 20);
	TestEqual(TEXT("Fallback custom struct array should restore length"), Target->SavedStructArray.Num(), 2);
	if (Target->SavedStructArray.Num() == 2)
	{
		TestEqual(TEXT("Fallback custom struct array first element should restore"), Target->SavedStructArray[0].SavedInt, 101);
		TestEqual(TEXT("Fallback custom struct array second element should restore"), Target->SavedStructArray[1].SavedInt, 202);
	}

	return true;
}
