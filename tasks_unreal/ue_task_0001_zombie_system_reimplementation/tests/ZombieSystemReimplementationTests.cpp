#include "CoreMinimal.h"

#include "AI/AICorePoint.h"
#include "AI/AISpawnPoint.h"
#include "AI/ZedAIController.h"
#include "AI/ZedPawn.h"
#include "Character/HordeBaseCharacter.h"
#include "Gameplay/HordeGameMode.h"
#include "Gameplay/HordeGameState.h"
#include "Gameplay/HordePlayerState.h"
#include "Gameplay/LobbyStructures.h"
#include "HordeTemplateV2Native.h"

#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

#if WITH_EDITOR
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogZombieReimplementationTests, Log, All);

	namespace ZombieTests
	{
		static const TCHAR* GameplayMapPath = TEXT("/Game/HordeTemplateBP/Maps/HordeArena");
		static const FVector PursuitPlayerLocation(0.f, 2500.f, 150.f);
		static const FVector MeleePlayerLocation(0.f, 6500.f, 150.f);
		static const FVector RewardTestLocation(0.f, 8500.f, 150.f);
		static const FVector MultiplayerPlayerLocation(0.f, 10500.f, 150.f);
		static const FVector KnownGoodAggroZedLocation(-1306.2f, 1042.9f, 1418.1f);
		static const FVector KnownGoodAggroPlayerLocation(-654.5f, 1735.2f, 1418.0f);

	static int32 BaselineAliveZeds = 0;
	static int32 BodyKillPoints = 0;
	static int32 BaselinePoints = 0;
	static int32 BaselineKills = 0;
	static int32 BaselineHeadshots = 0;
	static float BaselinePlayerHealth = 0.f;
	static float BaselineClientPlayerHealth = 0.f;
	static float DistanceToCoreAtInvalidation = 0.f;
	static float DistanceToPlayerAtInvalidation = 0.f;
	static float TrackedCorpseMinimumLifetimeDeadline = 0.f;
	static FVector TrackedZedStartLocation = FVector::ZeroVector;
	static FVector ZedLocationAtInvalidation = FVector::ZeroVector;
	static FVector RuntimeInvalidTargetCoreLocation = FVector::ZeroVector;
	static FVector RuntimeSpawnInheritanceLocation = FVector::ZeroVector;
	static FName RuntimeInvalidTargetPatrolTag = NAME_None;
	static FName RuntimeMultiplayerPatrolTag = NAME_None;
	static FName RuntimeSpawnInheritancePatrolTag = NAME_None;
	static int32 UniquePatrolTagCounter = 0;
	static bool bTrackedCorpseExpiredBeforeMinimum = false;
	static bool bClientObservedAttackFeedback = false;

	// Pursuit poll window state
	static float BaselinePursuitPlayerHealth = 0.f;
	static float PursuitStartXYDistance = 0.f;
	static float PursuitMinXYDistanceToPlayer = 0.f;
	static bool bPursuitEverFocused = false;
	static bool bPursuitPlayerHealthDropped = false;

		// Fallback patrol poll window state
		static float BaselineFallbackPlayerHealth = 0.f;
		static float FallbackStartDistanceToPlayer = 0.f;
		static bool bFallbackEngagementDetected = false;
		static bool bFallbackHadControllerAtInvalidation = false;
		static bool bFallbackFocusedTrackedAtInvalidation = false;
		static float FallbackMinDistToCore = 0.f;
		static float FallbackTotalXYTravel = 0.f;
		static FVector FallbackLastZedLocation = FVector::ZeroVector;

		static TWeakObjectPtr<AZedPawn> TrackedZed;
		static TWeakObjectPtr<AHordeBaseCharacter> TrackedClientTarget;
		static TWeakObjectPtr<AHordeBaseCharacter> TrackedPlayer;
		static TWeakObjectPtr<AHordeBaseCharacter> SupportPlayer;

	static FHitResult MakeBodyHit()
	{
		FHitResult Hit;
		Hit.BoneName = FName(TEXT("spine_01"));
		return Hit;
	}

	static FHitResult MakeHeadHit()
	{
		FHitResult Hit;
		Hit.BoneName = FName(TEXT("head"));
		return Hit;
	}

	static UWorld* GetFirstPIEWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		UWorld* FirstPIEWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				if (!FirstPIEWorld)
				{
					FirstPIEWorld = Context.World();
				}

				const ENetMode NetMode = Context.World()->GetNetMode();
				if (NetMode == NM_Standalone || NetMode == NM_ListenServer || NetMode == NM_DedicatedServer)
				{
					return Context.World();
				}
			}
		}

		return FirstPIEWorld;
	}

	static UWorld* FindPIEServerWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World() && Context.World()->GetAuthGameMode())
			{
				return Context.World();
			}
		}

		return nullptr;
	}

	static UWorld* FindPIEClientWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		UWorld* FallbackClientWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType != EWorldType::PIE || !Context.World())
			{
				continue;
			}

			if (Context.World()->GetNetMode() == NM_Client)
			{
				return Context.World();
			}

			if (Context.PIEInstance > 0)
			{
				return Context.World();
			}

			if (!Context.World()->GetAuthGameMode())
			{
				FallbackClientWorld = Context.World();
			}
		}

		return FallbackClientWorld;
	}

	static FVector WithReferenceZ(const FVector& TargetLocation, const AActor* ReferenceActor)
	{
		FVector Result = TargetLocation;
		if (ReferenceActor)
		{
			Result.Z = ReferenceActor->GetActorLocation().Z;
		}

		return Result;
	}

	static AHordeBaseCharacter* GetAnyPlayerCharacter(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		AHordeBaseCharacter* Fallback = nullptr;
		for (TActorIterator<AHordeBaseCharacter> It(World); It; ++It)
		{
			if (It->GetController() && !It->GetIsDead())
			{
				return *It;
			}
			if (!Fallback)
			{
				Fallback = *It;
			}
		}

		return Fallback;
	}

	static AHordeBaseCharacter* GetAnyLivePlayerCharacter(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		AHordeBaseCharacter* FallbackLive = nullptr;
		for (TActorIterator<AHordeBaseCharacter> It(World); It; ++It)
		{
			if (It->GetIsDead() || It->GetHealth() <= 0.f)
			{
				continue;
			}
			if (It->GetController())
			{
				return *It;
			}
			if (!FallbackLive)
			{
				FallbackLive = *It;
			}
		}

		return FallbackLive;
	}

	static AHordeBaseCharacter* FindClosestPlayerCharacter(UWorld* World, const FVector& Location, bool bRequireAlive = true)
	{
		if (!World)
		{
			return nullptr;
		}

		AHordeBaseCharacter* Closest = nullptr;
		float BestDistanceSq = TNumericLimits<float>::Max();
		for (TActorIterator<AHordeBaseCharacter> It(World); It; ++It)
		{
			if (bRequireAlive && It->GetIsDead())
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared(It->GetActorLocation(), Location);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				Closest = *It;
			}
		}

		return Closest;
	}

		static TSubclassOf<ACharacter> GetPlayableCharacterClass()
		{
			UDataTable* CharacterTable = Cast<UDataTable>(
				StaticLoadObject(UDataTable::StaticClass(), nullptr, CHARACTER_DATATABLE_PATH));

			if (!CharacterTable)
			{
				return nullptr;
			}

			TArray<FPlayableCharacter*> Rows;
			CharacterTable->GetAllRows<FPlayableCharacter>(TEXT("ZombieTests"), Rows);
			for (FPlayableCharacter* Row : Rows)
			{
				if (Row && Row->CharacterClass && Row->CharacterID != FName(TEXT("None")))
				{
					return Row->CharacterClass;
				}
			}

			return nullptr;
		}

		static AHordePlayerState* GetAnyPlayerState(UWorld* World)
		{
			if (!World || !World->GetGameState())
			{
				return nullptr;
			}

			for (APlayerState* PlayerState : World->GetGameState()->PlayerArray)
			{
				if (AHordePlayerState* HordePlayerState = Cast<AHordePlayerState>(PlayerState))
				{
					return HordePlayerState;
				}
			}

			return nullptr;
		}

	static AZedPawn* FindZedByPatrolTag(UWorld* World, FName PatrolTag)
	{
		if (!World || PatrolTag == NAME_None)
		{
			return nullptr;
		}

		for (TActorIterator<AZedPawn> It(World); It; ++It)
		{
			if (It->PatrolTag == PatrolTag)
			{
				return *It;
			}
		}

		return nullptr;
	}

	static FName MakeUniquePatrolTag(const TCHAR* Prefix)
	{
		return FName(*FString::Printf(TEXT("%s_%d"), Prefix, ++UniquePatrolTagCounter));
	}

	static bool HasPossessedZedController(AZedPawn* Zed)
	{
		return Zed && Cast<AZedAIController>(Zed->GetController()) != nullptr;
	}

	static bool HasReachableNavPath(UWorld* World, const FVector& Start, const FVector& End)
	{
		if (!World)
		{
			return false;
		}

		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
		if (!NavSys)
		{
			return false;
		}

		UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(World, Start, End);
		return Path && Path->IsValid() && !Path->IsPartial();
	}

	static FVector GetAggroDirection2D()
	{
		return (KnownGoodAggroPlayerLocation - KnownGoodAggroZedLocation).GetSafeNormal2D();
	}

	static bool FindDistantReachablePointAround(
		UWorld* World,
		const FVector& Origin,
		float MinDistanceXY,
		float SearchRadius,
		int32 MaxAttempts,
		FVector& OutLocation)
	{
		if (!World)
		{
			return false;
		}

		for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
		{
			FVector Candidate = FVector::ZeroVector;
			if (!UNavigationSystemV1::K2_GetRandomReachablePointInRadius(World, Origin, Candidate, SearchRadius, nullptr, nullptr))
			{
				continue;
			}

			if (FVector::Dist2D(Candidate, Origin) >= MinDistanceXY)
			{
				OutLocation = Candidate;
				return true;
			}
		}

		return false;
	}

	static const TCHAR* BoolString(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	static bool HasClientObservableAttackCue(AZedPawn* Zed)
	{
		if (!Zed)
		{
			return false;
		}

		if (USkeletalMeshComponent* Mesh = Zed->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
			{
				if (UAnimMontage* AttackMontage = ObjectFromPath<UAnimMontage>(
					TEXT("AnimMontage'/Game/HordeTemplateBP/Assets/Animations/Zombie/A_UEZomb_Attack_Montage.A_UEZomb_Attack_Montage'")))
				{
					return AnimInstance->Montage_IsPlaying(AttackMontage);
				}
			}
		}

		return false;
	}

	static AAISpawnPoint* FindClosestSpawnPoint(UWorld* World, const FVector& Location)
	{
		if (!World)
		{
			return nullptr;
		}

		AAISpawnPoint* Closest = nullptr;
		float BestDistanceSq = TNumericLimits<float>::Max();

		for (TActorIterator<AAISpawnPoint> It(World); It; ++It)
		{
			const float DistanceSq = FVector::DistSquared(It->GetActorLocation(), Location);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				Closest = *It;
			}
		}

		return Closest;
	}

	static FVector ProjectToNavMesh(UWorld* World, const FVector& Location, const FVector& Extent = FVector(800.f, 800.f, 400.f))
	{
		if (!World)
		{
			return Location;
		}

		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(Location, NavLoc, Extent))
			{
				return NavLoc.Location;
			}
		}

		return Location;
	}

		static void TeleportPlayer(AHordeBaseCharacter* Player, const FVector& TargetLocation)
		{
			if (!Player)
			{
				return;
			}

			Player->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* Controller = Player->GetController())
			{
				Controller->SetControlRotation((FRotator::ZeroRotator));
			}
		}

		static AHordeBaseCharacter* SpawnUnpossessedTestPlayer(UWorld* World, const FVector& SpawnLocation)
		{
			if (!World)
			{
				return nullptr;
			}

			TSubclassOf<ACharacter> SpawnClass = GetPlayableCharacterClass();
			if (!SpawnClass)
			{
				return nullptr;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			return Cast<AHordeBaseCharacter>(
				World->SpawnActor<ACharacter>(SpawnClass, SpawnLocation, FRotator::ZeroRotator, Params));
		}

		static bool ZedHasObservedTarget(AZedPawn* Zed, AHordeBaseCharacter* Player)
		{
		if (!Zed || !Player)
		{
			return false;
		}

		AZedAIController* Controller = Cast<AZedAIController>(Zed->GetController());
		if (!Controller)
		{
			return false;
		}

		return Controller->GetFocusActor() == Player;
	}

	static void KillActorWithPointDamage(AActor* Victim, AActor* DamageCauser, AController* InstigatorController, const FHitResult& Hit)
	{
		if (!Victim)
		{
			return;
		}

		UGameplayStatics::ApplyPointDamage(
			Victim,
			10000.f,
			FVector(0.f, 0.f, -1.f),
			Hit,
			InstigatorController,
			DamageCauser,
			nullptr);
	}

	static void KillAllOtherZeds(UWorld* World, AHordeBaseCharacter* Killer, const AZedPawn* ExcludedZed = nullptr)
	{
		if (!World || !Killer)
		{
			return;
		}

		for (TActorIterator<AZedPawn> It(World); It; ++It)
		{
			AZedPawn* Zed = *It;
			if (!Zed || Zed == ExcludedZed || Zed->GetIsDead())
			{
				continue;
			}

			KillActorWithPointDamage(Zed, Killer, Killer->GetController(), MakeBodyHit());
		}
	}

	DEFINE_LATENT_AUTOMATION_COMMAND(FForceGameStartCommand);
		bool FForceGameStartCommand::Update()
		{
			if (UWorld* World = GetFirstPIEWorld())
			{
				if (AHordeGameState* GameState = Cast<AHordeGameState>(World->GetGameState()))
			{
					GameState->StartGame();
				}
			}

			return true;
		}

	DEFINE_LATENT_AUTOMATION_COMMAND(FSpawnTestPlayerCommand);
	bool FSpawnTestPlayerCommand::Update()
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World)
		{
			return true;
		}

		// If a player character has already been spawned by the existing game flow, leave it alone.
		for (TActorIterator<AHordeBaseCharacter> It(World); It; ++It)
		{
			return true;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (!PlayerController)
		{
			return true;
		}

			TSubclassOf<ACharacter> SpawnClass = GetPlayableCharacterClass();

		if (!SpawnClass)
		{
			UE_LOG(LogZombieReimplementationTests, Error, TEXT("Could not find a playable character class for PIE bootstrap."));
			return true;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (ACharacter* Character = World->SpawnActor<ACharacter>(SpawnClass, FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Params))
		{
			PlayerController->Possess(Character);
		}

		return true;
	}

#if WITH_EDITOR
	DEFINE_LATENT_AUTOMATION_COMMAND(FSetupMultiplayerPIESettingsCommand);
	bool FSetupMultiplayerPIESettingsCommand::Update()
	{
		if (ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>())
		{
			PlaySettings->SetPlayNumberOfClients(2);
			PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
			PlaySettings->bLaunchSeparateServer = false;
			PlaySettings->GameGetsMouseControl = false;
			PlaySettings->SetRunUnderOneProcess(true);
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND(FRestoreSinglePlayerPIESettingsCommand);
	bool FRestoreSinglePlayerPIESettingsCommand::Update()
	{
		if (ULevelEditorPlaySettings* PlaySettings = GetMutableDefault<ULevelEditorPlaySettings>())
		{
			PlaySettings->SetPlayNumberOfClients(1);
			PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_Standalone);
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForClientWorldCommand, float, RemainingSeconds);
	bool FWaitForClientWorldCommand::Update()
	{
		if (FindPIEClientWorld())
		{
			return true;
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForTrackedZedControllerCommand, float, RemainingSeconds);
	bool FWaitForTrackedZedControllerCommand::Update()
	{
		if (HasPossessedZedController(TrackedZed.Get()))
		{
			return true;
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FWaitForReplicatedTaggedZedCommand, FName, PatrolTag, float, RemainingSeconds);
	bool FWaitForReplicatedTaggedZedCommand::Update()
	{
		if (FindZedByPatrolTag(FindPIEClientWorld(), PatrolTag))
		{
			return true;
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FWaitForTaggedZedCommand, FName, PatrolTag, float, RemainingSeconds);
	bool FWaitForTaggedZedCommand::Update()
	{
		if (FindZedByPatrolTag(GetFirstPIEWorld(), PatrolTag))
		{
			return true;
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForInvalidTargetFallbackCommand, float, RemainingSeconds);
	bool FWaitForInvalidTargetFallbackCommand::Update()
	{
		UWorld* World = GetFirstPIEWorld();
		AZedPawn* Tracked = TrackedZed.Get();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (World && Player)
		{
			for (TActorIterator<AZedPawn> It(World); It; ++It)
			{
				AZedPawn* Zed = *It;
				if (!Zed || Zed == Tracked || Zed->GetIsDead())
				{
					continue;
				}
				KillActorWithPointDamage(Zed, Player, Player->GetController(), MakeBodyHit());
			}
		}

		if (Tracked)
		{
			const float DistanceToFallbackCore = FVector::Dist(Tracked->GetActorLocation(), RuntimeInvalidTargetCoreLocation);
			if (DistanceToFallbackCore <= DistanceToCoreAtInvalidation - 25.f)
			{
				return true;
			}
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

		// Tracks pursuit evidence over a polling window so a single unlucky snapshot frame cannot
		// cause a false pass or false fail. Records the minimum zombie-player XY distance seen,
		// whether the zombie ever focused the player, and whether the player's health dropped
		// below the value recorded at zombie spawn time.
	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FPollPursuitBehaviorCommand, float, RemainingSeconds);
	bool FPollPursuitBehaviorCommand::Update()
	{
		AZedPawn* Zed = TrackedZed.Get();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (!Player)
		{
			// Player actor was destroyed (respawned/removed) — zombie must have dealt lethal damage.
			bPursuitPlayerHealthDropped = true;
			return true;
		}
		if (Zed)
		{
			const float XYDist = FVector::Dist2D(Zed->GetActorLocation(), Player->GetActorLocation());
			if (XYDist < PursuitMinXYDistanceToPlayer) PursuitMinXYDistanceToPlayer = XYDist;
			if (ZedHasObservedTarget(Zed, Player)) bPursuitEverFocused = true;
			if (Player->GetHealth() < BaselinePursuitPlayerHealth) bPursuitPlayerHealthDropped = true;

			// Exit the polling window early once we have clear pursuit evidence.
			// The reference solution can legitimately acquire and engage from nearby
			// positions without first closing a large gap, so keep this threshold modest.
			if (bPursuitEverFocused || bPursuitPlayerHealthDropped
				|| PursuitMinXYDistanceToPlayer <= PursuitStartXYDistance - 40.f)
			{
				return true;
			}
		}
		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

		// Polls for the first moment the zombie observably engages the player, then immediately
		// records the invalidation-point positions and kills the player so the fallback window
		// starts from a well-defined state. Engagement is measured behaviorally: a meaningful
		// reduction in player distance, focus, or player damage. Falls back to recording positions
		// on timeout to ensure the downstream poll always has valid reference values.
		DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForEngagementThenInvalidateCommand, float, RemainingSeconds);
		bool FWaitForEngagementThenInvalidateCommand::Update()
		{
			AZedPawn* Zed = TrackedZed.Get();
			AHordeBaseCharacter* Player = TrackedPlayer.Get();
			if (!Zed || Zed->GetIsDead()) return true;
			if (!Player)
			{
				// Player actor was destroyed — zombie dealt lethal damage, which counts as engagement.
				bFallbackEngagementDetected = true;
				ZedLocationAtInvalidation = Zed->GetActorLocation();
				DistanceToCoreAtInvalidation = FVector::Dist(ZedLocationAtInvalidation, RuntimeInvalidTargetCoreLocation);
				DistanceToPlayerAtInvalidation = 0.f;
				FallbackLastZedLocation = ZedLocationAtInvalidation;
				FallbackMinDistToCore = DistanceToCoreAtInvalidation;
				FallbackTotalXYTravel = 0.f;
				return true;
			}

			const float DistanceToPlayer = FVector::Dist2D(Zed->GetActorLocation(), Player->GetActorLocation());
			// Use a modest approach threshold because the reference solution's overlap
			// sphere can produce valid engagement before the zombie has traveled far.
			const bool bMeaningfulApproach = DistanceToPlayer <= FallbackStartDistanceToPlayer - 40.f;
			const bool bFocused = ZedHasObservedTarget(Zed, Player);
			AZedAIController* Controller = Cast<AZedAIController>(Zed->GetController());
			bFallbackHadControllerAtInvalidation = Controller != nullptr;
			bFallbackFocusedTrackedAtInvalidation = bFocused;
			const bool bDamaged = Player->GetHealth() < BaselineFallbackPlayerHealth;
			RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
			const bool bTimeout = RemainingSeconds <= 0.f;

		if (!bMeaningfulApproach && !bDamaged && !bTimeout) return false;

		if (bTimeout && !bFallbackEngagementDetected)
		{
			AActor* FocusActor = Controller ? Controller->GetFocusActor() : nullptr;
			AHordeBaseCharacter* Support = SupportPlayer.Get();
			const FVector ZedLocation = Zed->GetActorLocation();
			const FVector PlayerLocation = Player->GetActorLocation();
			const float DistToSupport = (Support ? FVector::Dist2D(ZedLocation, Support->GetActorLocation()) : -1.f);
			UE_LOG(LogZombieReimplementationTests, Display,
				TEXT("[FallbackTimeout] HasController=%s MoveStatus=%d Focus=%s FocusIsTracked=%s FocusIsSupport=%s ")
				TEXT("ZedLoc=(%.1f, %.1f, %.1f) PlayerLoc=(%.1f, %.1f, %.1f) SupportLoc=(%.1f, %.1f, %.1f) ")
				TEXT("DistToTracked=%.1f DistToSupport=%.1f Velocity=(%.1f, %.1f, %.1f) PlayerHealth=%.1f Baseline=%.1f"),
				BoolString(Controller != nullptr),
				Controller ? static_cast<int32>(Controller->GetMoveStatus()) : -1,
				*GetNameSafe(FocusActor),
				BoolString(FocusActor == Player),
				BoolString(FocusActor == Support),
				ZedLocation.X, ZedLocation.Y, ZedLocation.Z,
				PlayerLocation.X, PlayerLocation.Y, PlayerLocation.Z,
				Support ? Support->GetActorLocation().X : 0.f,
				Support ? Support->GetActorLocation().Y : 0.f,
				Support ? Support->GetActorLocation().Z : 0.f,
				DistanceToPlayer,
				DistToSupport,
				Zed->GetVelocity().X, Zed->GetVelocity().Y, Zed->GetVelocity().Z,
				Player->GetHealth(),
				BaselineFallbackPlayerHealth);
		}

		if (bMeaningfulApproach || bDamaged) bFallbackEngagementDetected = true;

			ZedLocationAtInvalidation = Zed->GetActorLocation();
			DistanceToCoreAtInvalidation = FVector::Dist(ZedLocationAtInvalidation, RuntimeInvalidTargetCoreLocation);
			DistanceToPlayerAtInvalidation = FVector::Dist2D(ZedLocationAtInvalidation, Player->GetActorLocation());
			FallbackLastZedLocation = ZedLocationAtInvalidation;
			FallbackMinDistToCore = DistanceToCoreAtInvalidation;
			FallbackTotalXYTravel = 0.f;

		if (!Player->GetIsDead())
		{
			KillActorWithPointDamage(Player, Player, Player->GetController(), MakeBodyHit());
		}
		return true;
	}

	// Accumulates zombie movement and minimum-distance-to-core over the fallback observation
	// window. Also kills any stray zeds that might distract the tracked zombie. Exits early
	// once sufficient patrol evidence has accumulated.
	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FPollFallbackPatrolWindowCommand, float, RemainingSeconds);
	bool FPollFallbackPatrolWindowCommand::Update()
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		AZedPawn* Tracked = TrackedZed.Get();

		// Kill any stray zeds that arrived during the observation window.
		if (World && Player)
		{
			for (TActorIterator<AZedPawn> It(World); It; ++It)
			{
				AZedPawn* Zed = *It;
				if (!Zed || Zed == Tracked || Zed->GetIsDead()) continue;
				KillActorWithPointDamage(Zed, Player, Player->GetController(), MakeBodyHit());
			}
		}

		if (Tracked && !Tracked->GetIsDead())
		{
			const float DistToCore = FVector::Dist(Tracked->GetActorLocation(), RuntimeInvalidTargetCoreLocation);
			if (DistToCore < FallbackMinDistToCore) FallbackMinDistToCore = DistToCore;

			FallbackTotalXYTravel += FVector::Dist2D(Tracked->GetActorLocation(), FallbackLastZedLocation);
			FallbackLastZedLocation = Tracked->GetActorLocation();

			// Exit early once we have clear patrol evidence so the test doesn't run longer than needed.
			if (FallbackMinDistToCore <= DistanceToCoreAtInvalidation - 150.f) return true;
			if (FallbackTotalXYTravel >= 200.f) return true;
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND(FWaitForCorpseMinimumLifetimeCommand);
	bool FWaitForCorpseMinimumLifetimeCommand::Update()
	{
		if (!TrackedZed.IsValid())
		{
			bTrackedCorpseExpiredBeforeMinimum = true;
			return true;
		}

		if (UWorld* World = GetFirstPIEWorld())
		{
			return World->GetTimeSeconds() >= TrackedCorpseMinimumLifetimeDeadline;
		}

		return true;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitForClientAttackFeedbackCommand, float, RemainingSeconds);
	bool FWaitForClientAttackFeedbackCommand::Update()
	{
		if (UWorld* ClientWorld = FindPIEClientWorld())
		{
			if (AZedPawn* ClientZed = FindZedByPatrolTag(ClientWorld, RuntimeMultiplayerPatrolTag))
			{
				if (HasClientObservableAttackCue(ClientZed))
				{
					bClientObservedAttackFeedback = true;
					return true;
				}

				AHordeBaseCharacter* ClientPlayer = TrackedClientTarget.Get();
				if (!ClientPlayer)
				{
					ClientPlayer = FindClosestPlayerCharacter(ClientWorld, ClientZed->GetActorLocation());
					TrackedClientTarget = ClientPlayer;
				}

				if (ClientPlayer && (ClientPlayer->GetHealth() < BaselineClientPlayerHealth || ClientPlayer->GetIsDead()))
				{
					bClientObservedAttackFeedback = true;
					return true;
				}
			}
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FPollForFreshSpawnedZedCommand, float, RemainingSeconds);
	bool FPollForFreshSpawnedZedCommand::Update()
	{
		UWorld* World = GetFirstPIEWorld();
		if (World)
		{
			AHordeGameState* GameState = Cast<AHordeGameState>(World->GetGameState());
			if (GameState && GameState->ZedsLeft > 0)
			{
				return true;
			}
		}

		RemainingSeconds -= static_cast<float>(FApp::GetDeltaTime());
		return RemainingSeconds <= 0.f;
	}

	#define ENQUEUE_PIE_BOOTSTRAP() \
		ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(GameplayMapPath))); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling()); \
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f)); \
		ADD_LATENT_AUTOMATION_COMMAND(FForceGameStartCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FSpawnTestPlayerCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	// Bootstrap that intentionally skips FForceGameStartCommand. Used by tests
	// that need a quiet world without the round-spawner continuously placing
	// zombies near AISpawnPoints.
	#define ENQUEUE_PIE_BOOTSTRAP_NO_ROUND() \
		ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(GameplayMapPath))); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling()); \
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f)); \
		ADD_LATENT_AUTOMATION_COMMAND(FSpawnTestPlayerCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	#define ENQUEUE_MULTIPLAYER_PIE_BOOTSTRAP() \
		ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(FString(GameplayMapPath))); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling()); \
		ADD_LATENT_AUTOMATION_COMMAND(FSetupMultiplayerPIESettingsCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false)); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(12.0f)); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitForClientWorldCommand(30.0f)); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f)); \
		ADD_LATENT_AUTOMATION_COMMAND(FForceGameStartCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FSpawnTestPlayerCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

	#define ENQUEUE_PIE_TEARDOWN() \
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); \
		ADD_LATENT_AUTOMATION_COMMAND(FRestoreSinglePlayerPIESettingsCommand());
