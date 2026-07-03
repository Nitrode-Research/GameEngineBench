#include "Utility/TVGameplayTags.h"

namespace ActorTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Player, TEXT("Game.Actor Type.Player"))
	UE_DEFINE_GAMEPLAY_TAG(AI, TEXT("Game.Actor Type.AI"))
	UE_DEFINE_GAMEPLAY_TAG(LevelActor, TEXT("Game.Actor Type.Level Actor"))
}

namespace BaseGameModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Custom, TEXT("Game.Base Game Mode.Custom"))
	UE_DEFINE_GAMEPLAY_TAG(Campaign, TEXT("Game.Base Game Mode.Campaign"))
	UE_DEFINE_GAMEPLAY_TAG(DeathMatch, TEXT("Game.Base Game Mode.Death Match"))
	UE_DEFINE_GAMEPLAY_TAG(TeamDeathMatch, TEXT("Game.Base Game Mode.Team Death Match"))
	UE_DEFINE_GAMEPLAY_TAG(CaptureTheFlag, TEXT("Game.Base Game Mode.Capture The Flag"))
	UE_DEFINE_GAMEPLAY_TAG(BattleRoyale, TEXT("Game.Base Game Mode.Battle Royale"))
	UE_DEFINE_GAMEPLAY_TAG(Extraction, TEXT("Game.Base Game Mode.Extraction"))
	UE_DEFINE_GAMEPLAY_TAG(Invasion, TEXT("Game.Base Game Mode.Invasion"))
}

