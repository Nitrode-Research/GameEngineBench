#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "GameplayTagContainer.h"
#include "UObject/Package.h"
#include "Components/Character/ALSXTCharacterSoundComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterSoundTests, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Test-only access to private timing members (no production code is modified). Uses
// the standard explicit-instantiation idiom: the address-of a private member in an
// explicit instantiation is not subject to access control. We only READ the gate's
// behavior through the public ShouldPlay* predicates and set the inputs that feed
// them, so the function under test is still the editable predicate logic.
namespace SoundAccess
{
	template <class Tag> struct TBag { static typename Tag::Type Ptr; };
	template <class Tag> typename Tag::Type TBag<Tag>::Ptr{};

	template <class Tag, typename Tag::Type P> struct TPick
	{
		struct FInit { FInit() { TBag<Tag>::Ptr = P; } };
		static FInit Init;
	};
	template <class Tag, typename Tag::Type P> typename TPick<Tag, P>::FInit TPick<Tag, P>::Init;
}

#define SOUND_STEAL(TAG, MEMBERPTRTYPE, MEMBER) \
	namespace SoundAccess { \
		struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &UAlsxtCharacterSoundComponent::MEMBER>; \
	}
#define SOUND_SET(COMP, TAG, VALUE) ((COMP)->*SoundAccess::TBag<SoundAccess::TAG>::Ptr = (VALUE))

#define SOUND_CALL(COMP, TAG, ...) ((COMP)->*SoundAccess::TBag<SoundAccess::TAG>::Ptr)(__VA_ARGS__)

using FSoundComp = UAlsxtCharacterSoundComponent;
SOUND_STEAL(FTimeSinceAttackTag,   float FSoundComp::*, TimeSinceLastAttackSound)
SOUND_STEAL(FTargetAttackDelayTag, float FSoundComp::*, TargetAttackSoundDelay)
// The ShouldPlay* predicates are private (the public declarations at the top of the
// header are inside a /* ... */ comment block); thief them to call directly.
SOUND_STEAL(FActionSoundTag, bool (FSoundComp::*)(const FGameplayTag&, float),                                              ShouldPlayActionSound)
SOUND_STEAL(FAttackSoundTag, bool (FSoundComp::*)(const FGameplayTag&, const FGameplayTag&, float),                         ShouldPlayAttackSound)
SOUND_STEAL(FDamageSoundTag, bool (FSoundComp::*)(const FGameplayTag&, const FGameplayTag&, const FGameplayTag&, float),    ShouldPlayDamageSound)
SOUND_STEAL(FDeathSoundTag,  bool (FSoundComp::*)(const FGameplayTag&, const FGameplayTag&, const FGameplayTag&, float),    ShouldPlayDeathSound)

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0051 (ALSXT Character Sound).
//
// EDITABLE FILE UNDER TEST: ALSXTCharacterSoundComponent.cpp. In start/ the sound
// gating predicates are disabled stubs:
//   ShouldPlayActionSound / ShouldPlayAttackSound / ShouldPlayDamageSound /
//   ShouldPlayDeathSound / ShouldPlayHoldingBreathSoundDelegate  → all `return false;`
// so the component would never play ANY action/attack/damage/death/holding-breath
// sound. The solution enables them: most return true (the class is allowed), and
// ShouldPlayAttackSound implements the real rate gate `TimeSinceLastAttackSound >=
// TargetAttackSoundDelay`. These predicates are the spec behaviors "rapid repeated
// events are gated by the existing timing logic instead of spamming the same sound
// every frame" and "...respond to state instead of playing unconditionally".
//
// WHY NON-PIE: these predicates are pure (no owner/world/audio-device dependency), so
// the tests construct a bare component and call them directly — far more reliable
// than booting PIE. The remaining behavior of this component is actually emitting
// audio (SpawnSoundAttached/AtLocation → UAudioComponent), which is NOT observable in
// headless automation (no audio device under -NullRHI; SpawnSound returns null), and
// the Select*/Play* paths depend on a fully-configured ALSXT character owner +
// content-authored sound settings. Those are documented as not robustly testable
// headless and are intentionally not asserted; the gating predicates are the
// reliable, discriminating surface.
// ─────────────────────────────────────────────────────────────────────────────

