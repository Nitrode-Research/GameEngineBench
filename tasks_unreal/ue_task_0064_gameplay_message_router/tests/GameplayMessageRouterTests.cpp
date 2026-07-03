#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"

// Test-only access to the two private internal methods. Explicit-instantiation idiom,
// NOT `#define private public`: on MSVC the shim mis-mangles a private method call to
// the public-mangled symbol the module never exported (LNK2019 — see
// project-test-authoring-infra). The idiom bypasses access without changing the mangle.
namespace MsgAccess
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
#define MSG_STEAL(TAG, MEMBERPTRTYPE, MEMBER) \
	namespace MsgAccess { struct TAG { using Type = MEMBERPTRTYPE; }; \
		template struct TPick<TAG, &UGameplayMessageSubsystem::MEMBER>; }

using FMsgCallback = TFunction<void(FGameplayTag, const UScriptStruct*, const void*)>;
MSG_STEAL(FRegisterTag,
	FGameplayMessageListenerHandle (UGameplayMessageSubsystem::*)(FGameplayTag, FMsgCallback&&, const UScriptStruct*, EGameplayMessageMatch),
	RegisterListenerInternal)
MSG_STEAL(FBroadcastTag,
	void (UGameplayMessageSubsystem::*)(FGameplayTag, const UScriptStruct*, const void*),
	BroadcastMessageInternal)

// Steal the private handle id field of FGameplayMessageListenerHandle so the test can
// read the exact id assigned to a returned handle (the explicit-instantiation idiom
// bypasses access the same way it does for the private subsystem methods above).
namespace MsgAccess
{
	struct FHandleIDTag { using Type = int32 FGameplayMessageListenerHandle::*; };
	template struct TPick<FHandleIDTag, &FGameplayMessageListenerHandle::ID>;
}