namespace PlayerUnitTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.Group Type.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(AllFireteams, TEXT("Game.Group Type.All Fireteams"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.Group Type.Section"))
	UE_DEFINE_GAMEPLAY_TAG(AllSections, TEXT("Game.Group Type.All Sections"))
	UE_DEFINE_GAMEPLAY_TAG(Player, TEXT("Game.Group Type.Player"))
	UE_DEFINE_GAMEPLAY_TAG(AllPlayers, TEXT("Game.Group Type.All Players"))
	UE_DEFINE_GAMEPLAY_TAG(SoloPlayers, TEXT("Game.Group Type.Solo Players"))
	UE_DEFINE_GAMEPLAY_TAG(Security, TEXT("Game.Group Type.Security"))
}

namespace PlayerFilterTags
{
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.Player Filter.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(Command, TEXT("Game.Player Filter.Command"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.Player Filter.Section"))
	UE_DEFINE_GAMEPLAY_TAG(Organization, TEXT("Game.Player Filter.Organization"))
	UE_DEFINE_GAMEPLAY_TAG(All, TEXT("Game.Player Filter.All"))
}

namespace AwardedTags
{
	UE_DEFINE_GAMEPLAY_TAG(All, TEXT("Game.Awardred.Player"))
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.Awardred.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(Command, TEXT("Game.Awardred.Command"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.Awardred.Section"))
	UE_DEFINE_GAMEPLAY_TAG(Organization, TEXT("Game.Awardred.Organization"))
}

namespace IDTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Player, TEXT("Game.ID Type.Player"))
	UE_DEFINE_GAMEPLAY_TAG(AI, TEXT("Game.ID Type.AI"))
	UE_DEFINE_GAMEPLAY_TAG(Organization, TEXT("Game.ID Type.Organization"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.ID Type.Section"))
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.ID Type.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(LevelActor, TEXT("Game.ID Type.LevelActor"))
}

namespace SimpleCommsChannelTags
{
	UE_DEFINE_GAMEPLAY_TAG(Global, TEXT("Game.Simple Comms Channel.Global"))
	UE_DEFINE_GAMEPLAY_TAG(Area, TEXT("Game.Simple Comms Channel.Area"))
	UE_DEFINE_GAMEPLAY_TAG(Organization, TEXT("Game.Simple Comms Channel.Organization"))
	UE_DEFINE_GAMEPLAY_TAG(Command, TEXT("Game.Simple Comms Channel.Command"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.Simple Comms Channel.Section"))
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.Simple Comms Channel.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(Whisper, TEXT("Game.Simple Comms Channel.Whisper"))
}

namespace HitConfirmationTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Elimination, TEXT("Game.Hit Confirmation.Hit"))
	UE_DEFINE_GAMEPLAY_TAG(Incapacitated, TEXT("Game.Hit Confirmation.Incapacitated"))
	UE_DEFINE_GAMEPLAY_TAG(Unconscious, TEXT("Game.Hit Confirmation.Unconscious"))
}

namespace MarkerTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Self, TEXT("Game.Marker Type.Self"))
	UE_DEFINE_GAMEPLAY_TAG(Organization, TEXT("Game.Marker Type.Organization"))
	UE_DEFINE_GAMEPLAY_TAG(Command, TEXT("Game.Marker Type.Command"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.Marker Type.Section"))
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.Marker Type.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(Global, TEXT("Game.Marker Type.Global"))
}

namespace SpectatorViewModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(KillCam, TEXT("Game.Spectator View Mode.KillCam"))
	UE_DEFINE_GAMEPLAY_TAG(FirstPerson, TEXT("Game.Spectator View Mode.First Person"))
	UE_DEFINE_GAMEPLAY_TAG(FirstPersonAsItem, TEXT("Game.Spectator View Mode.First Person As Item"))
	UE_DEFINE_GAMEPLAY_TAG(ThirdPerson, TEXT("Game.Spectator View Mode.Third Person"))
	UE_DEFINE_GAMEPLAY_TAG(ThirdPersonAsItem, TEXT("Game.Spectator View Mode.Third Person As Item"))
	UE_DEFINE_GAMEPLAY_TAG(Fixed, TEXT("Game.Spectator View Mode.Fixed"))
	UE_DEFINE_GAMEPLAY_TAG(NearbyAI, TEXT("Game.Spectator View Mode.Nearby AI"))
}

namespace ActionTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Connected, TEXT("Game.Action Type.Connected"))
	UE_DEFINE_GAMEPLAY_TAG(Disconnected, TEXT("Game.Action Type.Disconnected"))
	UE_DEFINE_GAMEPLAY_TAG(Kicked, TEXT("Game.Action Type.Kicked"))
	UE_DEFINE_GAMEPLAY_TAG(Banned, TEXT("Game.Action Type.Banned"))
	UE_DEFINE_GAMEPLAY_TAG(Died, TEXT("Game.Action Type.Died"))
	UE_DEFINE_GAMEPLAY_TAG(Killed, TEXT("Game.Action Type.Killed"))
	UE_DEFINE_GAMEPLAY_TAG(KillAssist, TEXT("Game.Action Type.Kill Assist"))
	UE_DEFINE_GAMEPLAY_TAG(DisabledVehicle, TEXT("Game.Action Type.Disabled Vehicle"))
	UE_DEFINE_GAMEPLAY_TAG(DisabledVehicleAssist, TEXT("Game.Action Type.Disabled Vehicle Assist"))
	UE_DEFINE_GAMEPLAY_TAG(DefendedZone, TEXT("Game.Action Type.Defended Zone"))
	UE_DEFINE_GAMEPLAY_TAG(OccupiedZone, TEXT("Game.Action Type.Occupied Zone"))
	UE_DEFINE_GAMEPLAY_TAG(ControlledZone, TEXT("Game.Action Type.Controlled Zone"))
	UE_DEFINE_GAMEPLAY_TAG(Supressed, TEXT("Game.Action Type.Supressed"))
	UE_DEFINE_GAMEPLAY_TAG(GaveCommand, TEXT("Game.Action Type.Gave Command"))
	UE_DEFINE_GAMEPLAY_TAG(CompletedObjective, TEXT("Game.Action Type.Completed Objective"))
	UE_DEFINE_GAMEPLAY_TAG(CompletedSideMission, TEXT("Game.Action Type.Completed Side Mission"))
	UE_DEFINE_GAMEPLAY_TAG(FoundSpecialItem, TEXT("Game.Action Type.Found Special Item"))
	UE_DEFINE_GAMEPLAY_TAG(MadeSpecialNPCContact, TEXT("Game.Action Type.Made Special NPC Contact"))
	UE_DEFINE_GAMEPLAY_TAG(MadeSpecialNPCAchievement, TEXT("Game.Action Type.Made Special NPC Achievement"))
	UE_DEFINE_GAMEPLAY_TAG(Incapacitated, TEXT("Game.Action Type.Incapacitated"))
	UE_DEFINE_GAMEPLAY_TAG(WipedFireteam, TEXT("Game.Action Type.Wiped Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(WipedSection, TEXT("Game.Action Type.Wiped Section"))
	UE_DEFINE_GAMEPLAY_TAG(CompletedMission, TEXT("Game.Action Type.Completed Mission"))
	UE_DEFINE_GAMEPLAY_TAG(Pet, TEXT("Game.Action Type.Pet"))
}

namespace RewardCriteriaTags
{
	UE_DEFINE_GAMEPLAY_TAG(Immeadiate, TEXT("Game.Reward Criteria.Immeadiate"))
	UE_DEFINE_GAMEPLAY_TAG(OnAction, TEXT("Game.Reward Criteria.On Action"))
	UE_DEFINE_GAMEPLAY_TAG(AfterLeavingZone, TEXT("Game.Reward Criteria.After Leaving Zone"))
	UE_DEFINE_GAMEPLAY_TAG(AfterObjectiveCompletion, TEXT("Game.Reward Criteria.After Objective Completion"))
	UE_DEFINE_GAMEPLAY_TAG(AfterMissionCompletion, TEXT("Game.Reward Criteria.After Mission Completion"))
	UE_DEFINE_GAMEPLAY_TAG(EndOfGame, TEXT("Game.Reward Criteria.End Of Game"))
	UE_DEFINE_GAMEPLAY_TAG(EndOfGameIfSameLife, TEXT("Game.Reward Criteria.End Of Game If Same Life"))
}

namespace BonusTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Flat, TEXT("Game.Bonus Type.Flat"))
	UE_DEFINE_GAMEPLAY_TAG(Multiply, TEXT("Game.Bonus Type.Multiply"))
	UE_DEFINE_GAMEPLAY_TAG(MultiplyTotal, TEXT("Game.Bonus Type.Multiply Total"))
}

namespace ProjectileSonicsTags
{
	UE_DEFINE_GAMEPLAY_TAG(Subsonic, TEXT("Game.Projectile Sonics.Subsonic"))
	UE_DEFINE_GAMEPLAY_TAG(Supersonic, TEXT("Game.Projectile Sonics.Supersonic"))
}

namespace ServerStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(Initializing, TEXT("Game.Server State.Initializing"))
	UE_DEFINE_GAMEPLAY_TAG(Lobby, TEXT("Game.Server State.Lobby"))
	UE_DEFINE_GAMEPLAY_TAG(PreGame, TEXT("Game.Server State.PreGame"))
	UE_DEFINE_GAMEPLAY_TAG(InGame, TEXT("Game.Server State.InGame"))
	UE_DEFINE_GAMEPLAY_TAG(PostGame, TEXT("Game.Server State.PostGame"))
	UE_DEFINE_GAMEPLAY_TAG(StartingNewGame, TEXT("Game.Server State.StartingNewGame"))
	UE_DEFINE_GAMEPLAY_TAG(ShuttingDown, TEXT("Game.Server State.ShuttingDown"))
	
}

namespace PlayerSessionStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(Lobby, TEXT("Game.Player Session State.Lobby"))
	UE_DEFINE_GAMEPLAY_TAG(Game, TEXT("Game.Player Session State.Game"))
	UE_DEFINE_GAMEPLAY_TAG(Goal, TEXT("Game.Player Session State.Goal"))
	UE_DEFINE_GAMEPLAY_TAG(RespawnContest, TEXT("Game.Player Session State.Respawn Contest"))
	UE_DEFINE_GAMEPLAY_TAG(Transitioning, TEXT("Game.Player Session State.Transitioning"))	
}

