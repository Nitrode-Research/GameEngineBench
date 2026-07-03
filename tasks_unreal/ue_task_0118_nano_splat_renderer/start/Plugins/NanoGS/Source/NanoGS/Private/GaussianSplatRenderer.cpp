// Copyright Epic Games, Inc. All Rights Reserved.

#include "GaussianSplatRenderer.h"
#include "GaussianSplatShaders.h"
#include "GaussianSplatSceneProxy.h"
#include "GaussianGlobalAccumulator.h"
#include "GaussianClusterTypes.h"
#include "RHICommandList.h"
#include "RHIGPUReadback.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneView.h"
#include "SceneRendering.h"  // For FViewInfo::ViewRect (screen percentage support)
#include "RenderCore.h"
#include "CommonRenderResources.h"

// Console variables (declared in GaussianSplatting.cpp)
extern TAutoConsoleVariable<int32> CVarShowClusterBounds;
extern TAutoConsoleVariable<int32> CVarDebugForceLODLevel;

// Helper: Set pixel shader velocity parameters using self-tracked previous frame data.
// UE5's PrevViewInfo is not populated for PostOpaqueRender callbacks, so we store
// current frame matrices and use them as "previous" data next frame.
// Data is tracked per-view (keyed by FSceneViewState*) to handle multiple viewports
// (e.g., main editor viewport + camera preview) without cross-contamination.
//
// IMPORTANT: We use UN-JITTERED projection matrices for velocity calculation.
// This ensures ClipPosition is stable when camera is static (we skip CalcViewData
// when camera doesn't move, so jittered positions would cause velocity errors).
static void SetVelocityPSParameters(
	FGaussianSplatPS::FParameters& PSParameters,
	const FSceneView& View,
	FGaussianGlobalAccumulator* Accumulator)
{
	// Current frame data - use UN-JITTERED matrix to match CalcViewData's WorldToClip
	FMatrix CurTranslatedVP = View.ViewMatrices.GetTranslatedViewMatrix() * View.ViewMatrices.GetProjectionNoAAMatrix();
	FVector CurPreViewTranslation = View.ViewMatrices.GetPreViewTranslation();

	// Use View.State as per-view key (unique per persistent viewport)
	const void* ViewKey = View.State;

	// Look up previous frame data for this specific view
	FGaussianGlobalAccumulator::FPrevFrameViewData* PrevData = nullptr;
	if (Accumulator && ViewKey)
	{
		PrevData = Accumulator->PrevFrameDataPerView.Find(ViewKey);
	}

	if (PrevData)
	{
		PSParameters.PrevTranslatedWorldToClip = FMatrix44f(PrevData->TranslatedViewProjectionMatrix);
		PSParameters.PrevPreViewTranslation = FVector3f(PrevData->PreViewTranslation);
	}
	else
	{
		// First frame for this view: use current frame data (produces zero velocity)
		PSParameters.PrevTranslatedWorldToClip = FMatrix44f(CurTranslatedVP);
		PSParameters.PrevPreViewTranslation = FVector3f(CurPreViewTranslation);
	}

	PSParameters.PreViewTranslation = FVector3f(CurPreViewTranslation);

	// Store current frame data for next frame
	if (Accumulator && ViewKey)
	{
		FGaussianGlobalAccumulator::FPrevFrameViewData& Data = Accumulator->PrevFrameDataPerView.FindOrAdd(ViewKey);
		Data.TranslatedViewProjectionMatrix = CurTranslatedVP;
		Data.PreViewTranslation = CurPreViewTranslation;
	}
}

FGaussianSplatRenderer::FGaussianSplatRenderer()
{

}

FGaussianSplatRenderer::~FGaussianSplatRenderer()
{
}

uint32 FGaussianSplatRenderer::NextPowerOfTwo(uint32 Value)
{
	return 0;
}

// Compute WorldToPLY matrix for SH evaluation
// SH coefficients are stored in PLY space, so we need to transform view direction from world to PLY space
// This combines: (1) WorldToLocal from actor transform, and (2) LocalToPLY coordinate system conversion
// LocalToPLY rotation: UE(X,Y,Z) -> PLY(Y,-Z,X) where PLY is X-right, Y-down, Z-forward
static FMatrix ComputeWorldToPLY(const FMatrix& LocalToWorld)
{
	static const FMatrix LocalToPLY(
		FPlane(0, 0, 1, 0),   // UE X -> PLY Z
		FPlane(1, 0, 0, 0),   // UE Y -> PLY X
		FPlane(0, -1, 0, 0),  // UE Z -> PLY -Y
		FPlane(0, 0, 0, 1)
	);
	return LocalToWorld.Inverse() * LocalToPLY;
}

void FGaussianSplatRenderer::DispatchCalcViewData(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianSplatGPUResources* GPUResources,
	const FMatrix& LocalToWorld,
	int32 SplatCount,
	int32 SHOrder,
	float OpacityScale,
	float SplatScale,
	bool bUseLODRendering)
{
	return;
}

