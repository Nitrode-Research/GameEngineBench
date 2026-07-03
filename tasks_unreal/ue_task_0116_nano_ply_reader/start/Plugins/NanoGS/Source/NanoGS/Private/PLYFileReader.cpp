#include "PLYFileReader.h"

bool FPLYFileReader::ReadPLYFile(const FString& FilePath, TArray<FGaussianSplatData>& OutSplats, FString& OutError, int32* OutSHBands)
{
	OutSplats.Empty();
	if (OutSHBands) { *OutSHBands = 0; }
	OutError = TEXT("PLY reader is unavailable");
	return false;
}

bool FPLYFileReader::IsValidPLYFile(const FString& FilePath)
{
	return false;
}