namespace LobbyModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Free, TEXT("Game.Lobby Mode.Free"))
	UE_DEFINE_GAMEPLAY_TAG(Briefing, TEXT("Game.Lobby Mode.Briefing"))
	UE_DEFINE_GAMEPLAY_TAG(Atmospheric, TEXT("Game.Lobby Mode.Atmospheric"))
	UE_DEFINE_GAMEPLAY_TAG(Cinematic, TEXT("Game.Lobby Mode.Cinematic"))
}

namespace LobbyLevelModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(GameAllPlayers, TEXT("Game.Lobby Level Mode.Game All Players"))
	UE_DEFINE_GAMEPLAY_TAG(GameOrganization, TEXT("Game.Lobby Level Mode.Game Organization"))
	UE_DEFINE_GAMEPLAY_TAG(UI, TEXT("Game.Lobby Level Mode.UI"))
}

namespace RespawnContestTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(DeathMatch, TEXT("Game.Respawn Contest Type.Death Match"))
	UE_DEFINE_GAMEPLAY_TAG(Arcade, TEXT("Game.Respawn Contest Type.Arcade"))
}

namespace PreGameModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Summary, TEXT("Game.PreGame Mode.Summary"))
	UE_DEFINE_GAMEPLAY_TAG(Briefing, TEXT("Game.PreGame Mode.Briefing"))
	UE_DEFINE_GAMEPLAY_TAG(Credits, TEXT("Game.PreGame Mode.Credits"))
	UE_DEFINE_GAMEPLAY_TAG(Atmospheric, TEXT("Game.PreGame Mode.Atmospheric"))
	UE_DEFINE_GAMEPLAY_TAG(Cinematic, TEXT("Game.PreGame Mode.Cinematic"))
}

