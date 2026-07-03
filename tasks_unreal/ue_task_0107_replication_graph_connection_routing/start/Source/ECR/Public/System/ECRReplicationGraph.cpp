// Copyright Epic Games, Inc. All Rights Reserved.

/**
*	
*	===================== ECRReplicationGraph Replication =====================
*
*	Overview
*	
*		This changes the way actor relevancy works. AActor::IsNetRelevantFor is NOT used in this system!
*		
*		Instead, The UECRReplicationGraph contains UReplicationGraphNodes. These nodes are responsible for generating lists of actors to replicate for each connection.
*		Most of these lists are persistent across frames. This enables most of the gathering work ("which actors should be considered for replication) to be shared/reused.
*		Nodes may be global (used by all connections), connection specific (each connection gets its own node), or shared (e.g, teams: all connections on the same team share).
*		Actors can be in multiple nodes! For example a pawn may be in the spatialization node but also in the always-relevant-for-team node. It will be returned twice for 
*		teammates. This is ok though should be minimized when possible.
*		
*		UECRReplicationGraph is intended to not be directly used by the game code. That is, you should not have to include ECRReplicationGraph.h anywhere else.
*		Rather, UECRReplicationGraph depends on the game code and registers for events that the game code broadcasts (e.g., events for players joining/leaving teams).
*		This choice was made because it gives UECRReplicationGraph a complete holistic view of actor replication. Rather than exposing generic public functions that any
*		place in game code can invoke, all notifications are explicitly registered in UECRReplicationGraph::InitGlobalActorClassSettings.
*		
*	ECR Nodes
*	
*		These are the top level nodes currently used:
*		
*		UReplicationGraphNode_GridSpatialization2D: 
*		This is the spatialization node. All "distance based relevant" actors will be routed here. This node divides the map into a 2D grid. Each cell in the grid contains 
*		children nodes that hold lists of actors based on how they update/go dormant. Actors are put in multiple cells. Connections pull from the single cell they are in.
*		
*		UReplicationGraphNode_ActorList
*		This is an actor list node that contains the always relevant actors. These actors are always relevant to every connection.
*		
*		UECRReplicationGraphNode_AlwaysRelevant_ForConnection
*		This is the node for connection specific always relevant actors. This node does not maintain a persistent list but builds it each frame. This is possible because (currently)
*		these actors are all easily accessed from the PlayerController. A persistent list would require notifications to be broadcast when these actors change, which would be possible
*		but currently not necessary.
*
*		UReplicationGraphNode_TearOff_ForConnection
*		Connection specific node for handling tear off actors. This is created and managed in the base implementation of Replication Graph.
*		
*	How To Use
*	
*		Making something always relevant: Please avoid if you can :) If you must, just setting AActor::bAlwaysRelevant = true in the class defaults will do it.
*		
*		Making something always relevant to connection: You will need to modify UECRReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection. You will also want 
*		to make sure the actor does not get put in one of the other nodes. The safest way to do this is by setting its EClassRepNodeMapping to NotRouted in UECRReplicationGraph::InitGlobalActorClassSettings.
*
*	How To Debug
*	
*		Its a good idea to just disable rep graph to see if your problem is specific to this system or just general replication/game play problem.
*		
*		If it is replication graph related, there are several useful commands that can be used: see ReplicationGraph_Debugging.cpp. The most useful are below. Use the 'cheat' command to run these on the server from a client.
*	
*		"Net.RepGraph.PrintGraph" - this will print the graph to the log: each node and actor. 
*		"Net.RepGraph.PrintGraph class" - same as above but will group by class.
*		"Net.RepGraph.PrintGraph nclass" - same as above but will group by native classes (hides blueprint noise)
*		
*		Net.RepGraph.PrintAll <Frames> <ConnectionIdx> <"Class"/"Nclass"> -  will print the entire graph, the gathered actors, and how they were prioritized for a given connection for X amount of frames.
*		
*		Net.RepGraph.PrintAllActorInfo <ActorMatchString> - will print the class, global, and connection replication info associated with an actor/class. If MatchString is empty will print everything. Call directly from client.
*		
*		ECR.RepGraph.PrintRouting - will print the EClassRepNodeMapping for each class. That is, how a given actor class is routed (or not) in the Replication Graph.
*	
*/

#include "ECRReplicationGraph.h"

