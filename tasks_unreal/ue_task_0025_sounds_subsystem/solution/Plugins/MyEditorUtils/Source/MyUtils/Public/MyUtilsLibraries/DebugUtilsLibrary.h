// Copyright (c) Yevhenii Selivanov

#pragma once

#include "CoreMinimal.h"

/** __CALLER__ is alternative to engine's __FUNCTION__, but displays previous function in callstack,
 * can be displayed in log by %hs. e.g: UE_LOG(LogTemp, Log, TEXT("%hs"), __CALLER__); */
#define __CALLER__ FDebugUtilsLibrary::GetCallerFunctionANSI(1)

/** _CALLERS__(N) is the same as __CALLER__, but displays N last callers in callstack,
 * can be displayed in log by %hs. e.g: UE_LOG(LogTemp, Log, TEXT("%hs"), _CALLERS__(3)); // FuncA -> FuncB -> FuncC */
#define __CALLERS__(N) FDebugUtilsLibrary::GetCallerFunctionANSI(N)

/**
 * Contains debug utilities for non-shipping game.
 * Is useful for debugging and profiling.
 */
class MYUTILS_API FDebugUtilsLibrary
{
public:
	/** Returns previous function(s) in current callstack.
	 * Can be passed in log by as %hs as __CALLER__ or _CALLERS__(N) */
	static const ANSICHAR* GetCallerFunctionANSI(int32 NumCallers);
};