namespace GameStartModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Headquarters, TEXT("Game.Game Start Mode.Organization Headquarters"))
	UE_DEFINE_GAMEPLAY_TAG(AirdropOrganization, TEXT("Game.Game Start Mode.Airdrop Grouped by Organization"))
	UE_DEFINE_GAMEPLAY_TAG(AirdropFireteam, TEXT("Game.Game Start Mode.Airdrop Grouped by Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(AirdropRandom, TEXT("Game.Game Start Mode.Airdrop Random"))
	UE_DEFINE_GAMEPLAY_TAG(GroundOrganization, TEXT("Game.Game Start Mode.Ground Grouped by Organization"))
	UE_DEFINE_GAMEPLAY_TAG(GroundFireteam, TEXT("Game.Game Start Mode.Ground Grouped by Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(GroundRandom, TEXT("Game.Game Start Mode.Ground Random Location"))
}

namespace GoalSessionTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(SinglePlayer, TEXT("Game.Goal Session Type.Single Player"))
	UE_DEFINE_GAMEPLAY_TAG(CoOp, TEXT("Game.Goal Session Type.Co-Op"))
	UE_DEFINE_GAMEPLAY_TAG(Multiplayer, TEXT("Game.Goal Session Type.Multiplayer"))
}

namespace DynamicControlTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(FirstToFinish, TEXT("Game.Dynamic Control Type.First To Finish"))
	UE_DEFINE_GAMEPLAY_TAG(HoldUntilTime, TEXT("Game.Dynamic Control Type.Hold Until Time"))
	UE_DEFINE_GAMEPLAY_TAG(HoldUntilEvent, TEXT("Game.Dynamic Control Type.Hold Until Event"))
	UE_DEFINE_GAMEPLAY_TAG(Continuous, TEXT("Game.Dynamic Control Type.Continuous"))
}

namespace MissionTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Elimination, TEXT("Game.Mission Type.Elimination"))
	UE_DEFINE_GAMEPLAY_TAG(Exfiltration, TEXT("Game.Mission Type.Exfiltration"))
}

