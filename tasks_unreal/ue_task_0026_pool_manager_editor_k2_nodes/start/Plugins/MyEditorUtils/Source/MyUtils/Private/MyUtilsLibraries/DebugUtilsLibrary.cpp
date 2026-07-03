// Copyright (c) Yevhenii Selivanov

#include "MyUtilsLibraries/DebugUtilsLibrary.h"

#if !UE_BUILD_SHIPPING
#include "HAL/PlatformStackWalk.h"
#endif // !UE_BUILD_SHIPPING

// Returns previous function(s) in current callstack
const ANSICHAR* FDebugUtilsLibrary::GetCallerFunctionANSI(int32 NumCallers)
{
#if UE_BUILD_SHIPPING
	static constexpr ANSICHAR UnavailableMsg[] = "None";
	return UnavailableMsg;
#else
	static constexpr int32 MaxDepth = 10;
	static constexpr int32 CallerFrameIndex = 3;
	static constexpr int32 FunctionNameSize = 1024;
	static constexpr int32 TempFunctionSize = 256;
	static constexpr ANSICHAR Arrow[] = " -> ";

	static ANSICHAR FunctionName[FunctionNameSize] = "Unknown caller";
	uint64 StackTrace[MaxDepth] = {0};

	const int32 Depth = FPlatformStackWalk::CaptureStackBackTrace(StackTrace, MaxDepth);
	if (Depth <= CallerFrameIndex)
	{
		return FunctionName;
	}

	FunctionName[0] = '\0';
	const int32 FramesToCapture = FMath::Clamp(NumCallers, 1, MaxDepth - CallerFrameIndex);

	for (int32 Index = 0; Index < FramesToCapture; ++Index)
	{
		const int32 FrameIndex = CallerFrameIndex + Index;
		if (FrameIndex >= Depth)
		{
			break;
		}

		FProgramCounterSymbolInfo SymbolInfo;
		FPlatformStackWalk::ProgramCounterToSymbolInfo(StackTrace[FrameIndex], /*out*/ SymbolInfo);
		if (SymbolInfo.FunctionName[0] == '\0')
		{
			continue;
		}

		ANSICHAR TempFunction[TempFunctionSize];
		FCStringAnsi::Strncpy(TempFunction, SymbolInfo.FunctionName, TempFunctionSize - 1);
		TempFunction[TempFunctionSize - 1] = '\0';

		ANSICHAR* BracketPos = FCStringAnsi::Strrchr(TempFunction, '(');
		if (BracketPos)
		{
			*BracketPos = '\0';
		}

		if (Index > 0)
		{
			FCStringAnsi::StrncatTruncateDest(FunctionName, FunctionNameSize, Arrow);
		}
		FCStringAnsi::StrncatTruncateDest(FunctionName, FunctionNameSize, TempFunction);
	}

	return FunctionName;
#endif // !UE_BUILD_SHIPPING
}
