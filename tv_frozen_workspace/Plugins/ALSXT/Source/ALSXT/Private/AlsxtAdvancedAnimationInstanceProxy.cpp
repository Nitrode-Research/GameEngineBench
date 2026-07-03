#include "AlsxtAdvancedAnimationInstanceProxy.h"
#include "AlsxtAnimationInstanceProxy.h"
#include "AlsxtAdvancedAnimationInstance.h"
#include "AlsxtAnimationInstance.h"

FAlsxtAdvancedAnimationInstanceProxy::FAlsxtAdvancedAnimationInstanceProxy(UAnimInstance* AnimationInstance): FAlsxtAnimationInstanceProxy{AnimationInstance} {}

void FAlsxtAdvancedAnimationInstanceProxy::PostUpdate(UAnimInstance* AnimationInstance) const
{
	FAlsxtAnimationInstanceProxy::PostUpdate(AnimationInstance);

	// Epic does not allow to override the UAnimInstance::PostUpdateAnimation()
	// function in child classes, so we have to resort to this workaround.

	auto* ALSXTAdvancedAnimationInstance{Cast<UAlsxtAdvancedAnimationInstance>(AnimationInstance)};
	if (IsValid(ALSXTAdvancedAnimationInstance))
	{
		ALSXTAdvancedAnimationInstance->NativePostUpdateAnimation();
	}
}