namespace MissionUnitTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Any, TEXT("Game.Mission Unit Type.Any"))
	UE_DEFINE_GAMEPLAY_TAG(Unaffiliated, TEXT("Game.Mission Unit Type.Unaffiliated"))
	UE_DEFINE_GAMEPLAY_TAG(Fireteam, TEXT("Game.Mission Unit Type.Fireteam"))
	UE_DEFINE_GAMEPLAY_TAG(Section, TEXT("Game.Mission Unit Type.Section"))
	UE_DEFINE_GAMEPLAY_TAG(Organization, TEXT("Game.Mission Unit Type.Organization"))
}

namespace MissionCriteriaTags
{
	UE_DEFINE_GAMEPLAY_TAG(MostObjectives, TEXT("Game.Mission Criteria.Most Objectives"))
	UE_DEFINE_GAMEPLAY_TAG(AllObjectives, TEXT("Game.Mission Criteria.All Objectives"))
	UE_DEFINE_GAMEPLAY_TAG(NoFatalities, TEXT("Game.Mission Criteria.No Fatalities"))
	UE_DEFINE_GAMEPLAY_TAG(NoSecurityResponse, TEXT("Game.Mission Criteria.No Security Response"))
	UE_DEFINE_GAMEPLAY_TAG(NoDetection, TEXT("Game.Mission Criteria.No Detection"))
	UE_DEFINE_GAMEPLAY_TAG(NoAssetDamage, TEXT("Game.Mission Criteria.No Asset Damage"))
}

namespace MissionStatusTags
{
	UE_DEFINE_GAMEPLAY_TAG(Pending, TEXT("Game.Mission Status.Pending"))
	UE_DEFINE_GAMEPLAY_TAG(Active, TEXT("Game.Mission Status.Active"))
	UE_DEFINE_GAMEPLAY_TAG(PartialSuccess, TEXT("Game.Mission Status.Partial Success"))
	UE_DEFINE_GAMEPLAY_TAG(Success, TEXT("Game.Mission Status.Success"))
	UE_DEFINE_GAMEPLAY_TAG(Fail, TEXT("Game.Mission Status.Fail"))
}

namespace ObjectiveTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(MeetContact, TEXT("Game.Objective Type.Meet Contact"))
	UE_DEFINE_GAMEPLAY_TAG(Location, TEXT("Game.Objective Type.Arrive at Location"))
	UE_DEFINE_GAMEPLAY_TAG(Zone, TEXT("Game.Objective Type.Enter Zone"))
}

namespace ObjectiveSuccessCriteriaTags
{
	UE_DEFINE_GAMEPLAY_TAG(NoFatalities, TEXT("Game.Objective Success Criteria.No Fatalities"))
	UE_DEFINE_GAMEPLAY_TAG(NoSecurityResponse, TEXT("Game.Objective Success Criteria.No Security Response"))
	UE_DEFINE_GAMEPLAY_TAG(NoDetection, TEXT("Game.Objective Success Criteria.No Detection"))
	UE_DEFINE_GAMEPLAY_TAG(NoAssetDamage, TEXT("Game.Objective Success Criteria.No Asset Damage"))
}

namespace ObjectiveStatusTags
{
	UE_DEFINE_GAMEPLAY_TAG(NotCompleted, TEXT("Game.Objective Status.Not Completed"))
	UE_DEFINE_GAMEPLAY_TAG(Completed, TEXT("Game.Objective Status.Completed"))
	UE_DEFINE_GAMEPLAY_TAG(Fail, TEXT("Game.Objective Status.Fail"))
}

namespace NotificationTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(General, TEXT("Game.Notification Type.General"))
	UE_DEFINE_GAMEPLAY_TAG(Leaderboard, TEXT("Game.Notification Type.Leaderboard"))
	UE_DEFINE_GAMEPLAY_TAG(Achievement, TEXT("Game.Notification Type.Achievement"))
	UE_DEFINE_GAMEPLAY_TAG(Progress, TEXT("Game.Notification Type.Progress"))
	UE_DEFINE_GAMEPLAY_TAG(Item, TEXT("Game.Notification Type.Item"))
	UE_DEFINE_GAMEPLAY_TAG(Perk, TEXT("Game.Notification Type.Perk"))
	UE_DEFINE_GAMEPLAY_TAG(ItemSkin, TEXT("Game.Notification Type.Item Skin"))
}