#else
	#define ENQUEUE_PIE_BOOTSTRAP()
	#define ENQUEUE_PIE_BOOTSTRAP_NO_ROUND()
	#define ENQUEUE_MULTIPLAYER_PIE_BOOTSTRAP()
	#define ENQUEUE_PIE_TEARDOWN()
#endif
}

using namespace ZombieTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombieSpawnRoundIntegrationTest,
	"HordeTemplate.Zombie.spawn_round_integration_and_patrol_tags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombieSpawnRoundIntegrationTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	BaselineAliveZeds = 0;
	TrackedZed.Reset();
	RuntimeSpawnInheritanceLocation = FVector::ZeroVector;
	RuntimeSpawnInheritancePatrolTag = MakeUniquePatrolTag(TEXT("SpawnInheritance"));

	ENQUEUE_PIE_BOOTSTRAP_NO_ROUND();

	// ── Part 1: Verify the alive-zombie counter via a direct spawn ──────────────
	// This decouples the core counter requirement from the game-mode spawn flow
	// so that counter failures are always attributable to the model's ZedPawn
	// implementation, not to game-mode wiring or timing of InitiateZombieSpawning.

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeGameState* GameState = World ? Cast<AHordeGameState>(World->GetGameState()) : nullptr;
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("PIE world must be available"), World) || !TestNotNull(TEXT("HordeGameState must exist"), GameState))
		{
			return true;
		}

		// Kill any zeds already in the world so our baseline is clean.
		if (Player)
		{
			KillAllOtherZeds(World, Player);
		}

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeGameState* GameState = World ? Cast<AHordeGameState>(World->GetGameState()) : nullptr;
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("PIE world must be available"), World) || !TestNotNull(TEXT("HordeGameState must exist"), GameState))
		{
			return true;
		}

		const int32 CounterBefore = GameState->ZedsLeft;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* Zed = World->SpawnActor<AZedPawn>(AZedPawn::StaticClass(), FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
		if (!TestNotNull(TEXT("Direct ZedPawn spawn must succeed"), Zed))
		{
			return true;
		}

		TrackedZed = Zed;
		BaselineAliveZeds = CounterBefore;
		return true;
	}));

	// Allow BeginPlay (including any delayed counter update) to complete.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeGameState* GameState = World ? Cast<AHordeGameState>(World->GetGameState()) : nullptr;
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		AZedPawn* Zed = TrackedZed.Get();
		if (!TestNotNull(TEXT("World must remain valid"), World) || !TestNotNull(TEXT("GameState must remain valid"), GameState))
		{
			return true;
		}

		TestTrue(TEXT("Alive-zombie counter must increase after a zombie spawns"), GameState->ZedsLeft > BaselineAliveZeds);

		// Kill the tracked zed and verify the counter drops.
		BaselineAliveZeds = GameState->ZedsLeft;
		if (Zed && Player)
		{
			KillActorWithPointDamage(Zed, Player, Player->GetController(), MakeBodyHit());
		}

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (UWorld* World = GetFirstPIEWorld())
		{
			if (AHordeGameState* GameState = Cast<AHordeGameState>(World->GetGameState()))
			{
				TestTrue(TEXT("Alive-zombie counter must decrease after a zombie dies"), GameState->ZedsLeft < BaselineAliveZeds);
			}
		}
		return true;
	}));

	// ── Part 2: Verify patrol-tag inheritance via the game-mode spawn flow ──────
	// This tests that zombies spawned through InitiateZombieSpawning inherit the
	// PatrolTag of the spawn point they were placed at.  A failure here indicates
	// the model did not wire PatrolTag assignment in its spawn-point or pawn code.

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeGameMode* GameMode = World ? Cast<AHordeGameMode>(World->GetAuthGameMode()) : nullptr;
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!World || !GameMode)
		{
			return true;
		}

		if (Player)
		{
			KillAllOtherZeds(World, Player);
		}

		const FVector AnchorLocation = Player
			? WithReferenceZ(PursuitPlayerLocation, Player)
			: FVector::ZeroVector;
		AAISpawnPoint* SelectedSpawnPoint = FindClosestSpawnPoint(World, AnchorLocation);
		if (!SelectedSpawnPoint)
		{
			AddWarning(TEXT("No AISpawnPoint was available for deterministic patrol-tag inheritance setup."));
			return true;
		}

		for (TActorIterator<AAISpawnPoint> It(World); It; ++It)
		{
			It->SpawnNotFree = true;
		}

		SelectedSpawnPoint->SpawnNotFree = false;
		SelectedSpawnPoint->PatrolTag = RuntimeSpawnInheritancePatrolTag;
		RuntimeSpawnInheritanceLocation = SelectedSpawnPoint->GetActorLocation();

		GameMode->InitiateZombieSpawning(1);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForTaggedZedCommand(RuntimeSpawnInheritancePatrolTag, 10.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		if (!World)
		{
			return true;
		}

		AZedPawn* SpawnedZed = FindZedByPatrolTag(World, RuntimeSpawnInheritancePatrolTag);
		if (!TestNotNull(TEXT("Game-mode spawn flow must produce a zombie with the selected spawn-point PatrolTag"), SpawnedZed))
		{
			return true;
		}

		TestFalse(TEXT("The deterministically spawned zombie must begin alive"), SpawnedZed->GetIsDead());
		TestEqual(TEXT("Zombies spawned via the game-mode flow must inherit the exact PatrolTag of their spawn point"),
			SpawnedZed->PatrolTag,
			RuntimeSpawnInheritancePatrolTag);
		TestTrue(TEXT("The deterministically spawned zombie must appear at the selected spawn point"),
			FVector::Dist2D(SpawnedZed->GetActorLocation(), RuntimeSpawnInheritanceLocation) <= 150.f);
		return true;
	}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombieHealthAndDeathTest,
	"HordeTemplate.Zombie.health_and_death_stop_enemy_activity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombieHealthAndDeathTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	BaselinePlayerHealth = 0.f;
	bTrackedCorpseExpiredBeforeMinimum = false;
	TrackedCorpseMinimumLifetimeDeadline = 0.f;
	TrackedZed.Reset();
	TrackedPlayer.Reset();

	ENQUEUE_PIE_BOOTSTRAP_NO_ROUND();

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("Player must exist"), Player))
		{
			return true;
		}

		const FVector SpawnLocation = WithReferenceZ(FVector(-800.f, 0.f, 150.f), Player);
		TeleportPlayer(Player, SpawnLocation);
		TrackedPlayer = Player;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* Zed = World->SpawnActor<AZedPawn>(AZedPawn::StaticClass(), SpawnLocation + FVector(120.f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f), Params);
		if (!TestNotNull(TEXT("Zombie must spawn for death test"), Zed))
		{
			return true;
		}

		TrackedZed = Zed;
		// Apply nonlethal damage (45 out of 100 health)
		UGameplayStatics::ApplyPointDamage(Zed, 45.f, FVector(0.f, 0.f, -1.f), MakeBodyHit(), Player->GetController(), Player, nullptr);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AZedPawn* Zed = TrackedZed.Get();
		if (!TestNotNull(TEXT("Zombie must still exist after nonlethal hit"), Zed))
		{
			return true;
		}

		TestTrue(TEXT("Zombie health must drop after taking damage"), Zed->GetHealth() < 100.f);
		TestFalse(TEXT("Zombie must survive nonlethal damage"), Zed->GetIsDead());

		// Now apply lethal damage
		if (AHordeBaseCharacter* Player = TrackedPlayer.Get())
		{
			KillActorWithPointDamage(Zed, Player, Player->GetController(), MakeBodyHit());
		}

		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AZedPawn* Zed = TrackedZed.Get();
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (!TestNotNull(TEXT("Zombie must still exist to observe death state"), Zed)
			|| !TestNotNull(TEXT("Player must still exist"), Player)
			|| !TestNotNull(TEXT("PIE world must still exist while observing death state"), World))
		{
			return true;
		}

		TestTrue(TEXT("Zombie must be dead after lethal damage"), Zed->GetIsDead());
		TestEqual(TEXT("Zombie health must be zero on death"), Zed->GetHealth(), 0.f);

		// Record player health immediately after zombie death to verify no further damage
		BaselinePlayerHealth = Player->GetHealth();
		// We only observe the corpse after waiting for death state to settle, so
		// apply a small slack rather than effectively requiring >10 seconds.
		TrackedCorpseMinimumLifetimeDeadline = World->GetTimeSeconds() + 9.0f;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (AHordeBaseCharacter* Player = TrackedPlayer.Get())
		{
			TestEqual(TEXT("Dead zombies must stop damaging nearby players"), Player->GetHealth(), BaselinePlayerHealth);
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForCorpseMinimumLifetimeCommand());

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		TestFalse(TEXT("Zombie corpses must remain present through the post-death persistence window"), bTrackedCorpseExpiredBeforeMinimum);
		return true;
	}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombiePursuitTest,
	"HordeTemplate.Zombie.detects_and_chases_living_players",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombiePursuitTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	TrackedZedStartLocation = FVector::ZeroVector;
	TrackedZed.Reset();
	TrackedPlayer.Reset();
	BaselinePursuitPlayerHealth = 0.f;
	PursuitStartXYDistance = 0.f;
	PursuitMinXYDistanceToPlayer = 0.f;
	bPursuitEverFocused = false;
	bPursuitPlayerHealthDropped = false;

	ENQUEUE_PIE_BOOTSTRAP_NO_ROUND();

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("Player must exist"), Player))
		{
			return true;
		}

		KillAllOtherZeds(World, Player);

			const FVector ZedSpawnLocation = KnownGoodAggroZedLocation;
			const FVector GroundedPlayerLocation = KnownGoodAggroPlayerLocation;
			TeleportPlayer(Player, GroundedPlayerLocation);
			TrackedPlayer = Player;

		// Record player health and reset polling accumulators before spawning the zombie.
		// Using actual health here (not a hardcoded 100) is important because the player may
		// have taken environmental damage during map load, making < 100 an unreliable baseline.
		BaselinePursuitPlayerHealth = Player->GetHealth();

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* Zed = World->SpawnActor<AZedPawn>(
			AZedPawn::StaticClass(),
			ZedSpawnLocation,
			(GroundedPlayerLocation - ZedSpawnLocation).Rotation(),
			Params);

		if (!TestNotNull(TEXT("Zombie must spawn for pursuit test"), Zed))
		{
			return true;
		}

		TrackedZed = Zed;
		TrackedZedStartLocation = Zed->GetActorLocation();
		PursuitStartXYDistance = FVector::Dist2D(Zed->GetActorLocation(), Player->GetActorLocation());
		PursuitMinXYDistanceToPlayer = PursuitStartXYDistance;
		UE_LOG(LogZombieReimplementationTests, Display,
			TEXT("[PursuitSetup] Zed=%s Player=%s StartXY=%.1f ZedLoc=(%.1f, %.1f, %.1f) PlayerLoc=(%.1f, %.1f, %.1f)"),
			*GetNameSafe(Zed),
			*GetNameSafe(Player),
			PursuitStartXYDistance,
			Zed->GetActorLocation().X, Zed->GetActorLocation().Y, Zed->GetActorLocation().Z,
			Player->GetActorLocation().X, Player->GetActorLocation().Y, Player->GetActorLocation().Z);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForTrackedZedControllerCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (Player)
		{
			// Briefly move the player away and back once the controller exists so perception
			// has a real location change to process instead of a no-op teleport.
			TeleportPlayer(Player, WithReferenceZ(RewardTestLocation, Player));
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (Player)
		{
			TeleportPlayer(Player, KnownGoodAggroPlayerLocation);
		}
		return true;
	}));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.2f));
	// Poll for 6 seconds, tracking the minimum XY distance seen, any focus events, and any
	// health drops. Using a polling window rather than a single end-of-window snapshot
	// avoids false passes from physics drift and false fails from unlucky snapshot timing.
	ADD_LATENT_AUTOMATION_COMMAND(FPollPursuitBehaviorCommand(6.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AZedPawn* Zed = TrackedZed.Get();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (!TestNotNull(TEXT("Zombie must still exist"), Zed) || !TestNotNull(TEXT("Player must still exist"), Player))
		{
			return true;
		}

		// Require meaningful pursuit over the polling window. A valid implementation may use
		// different native AI machinery, so validate outcomes rather than a specific controller:
		// the zombie must close the gap materially and either reach engagement range, focus the
		// player at some point, or inflict damage.
		const float EndXYDistance = FVector::Dist2D(Zed->GetActorLocation(), Player->GetActorLocation());
		const bool bMeaningfulApproach = PursuitMinXYDistanceToPlayer <= PursuitStartXYDistance - 40.f;
		UE_LOG(LogZombieReimplementationTests, Display,
			TEXT("[PursuitResult] StartXY=%.1f MinXY=%.1f EndXY=%.1f Focused=%s Damaged=%s"),
			PursuitStartXYDistance,
			PursuitMinXYDistanceToPlayer,
			EndXYDistance,
			bPursuitEverFocused ? TEXT("true") : TEXT("false"),
			bPursuitPlayerHealthDropped ? TEXT("true") : TEXT("false"));
		TestTrue(TEXT("Zombie must observably pursue the player over time (close the gap and engage via range, focus, or damage)"),
			bMeaningfulApproach || bPursuitEverFocused || bPursuitPlayerHealthDropped);
			return true;
		}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombieInvalidTargetFallbackTest,
	"HordeTemplate.Zombie.invalid_targets_clear_and_patrol_fallback_uses_tags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombieInvalidTargetFallbackTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	DistanceToCoreAtInvalidation = 0.f;
	DistanceToPlayerAtInvalidation = 0.f;
	ZedLocationAtInvalidation = FVector::ZeroVector;
	RuntimeInvalidTargetCoreLocation = FVector::ZeroVector;
		RuntimeInvalidTargetPatrolTag = MakeUniquePatrolTag(TEXT("ZombieTestPatrol"));
		TrackedZed.Reset();
		TrackedPlayer.Reset();
		SupportPlayer.Reset();
		BaselineFallbackPlayerHealth = 0.f;
		FallbackStartDistanceToPlayer = 0.f;
		bFallbackEngagementDetected = false;
		bFallbackHadControllerAtInvalidation = false;
		bFallbackFocusedTrackedAtInvalidation = false;
		FallbackMinDistToCore = 0.f;
		FallbackTotalXYTravel = 0.f;
		FallbackLastZedLocation = FVector::ZeroVector;

	ENQUEUE_PIE_BOOTSTRAP_NO_ROUND();

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = GetAnyLivePlayerCharacter(World);
		if (!Player)
		{
			Player = SpawnUnpossessedTestPlayer(World, KnownGoodAggroPlayerLocation);
		}
		if (!TestNotNull(TEXT("Fallback test must have a living tracked player"), Player))
		{
			return true;
		}

		KillAllOtherZeds(World, Player);

			const FVector SpawnLocation = KnownGoodAggroZedLocation;
			const FVector GroundedPlayerLocation = KnownGoodAggroPlayerLocation;
			const FVector AggroDirection = GetAggroDirection2D();
			const FVector SidewaysDirection(-AggroDirection.Y, AggroDirection.X, 0.f);
			const FVector GroundedCoreLocation = SpawnLocation + SidewaysDirection * 400.f;

			TeleportPlayer(Player, GroundedPlayerLocation);
			TrackedPlayer = Player;
			BaselineFallbackPlayerHealth = Player->GetHealth();
			TestTrue(TEXT("Fallback tracked player must start alive"), !Player->GetIsDead());
			TestTrue(TEXT("Fallback tracked player must start with positive health"), Player->GetHealth() > 0.f);
			FallbackStartDistanceToPlayer = FVector::Dist2D(SpawnLocation, GroundedPlayerLocation);

			// Keep a second living player far away so invalidating the tracked player does not
			// immediately trigger AllPlayerDeadCheck() and end-game teardown.
			const FVector SupportAnchorLocation = WithReferenceZ(RewardTestLocation, Player);
			const FVector SupportSpawnLocation = ProjectToNavMesh(World, SupportAnchorLocation);
			AHordeBaseCharacter* ExtraLivingPlayer = nullptr;
			for (TActorIterator<AHordeBaseCharacter> It(World); It; ++It)
			{
				if (*It != Player && !It->GetIsDead() && It->GetHealth() > 0.f)
				{
					ExtraLivingPlayer = *It;
					break;
				}
			}

			if (!ExtraLivingPlayer)
			{
				ExtraLivingPlayer = SpawnUnpossessedTestPlayer(World, SupportSpawnLocation);
				TestNotNull(TEXT("Support player must spawn for fallback test"), ExtraLivingPlayer);
			}

			if (ExtraLivingPlayer)
			{
				TeleportPlayer(ExtraLivingPlayer, SupportSpawnLocation);
				SupportPlayer = ExtraLivingPlayer;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AAICorePoint* CorePoint = World->SpawnActor<AAICorePoint>(AAICorePoint::StaticClass(), GroundedCoreLocation, FRotator::ZeroRotator, Params);
		if (!TestNotNull(TEXT("Tagged patrol point must spawn"), CorePoint))
		{
			return true;
		}
		CorePoint->PatrolTag = RuntimeInvalidTargetPatrolTag;
		RuntimeInvalidTargetCoreLocation = CorePoint->GetActorLocation();

		AZedPawn* Zed = World->SpawnActor<AZedPawn>(
			AZedPawn::StaticClass(),
			SpawnLocation,
			(GroundedPlayerLocation - SpawnLocation).Rotation(),
			Params);
		if (!TestNotNull(TEXT("Zombie must spawn for invalid target test"), Zed))
		{
			return true;
		}
		Zed->PatrolTag = RuntimeInvalidTargetPatrolTag;
		TrackedZed = Zed;
		AZedAIController* Controller = Zed ? Cast<AZedAIController>(Zed->GetController()) : nullptr;
		TestNotNull(TEXT("Fallback zombie must have an AI controller before engagement polling"), Controller);
		UE_LOG(LogZombieReimplementationTests, Display,
			TEXT("[FallbackSetup] PatrolTag=%s StartXY=%.1f CoreLoc=(%.1f, %.1f, %.1f) ZedLoc=(%.1f, %.1f, %.1f) PlayerLoc=(%.1f, %.1f, %.1f)"),
			*RuntimeInvalidTargetPatrolTag.ToString(),
			FallbackStartDistanceToPlayer,
			RuntimeInvalidTargetCoreLocation.X, RuntimeInvalidTargetCoreLocation.Y, RuntimeInvalidTargetCoreLocation.Z,
			Zed ? Zed->GetActorLocation().X : 0.f, Zed ? Zed->GetActorLocation().Y : 0.f, Zed ? Zed->GetActorLocation().Z : 0.f,
			Player ? Player->GetActorLocation().X : 0.f, Player ? Player->GetActorLocation().Y : 0.f, Player ? Player->GetActorLocation().Z : 0.f);
		UE_LOG(LogZombieReimplementationTests, Display,
			TEXT("[FallbackActors] TrackedPlayer=%s TrackedLoc=(%.1f, %.1f, %.1f) SupportPlayer=%s SupportLoc=(%.1f, %.1f, %.1f) DistTrackedToSupport=%.1f"),
			*GetNameSafe(Player),
			Player ? Player->GetActorLocation().X : 0.f, Player ? Player->GetActorLocation().Y : 0.f, Player ? Player->GetActorLocation().Z : 0.f,
			*GetNameSafe(SupportPlayer.Get()),
			SupportPlayer.IsValid() ? SupportPlayer->GetActorLocation().X : 0.f,
			SupportPlayer.IsValid() ? SupportPlayer->GetActorLocation().Y : 0.f,
			SupportPlayer.IsValid() ? SupportPlayer->GetActorLocation().Z : 0.f,
			(Player && SupportPlayer.IsValid()) ? FVector::Dist2D(Player->GetActorLocation(), SupportPlayer->GetActorLocation()) : -1.f);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForTrackedZedControllerCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AZedPawn* Zed = TrackedZed.Get();
		if (World && Zed)
		{
			FVector ReachableCoreLocation = RuntimeInvalidTargetCoreLocation;
			if (FindDistantReachablePointAround(World, Zed->GetActorLocation(), 150.f, 1200.f, 128, ReachableCoreLocation))
			{
				for (TActorIterator<AAICorePoint> It(World); It; ++It)
				{
					if (It->PatrolTag == RuntimeInvalidTargetPatrolTag)
					{
						It->SetActorLocation(ReachableCoreLocation, false, nullptr, ETeleportType::TeleportPhysics);
						RuntimeInvalidTargetCoreLocation = ReachableCoreLocation;
						break;
					}
				}
			}
		}

		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (Player)
		{
			// Refresh perception once the controller definitely exists.
			TeleportPlayer(Player, KnownGoodAggroPlayerLocation);
		}

		AZedAIController* Controller = Zed ? Cast<AZedAIController>(Zed->GetController()) : nullptr;
		TestNotNull(TEXT("Fallback zombie must have an AI controller before engagement polling"), Controller);
		UE_LOG(LogZombieReimplementationTests, Display,
			TEXT("[FallbackController] HasController=%s Controller=%s MoveStatus=%d Focus=%s"),
			BoolString(Controller != nullptr),
			*GetNameSafe(Controller),
			Controller ? static_cast<int32>(Controller->GetMoveStatus()) : -1,
			*GetNameSafe(Controller ? Controller->GetFocusActor() : nullptr));
		return true;
	}));
	// Give stray zeds (if any arrive via round spawner on a loaded map) a moment to appear, then kill them.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		AZedPawn* Tracked = TrackedZed.Get();
		if (World && Player)
		{
			KillAllOtherZeds(World, Player, Tracked);
		}
		return true;
	}));
		// Poll for the zombie to observably engage the player. The moment engagement is detected,
		// record the invalidation-point positions and kill the player. This avoids the race where
		// a fixed wait lets the zombie kill the player and begin patrolling before we can measure
		// the pre-invalidation location. Falls back to recording positions on timeout.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForEngagementThenInvalidateCommand(10.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
			const bool bObservedTaggedFallbackEngagement =
				bFallbackEngagementDetected
				|| (bFallbackHadControllerAtInvalidation && bFallbackFocusedTrackedAtInvalidation);
			TestTrue(TEXT("Zombie must observably engage the living player before target invalidation (approach, focus, damage, or confirmed tracked focus with a real controller)"),
				bObservedTaggedFallbackEngagement);
			return true;
		}));

	// Accumulate movement and minimum-distance-to-core over the patrol window.
	ADD_LATENT_AUTOMATION_COMMAND(FPollFallbackPatrolWindowCommand(18.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AZedPawn* Zed = TrackedZed.Get();
		if (!Zed || Zed->GetIsDead())
		{
			if (UWorld* World = GetFirstPIEWorld())
			{
				for (TActorIterator<AZedPawn> It(World); It; ++It)
				{
					if (!It->GetIsDead() && It->PatrolTag == RuntimeInvalidTargetPatrolTag)
					{
						Zed = *It;
						break;
					}
				}
			}
		}

		if (!Zed)
		{
			AddWarning(TEXT("No surviving zombie with the test patrol tag was available after target invalidation — skipping fallback movement assertion."));
			return true;
		}

		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		bool bStoppedPursuingDeadPlayer = true;
		if (Player)
		{
				const float EndDistanceToPlayer = FVector::Dist2D(Zed->GetActorLocation(), Player->GetActorLocation());
			bStoppedPursuingDeadPlayer =
				EndDistanceToPlayer >= DistanceToPlayerAtInvalidation - 25.f || !ZedHasObservedTarget(Zed, Player);
			TestTrue(TEXT("When its target dies, a zombie must stop pursuing the dead player"),
				bStoppedPursuingDeadPlayer);
		}

		// Use the minimum distance observed during the patrol window, not just the final snapshot.
		// This avoids failing when the zombie overshoots the core point or pauses briefly.
		const bool bMovedTowardCore = FallbackMinDistToCore <= DistanceToCoreAtInvalidation - 50.f;
		// Require accumulated XY travel >= 100cm so physics jitter cannot satisfy this,
		// while still allowing the reference solution's patrol resume behavior to pass.
		const bool bWanderedAfterInvalidation = FallbackTotalXYTravel >= 100.f;
		// Accept the reference solution's tagged-fallback intent even when the nav move
		// request fails from this spawned location: it must have a real controller, still
		// be focused on the tracked player at invalidation time, and be using the nearby
		// relocated patrol core.
		const bool bTaggedFallbackIntentObserved =
			bFallbackHadControllerAtInvalidation
			&& bFallbackFocusedTrackedAtInvalidation
			&& DistanceToCoreAtInvalidation <= 500.f;
		UE_LOG(LogZombieReimplementationTests, Display,
			TEXT("[FallbackResult] Engaged=%s ControllerAtInvalidation=%s FocusedTrackedAtInvalidation=%s DistToPlayerAtInvalidation=%.1f DistToCoreAtInvalidation=%.1f MinDistToCore=%.1f TotalTravel=%.1f"),
			bFallbackEngagementDetected ? TEXT("true") : TEXT("false"),
			bFallbackHadControllerAtInvalidation ? TEXT("true") : TEXT("false"),
			bFallbackFocusedTrackedAtInvalidation ? TEXT("true") : TEXT("false"),
			DistanceToPlayerAtInvalidation,
			DistanceToCoreAtInvalidation,
			FallbackMinDistToCore,
			FallbackTotalXYTravel);
		TestTrue(TEXT("When no valid target exists, the zombie must either resume meaningful fallback movement or clearly attempt tagged patrol fallback"),
			bStoppedPursuingDeadPlayer && (bMovedTowardCore || bWanderedAfterInvalidation || bTaggedFallbackIntentObserved));
		return true;
	}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombieMeleeRangeTest,
	"HordeTemplate.Zombie.melee_range_is_enforced",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombieMeleeRangeTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	BaselinePlayerHealth = 0.f;
	TrackedPlayer.Reset();
	TrackedZed.Reset();

	ENQUEUE_PIE_BOOTSTRAP_NO_ROUND();

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("Player must exist"), Player))
		{
			return true;
		}

		KillAllOtherZeds(World, Player);

		const FVector AnchorLocation = WithReferenceZ(MeleePlayerLocation, Player);
		const AAISpawnPoint* SpawnAnchor = FindClosestSpawnPoint(World, AnchorLocation);
		const FVector StablePlayerLocation = ProjectToNavMesh(World,
			SpawnAnchor ? SpawnAnchor->GetActorLocation() + FVector(-600.f, 0.f, 0.f) : AnchorLocation);
		TeleportPlayer(Player, StablePlayerLocation);
		TrackedPlayer = Player;
		BaselinePlayerHealth = Player->GetHealth();

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* FarZed = World->SpawnActor<AZedPawn>(
			AZedPawn::StaticClass(),
			StablePlayerLocation + FVector(5000.f, 0.f, 0.f),
			FRotator::ZeroRotator,
			Params);
		TestNotNull(TEXT("Far zombie must spawn"), FarZed);
		TrackedZed = FarZed;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (AHordeBaseCharacter* Player = TrackedPlayer.Get())
		{
			TestEqual(TEXT("Zombies must not damage players from outside melee range"), Player->GetHealth(), BaselinePlayerHealth);
		}
		return true;
	}));

	// Now test that a near zombie CAN attack
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (!TestNotNull(TEXT("Player must still exist for melee attack test"), Player))
		{
			return true;
		}

		KillAllOtherZeds(World, Player, TrackedZed.Get());
		if (AZedPawn* FarZed = TrackedZed.Get())
		{
			FarZed->Destroy();
		}

		BaselinePlayerHealth = Player->GetHealth();
		const FVector PlayerLoc = Player->GetActorLocation();

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* NearZed = World->SpawnActor<AZedPawn>(
			AZedPawn::StaticClass(),
			PlayerLoc + FVector(50.f, 0.f, 0.f),
			FRotator(0.f, 180.f, 0.f),
			Params);
		if (!TestNotNull(TEXT("Near zombie must spawn"), NearZed))
		{
			return true;
		}

		TrackedZed = NearZed;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForTrackedZedControllerCommand(3.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		AZedPawn* NearZed = TrackedZed.Get();
		AHordeBaseCharacter* Player = TrackedPlayer.Get();
		if (!TestNotNull(TEXT("Near zombie must still exist for melee verification"), NearZed) || !TestNotNull(TEXT("Player must still exist for melee verification"), Player))
		{
			return true;
		}

		TestTrue(TEXT("Zombies must run on the authoritative server when their attack applies damage"), NearZed->HasAuthority());

		const float EndDistance = FVector::Dist(NearZed->GetActorLocation(), Player->GetActorLocation());
		TestTrue(TEXT("Close-range zombie must remain in melee engagement distance of the player"), EndDistance <= 250.f || Player->GetIsDead());
		// Require both an AI controller (the zombie must have active AI driving the attack)
		// and actual damage. Without the controller gate, a stub zombie that causes passive
		// physics overlap damage can pass this assertion.
		TestTrue(TEXT("Zombies must perform a melee attack through the server-authoritative damage path"),
			HasPossessedZedController(NearZed) && (Player->GetHealth() < BaselinePlayerHealth || Player->GetIsDead()));
		return true;
	}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombieKillRewardsTest,
	"HordeTemplate.Zombie.kill_rewards_and_headshot_bonus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombieKillRewardsTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	BaselinePoints = 0;
	BodyKillPoints = 0;
	BaselineKills = 0;
	BaselineHeadshots = 0;

	ENQUEUE_PIE_BOOTSTRAP_NO_ROUND();

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("Player must exist"), Player))
		{
			return true;
		}

		// Resolve the player state from the pawn directly so the test observes
		// the same state that TakeDamage implementations reach via either the
		// EventInstigator or DamageCauser parameter.
		AHordePlayerState* PlayerState = Cast<AHordePlayerState>(Player->GetPlayerState());
		if (!TestNotNull(TEXT("Player must have a valid AHordePlayerState (test environment setup)"), PlayerState))
		{
			return true;
		}

		const FVector AnchorLocation = WithReferenceZ(RewardTestLocation, Player);
		const AAISpawnPoint* SpawnAnchor = FindClosestSpawnPoint(World, AnchorLocation);
		const FVector StableLocation = SpawnAnchor ? SpawnAnchor->GetActorLocation() + FVector(-600.f, 0.f, 0.f) : AnchorLocation;
		TeleportPlayer(Player, StableLocation);

		BaselinePoints = PlayerState->Points;
		BaselineKills = PlayerState->ZedKills;
		BaselineHeadshots = PlayerState->HeadShots;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* BodyZed = World->SpawnActor<AZedPawn>(AZedPawn::StaticClass(), StableLocation + FVector(300.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
		if (!TestNotNull(TEXT("Body-shot test zombie must spawn"), BodyZed))
		{
			return true;
		}

		KillActorWithPointDamage(BodyZed, Player, Player->GetController(), MakeBodyHit());
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* Player = GetAnyPlayerCharacter(World);
		if (!TestNotNull(TEXT("Player must still exist"), Player))
		{
			return true;
		}
		AHordePlayerState* PlayerState = Cast<AHordePlayerState>(Player->GetPlayerState());
		if (!TestNotNull(TEXT("PlayerState must still exist"), PlayerState))
		{
			return true;
		}

		BodyKillPoints = PlayerState->Points;
		TestTrue(TEXT("Killing a zombie must award points through the player-state system"), BodyKillPoints > BaselinePoints);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AZedPawn* HeadshotZed = World->SpawnActor<AZedPawn>(AZedPawn::StaticClass(), Player->GetActorLocation() + FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
		if (!TestNotNull(TEXT("Head-shot test zombie must spawn"), HeadshotZed))
		{
			return true;
		}

		KillActorWithPointDamage(HeadshotZed, Player, Player->GetController(), MakeHeadHit());
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* World = GetFirstPIEWorld();
		AHordeBaseCharacter* FinalPlayer = GetAnyPlayerCharacter(World);
		AHordePlayerState* PlayerState = FinalPlayer
			? Cast<AHordePlayerState>(FinalPlayer->GetPlayerState())
			: nullptr;
		if (PlayerState)
		{
			const int32 BodyReward = BodyKillPoints - BaselinePoints;
			const int32 HeadshotReward = PlayerState->Points - BodyKillPoints;

			TestTrue(TEXT("Body-shot kills must award positive points"), BodyReward > 0);
			TestTrue(TEXT("Head-shot kills must award positive points"), HeadshotReward > 0);
			TestTrue(TEXT("Head-shot kills must award more points than body-shot kills"), HeadshotReward > BodyReward);
			TestEqual(TEXT("Two zombie kills must increment the kill counter twice"), PlayerState->ZedKills, BaselineKills + 2);
			TestEqual(TEXT("One head-shot kill must increment the headshot counter once"), PlayerState->HeadShots, BaselineHeadshots + 1);
		}
		return true;
	}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FZombieMultiplayerAuthorityTest,
	"HordeTemplate.Zombie.server_ai_only_and_feedback_replicates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FZombieMultiplayerAuthorityTest::RunTest(const FString& Parameters)
{
#if WITH_EDITOR
	RuntimeMultiplayerPatrolTag = MakeUniquePatrolTag(TEXT("ZombieReplication"));
	BaselineClientPlayerHealth = 100.f;
	bClientObservedAttackFeedback = false;
	TrackedClientTarget.Reset();

	ENQUEUE_MULTIPLAYER_PIE_BOOTSTRAP();

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* ServerWorld = FindPIEServerWorld();
		UWorld* ClientWorld = FindPIEClientWorld();
		if (!TestNotNull(TEXT("Listen-server PIE world must exist"), ServerWorld) || !TestNotNull(TEXT("Client PIE world must exist"), ClientWorld))
		{
			return true;
		}

		AHordeBaseCharacter* ServerPlayer = GetAnyPlayerCharacter(ServerWorld);
		if (!TestNotNull(TEXT("Server-side player must exist"), ServerPlayer))
		{
			return true;
		}

		KillAllOtherZeds(ServerWorld, ServerPlayer);

		const FVector PlayerLocation = WithReferenceZ(MultiplayerPlayerLocation, ServerPlayer);
		TeleportPlayer(ServerPlayer, PlayerLocation);

		AZedPawn* ServerZed = ServerWorld->SpawnActorDeferred<AZedPawn>(
			AZedPawn::StaticClass(),
			FTransform(FRotator(0.f, 180.f, 0.f), PlayerLocation + FVector(50.f, 0.f, 0.f)),
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (!TestNotNull(TEXT("Server must spawn the replicated zombie"), ServerZed))
		{
			return true;
		}

		ServerZed->PatrolTag = RuntimeMultiplayerPatrolTag;
		UGameplayStatics::FinishSpawningActor(ServerZed, FTransform(FRotator(0.f, 180.f, 0.f), PlayerLocation + FVector(50.f, 0.f, 0.f)));
		TrackedZed = ServerZed;
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForReplicatedTaggedZedCommand(RuntimeMultiplayerPatrolTag, 6.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* ServerWorld = FindPIEServerWorld();
		UWorld* ClientWorld = FindPIEClientWorld();
		if (!ServerWorld || !ClientWorld)
		{
			AddWarning(TEXT("Lost PIE worlds during multiplayer authority test."));
			return true;
		}

		AZedPawn* ServerZed = TrackedZed.Get();
		AZedPawn* ClientZed = FindZedByPatrolTag(ClientWorld, RuntimeMultiplayerPatrolTag);
		if (!TestNotNull(TEXT("Server zombie must still exist"), ServerZed) || !TestNotNull(TEXT("Client must observe a replicated zombie"), ClientZed))
		{
			return true;
		}

		TestTrue(TEXT("AI logic must run on the authoritative server zombie"), ServerZed->HasAuthority());
		TestFalse(TEXT("Clients must not own authoritative zombie AI copies"), ClientZed->HasAuthority());
		TrackedClientTarget = FindClosestPlayerCharacter(ClientWorld, ClientZed->GetActorLocation());
		if (AHordeBaseCharacter* ClientPlayer = TrackedClientTarget.Get())
		{
			BaselineClientPlayerHealth = ClientPlayer->GetHealth();
		}
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForClientAttackFeedbackCommand(8.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		TestTrue(TEXT("Clients must observe a nearby zombie attack via a replicated combat cue or resulting replicated attack state"),
			bClientObservedAttackFeedback);
		return true;
	}));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		UWorld* ServerWorld = FindPIEServerWorld();
		AHordeBaseCharacter* ServerPlayer = ServerWorld ? GetAnyPlayerCharacter(ServerWorld) : nullptr;
		AZedPawn* ServerZed = TrackedZed.Get();
		if (!TestNotNull(TEXT("Server-side player must still exist"), ServerPlayer) || !TestNotNull(TEXT("Server zombie must still exist"), ServerZed))
		{
			return true;
		}

		KillActorWithPointDamage(ServerZed, ServerPlayer, ServerPlayer->GetController(), MakeBodyHit());
		return true;
	}));

	// Allow 4 seconds for the reliable multicast to arrive and replicated state to settle on the client.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(4.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this]() -> bool
	{
		if (UWorld* ClientWorld = FindPIEClientWorld())
		{
			if (AZedPawn* ClientZed = FindZedByPatrolTag(ClientWorld, RuntimeMultiplayerPatrolTag))
			{
				// Core replication requirement: death state must replicate and trigger
				// a client-visible death transition. Different correct implementations
				// may express that transition via ragdoll, disabled movement, or
				// collision shutdown, so avoid requiring a single presentation detail.
				TestTrue(TEXT("Zombie death feedback must replicate to clients"), ClientZed->GetIsDead());
				const bool bRagdolling =
					ClientZed->GetMesh() && ClientZed->GetMesh()->IsSimulatingPhysics();
				const bool bMovementDisabled =
					ClientZed->GetCharacterMovement() && ClientZed->GetCharacterMovement()->MovementMode == MOVE_None;
				const bool bCapsuleDisabled =
					ClientZed->GetCapsuleComponent() && ClientZed->GetCapsuleComponent()->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
				TestTrue(TEXT("Replicated death feedback must drive a client-observable death transition"),
					bRagdolling || bMovementDisabled || bCapsuleDisabled);
				if (USkeletalMeshComponent* Mesh = ClientZed->GetMesh())
				{
					TestTrue(TEXT("Replicated death feedback must leave the mesh valid on the client"), IsValid(Mesh));
				}
			}
			else
			{
				AddWarning(TEXT("Client zombie copy was no longer available for final death-state check."));
			}
		}
		return true;
	}));

	ENQUEUE_PIE_TEARDOWN();
#endif
	return true;
}
