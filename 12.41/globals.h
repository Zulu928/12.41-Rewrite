#pragma once

#include <atomic>

#include "inc.h"

namespace Globals
{
	extern inline int NonJoinable = 120;
	extern inline int SystemEnded = 0;
	extern inline int SystemUptime = 0;
	extern inline bool LateGame = true;
	extern inline bool AutoLateGame = true;
	extern inline bool bRefreshMatsOnKill = true;
	extern inline int Refreshmatsonpawn = 50;
	extern inline bool bSpawnCrownOnKill = true;
	extern inline int SpawnCrown = 1;
	extern inline int StartingShield = 100;
	extern inline bool bAllowNoGuiMode = true;
	extern inline bool tesplease = false;
	extern inline bool bSendWebhook = true;
	extern inline bool bCreative = false;
	extern inline bool bGoingToPlayEvent = false;
	extern inline bool bEnableAGIDs = true;
	extern inline bool bLogProcessEvent = false;
	extern inline std::atomic<bool> bLateGame(Globals::LateGame);
	extern inline bool bPlayersForFullMap = 45; // not implemented adam dont touch
	extern inline bool bKnockbackfix = true; // dumbass really thought
	extern inline bool EventForCum = false;
	extern inline bool UPTimeStarted = false;
	extern inline bool bServerBeforeStarted = false;
	extern inline bool bProperBelowChest = true;
	extern inline int  MaxMats = 500;
	extern inline bool bSentStart = false;
	extern inline bool bSentEnd = false;
	extern inline bool SentEndWebhook = false;
	extern inline bool bIsTickTiming = false;
	extern inline bool wasWebhookSent = false;
	extern inline bool IsGuiAlive = false;
	extern inline bool bPlayerJoined = false;
	extern inline bool bEnoughPlayers = false;
	extern inline bool bKillAllNiggers = false;
	extern inline int AlivePlayers = 0;
	extern inline int AmountOfPlayersWhenBusStart = 0;
	extern inline int bMaxPlayersForLategame = 30; // doesnt work i think
	extern inline int RequiredPlayers = 2;
	extern inline int LaunchTimeForBus = 10;
	extern inline bool bPlayersForRestart = false; // bus stuff
	extern inline bool bStarting = false; // bus stuff
	extern inline bool bStarted = false; // bus stuff
	extern inline bool bEnded = false; // bus stuff
	extern inline bool bEnded0 = false; // bus stuff
	extern inline bool bEndedforWebhook = false;
	extern inline bool bStartedBus = false;
	extern inline bool bdontworry = true;
	extern inline int gameserverkill = 5;
	extern inline bool bInfiniteMaterials = false;
	extern inline bool bInfiniteAmmo = false;
	extern inline bool bShouldUseReplicationGraph = false;
	extern inline std::string Gamemode = "";
	extern inline std::string BackendIP = "http://127.0.0.1:1738";
	extern inline std::string BackendAPIKey = "InfernoSex1!aaaaaaa";
	extern inline bool EnableRewards = true;
	extern inline bool bAutoRestart = false; // doesnt work fyi
	extern inline bool bFillVendingMachines = true;
	extern inline bool bPrivateIPsAreOperator = true;
	extern inline bool bDeveloperMode = true;
	extern inline bool bNoMCP = false;
	extern inline int AmountOfHealthSiphon = 50;
	extern inline bool bHitReadyToStartMatch = false;
	extern inline bool bInitializedPlaylist = false;
	extern inline bool bStartedListening = false;
	extern inline bool bEnableScoringSystem = true;
	extern inline int AmountOfListens = 0; // TODO: Switch to this for LastNum
}
extern inline int NumToSubtractFromSquadId = 0; // I think 2?

extern inline std::string PlaylistName =
"/Game/Athena/Playlists/Showdown/Playlist_ShowdownAlt_Solo.Playlist_ShowdownAlt_Solo";
// "/Game/Athena/Playlists/gg/Playlist_Gg_Reverse.Playlist_Gg_Reverse";
// "/Game/Athena/Playlists/Playlist_DefaultDuo.Playlist_DefaultDuo";
// "/Game/Athena/Playlists/Playground/Playlist_Playground.Playlist_Playground";
// "/Game/Athena/Playlists/Carmine/Playlist_Carmine.Playlist_Carmine";
// "/Game/Athena/Playlists/Fill/Playlist_Fill_Solo.Playlist_Fill_Solo";
// "/Game/Athena/Playlists/Low/Playlist_Low_Solo.Playlist_Low_Solo";
// "/Game/Athena/Playlists/Bling/Playlist_Bling_Solo.Playlist_Bling_Solo";
// "/Game/Athena/Playlists/Creative/Playlist_PlaygroundV2.Playlist_PlaygroundV2";
// "/Game/Athena/Playlists/Ashton/Playlist_Ashton_Sm.Playlist_Ashton_Sm";
// "/Game/Athena/Playlists/BattleLab/Playlist_BattleLab.Playlist_BattleLab";
