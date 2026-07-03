// Copyright Epic Games, Inc. All Rights Reserved.

#include "GaussianGlobalAccumulator.h"
#include "GaussianDataTypes.h"
#include "RHICommandList.h"
#include "RHIResources.h"

void FGaussianGlobalAccumulator::ResizeIfNeeded(FRHICommandListBase& RHICmdList, uint32 NewTotalCount)
{
	return;
}

void FGaussianGlobalAccumulator::EnsureCompactionBuffersAllocated(FRHICommandListBase& RHICmdList)
{
	return;
}

void FGaussianGlobalAccumulator::Release()
{
	AllocatedCount = 0;
	AllocatedNumTiles = 0;
	bCompactionBuffersAllocated = false;
	return;
}