void FGaussianSplatRenderer::DispatchCalcDistances(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources,
	int32 SplatCount)
{
	return;
}

void FGaussianSplatRenderer::DispatchRadixSort(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources,
	int32 SplatCount)
{
	return;
}

void FGaussianSplatRenderer::DispatchRadixSortIndirect(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources)
{
	return;
}

void FGaussianSplatRenderer::DrawSplats(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianSplatGPUResources* GPUResources,
	int32 SplatCount)
{
	return;
}

// NOTE: DispatchCalcLODViewDataGPUDriven and DispatchUpdateDrawArgs have been removed
// in the unified approach. LOD splats are now processed by CalcViewData shader using
// the same buffers as original splats.

//----------------------------------------------------------------------
// Splat Compaction Implementation
//----------------------------------------------------------------------

void FGaussianSplatRenderer::DispatchCompactSplats(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources,
	int32 TotalSplatCount,
	int32 OriginalSplatCount,
	bool bUseLODRendering)
{
	return;
}

void FGaussianSplatRenderer::DispatchPrepareIndirectArgs(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources)
{
	return;
}

void FGaussianSplatRenderer::DispatchCalcViewDataCompacted(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianSplatGPUResources* GPUResources,
	const FMatrix& LocalToWorld,
	int32 SplatCount,
	int32 OriginalSplatCount,
	int32 SHOrder,
	float OpacityScale,
	float SplatScale)
{
	return;
}

void FGaussianSplatRenderer::DispatchCalcDistancesIndirect(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources)
{
	return;
}

//----------------------------------------------------------------------
// Global Accumulator dispatch implementations
//----------------------------------------------------------------------

void FGaussianSplatRenderer::DispatchCalcViewDataGlobal(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianSplatGPUResources* GPUResources,
	const FMatrix& LocalToWorld,
	int32 SplatCount,
	int32 SHOrder,
	float OpacityScale,
	float SplatScale,
	bool bUseLODRendering,
	uint32 GlobalBaseOffset,
	FGaussianGlobalAccumulator* GlobalAccumulator)
{
	return;
}

void FGaussianSplatRenderer::DispatchCalcDistancesGlobal(
	FRHICommandListImmediate& RHICmdList,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	int32 TotalSplatCount)
{
	return;
}

void FGaussianSplatRenderer::DispatchRadixSortGlobal(
	FRHICommandListImmediate& RHICmdList,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	int32 TotalSplatCount)
{
	return;
}

void FGaussianSplatRenderer::DrawSplatsGlobal(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	FBufferRHIRef IndexBuffer,
	int32 TotalSplatCount,
	int32 DebugMode)
{
	return;
}

//----------------------------------------------------------------------
// Global Accumulator + Nanite Compaction dispatch implementations
//----------------------------------------------------------------------

void FGaussianSplatRenderer::DispatchGatherVisibleCount(
	FRHICommandListImmediate& RHICmdList,
	FGaussianSplatGPUResources* GPUResources,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	int32 ProxyIndex)
{
	return;
}

void FGaussianSplatRenderer::DispatchPrefixSumVisibleCounts(
	FRHICommandListImmediate& RHICmdList,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	int32 ProxyCount,
	uint32 MaxRenderBudget)
{
	return;
}

void FGaussianSplatRenderer::DispatchCalcViewDataCompactedGlobal(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianSplatGPUResources* GPUResources,
	const FMatrix& LocalToWorld,
	int32 SplatCount,
	int32 OriginalSplatCount,
	int32 SHOrder,
	float OpacityScale,
	float SplatScale,
	int32 ProxyIndex,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	uint32 MaxRenderBudget)
{
	return;
}

void FGaussianSplatRenderer::DispatchCalcDistancesGlobalIndirect(
	FRHICommandListImmediate& RHICmdList,
	FGaussianGlobalAccumulator* GlobalAccumulator)
{
	return;
}

void FGaussianSplatRenderer::DispatchRadixSortGlobalIndirect(
	FRHICommandListImmediate& RHICmdList,
	FGaussianGlobalAccumulator* GlobalAccumulator)
{
	return;
}

void FGaussianSplatRenderer::DrawSplatsGlobalIndirect(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianGlobalAccumulator* GlobalAccumulator,
	FBufferRHIRef IndexBuffer,
	int32 DebugMode)
{
	return;
}

void FGaussianSplatRenderer::ExtractFrustumPlanes(const FMatrix& ViewProjection, FVector4f OutPlanes[6])
{
	return;
}

int32 FGaussianSplatRenderer::DispatchClusterCulling(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FGaussianSplatGPUResources* GPUResources,
	const FMatrix& LocalToWorld,
	float ErrorThreshold,
	bool bUseLODRendering)
{
	return 0;
}

void FGaussianSplatRenderer::CompositeToSceneColor(
	FRHICommandListImmediate& RHICmdList,
	const FSceneView& View,
	FTextureRHIRef IntermediateTexture)
{
	return;
}