namespace ContextualInputActionTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Primary, TEXT("Game.Contextual Input Action Type.Primary"))
	UE_DEFINE_GAMEPLAY_TAG(Secondary, TEXT("Game.Contextual Input Action Type.Secondary"))
}

namespace IntelSourceTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Surveillance, TEXT("Game.Intel Source Type.Surveillance"))
	UE_DEFINE_GAMEPLAY_TAG(FirstHand, TEXT("Game.Intel Source Type.First Hand"))
	UE_DEFINE_GAMEPLAY_TAG(InternalReport, TEXT("Game.Intel Source Type.Internal Report"))
	UE_DEFINE_GAMEPLAY_TAG(Informant, TEXT("Game.Intel Source Type.Informant"))
	UE_DEFINE_GAMEPLAY_TAG(ExternalReport, TEXT("Game.Intel Source Type.External Report"))
	UE_DEFINE_GAMEPLAY_TAG(Chatter, TEXT("Game.Intel Source Type.Chatter"))
	UE_DEFINE_GAMEPLAY_TAG(Rumor, TEXT("Game.Intel Source Type.Rumor"))
}

namespace IntelValidationTags
{
	UE_DEFINE_GAMEPLAY_TAG(Undetermined, TEXT("Game.Intel Validation.Undetermined"))
	UE_DEFINE_GAMEPLAY_TAG(Valid, TEXT("Game.Intel Validation.Valid"))
	UE_DEFINE_GAMEPLAY_TAG(Invalid, TEXT("Game.Intel Validation.Invalid"))
}

namespace AppTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ninepage, TEXT("UI.App.9page"))
	UE_DEFINE_GAMEPLAY_TAG(Aegis, TEXT("UI.App.Aegis Solutions"))
	UE_DEFINE_GAMEPLAY_TAG(Altiplano, TEXT("UI.App.Altiplano"))
	UE_DEFINE_GAMEPLAY_TAG(AltiplanoFashion, TEXT("UI.App.Altiplano Fashion"))
	UE_DEFINE_GAMEPLAY_TAG(Aris, TEXT("UI.App.Aris"))
	UE_DEFINE_GAMEPLAY_TAG(Astra, TEXT("UI.App.Astra"))
	UE_DEFINE_GAMEPLAY_TAG(Broadway, TEXT("UI.App.Broadway Taxi Co"))
	UE_DEFINE_GAMEPLAY_TAG(Cyberian, TEXT("UI.App.Cyberian"))
	UE_DEFINE_GAMEPLAY_TAG(Dap, TEXT("UI.App.Dap"))
	UE_DEFINE_GAMEPLAY_TAG(EMR, TEXT("UI.App.EMR"))
	UE_DEFINE_GAMEPLAY_TAG(Gladius, TEXT("UI.App.Gladius"))
	UE_DEFINE_GAMEPLAY_TAG(Gregslist, TEXT("UI.App.Gregslist"))
	UE_DEFINE_GAMEPLAY_TAG(Hype, TEXT("UI.App.HYPE"))
	UE_DEFINE_GAMEPLAY_TAG(Mikes, TEXT("UI.App.Mohawk Mikes"))
	UE_DEFINE_GAMEPLAY_TAG(Munch, TEXT("UI.App.Munch"))
	UE_DEFINE_GAMEPLAY_TAG(Sentry, TEXT("UI.App.Sentry Security"))
	UE_DEFINE_GAMEPLAY_TAG(Sprout, TEXT("UI.App.Sprout"))
	UE_DEFINE_GAMEPLAY_TAG(Sudo, TEXT("UI.App.sudo"))
	UE_DEFINE_GAMEPLAY_TAG(Unicert, TEXT("UI.App.Unicert"))
	UE_DEFINE_GAMEPLAY_TAG(Victoria, TEXT("UI.App.Victoria"))
	UE_DEFINE_GAMEPLAY_TAG(WDBeckett, TEXT("UI.App.WD Beckett & Associates"))
	UE_DEFINE_GAMEPLAY_TAG(Zoot, TEXT("UI.App.zoot"))
}

