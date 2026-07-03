#pragma once

#include "HAL/UnrealMemory.h"

// UE checks uninitialized UPROPERTY() and reports warning on debug builds.
// The check can be manually invoked with `CoreUObject.AttemptToFindUninitializedScriptStructMembers`
// use this to generate a zeroinit constructor
#define DCEXTRA_ZEROINIT_CONSTRUCTOR(ClassName) \
	ClassName() { FMemory::Memzero(this, sizeof(ClassName)); }