#include "Net/UnrealNetwork.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "CoreGlobals.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategoryReplicator.h"
#endif

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/NetConnection.h"
#include "UObject/UObjectIterator.h"

#include "ECRReplicationGraphSettings.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "Gameplay/Player/ECRPlayerController.h"

DEFINE_LOG_CATEGORY(LogECRRepGraph);

namespace ECR::RepGraph
{
	float DestructionInfoMaxDist = 30000.f;
	static FAutoConsoleVariableRef CVarECRRepGraphDestructMaxDist(
		TEXT("ECR.RepGraph.DestructInfo.MaxDist"), DestructionInfoMaxDist,
		TEXT("Max distance (not squared) to rep destruct infos at"), ECVF_Default);

	int32 DisplayClientLevelStreaming = 0;
	static FAutoConsoleVariableRef CVarECRRepGraphDisplayClientLevelStreaming(
		TEXT("ECR.RepGraph.DisplayClientLevelStreaming"), DisplayClientLevelStreaming, TEXT(""), ECVF_Default);

	float CellSize = 10000.f;
	static FAutoConsoleVariableRef CVarECRRepGraphCellSize(
		TEXT("ECR.RepGraph.CellSize"), CellSize, TEXT(""), ECVF_Default);

	// Essentially "Min X" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	float SpatialBiasX = -150000.f;
	static FAutoConsoleVariableRef CVarECRRepGraphSpatialBiasX(
		TEXT("ECR.RepGraph.SpatialBiasX"), SpatialBiasX, TEXT(""), ECVF_Default);

	// Essentially "Min Y" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	float SpatialBiasY = -200000.f;
	static FAutoConsoleVariableRef CVarECRRepSpatialBiasY(
		TEXT("ECR.RepGraph.SpatialBiasY"), SpatialBiasY, TEXT(""), ECVF_Default);

	// How many buckets to spread dynamic, spatialized actors across. High number = more buckets = smaller effective replication frequency. This happens before individual actors do their own NetUpdateFrequency check.
	int32 DynamicActorFrequencyBuckets = 3;
	static FAutoConsoleVariableRef CVarECRRepDynamicActorFrequencyBuckets(
		TEXT("ECR.RepGraph.DynamicActorFrequencyBuckets"), DynamicActorFrequencyBuckets, TEXT(""), ECVF_Default);

	int32 DisableSpatialRebuilds = 1;
	static FAutoConsoleVariableRef CVarECRRepDisableSpatialRebuilds(
		TEXT("ECR.RepGraph.DisableSpatialRebuilds"), DisableSpatialRebuilds, TEXT(""), ECVF_Default);

	int32 LogLazyInitClasses = 0;
	static FAutoConsoleVariableRef CVarECRRepLogLazyInitClasses(
		TEXT("ECR.RepGraph.LogLazyInitClasses"), LogLazyInitClasses, TEXT(""), ECVF_Default);

	// How much bandwidth to use for FastShared movement updates. This is counted independently of the NetDriver's target bandwidth.
	int32 TargetKBytesSecFastSharedPath = 2000;
	static FAutoConsoleVariableRef CVarECRRepTargetKBytesSecFastSharedPath(
		TEXT("ECR.RepGraph.TargetKBytesSecFastSharedPath"), TargetKBytesSecFastSharedPath, TEXT(""), ECVF_Default);

	float FastSharedPathCullDistPct = 0.80f;
	static FAutoConsoleVariableRef CVarECRRepFastSharedPathCullDistPct(
		TEXT("ECR.RepGraph.FastSharedPathCullDistPct"), FastSharedPathCullDistPct, TEXT(""), ECVF_Default);