namespace SoundTests
{
	static UAlsxtCharacterSoundComponent* MakeComponent()
	{
		UAlsxtCharacterSoundComponent* Comp =
			NewObject<UAlsxtCharacterSoundComponent>(GetTransientPackage());
		if (Comp)
		{
			Comp->AddToRoot(); // keep alive for the duration of the test
		}
		return Comp;
	}
}

// ── REQUIRED: the gating predicates enable their sound classes ────────────────
// Spec: action/attack/damage/death and holding-breath sounds must be allowed to play
// (instead of being unconditionally suppressed). Fails on start/ (every predicate is a
// `return false` stub → no sound class ever plays). Passes on solution/ (the classes
// are enabled). Catches a component that can never emit these sounds.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSoundGatingEnablesPlaybackTest,
	"ALSXT.CharacterSound.gating_predicates_enable_playback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSoundGatingEnablesPlaybackTest::RunTest(const FString&)
{
	UAlsxtCharacterSoundComponent* Comp = SoundTests::MakeComponent();
	if (!TestNotNull(TEXT("UAlsxtCharacterSoundComponent must construct"), Comp))
	{
		return true;
	}

	const FGameplayTag None;       // predicates ignore the tag args; empty is fine
	const float Stamina = 1.0f;

	TestTrue(TEXT("Action sounds must be allowed to play (start stub suppresses them)"),
		SOUND_CALL(Comp, FActionSoundTag, None, Stamina));
	TestTrue(TEXT("Damage sounds must be allowed to play (start stub suppresses them)"),
		SOUND_CALL(Comp, FDamageSoundTag, None, None, None, 50.0f));
	TestTrue(TEXT("Death sounds must be allowed to play (start stub suppresses them)"),
		SOUND_CALL(Comp, FDeathSoundTag, None, None, None, 50.0f));
	TestTrue(TEXT("Holding-breath sounds must be allowed to play (start stub suppresses them)"),
		Comp->ShouldPlayHoldingBreathSoundDelegate(None, Stamina));

	Comp->RemoveFromRoot();
	return true;
}

// ── REQUIRED: attack-sound gate respects the repeat-rate timing ───────────────
// Spec: "rapid repeated events are gated by the existing timing logic instead of
// spamming the same sound every frame." ShouldPlayAttackSound opens only when
// TimeSinceLastAttackSound >= TargetAttackSoundDelay. Fails on start/ (constant
// `return false` → the gate never opens even when the delay has elapsed). Passes on
// solution/ (gate opens once enough time has passed). The closed-gate assertion also
// rejects a trivial `return true` impl, so the test verifies a real timing gate, not
// just a flipped constant.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAttackSoundGateTimingTest,
	"ALSXT.CharacterSound.attack_sound_gate_respects_timing",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAttackSoundGateTimingTest::RunTest(const FString&)
{
	UAlsxtCharacterSoundComponent* Comp = SoundTests::MakeComponent();
	if (!TestNotNull(TEXT("UAlsxtCharacterSoundComponent must construct"), Comp))
	{
		return true;
	}

	const FGameplayTag None;
	const float Stamina = 1.0f;
	SOUND_SET(Comp, FTargetAttackDelayTag, 1.0f);

	// Enough time has elapsed → the gate must be OPEN. (start: false → fails here.)
	SOUND_SET(Comp, FTimeSinceAttackTag, 5.0f);
	TestTrue(TEXT("Attack-sound gate must OPEN once the repeat delay has elapsed"),
		SOUND_CALL(Comp, FAttackSoundTag, None, None, Stamina));

	// Not enough time has elapsed → the gate must be CLOSED. This rejects a constant
	// `return true` implementation (start is also false here, so this is a robustness
	// check, not the discriminator).
	SOUND_SET(Comp, FTimeSinceAttackTag, 0.0f);
	TestFalse(TEXT("Attack-sound gate must stay CLOSED before the repeat delay elapses"),
		SOUND_CALL(Comp, FAttackSoundTag, None, None, Stamina));

	Comp->RemoveFromRoot();
	return true;
}
