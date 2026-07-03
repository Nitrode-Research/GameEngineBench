// Copyright 2026 GameDevBench. ALSXT movement effect notify source tests.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ALSXTMovementEffectsTests
{
	static bool LoadNotifySource(const TCHAR* Name, FString& Out)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Plugins/ALSXT/Source/ALSXT/Private/Notify") / Name);
		return FFileHelper::LoadFileToString(Out, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTMovementEffects_FootstepSource,
	"TargetVector.ALSXTMovementEffects.footstep_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTMovementEffects_FootstepSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates ALSXT-specific footstep tracing, surface selection, footwear, vertex paint, audio, decals, and Niagara.
	FString Source;
	if (!TestTrue(TEXT("Footstep notify source readable"), ALSXTMovementEffectsTests::LoadNotifySource(TEXT("AlsxtAnimNotify_FootstepEffects.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Footstep notify must validate settings, character interface, and in-air skip"),
		Source.Contains(TEXT("ALS_ENSURE(IsValid(FootstepEffectsSettings))")) && Source.Contains(TEXT("Implements<UAlsxtCharacterInterface>"))
		&& Source.Contains(TEXT("bSkipEffectsWhenInAir")) && Source.Contains(TEXT("AlsLocomotionModeTags::InAir")));
	TestTrue(TEXT("Footstep notify must trace from foot socket with physical material return"),
		Source.Contains(TEXT("FootLeftBoneName")) && Source.Contains(TEXT("FootRightBoneName")) && Source.Contains(TEXT("LineTraceSingleByChannel"))
		&& Source.Contains(TEXT("bReturnPhysicalMaterial = true")) && Source.Contains(TEXT("SurfaceTraceDistance")) && Source.Contains(TEXT("SurfaceTraceChannel")));
	TestTrue(TEXT("Footstep notify must select surface effects with fallback"),
		Source.Contains(TEXT("PhysMaterial")) && Source.Contains(TEXT("SurfaceType")) && Source.Contains(TEXT("Effects.Find(SurfaceType)"))
		&& Source.Contains(TEXT("for (const auto& Pair : FootstepEffectsSettings->Effects)")));
	TestTrue(TEXT("Footstep notify must use ALSXT footwear/gait/stance data"),
		Source.Contains(TEXT("Execute_GetCharacterGait")) && Source.Contains(TEXT("Execute_GetCharacterStance"))
		&& Source.Contains(TEXT("Execute_GetCharacterFootwearDetails")) && Source.Contains(TEXT("FootwearSoleTexture")));
	TestTrue(TEXT("Footstep notify must support vertex-paint material lookup"),
		Source.Contains(TEXT("EnableVertexPaintTrace")) && Source.Contains(TEXT("UAlsxtVertexFunctionLibrary::GetClosestVertexIDFromStaticMesh"))
		&& Source.Contains(TEXT("GetProminentVertexColorChannel")) && Source.Contains(TEXT("VertexColorPhysicalMaterialMap")));
	TestTrue(TEXT("Footstep notify must support sound, Niagara, and decal spawning"),
		Source.Contains(TEXT("FootstepSoundBlockCurveName")) && Source.Contains(TEXT("SpawnSoundAtLocation")) && Source.Contains(TEXT("SpawnSoundAttached"))
		&& Source.Contains(TEXT("UNiagaraFunctionLibrary")) && Source.Contains(TEXT("SpawnDecalAtLocation")));
	TestTrue(TEXT("Footstep decal path must configure dynamic material parameters"),
		Source.Contains(TEXT("UMaterialInstanceDynamic* MI")) && Source.Contains(TEXT("SetTextureParameterValue")) && Source.Contains(TEXT("SetScalarParameterValue")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTMovementEffects_HandPlantSource,
	"TargetVector.ALSXTMovementEffects.handplant_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTMovementEffects_HandPlantSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates hand-plant editor lifecycle, trace fallback, surface lookup, and sound routing.
	FString Source;
	if (!TestTrue(TEXT("HandPlant notify source readable"), ALSXTMovementEffectsTests::LoadNotifySource(TEXT("AlsxtAnimNotify_HandPlantEffects.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Hand-plant settings must propagate nested editor changes"),
		Source.Contains(TEXT("PostEditChangeProperty")) && Source.Contains(TEXT("Tuple.Value.PostEditChangeProperty")));
	TestTrue(TEXT("Editor-created hand-plant notifies must not trigger on dedicated server"),
		Source.Contains(TEXT("OnAnimNotifyCreatedInEditor")) && Source.Contains(TEXT("bTriggerOnDedicatedServer = false")));
	TestTrue(TEXT("Hand-plant notify must validate settings and in-air skip"),
		Source.Contains(TEXT("ALS_ENSURE(IsValid(HandPlantEffectsSettings))")) && Source.Contains(TEXT("bSkipEffectsWhenInAir"))
		&& Source.Contains(TEXT("AlsLocomotionModeTags::InAir")));
	TestTrue(TEXT("Hand-plant notify must trace from hand socket and fallback to world down"),
		Source.Contains(TEXT("LineTraceSingleByChannel")) && Source.Contains(TEXT("SurfaceTraceDistance")) && Source.Contains(TEXT("SurfaceTraceChannel"))
		&& Source.Contains(TEXT("FootTransform.GetLocation() - FVector{"))
		&& Source.Contains(TEXT("0.0f, 0.0f, HandPlantEffectsSettings->SurfaceTraceDistance")));
	TestTrue(TEXT("Hand-plant notify must select surface effect settings and spawn sound"),
		Source.Contains(TEXT("PhysMaterial")) && Source.Contains(TEXT("Effects.Find(SurfaceType)")) && Source.Contains(TEXT("SpawnSound("))
		&& Source.Contains(TEXT("SpawnSoundAtLocation")) && Source.Contains(TEXT("SpawnSoundAttached")));
	TestTrue(TEXT("Hand-plant sound must respect curve blocking and animation relevance"),
		Source.Contains(TEXT("bIgnoreHandPlantSoundBlockCurve")) && Source.Contains(TEXT("FootstepSoundBlockCurveName")) && Source.Contains(TEXT("FAnimWeight::IsRelevant")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FALSXTMovementEffects_SlideSource,
	"TargetVector.ALSXTMovementEffects.slide_source",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FALSXTMovementEffects_SlideSource::RunTest(const FString& Parameters)
{
	// REQUIRED: validates slide tracing, surface fallback, sound, Niagara, and decal behavior.
	FString Source;
	if (!TestTrue(TEXT("Slide notify source readable"), ALSXTMovementEffectsTests::LoadNotifySource(TEXT("AlsxtAnimNotify_SlideEffects.cpp"), Source)))
	{
		return true;
	}

	TestTrue(TEXT("Slide notify must validate settings and skip in-air characters"),
		Source.Contains(TEXT("ALS_ENSURE(IsValid(SlideEffectsSettings))")) && Source.Contains(TEXT("AlsLocomotionModeTags::InAir")));
	TestTrue(TEXT("Slide notify must trace from foot socket with physical material return"),
		Source.Contains(TEXT("LineTraceSingleByChannel")) && Source.Contains(TEXT("bReturnPhysicalMaterial = true"))
		&& Source.Contains(TEXT("SurfaceTraceDistance")) && Source.Contains(TEXT("SurfaceTraceChannel")));
	TestTrue(TEXT("Slide notify must select surface settings with fallback"),
		Source.Contains(TEXT("UGameplayStatics::GetSurfaceType")) && Source.Contains(TEXT("Effects.Find(SurfaceType)"))
		&& Source.Contains(TEXT("for (const auto& Pair : SlideEffectsSettings->Effects)")));
	TestTrue(TEXT("Slide notify must spawn sound and respect sound block curve"),
		Source.Contains(TEXT("bIgnoreSlideSoundBlockCurve")) && Source.Contains(TEXT("FootstepSoundBlockCurveName"))
		&& Source.Contains(TEXT("SpawnSoundAtLocation")) && Source.Contains(TEXT("SpawnSoundAttached")));
	TestTrue(TEXT("Slide notify must spawn Niagara at hit or attach to bone"),
		Source.Contains(TEXT("UNiagaraFunctionLibrary::SpawnSystemAtLocation")) && Source.Contains(TEXT("UNiagaraFunctionLibrary::SpawnSystemAttached"))
		&& Source.Contains(TEXT("ENCPoolMethod::AutoRelease")));
	return true;
}
