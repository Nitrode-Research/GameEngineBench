#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PLYFileReader.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNanoPLYReaderValidationTest, "Nano.PLYReader.ValidatesMagicAndRejectsAscii", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FNanoPLYReaderValidationTest::RunTest(const FString& Parameters)
{
	const FString FilePath = FPaths::CreateTempFilename(*FPaths::ProjectSavedDir(), TEXT("NanoAscii"), TEXT(".ply"));
	const FString Text = TEXT("ply\nformat ascii 1.0\nelement vertex 0\nend_header\n");
	FFileHelper::SaveStringToFile(Text, *FilePath);
	TestTrue(TEXT("PLY magic is recognized by IsValidPLYFile"), FPLYFileReader::IsValidPLYFile(FilePath));
	TArray<FGaussianSplatData> Splats;
	FString Error;
	int32 SHBands = -1;
	TestFalse(TEXT("ASCII PLY is rejected with an error instead of silently loading"), FPLYFileReader::ReadPLYFile(FilePath, Splats, Error, &SHBands));
	TestTrue(TEXT("ASCII rejection reports a useful error"), Error.Contains(TEXT("ASCII")));
	IFileManager::Get().Delete(*FilePath);
	return true;
}
