#include "Gameplay/Camera/ECRCameraMode_PenetrationAvoidant.h"
#include "GameFramework/CameraBlockingVolume.h"
#include "Gameplay/Camera/ECRCameraAssistInterface.h"
#include "Engine/Canvas.h"
#include "Gameplay/Player/ECRPlayerController.h"

namespace ECRCameraMode_PenetrationAvoidant_Statics
{
	static const FName NAME_IgnoreCameraCollision = TEXT("IgnoreCameraCollision");
}


UECRCameraMode_PenetrationAvoidant::UECRCameraMode_PenetrationAvoidant()
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRCameraMode_PenetrationAvoidant::UpdateView(float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraMode_PenetrationAvoidant::UpdatePreventPenetration(float DeltaTime)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRCameraMode_PenetrationAvoidant::PreventCameraPenetration(class AActor const& ViewTarget,
                                                                  FVector const& SafeLoc, FVector& CameraLoc,
                                                                  float const& DeltaTime, float& DistBlockedPct,
                                                                  bool bSingleRayOnly)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}



void UECRCameraMode_PenetrationAvoidant::DrawDebug(UCanvas* Canvas) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}