namespace LocationIconTags
{
	UE_DEFINE_GAMEPLAY_TAG(NeutralPin, TEXT("Game.Location Icon.Neutral.Pin"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralAmmunition, TEXT("Game.Location Icon.Neutral.Ammunition"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralHeavyMachineGun, TEXT("Game.Location Icon.Neutral.Heavy Machine Gun"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralAntiTank, TEXT("Game.Location Icon.Neutral.Anti-Tank"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralArtillery, TEXT("Game.Location Icon.Neutral.Artillery"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralAirDefense, TEXT("Game.Location Icon.Neutral.Air Defense"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralDetection, TEXT("Game.Location Icon.Neutral.Detection"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralElectronicWarfare, TEXT("Game.Location Icon.Neutral.Electronic Warfare"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralChemicalHazard, TEXT("Game.Location Icon.Neutral.Chemical Hazard"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralFuel, TEXT("Game.Location Icon.Neutral.Fuel"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralSupply, TEXT("Game.Location Icon.Neutral.Supply"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralUnmannedAviation, TEXT("Game.Location Icon.Neutral.Unmanned Aviation"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralInfantry, TEXT("Game.Location Icon.Neutral.Infantry"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralVehicle, TEXT("Game.Location Icon.Neutral.Vehicle"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralArmor, TEXT("Game.Location Icon.Neutral.Armor"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralAviation, TEXT("Game.Location Icon.Neutral.Aviation"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralNaval, TEXT("Game.Location Icon.Neutral.Naval"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralHQ, TEXT("Game.Location Icon.Neutral.HQ"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralDrop, TEXT("Game.Location Icon.Neutral.Drop"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralLift, TEXT("Game.Location Icon.Neutral.Lift"))
	UE_DEFINE_GAMEPLAY_TAG(NeutralBullseye, TEXT("Game.Location Icon.Neutral.Bullseye"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyPin, TEXT("Game.Location Icon.Friendly.Pin"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyAmmunition, TEXT("Game.Location Icon.Friendly.Ammunition"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyHeavyMachineGun, TEXT("Game.Location Icon.Friendly.Heavy Machine Gun"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyAntiTank, TEXT("Game.Location Icon.Friendly.Anti-Tank"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyArtillery, TEXT("Game.Location Icon.Friendly.Artillery"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyAirDefense, TEXT("Game.Location Icon.Friendly.Air Defense"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyDetection, TEXT("Game.Location Icon.Friendly.Detection"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyElectronicWarfare, TEXT("Game.Location Icon.Friendly.Electronic Warfare"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyChemicalHazard, TEXT("Game.Location Icon.Friendly.Chemical Hazard"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyFuel, TEXT("Game.Location Icon.Friendly.Fuel"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlySupply, TEXT("Game.Location Icon.Friendly.Supply"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyUnmannedAviation, TEXT("Game.Location Icon.Friendly.Unmanned Aviation"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyMedic, TEXT("Game.Location Icon.Friendly.Medic"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyRepair, TEXT("Game.Location Icon.Friendly.Repair"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyInfantry, TEXT("Game.Location Icon.Friendly.Infantry"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyVehicle, TEXT("Game.Location Icon.Friendly.Vehicle"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyArmor, TEXT("Game.Location Icon.Friendly.Armor"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyAviation, TEXT("Game.Location Icon.Friendly.Aviation"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyNaval, TEXT("Game.Location Icon.Friendly.Naval"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyHQ, TEXT("Game.Location Icon.Friendly.HQ"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyDrop, TEXT("Game.Location Icon.Friendly.Drop"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyLift, TEXT("Game.Location Icon.Friendly.Lift"))
	UE_DEFINE_GAMEPLAY_TAG(FriendlyBullseye, TEXT("Game.Location Icon.Friendly.Bullseye"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyPin, TEXT("Game.Location Icon.Enemy.Pin"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyAmmunition, TEXT("Game.Location Icon.Enemy.Ammunition"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyHeavyMachineGun, TEXT("Game.Location Icon.Enemy.Heavy Machine Gun"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyAntiTank, TEXT("Game.Location Icon.Enemy.Anti-Tank"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyArtillery, TEXT("Game.Location Icon.Enemy.Artillery"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyAirDefense, TEXT("Game.Location Icon.Enemy.Air Defense"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyDetection, TEXT("Game.Location Icon.Enemy.Detection"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyElectronicWarfare, TEXT("Game.Location Icon.Enemy.Electronic Warfare"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyChemicalHazard, TEXT("Game.Location Icon.Enemy.Chemical Hazard"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyFuel, TEXT("Game.Location Icon.Enemy.Fuel"))
	UE_DEFINE_GAMEPLAY_TAG(EnemySupply, TEXT("Game.Location Icon.Enemy.Supply"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyUnmannedAviation, TEXT("Game.Location Icon.Enemy.Unmanned Aviation"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyInfantry, TEXT("Game.Location Icon.Enemy.Infantry"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyVehicle, TEXT("Game.Location Icon.Enemy.Vehicle"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyArmor, TEXT("Game.Location Icon.Enemy.Armor"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyAviation, TEXT("Game.Location Icon.Enemy.Aviation"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyNaval, TEXT("Game.Location Icon.Enemy.Naval"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyHQ, TEXT("Game.Location Icon.Enemy.HQ"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyDrop, TEXT("Game.Location Icon.Enemy.Drop"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyLift, TEXT("Game.Location Icon.Enemy.Lift"))
	UE_DEFINE_GAMEPLAY_TAG(EnemyBullseye, TEXT("Game.Location Icon.Enemy.Bullseye"))
}

namespace BuildingComponentStyleTag
{
	UE_DEFINE_GAMEPLAY_TAG(Residential, TEXT("Game.Building Component Style.Residential"))
	UE_DEFINE_GAMEPLAY_TAG(Retail, TEXT("Game.Building Component Style.Retail"))
	UE_DEFINE_GAMEPLAY_TAG(Industrial, TEXT("Game.Building Component Style.Industrial"))
	UE_DEFINE_GAMEPLAY_TAG(Construction, TEXT("Game.Building Component Style.Construction"))
}

namespace BuildingWallHeightTag
{
	UE_DEFINE_GAMEPLAY_TAG(Pony, TEXT("Game.Building Wall Height.Pony"))
	UE_DEFINE_GAMEPLAY_TAG(Single, TEXT("Game.Building Wall Height.Single"))
	UE_DEFINE_GAMEPLAY_TAG(Double, TEXT("Game.Building Wall Height.Double"))
}

namespace BuildingComponentPlacementTag
{
	UE_DEFINE_GAMEPLAY_TAG(Default, TEXT("Game.Building Component Placement.Default"))
	UE_DEFINE_GAMEPLAY_TAG(Corner, TEXT("Game.Building Component Placement.Corner"))
	UE_DEFINE_GAMEPLAY_TAG(InnerCorner, TEXT("Game.Building Component Placement.Inner Corner"))
}

namespace BuildingWallTypeTag
{
	UE_DEFINE_GAMEPLAY_TAG(Exterior, TEXT("Game.Building Wall Type.Exterior"))
	UE_DEFINE_GAMEPLAY_TAG(Interior, TEXT("Game.Building Wall Type.Interior"))
}

namespace BuildingWallVariantTag
{
	UE_DEFINE_GAMEPLAY_TAG(Solid, TEXT("Game.Building Wall Variant.Solid"))
	UE_DEFINE_GAMEPLAY_TAG(DoubleDoor, TEXT("Game.Building Wall Variant.Double Door"))
	UE_DEFINE_GAMEPLAY_TAG(DoubleWindow, TEXT("Game.Building Wall Variant.Double Window"))
}