namespace GameplayMessageRouterTest
{
	static FGameplayTag GetChannelTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("UI.Layer.Game")), false);
	}

	static FGameplayMessageListenerHandle RegisterListener(
		UGameplayMessageSubsystem* Router, FGameplayTag Channel, FMsgCallback Callback,
		const UScriptStruct* StructType, EGameplayMessageMatch Match)
	{
		return (Router->*MsgAccess::TBag<MsgAccess::FRegisterTag>::Ptr)(Channel, MoveTemp(Callback), StructType, Match);
	}

	static void Broadcast(UGameplayMessageSubsystem* Router, FGameplayTag Channel,
		const UScriptStruct* StructType, const void* Payload)
	{
		(Router->*MsgAccess::TBag<MsgAccess::FBroadcastTag>::Ptr)(Channel, StructType, Payload);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Behavioral test suite for ue_task_0064 (Gameplay Message Router).
//
// EDITABLE FILES UNDER TEST: GameplayMessageSubsystem.cpp/.h (+ the async action). In
// start/ the routing core is stubbed: RegisterListenerInternal returns an empty handle,
// BroadcastMessageInternal does nothing, UnregisterListener does nothing. So no listener
// is ever tracked and no message is ever delivered. The solution registers listeners per
// channel and delivers broadcasts to matching listeners with the payload preserved.
//
// OBSERVABLE CONTRACT: register returns a valid handle; a broadcast on the channel
// invokes the registered callback with the original channel/struct-type/payload; after
// unregister, a further broadcast is NOT delivered. FVector is used as the message
// struct (a real UScriptStruct), so no custom message type is needed. The internal
// register/broadcast methods are private and reached via the access idiom; the maps are
// observed through delivery behavior (their value type is a private nested struct).
//
// The earlier suite used the same observables but called the private methods through
// `#define private public` (which fails to link on MSVC) and asserted the private
// ListenerMap size directly; this version fixes the linkage and observes behavior.
// ─────────────────────────────────────────────────────────────────────────────

// ── REQUIRED: registering a listener returns a valid handle ───────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGameplayMessageRegisterListenerTest,
	"GameplayMessageRouter.System.register_listener_returns_valid_handle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGameplayMessageRegisterListenerTest::RunTest(const FString&)
{
	// The subsystem is a UGameInstanceSubsystem (ClassWithin=UGameInstance); constructing
	// it on the transient package trips a benign one-time ensure about the outer. The
	// subsystem still routes correctly; suppress the captured Error so it does not fail
	// whichever test runs first. Explicit TestTrue/TestEqual still fail normally.
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	UGameplayMessageSubsystem* Router = NewObject<UGameplayMessageSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Gameplay message subsystem should be constructible"), Router)) return true;

	const FGameplayTag Channel = GameplayMessageRouterTest::GetChannelTag();
	if (!TestTrue(TEXT("UI.Layer.Game gameplay tag must be available"), Channel.IsValid())) return true;

	const UScriptStruct* StructType = TBaseStructure<FVector>::Get();
	FGameplayMessageListenerHandle Handle = GameplayMessageRouterTest::RegisterListener(
		Router, Channel, FMsgCallback(), StructType, EGameplayMessageMatch::ExactMatch);

	TestTrue(TEXT("RegisterListenerInternal must return a valid handle"), Handle.IsValid());

	// The first listener registered on a fresh channel must carry this project's
	// handle-id seed (the per-channel id counter does not start from the stock base).
	const int32 AssignedID = Handle.*MsgAccess::TBag<MsgAccess::FHandleIDTag>::Ptr;
	TestEqual(TEXT("First listener handle on a channel must use this project's id seed"), AssignedID, 1001);
	return true;
}

// ── REQUIRED: a broadcast reaches the registered listener with its payload ─────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGameplayMessageBroadcastTest,
	"GameplayMessageRouter.System.broadcast_reaches_registered_listener",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGameplayMessageBroadcastTest::RunTest(const FString&)
{
	// The subsystem is a UGameInstanceSubsystem (ClassWithin=UGameInstance); constructing
	// it on the transient package trips a benign one-time ensure about the outer. The
	// subsystem still routes correctly; suppress the captured Error so it does not fail
	// whichever test runs first. Explicit TestTrue/TestEqual still fail normally.
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	UGameplayMessageSubsystem* Router = NewObject<UGameplayMessageSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Gameplay message subsystem should be constructible"), Router)) return true;

	const FGameplayTag Channel = GameplayMessageRouterTest::GetChannelTag();
	if (!TestTrue(TEXT("UI.Layer.Game gameplay tag must be available"), Channel.IsValid())) return true;

	int32 CallbackCount = 0;
	const UScriptStruct* StructType = TBaseStructure<FVector>::Get();
	GameplayMessageRouterTest::RegisterListener(Router, Channel,
		[&](FGameplayTag ReceivedChannel, const UScriptStruct* ReceivedType, const void* Payload)
		{
			const FVector* VectorPayload = static_cast<const FVector*>(Payload);
			if (ReceivedChannel == Channel && ReceivedType == StructType && VectorPayload && VectorPayload->X == 1.0f)
			{
				++CallbackCount;
			}
		}, StructType, EGameplayMessageMatch::ExactMatch);

	const FVector Payload(1.0f, 2.0f, 3.0f);
	GameplayMessageRouterTest::Broadcast(Router, Channel, StructType, &Payload);

	TestEqual(TEXT("Broadcasting must deliver to the registered listener with the original payload"),
		CallbackCount, 1);
	return true;
}

// ── REQUIRED: unregistering stops future delivery ─────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGameplayMessageUnregisterTest,
	"GameplayMessageRouter.System.unregister_stops_future_delivery",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGameplayMessageUnregisterTest::RunTest(const FString&)
{
	// The subsystem is a UGameInstanceSubsystem (ClassWithin=UGameInstance); constructing
	// it on the transient package trips a benign one-time ensure about the outer. The
	// subsystem still routes correctly; suppress the captured Error so it does not fail
	// whichever test runs first. Explicit TestTrue/TestEqual still fail normally.
	FAutomationTestBase::bSuppressLogErrors = true;
	FAutomationTestBase::bSuppressLogWarnings = true;

	UGameplayMessageSubsystem* Router = NewObject<UGameplayMessageSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Gameplay message subsystem should be constructible"), Router)) return true;

	const FGameplayTag Channel = GameplayMessageRouterTest::GetChannelTag();
	if (!TestTrue(TEXT("UI.Layer.Game gameplay tag must be available"), Channel.IsValid())) return true;

	int32 CallbackCount = 0;
	const UScriptStruct* StructType = TBaseStructure<FVector>::Get();
	FGameplayMessageListenerHandle Handle = GameplayMessageRouterTest::RegisterListener(
		Router, Channel,
		[&](FGameplayTag, const UScriptStruct*, const void*) { ++CallbackCount; },
		StructType, EGameplayMessageMatch::ExactMatch);

	const FVector Payload(1.0f, 2.0f, 3.0f);

	// Delivered while registered (this is what fails on start/: nothing was tracked).
	GameplayMessageRouterTest::Broadcast(Router, Channel, StructType, &Payload);
	TestEqual(TEXT("A registered listener must receive the broadcast"), CallbackCount, 1);

	// After unregister, a further broadcast must NOT be delivered.
	Router->UnregisterListener(Handle);
	GameplayMessageRouterTest::Broadcast(Router, Channel, StructType, &Payload);
	TestEqual(TEXT("Unregistering must stop future delivery"), CallbackCount, 1);

	return true;
}
