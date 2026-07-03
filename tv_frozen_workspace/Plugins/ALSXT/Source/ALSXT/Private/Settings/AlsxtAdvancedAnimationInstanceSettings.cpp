#include "Settings/AlsxtAdvancedAnimationInstanceSettings.h"

UAlsxtAdvancedAnimationInstanceSettings::UAlsxtAdvancedAnimationInstanceSettings()
{
	Param = true;
	// InAir.GroundPredictionResponseChannels =
	// {
	// 	ECC_WorldStatic,
	// 	ECC_WorldDynamic,
	// 	ECC_Destructible
	// };
	// 
	// InAir.GroundPredictionSweepResponses.WorldStatic = ECR_Block;
	// InAir.GroundPredictionSweepResponses.WorldDynamic = ECR_Block;
	// InAir.GroundPredictionSweepResponses.Destructible = ECR_Block;
}

#if WITH_EDITOR
void UAlsxtAdvancedAnimationInstanceSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// InAir.PostEditChangeProperty(PropertyChangedEvent);

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
