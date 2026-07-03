// Test-only support types for ue_task_0060 (CommonGame UI Policy).
//
// This project ships NO concrete UGameUIPolicy (it boots UI through the HUD path —
// B_HUD -> CBW_PrimaryLayout — not the CommonGame policy path), and both UGameUIPolicy and
// UPrimaryGameLayout are declared Abstract, so neither can be instantiated directly. These
// minimal concrete subclasses exist ONLY so the test can construct an instance; they add no
// behavior and inherit the editable base implementation under test (NotifyPlayerDestroyed is
// gutted in start/, real in solution/). This header is injected for evaluation only — the
// solver never sees it.
#pragma once

#include "CoreMinimal.h"
#include "GameUIPolicy.h"
#include "GameUIManagerSubsystem.h"
#include "PrimaryGameLayout.h"
#include "CommonGameUIPolicyTestSupport.generated.h"

UCLASS()
class UTestGameUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()
};

UCLASS()
class UTestGameUIPolicy : public UGameUIPolicy
{
	GENERATED_BODY()
};

UCLASS()
class UTestPrimaryGameLayout : public UPrimaryGameLayout
{
	GENERATED_BODY()
};