	UReplicationDriver* ConditionalCreateReplicationDriver(UNetDriver* ForNetDriver, UWorld* World)
	{
		// Only create for GameNetDriver
		if (World && ForNetDriver && ForNetDriver->NetDriverName == NAME_GameNetDriver)
		{
			const UECRReplicationGraphSettings* ECRRepGraphSettings = GetDefault<UECRReplicationGraphSettings>();

			// Enable/Disable via developer settings
			bool bEnableReplicationGraph = true;

			if (ECRRepGraphSettings && !ECRRepGraphSettings->bEnableReplicationGraph)
			{
				bEnableReplicationGraph = false;
			}

			int32 CmdEnableReplicationGraphOverride = 0;
			if (FParse::Value(FCommandLine::Get(), TEXT("repgraph="), CmdEnableReplicationGraphOverride))
			{
				UE_LOG(LogECRRepGraph, Display, TEXT("Received override for enabling replication graph %d"), CmdEnableReplicationGraphOverride);
				bEnableReplicationGraph = CmdEnableReplicationGraphOverride != 0;
			}

			if (!bEnableReplicationGraph)
			{
				UE_LOG(LogECRRepGraph, Display, TEXT("Replication graph is disabled via ECRReplicationGraphSettings or CMD."));
				return nullptr;
			}

			UE_LOG(LogECRRepGraph, Display, TEXT("Replication graph is enabled for %s in world %s."),
			       *GetNameSafe(ForNetDriver), *GetPathNameSafe(World));

			TSubclassOf<UECRReplicationGraph> GraphClass = ECRRepGraphSettings->DefaultReplicationGraphClass.
				TryLoadClass<UECRReplicationGraph>();
			if (GraphClass.Get() == nullptr)
			{
				GraphClass = UECRReplicationGraph::StaticClass();
			}

			UECRReplicationGraph* ECRReplicationGraph = NewObject<UECRReplicationGraph>(
				GetTransientPackage(), GraphClass.Get());
			return ECRReplicationGraph;
		}

		return nullptr;
	}
};

// ----------------------------------------------------------------------------------------------------------

UECRReplicationGraph::UECRReplicationGraph()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::ResetGameWorldState()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


EClassRepNodeMapping UECRReplicationGraph::GetClassNodeMapping(UClass* Class) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRReplicationGraph::RegisterClassRepNodeMapping(UClass* Class)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::InitClassReplicationInfo(FClassReplicationInfo& Info, UClass* Class, bool Spatialize) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


bool UECRReplicationGraph::ConditionalInitClassReplicationInfo(UClass* ReplicatedClass,
                                                               FClassReplicationInfo& ClassInfo)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRReplicationGraph::AddClassRepInfo(UClass* Class, EClassRepNodeMapping Mapping)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::RegisterClassReplicationInfo(UClass* ReplicatedClass)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::InitGlobalActorClassSettings()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::InitGlobalGraphNodes()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::InitConnectionGraphNodes(UNetReplicationGraphConnection* RepGraphConnection)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


EClassRepNodeMapping UECRReplicationGraph::GetMappingPolicy(UClass* Class)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}


void UECRReplicationGraph::RouteAddNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo,
                                                       FGlobalActorReplicationInfo& GlobalInfo)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraph::RouteRemoveNetworkActorToNodes(const FNewReplicatedActorInfo& ActorInfo)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


// Since we listen to global (static) events, we need to watch out for cross world broadcasts (PIE)
#if WITH_EDITOR
#define CHECK_WORLDS(X) if(X->GetWorld() != GetWorld()) return;
#else
#define CHECK_WORLDS(X)
#endif

#if WITH_GAMEPLAY_DEBUGGER
void UECRReplicationGraph::OnGameplayDebuggerOwnerChange(AGameplayDebuggerCategoryReplicator* Debugger,
                                                         APlayerController* OldOwner)
{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}

#endif

#undef CHECK_WORLDS

// ------------------------------------------------------------------------------

void UECRReplicationGraphNode_AlwaysRelevant_ForConnection::ResetGameWorldState()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection(
	const FConnectionGatherActorListParameters& Params)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd(
	FName LevelName, UWorld* StreamingWorld)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove(FName LevelName)
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


void UECRReplicationGraphNode_AlwaysRelevant_ForConnection::LogNode(FReplicationGraphDebugInfo& DebugInfo,
                                                                    const FString& NodeName) const
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


// ------------------------------------------------------------------------------

void UECRReplicationGraph::PrintRepNodePolicies()
{
	// BENCHMARK_STUB: implementation intentionally removed.
}


FAutoConsoleCommandWithWorldAndArgs ECRPrintRepNodePoliciesCmd(
	TEXT("ECR.RepGraph.PrintRouting"),TEXT("Prints how actor classes are routed to RepGraph nodes"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}
)
);

// ------------------------------------------------------------------------------

FAutoConsoleCommandWithWorldAndArgs ChangeFrequencyBucketsCmd(
	TEXT("ECR.RepGraph.FrequencyBuckets"), TEXT("Resets frequency bucket count."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
	// BENCHMARK_STUB: implementation intentionally removed.
	return {};
}
));
