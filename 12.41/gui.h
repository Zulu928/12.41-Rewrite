#pragma once

// TODO: Update ImGUI

#pragma comment(lib, "d3d9.lib")

#include <Windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3d9.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_dx9.h>

#include <string>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_stdlib.h>
#include <vector>
#include <format>
#include <imgui/imgui_internal.h>
#include <set>
#include <fstream>
#include <olectl.h>

#include "objectviewer.h"
#include "FortAthenaMutator_Disco.h"
#include "globals.h"
#include "Fonts/ruda-bold.h"
#include "Vector.h"
#include "reboot.h"
#include "FortGameModeAthena.h"
#include "FortGameStateAthena.h"
#include "UnrealString.h"
#include "KismetTextLibrary.h"
#include "KismetSystemLibrary.h"
#include "GameplayStatics.h"
#include "Text.h"
#include <Images/reboot_icon.h>
#include "FortGadgetItemDefinition.h"
#include "FortWeaponItemDefinition.h"
#include "events.h"
#include "FortAthenaMutator_Heist.h"
#include "BGA.h"
#include "vendingmachine.h"
#include "die.h"
#include "calendar.h"
#include "GameMode.h"
#include "bots.h"
#include "moderation.h"
#include "discord.h"

#define GAME_TAB 1
#define PLAYERS_TAB 2
#define GAMEMODE_TAB 3
#define THANOS_TAB 4
#define EVENT_TAB 5
#define ZONE_TAB 6
#define CALENDAR_TAB 7
#define TRICKSHOT_TAB 8
#define DUMP_TAB 9
#define UNBAN_TAB 10
#define FUN_TAB 11
#define LATEGAME_TAB 12
#define DEVELOPER_TAB 13
#define DEBUGLOG_TAB 14
#define SETTINGS_TAB 15
#define CREDITS_TAB 16

#define MAIN_PLAYERTAB 1
#define INVENTORY_PLAYERTAB 2
#define LOADOUT_PLAYERTAB 4
#define FUN_PLAYERTAB 5

static inline float DefaultCannonMultiplier = 1.f;

extern inline int StartReverseZonePhase = 7;
extern inline int EndReverseZonePhase = 5;
extern inline float StartingShield = 100;
extern inline bool bEnableReverseZone = false;
extern inline int AmountOfPlayersWhenBusStart = 0;
extern inline bool bHandleDeath = true;
extern inline bool bUseCustomMap = false;
extern inline std::string CustomMapName = "";
extern inline int AmountToSubtractIndex = 1;
extern inline int SecondsUntilTravel = 0;
extern inline bool bSwitchedInitialLevel = false;
extern inline bool bIsInAutoRestart = false;
extern inline float AutoBusStartSeconds = 90;
extern inline float AutoBusStartSecondsThatChanges = AutoBusStartSeconds;
extern inline int NumRequiredPlayersToStart = 2;
extern inline bool bDebugPrintLooting = false;
extern inline bool AircraftHasEnteredDropZone = false;
extern inline bool AircraftHasExitedDropZone = false;
extern inline bool bDebugPrintFloorLoot = false;
extern inline bool bDebugPrintSwapping = false;
extern inline bool bEnableBotTick = true;
extern inline bool bZoneReversing = false;
extern inline bool bEnableCombinePickup = false;
extern inline int AmountOfBotsToSpawn = 0;
extern inline bool bEnableRebooting = true;
extern inline bool bEngineDebugLogs = false;
extern inline bool bStartedBus = false;
extern inline bool bShouldDestroyAllPlayerBuilds = false;
extern inline int AmountOfHealthSiphon = 50;
extern inline bool bEnableCannonAnimations = true;
extern inline float* CannonXMultiplier = &DefaultCannonMultiplier;
extern inline float* CannonYMultiplier = &DefaultCannonMultiplier;
extern inline float* CannonZMultiplier = &DefaultCannonMultiplier;
// THE BASE CODE IS FROM IMGUI GITHUB

static inline LPDIRECT3D9              g_pD3D = NULL;
static inline LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static inline D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
static inline bool CreateDeviceD3D(HWND hWnd);
static inline void CleanupDeviceD3D();
static inline void ResetDevice();
static inline LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline FString* GetRequestURL(UObject* Connection)
{
	if (Engine_Version <= 420)
		return (FString*)(__int64(Connection) + 432);
	if (std::floor(Fortnite_Version) >= 5 && Engine_Version < 424)
		return (FString*)(__int64(Connection) + 424);
	else if (Engine_Version >= 424)
		return (FString*)(__int64(Connection) + 440);

	return nullptr;
}

struct ActorSpawnStruct
{
	UClass* ClassOfClass;
	std::string ClassToSpawn;
	FVector SpawnLocation;
	FRotator SpawnRotation;
	// std::function<void(UObject*)> OnSpawned;
};

inline std::vector<std::pair<AFortPlayerControllerAthena*, UNetConnection*>> AllControllers;
inline std::vector<ActorSpawnStruct> ActorsToSpawn;

static inline void SetIsLategame(bool Value)
{
	Globals::bLateGame.store(Value);
	StartingShield = Value ? 100 : 0;
}

static inline void Restart() // todo move?
{
	FString LevelA = Engine_Version < 424
		? L"open Athena_Terrain" : Engine_Version >= 500 ? Engine_Version >= 501
		? L"open Asteria_Terrain"
		: Globals::bCreative ? L"open Creative_NoApollo_Terrain"
		: L"open Artemis_Terrain"
		: Globals::bCreative ? L"open Creative_NoApollo_Terrain"
		: L"open Apollo_Terrain";

	static auto BeaconClass = FindObject<UClass>(L"/Script/FortniteGame.FortOnlineBeaconHost");
	auto AllFortBeacons = UGameplayStatics::GetAllActorsOfClass(GetWorld(), BeaconClass);

	for (int i = 0; i < AllFortBeacons.Num(); ++i)
	{
		AllFortBeacons.at(i)->K2_DestroyActor();
	}

	AllFortBeacons.Free();

	Globals::bInitializedPlaylist = false;
	Globals::bStartedListening = false;
	Globals::bHitReadyToStartMatch = false;
	bStartedBus = false;
	AmountOfRestarts++;

	LOG_INFO(LogDev, "Switching!");

	if (Fortnite_Version >= 3) // idk what ver
	{
		((AGameMode*)GetWorld()->GetGameMode())->RestartGame();
	}
	else
	{
		UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), LevelA, nullptr);
	}

	/*

	auto& LevelCollections = GetWorld()->Get<TArray<__int64>>("LevelCollections");
	int LevelCollectionSize = FindObject<UStruct>("/Script/Engine.LevelCollection")->GetPropertiesSize();

	*(UNetDriver**)(__int64(LevelCollections.AtPtr(0, LevelCollectionSize)) + 0x10) = nullptr;
	*(UNetDriver**)(__int64(LevelCollections.AtPtr(1, LevelCollectionSize)) + 0x10) = nullptr;

	*/

	// UGameplayStatics::OpenLevel(GetWorld(), UKismetStringLibrary::Conv_StringToName(LevelA), true, FString());
}

static inline std::string wstring_to_utf8(const std::wstring& str)
{
	if (str.empty()) return {};
	const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
	std::string str_to(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &str_to[0], size_needed, nullptr, nullptr);
	return str_to;
}

static inline void InitFont()
{
	ImFontConfig FontConfig;
	FontConfig.FontDataOwnedByAtlas = false;
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)ruda_bold_data, sizeof(ruda_bold_data), 17.f, &FontConfig);
}

static inline void InitStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.1000000014901161f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 12.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(30.0f, 30.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 5.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 10.0f;
	style.PopupBorderSize = 0.0f;
	style.FramePadding = ImVec2(5.0f, 3.5f);
	style.FrameRounding = 5.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(5.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 5.0f;
	style.ColumnsMinSpacing = 5.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 15.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	// Darkish blue and darkish orange colors
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White for text
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray for disabled text
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f); // Dark slate
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f); // Slightly darker slate
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.2f, 1.0f); // Dark slate for popups
	style.Colors[ImGuiCol_Border] = ImVec4(0.2f, 0.2f, 0.3f, 1.0f); // Medium slate
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.24f, 1.0f); // Darker slate for frames
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.3f, 1.0f); // Lighter slate for hovered frames
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.4f, 1.0f); // Active frame color
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f); // Dark slate for title bar
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f); // Active title bar
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Gray for collapsed title bar
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f); // Dark slate for menu bar
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2f, 0.2f, 0.24f, 1.0f); // Darker for scrollbar background
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.4f, 1.0f); // Lighter gray for scrollbar grab
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.45f, 0.5f, 1.0f); // Even lighter gray for hovered
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.55f, 0.6f, 1.0f); // Bright gray for active
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.8f, 0.1f, 1.0f); // Bright yellow for check marks
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4f, 0.4f, 0.5f, 1.0f); // Lighter gray for slider grabs
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5f, 0.5f, 0.6f, 1.0f); // Bright gray for active slider
	style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // Soft blue for buttons
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Brighter blue for hovered
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.5f, 0.7f, 0.9f, 1.0f); // Bright blue for active buttons
	style.Colors[ImGuiCol_Header] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // Soft blue for headers
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Brighter blue for hovered headers
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.5f, 0.7f, 0.9f, 1.0f); // Bright blue for active headers
	style.Colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // Soft blue for separators
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Brighter blue for hovered separators
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.5f, 0.7f, 0.9f, 1.0f); // Bright blue for active separators
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4f, 0.4f, 0.5f, 1.0f); // Lighter gray for resize grips
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.5f, 0.5f, 0.6f, 1.0f); // Brighter gray for hovered grips
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.6f, 0.6f, 0.7f, 1.0f); // Bright gray for active grips
	style.Colors[ImGuiCol_Tab] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // Soft blue for tabs
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Brighter blue for hovered tabs
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.5f, 0.7f, 0.9f, 1.0f); // Bright blue for active tabs
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f); // Dark gray for unfocused tabs
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly lighter gray for unfocused active tabs
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // Soft blue for plot lines
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Brighter blue for hovered plot lines
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.3f, 0.5f, 0.7f, 1.0f); // Soft blue for histograms
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Brighter blue for hovered histograms
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.18f, 0.2f, 1.0f); // Darker gray for table header
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.2f, 0.2f, 0.3f, 1.0f); // Medium navy for strong borders
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.25f, 0.25f, 0.3f, 1.0f); // Lighter navy for light borders
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.15f, 0.15f, 0.2f, 1.0f); // Dark gray for table rows
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.1f, 0.1f, 0.15f, 1.0f); // Slightly darker gray for alternate rows
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 0.8f, 0.1f, 1.0f); // Bright yellow for drag and drop targets
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Bright blue for navigation highlight
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4f, 0.6f, 0.8f, 1.0f); // Bright blue for windowing highlight
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.2f, 0.2f, 0.25f, 0.8f); // Semi-transparent dark gray for windowing background
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.2f, 0.2f, 0.25f, 0.8f); // Same for modal background
}

static inline void TextCentered(const std::string& text, bool bNewLine = true) {
	if (bNewLine)
		ImGui::NewLine();

	float win_width = ImGui::GetWindowSize().x;
	float text_width = ImGui::CalcTextSize(text.c_str()).x;

	// calculate the indentation that centers the text on one line, relative
	// to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
	float text_indentation = (win_width - text_width) * 0.5f;

	// if text is too long to be drawn on one line, `text_indentation` can
	// become too small or even negative, so we check a minimum indentation
	float min_indentation = 20.0f;
	if (text_indentation <= min_indentation) {
		text_indentation = min_indentation;
	}

	ImGui::SameLine(text_indentation);
	ImGui::PushTextWrapPos(win_width - text_indentation);
	ImGui::TextWrapped(text.c_str());
	ImGui::PopTextWrapPos();
}

static inline bool ButtonCentered(const std::string& text, bool bNewLine = true) {
	if (bNewLine)
		ImGui::NewLine();

	float win_width = ImGui::GetWindowSize().x;
	float text_width = ImGui::CalcTextSize(text.c_str()).x;

	// calculate the indentation that centers the text on one line, relative
	// to window left, regardless of the `ImGuiStyleVar_WindowPadding` value
	float text_indentation = (win_width - text_width) * 0.5f;

	// if text is too long to be drawn on one line, `text_indentation` can
	// become too small or even negative, so we check a minimum indentation
	float min_indentation = 20.0f;
	if (text_indentation <= min_indentation) {
		text_indentation = min_indentation;
	}

	ImGui::SameLine(text_indentation);
	ImGui::PushTextWrapPos(win_width - text_indentation);
	auto res = ImGui::Button(text.c_str());
	ImGui::PopTextWrapPos();
	return res;
}

static inline void InputVector(const std::string& baseText, FVector* vec)
{
#ifdef ABOVE_S20
	ImGui::InputDouble((baseText + " X").c_str(), &vec->X);
	ImGui::InputDouble((baseText + " Y").c_str(), &vec->Y);
	ImGui::InputDouble((baseText + " Z").c_str(), &vec->Z);
#else
	ImGui::InputFloat((baseText + " X").c_str(), &vec->X);
	ImGui::InputFloat((baseText + " Y").c_str(), &vec->Y);
	ImGui::InputFloat((baseText + " Z").c_str(), &vec->Z);
#endif
}

static int Width = 640;
static int Height = 480;

static int Tab = 1;
static int PlayerTab = -1;
static bool bIsEditingInventory = false;
static bool bInformationTab = false;
static int playerTabTab = MAIN_PLAYERTAB;

static inline void StaticUI()
{
	if (IsRestartingSupported())
	{
		// ImGui::Checkbox("Auto Restart", &Globals::bAutoRestart);

		if (Globals::bAutoRestart)
		{
			ImGui::InputInt("Players Required To Start:", &NumRequiredPlayersToStart);
			ImGui::InputFloat(std::format("Seconds for {} Players To Start", NumRequiredPlayersToStart).c_str(), &AutoBusStartSeconds);
		}
	}

	ImGui::InputInt("Shield/Health for Siphon", &AmountOfHealthSiphon);

#ifndef PROD
	ImGui::Checkbox("Log ProcessEvent", &Globals::bLogProcessEvent);
	// ImGui::InputInt("Amount of bots to spawn", &AmountOfBotsToSpawn);
#endif

	ImGui::Checkbox("Infinite Ammo", &Globals::bInfiniteAmmo);
	ImGui::Checkbox("Infinite Materials", &Globals::bInfiniteMaterials);

	ImGui::Checkbox("No MCP", &Globals::bNoMCP);

	if (Addresses::ApplyGadgetData && Addresses::RemoveGadgetData && Engine_Version < 424)
	{
		ImGui::Checkbox("Enable Gadgets", &Globals::bEnableAGIDs);
	}
}

static inline void MainTabs()
{
	if (ImGui::BeginTabBar(""))
	{
		if (ImGui::BeginTabItem("Game"))
		{
			Tab = GAME_TAB;
			PlayerTab = -1;
			bInformationTab = false;
			ImGui::EndTabItem();
		}

		if (Globals::bStartedListening)
		{
			if (ImGui::BeginTabItem("Players"))
			{
				Tab = PLAYERS_TAB;
				ImGui::EndTabItem();
			}
		}

		if (ImGui::BeginTabItem("Other"))
		{
			Tab = FUN_TAB;
			PlayerTab = -1;
			bInformationTab = false;
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

static inline void PlayerTabs()
{
	if (ImGui::BeginTabBar(""))
	{
		if (ImGui::BeginTabItem("Main"))
		{
			playerTabTab = MAIN_PLAYERTAB;
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(("Inventory")))
		{
			playerTabTab = INVENTORY_PLAYERTAB;
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(("Cosmetics")))
		{
			playerTabTab = LOADOUT_PLAYERTAB;
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(("Fun")))
		{
			playerTabTab = FUN_PLAYERTAB;
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

static inline DWORD WINAPI LateGameThread(LPVOID)
{
	if (Globals::bStartedBus == true)
	{
		const float MaxTickRate = 30.0f;
		const float Duration = 10.0f;
		const float EarlyDuration = Duration;
		const float DropDelay = 5.0f;
		const float DropEnd = 11.0f;
		const float FlightEnd = 11.0f;
		const int InitialSleepSeconds = 10;

		auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
		auto GameState = Cast<AFortGameStateAthena>(GameMode->GetGameState());

		static auto SafeZoneLocationsOffset = GameMode->GetOffset("SafeZoneLocations");
		const TArray<FVector>& SafeZoneLocations = GameMode->Get<TArray<FVector>>(SafeZoneLocationsOffset);

		if (SafeZoneLocations.Num() < 4)
		{
			LOG_WARN(LogLateGame, "Unable to find SafeZoneLocation! Disabling lategame..");
			SetIsLategame(false);
			return 0;
		}

		const FVector ZoneCenterLocation = SafeZoneLocations.at(3);
		FVector LocationToStartAircraft = ZoneCenterLocation;
		LocationToStartAircraft.Z += 15000;
		LocationToStartAircraft.X += 2000;

		GET_PLAYLIST(GameState);

		if (CurrentPlaylist)
		{
			bool bRespawning = CurrentPlaylist->GetRespawnType() == EAthenaRespawnType::InfiniteRespawn || CurrentPlaylist->GetRespawnType() == EAthenaRespawnType::InfiniteRespawnExceptStorm;

			if (bRespawning == true)
			{
				const FVector ZoneCenterLocation = SafeZoneLocations.at(3);
				FVector LocationToStartAircraft = ZoneCenterLocation;
				LocationToStartAircraft.Z += 15000;
				LocationToStartAircraft.X += -6000;
			}
		}

		auto GetAircrafts = [&]() -> std::vector<AActor*>
			{
				static auto AircraftsOffset = GameState->GetOffset("Aircrafts", false);
				std::vector<AActor*> Aircrafts;

				if (AircraftsOffset == -1)
				{
					static auto FortAthenaAircraftClass = FindObject<UClass>(L"/Script/FortniteGame.FortAthenaAircraft");
					auto AllAircrafts = UGameplayStatics::GetAllActorsOfClass(GetWorld(), FortAthenaAircraftClass);

					for (int i = 0; i < AllAircrafts.Num(); i++)
					{
						Aircrafts.push_back(AllAircrafts.at(i));
					}

					AllAircrafts.Free();
				}
				else
				{
					const auto& GameStateAircrafts = GameState->Get<TArray<AActor*>>(AircraftsOffset);
					for (int i = 0; i < GameStateAircrafts.Num(); i++)
					{
						Aircrafts.push_back(GameStateAircrafts.at(i));
					}
				}

				return Aircrafts;
			};

		static auto WarmupCountdownEndTimeOffset = GameState->GetOffset("WarmupCountdownEndTime");
		static auto WarmupCountdownDurationOffset = GameMode->GetOffset("WarmupCountdownDuration");
		static auto WarmupEarlyCountdownDurationOffset = GameMode->GetOffset("WarmupEarlyCountdownDuration");

		float TimeSeconds = GameState->GetServerWorldTimeSeconds();
		GameState->Get<float>(WarmupCountdownEndTimeOffset) = TimeSeconds + Duration;
		GameMode->Get<float>(WarmupCountdownDurationOffset) = Duration;
		GameMode->Get<float>(WarmupEarlyCountdownDurationOffset) = EarlyDuration;

		auto Aircrafts = GetAircrafts();
		float FlightSpeed = 0.0f;
		float DropStartTime = GameState->GetServerWorldTimeSeconds() + DropDelay;

		for (int i = 0; i < Aircrafts.size(); ++i)
		{
			auto CurrentAircraft = Aircrafts.at(i);

			std::this_thread::sleep_for(std::chrono::seconds(160) / MaxTickRate);

			if (CurrentAircraft)
			{
				//LOG_INFO(LogLateGame, "Attempting to teleport aircraft %d to location: %s", i, *LocationToStartAircraft.ToString());

				CurrentAircraft->TeleportTo(LocationToStartAircraft, FRotator());

				FVector NewLocation = CurrentAircraft->GetActorLocation();
				//LOG_INFO(LogLateGame, "New Location of Aircraft %d: %s", i, *NewLocation.ToString());

				static auto FlightInfoOffset = CurrentAircraft->GetOffset("FlightInfo", false);
				if (FlightInfoOffset == -1)
				{
					static auto FlightStartLocationOffset = CurrentAircraft->GetOffset("FlightStartLocation");
					static auto FlightSpeedOffset = CurrentAircraft->GetOffset("FlightSpeed");
					static auto DropStartTimeOffset = CurrentAircraft->GetOffset("DropStartTime");

					CurrentAircraft->Get<FVector>(FlightStartLocationOffset) = LocationToStartAircraft;
					CurrentAircraft->Get<float>(FlightSpeedOffset) = FlightSpeed;
					CurrentAircraft->Get<float>(DropStartTimeOffset) = DropStartTime;
				}
				else
				{
					auto FlightInfo = CurrentAircraft->GetPtr<FAircraftFlightInfo>(FlightInfoOffset);
					FlightInfo->GetFlightSpeed() = FlightSpeed;
					FlightInfo->GetFlightStartLocation() = LocationToStartAircraft;
					FlightInfo->GetTimeTillDropStart() = DropStartTime;
				}
			}
		}

		if (Globals::bStarted == true)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(6000));
			auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
			GameState->SkipAircraft();
		}

		GameState->GetGamePhase() = EAthenaGamePhase::SafeZones;
		GameState->OnRep_GamePhase();

		static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
		auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
		auto& ClientConnections = WorldNetDriver->GetClientConnections();

		for (int z = 0; z < ClientConnections.Num(); z++)
		{
			auto ClientConnection = ClientConnections.at(z);
			auto FortPC = Cast<AFortPlayerController>(ClientConnection->GetPlayerController());

			if (!FortPC)
				continue;

			auto WorldInventory = FortPC->GetWorldInventory();

			if (!WorldInventory)
				continue;

			std::random_device rd;
			std::mt19937 gen(rd());

			std::vector<const wchar_t*> RifleOptions = {
				L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_R_Ore_T03.WID_Assault_Auto_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_UC_Ore_T03.WID_Assault_Auto_Athena_UC_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_SR_Ore_T03.WID_Assault_AutoHigh_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_R_Ore_T03.WID_Assault_Auto_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_Auto_Athena_UC_Ore_T03.WID_Assault_Auto_Athena_UC_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Assault_AutoHigh_Athena_VR_Ore_T03.WID_Assault_AutoHigh_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/Boss/WID_Boss_Adventure_AR.WID_Boss_Adventure_AR",
				L"/Game/Athena/Items/Weapons/Boss/WID_Boss_Meow_HA.WID_Boss_Meow_HA",
			};

			std::vector<const wchar_t*> ShotgunOptions = {
				L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_VR_Ore_T03.WID_Shotgun_Standard_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_SR_Ore_T03.WID_Shotgun_Standard_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03.WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_HighSemiAuto_Athena_VR_Ore_T03.WID_Shotgun_HighSemiAuto_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_Standard_Athena_UC_Ore_T03.WID_Shotgun_Standard_Athena_UC_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03.WID_Shotgun_HighSemiAuto_Athena_SR_Ore_T03",
			};

			std::vector<const wchar_t*> SMGOptions = {
				L"/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavyPDW_Athena_SR_Ore_T03.WID_Pistol_AutoHeavyPDW_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavyPDW_Athena_VR_Ore_T03.WID_Pistol_AutoHeavyPDW_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Pistol_AutoHeavyPDW_Athena_R_Ore_T03.WID_Pistol_AutoHeavyPDW_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/Boss/WID_Boss_Adventure_GH.WID_Boss_Adventure_GH",
				L"/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_R_Ore_T03.WID_Pistol_RapidFireSMG_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_R_Ore_T03.WID_Pistol_RapidFireSMG_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_R_Ore_T03.WID_Pistol_RapidFireSMG_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_VR_Ore_T03.WID_Sniper_Heavy_Athena_VR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Sniper_Heavy_Athena_SR_Ore_T03.WID_Sniper_Heavy_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Pistol_RapidFireSMG_Athena_R_Ore_T03.WID_Pistol_RapidFireSMG_Athena_R_Ore_T03",
				L"/Game/Athena/Items/Weapons/WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03.WID_Sniper_BoltAction_Scope_Athena_SR_Ore_T03",
				L"/Game/Athena/Items/Weapons/Boss/WID_Boss_Midas.WID_Boss_Midas",
			};

			std::vector<const wchar_t*> OtherOptions = {
				L"/Game/Athena/Items/Consumables/ShockwaveGrenade/Athena_ShockGrenade.Athena_ShockGrenade",
				L"/Game/Athena/Items/Consumables/Grenade/Athena_Grenade.Athena_Grenade",
				L"/Game/Athena/Items/Consumables/AppleSun/WID_Athena_AppleSun.WID_Athena_AppleSun",
			};

			std::vector<const wchar_t*> ShieldOptions = {
				L"/Game/Athena/Items/Consumables/Shields/Athena_Shields.Athena_Shields",
				L"/Game/Athena/Items/Consumables/ShieldSmall/Athena_ShieldSmall.Athena_ShieldSmall",
				L"/Game/Athena/Items/Consumables/Medkit/Athena_Medkit.Athena_Medkit",
				L"/Game/Athena/Items/Consumables/Bandage/Athena_Bandage.Athena_Bandage",
				L"/Game/Athena/Items/Consumables/Flopper/Effective/WID_Athena_Flopper_Effective.WID_Athena_Flopper_Effective",
				L"/Game/Athena/Items/Consumables/Flopper/WID_Athena_Flopper.WID_Athena_Flopper",
			};

			std::vector<const wchar_t*> TrapOptions = {
				L"/Game/Athena/Items/Traps/TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena",
				L"/Game/Athena/Items/Traps/TID_Floor_Player_Campfire_Athena.TID_Floor_Player_Campfire_Athena",
				L"/Game/Athena/Items/Traps/TID_Context_BouncePad_Athena.TID_Context_BouncePad_Athena",
				L"/Game/Athena/Items/Traps/TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena",
				L"/Game/Athena/Items/Traps/TID_Context_BouncePad_Athena.TID_Context_BouncePad_Athena",
			};

			std::uniform_int_distribution<> RifleDist(0, RifleOptions.size() - 1);
			std::uniform_int_distribution<> ShotgunDist(0, ShotgunOptions.size() - 1);
			std::uniform_int_distribution<> SMGDist(0, SMGOptions.size() - 1);
			std::uniform_int_distribution<> OptionDist(0, OtherOptions.size() - 1);
			std::uniform_int_distribution<> ShieldDist(0, ShieldOptions.size() - 1);
			std::uniform_int_distribution<> TrapDist(0, TrapOptions.size() - 1);

			UFortItemDefinition* Rifle = FindObject<UFortItemDefinition>(RifleOptions[RifleDist(gen)]);
			UFortItemDefinition* Shotgun = FindObject<UFortItemDefinition>(ShotgunOptions[ShotgunDist(gen)]);
			UFortItemDefinition* SMG = FindObject<UFortItemDefinition>(SMGOptions[SMGDist(gen)]);
			UFortItemDefinition* Other = FindObject<UFortItemDefinition>(OtherOptions[OptionDist(gen)]);
			UFortItemDefinition* Shield = FindObject<UFortItemDefinition>(ShieldOptions[ShieldDist(gen)]);
			UFortItemDefinition* Trap = FindObject<UFortItemDefinition>(TrapOptions[TrapDist(gen)]);

			static auto WoodItemData = FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
			static auto StoneItemData = FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
			static auto MetalItemData = FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData");
			static auto Shells = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataShells.AthenaAmmoDataShells");
			static auto Medium = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsMedium.AthenaAmmoDataBulletsMedium");
			static auto Light = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsLight.AthenaAmmoDataBulletsLight");
			static auto Heavy = FindObject<UFortItemDefinition>(L"/Game/Athena/Items/Ammo/AthenaAmmoDataBulletsHeavy.AthenaAmmoDataBulletsHeavy");

			WorldInventory->AddItem(WoodItemData, nullptr, 500);
			WorldInventory->AddItem(StoneItemData, nullptr, 500);
			WorldInventory->AddItem(MetalItemData, nullptr, 500);
			WorldInventory->AddItem(Shotgun, nullptr, 1);
			WorldInventory->AddItem(SMG, nullptr, 1);
			WorldInventory->AddItem(Rifle, nullptr, 1);
			WorldInventory->AddItem(Other, nullptr, 2);
			WorldInventory->AddItem(Shield, nullptr, 3);
			WorldInventory->AddItem(Trap, nullptr, 2);
			WorldInventory->AddItem(Shells, nullptr, 25);
			WorldInventory->AddItem(Medium, nullptr, 90);
			WorldInventory->AddItem(Light, nullptr, 120);
			WorldInventory->AddItem(Heavy, nullptr, 14);

			WorldInventory->Update();
		}
		static const auto safeZonesStartTimeOffset = GameState->GetOffset("SafeZonesStartTime");

		if (safeZonesStartTimeOffset)
		{
			GameState->Get<float>(safeZonesStartTimeOffset) = 0.001f;
		}
		return 0;
	}
}

static inline DWORD WINAPI StartGameWithBusThread(LPVOID)
{
	auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
	auto GameState = Cast<AFortGameStateAthena>(GameMode->GetGameState());

	float Duration = Globals::LaunchTimeForBus;
	float EarlyDuration = Duration;

	float TimeSeconds = UGameplayStatics::GetTimeSeconds(GetWorld());

	static auto WarmupCountdownEndTimeOffset = GameState->GetOffset("WarmupCountdownEndTime");
	static auto WarmupCountdownStartTimeOffset = GameState->GetOffset("WarmupCountdownStartTime");
	static auto WarmupCountdownDurationOffset = GameMode->GetOffset("WarmupCountdownDuration");
	static auto WarmupEarlyCountdownDurationOffset = GameMode->GetOffset("WarmupEarlyCountdownDuration");

	GameState->Get<float>(WarmupCountdownEndTimeOffset) = TimeSeconds + Duration;
	GameMode->Get<float>(WarmupCountdownDurationOffset) = Duration;

	GameState->Get<float>(WarmupCountdownStartTimeOffset) = TimeSeconds;
	GameMode->Get<float>(WarmupEarlyCountdownDurationOffset) = EarlyDuration;

	LOG_INFO(LogDev, "Auto starting bus in {}.", Globals::LaunchTimeForBus);

	Sleep(1000 * Duration + 200);

	if (bStartedBus)
		return 0;

	bStartedBus = true;

	AmountOfPlayersWhenBusStart = GameState->GetPlayersLeft();

	if (Globals::bLateGame.load())
	{
		CreateThread(0, 0, LateGameThread, 0, 0, 0);
	}
	else
	{
		GameMode->StartAircraftPhase();
	}

	return 0;
}

static inline DWORD WINAPI UpdateSystemUptime(LPVOID)
{
	while (true)
	{
		int randomSleepTime = rand() % 1001 + 500;
		Sleep(randomSleepTime);

		Globals::SystemUptime += 1;

		if (rand() % 10 == 0)
		{
			("System uptime: " + std::to_string(Globals::SystemUptime) + " seconds");
		}
	}
}

static inline DWORD WINAPI ForceStart(LPVOID)
{
	auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
	auto GameState = Cast<AFortGameStateAthena>(GameMode->GetGameState());

	float Duration = 10;
	float EarlyDuration = Duration;

	float TimeSeconds = UGameplayStatics::GetTimeSeconds(GetWorld());

	static auto WarmupCountdownEndTimeOffset = GameState->GetOffset("WarmupCountdownEndTime");
	static auto WarmupCountdownStartTimeOffset = GameState->GetOffset("WarmupCountdownStartTime");
	static auto WarmupCountdownDurationOffset = GameMode->GetOffset("WarmupCountdownDuration");
	static auto WarmupEarlyCountdownDurationOffset = GameMode->GetOffset("WarmupEarlyCountdownDuration");

	GameState->Get<float>(WarmupCountdownEndTimeOffset) = TimeSeconds + Duration;
	GameMode->Get<float>(WarmupCountdownDurationOffset) = Duration;

	GameState->Get<float>(WarmupCountdownStartTimeOffset) = TimeSeconds;
	GameMode->Get<float>(WarmupEarlyCountdownDurationOffset) = EarlyDuration;

	LOG_INFO(LogDev, "Auto starting bus in {}.", AutoBusStartSeconds);

	Sleep(1000 * Duration + 200);
	bStartedBus = true;

	AmountOfPlayersWhenBusStart = GameState->GetPlayersLeft();

	if (Globals::bLateGame.load())
	{
		CreateThread(0, 0, LateGameThread, 0, 0, 0);
	}
	else
	{
		GameMode->StartAircraftPhase();
	}

	return 0;
}

static inline std::string convertToHMS(int totalSeconds) {
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;

	std::string result = "";

	if (hours > 0) {
		result += std::to_string(hours) + "h ";
	}
	if (minutes > 0) {
		result += std::to_string(minutes) + "m ";
	}
	if (seconds > 0 || (hours == 0 && minutes == 0)) {
		result += std::to_string(seconds) + "s";
	}

	return result;
}

static std::string UUIDv4() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	ss << std::setw(8) << dis(gen) << "-";
	ss << std::setw(4) << (dis(gen) & 0xFFFF) << "-";
	ss << std::setw(4) << ((dis(gen) & 0x0FFF) | 0x4000) << "-";
	ss << std::setw(4) << ((dis(gen) & 0x3FFF) | 0x8000) << "-";
	ss << std::setw(12) << dis(gen) << dis(gen);

	return ss.str();
}

static inline void NoGUI()
{
	Globals::bStarted = bStartedBus;
	bool bLoaded = true;
	bool uc = false;
	Globals::UPTimeStarted = true;
	Globals::bStarted = bStartedBus;
	Globals::IsGuiAlive = true;
	auto GameState = Cast<AFortGameStateAthena>(((AFortGameMode*)GetWorld()->GetGameMode())->GetGameState());

	Globals::AlivePlayers = GameState->GetPlayersLeft();


	static bool TriggerNotJoinableOnCrash = false;
	if (Globals::SystemUptime > 120)
	{
		TriggerNotJoinableOnCrash = true;
	}

	if (!Globals::bServerBeforeStarted && GameState->GetPlayersLeft() >= 1)
	{
		Globals::bServerBeforeStarted = true;
	}

	if (Globals::bStartedListening && !Globals::UPTimeStarted)
	{
		Globals::UPTimeStarted = true;
		CreateThread(0, 0, UpdateSystemUptime, 0, 0, 0);
	}

	if (Globals::AlivePlayers >= 1 && Globals::bStarting == false && Globals::bStarted == false)
	{
		Globals::bStarting = true;
		CreateThread(0, 0, StartGameWithBusThread, 0, 0, 0);
	}

	if (Globals::bStartedListening && !Globals::UPTimeStarted)
	{
		Globals::UPTimeStarted = true;
		CreateThread(0, 0, UpdateSystemUptime, 0, 0, 0);
	}

	if (Globals::bSendWebhook && !Globals::bSentStart && Globals::bStarted)
	{
		Globals::bSentStart = true;

		int playersLeft = GameState->GetPlayersLeft();

		std::string title = "NAE Server Started";
		std::string content = std::format("Game Started With **{}** **Players**", playersLeft);

		std::cout << "Webhook message: " << content << std::endl;

		UptimeWebHook.send_embed(title, content, 88000);
	}

	if (Globals::bSendWebhook == true && Globals::bSentEnd == false && bStartedBus == true)
	{
		int playersLeft = GameState->GetPlayersLeft() = 1;

		std::string title = "NAE Server Ended";
		std::string content = std::format("NA Server is Restarting!");

		std::cout << "Webhook message: " << content << std::endl;

		UptimeWebHook.send_embed(title, content, 16744448);

		Globals::bSentEnd = true;
	}

	if (!Globals::wasWebhookSent && Globals::SystemUptime > 5 && Globals::bStartedListening && Globals::bSendWebhook)
	{
		std::string roleMention = "<@&1241198290070933636>";
		UptimeWebHook.send_message(roleMention);

		std::string title = "NAE Server Online";
		std::string content = "Server Is Online Ready Up!";

		std::cout << "Webhook message: " << content << std::endl;

		UptimeWebHook.send_embed(title, content, 3066993);

		Globals::wasWebhookSent = true;
	}

	if (bStartedBus && !Globals::bStartedBus == true)
	{
		Globals::bStartedBus = true;
	}
}

static inline DWORD WINAPI NoGUIThread(LPVOID)
{
	while (true)
	{
		NoGUI();
	}

	return 0;
}

static inline void MainUI()
{
	bool bLoaded = true;

	if (PlayerTab == -1)
	{
		MainTabs();

		if (Tab == GAME_TAB)
		{
			if (bLoaded)
			{
				StaticUI();

				Globals::bStarted = bStartedBus;
				Globals::IsGuiAlive = true;

				auto GameState = Cast<AFortGameStateAthena>(((AFortGameMode*)GetWorld()->GetGameMode())->GetGameState());

				Globals::AlivePlayers = GameState->GetPlayersLeft();

				if (!Globals::bServerBeforeStarted && GameState->GetPlayersLeft() >= 1)
				{
					Globals::bServerBeforeStarted = true;
				}

				if (Globals::bStartedListening && !Globals::UPTimeStarted)
				{
					Globals::UPTimeStarted = true;
					CreateThread(0, 0, UpdateSystemUptime, 0, 0, 0);
				}

				if (Globals::AlivePlayers >= 1 && Globals::bStarting == false && Globals::bStarted == false)
				{
					Globals::bStarting = true;
					CreateThread(0, 0, StartGameWithBusThread, 0, 0, 0);
				}

				if (Globals::bStartedListening && !Globals::UPTimeStarted)
				{
					Globals::UPTimeStarted = true;
					CreateThread(0, 0, UpdateSystemUptime, 0, 0, 0);
				}

				if (Globals::bSendWebhook && !Globals::bSentStart && Globals::bStarted == true)
				{
					Globals::bSentStart = true;

					int playersLeft = GameState->GetPlayersLeft();

					std::string title = "NAE Server Started";
					std::string content = std::format("Game Started With **{}** **Players**", playersLeft);

					std::cout << "Webhook message: " << content << std::endl;

					UptimeWebHook.send_embed(title, content, 88000);
				}

				if (Globals::bSendWebhook == true && Globals::bSentEnd == false && bStartedBus == true)
				{
					int playersLeft = Globals::AlivePlayers;
					if (playersLeft <= 1)
					{
						std::string title = "NAE Server Ended";
						std::string content = std::format("NA Server is Restarting!");

						std::cout << "Webhook message: " << content << std::endl;

						UptimeWebHook.send_embed(title, content, 16744448);

						Globals::bSentEnd = true;
					}
				}

				if (!Globals::wasWebhookSent && Globals::SystemUptime > 5 && Globals::bStartedListening && Globals::bSendWebhook)
				{
					std::string roleMention = "<@&1270135502523207843>";
					UptimeWebHook.send_message(roleMention);

					std::string title = "NAE Server Online";
					std::string content = "Server Is Online Ready Up!";

					std::cout << "Webhook message: " << content << std::endl;

					UptimeWebHook.send_embed(title, content, 3066993);

					Globals::wasWebhookSent = true;
				}

				if (bStartedBus && !Globals::bStartedBus == true)
				{
					Globals::bStartedBus = true;
				}

				ImGui::Text(std::format("Uptime: {}", convertToHMS(Globals::SystemUptime)).c_str());
				ImGui::Text(std::format("Joinable {}", Globals::bStartedListening).c_str());
				ImGui::Text(std::format("Started {}", bStartedBus).c_str());
				ImGui::Text(std::format("Ended {}", Globals::bEnded0).c_str());
				ImGui::Text(std::format("Alive Players {}", Globals::AlivePlayers).c_str());

				static std::string ConsoleCommand;

				ImGui::InputText("Console command", &ConsoleCommand);

				if (ImGui::Button("Execute console command"))
				{
					auto wstr = std::wstring(ConsoleCommand.begin(), ConsoleCommand.end());

					auto aa = wstr.c_str();
					FString cmd = aa;

					UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), cmd, nullptr);
				}

				if (!bIsInAutoRestart && Engine_Version < 424 && ImGui::Button("Restart"))
				{
					if (Engine_Version < 424)
					{
						Restart();
						LOG_INFO(LogGame, "Restarting!");
					}
					else
					{
						LOG_ERROR(LogGame, "Restarting is not supported on chapter 2 and above!");
					}
				}

				if (!bStartedBus) {
					if (ImGui::Button("Start Bus Countdown")) {
						Globals::bStarting = true;
						bStartedBus = true;
						CreateThread(0, 0, ForceStart, 0, 0, 0);
					}
				}
			}
		}

		else if (Tab == PLAYERS_TAB)
		{
			auto World = GetWorld();
			if (bLoaded)
			{
				if (World)
				{
					static auto NetDriverOffset = World->GetOffset("NetDriver");
					auto NetDriver = *(UObject**)(__int64(World) + NetDriverOffset);

					if (NetDriver)
					{
						static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
						auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
						auto& ClientConnections = WorldNetDriver->GetClientConnections();

						// if (ClientConnections)
						{
							for (int i = 0; i < ClientConnections.Num(); i++) {
								auto Connection = ClientConnections.At(i);

								if (!Connection)
									continue;

								auto CurrentController = Cast<AFortPlayerControllerAthena>(ClientConnections.at(i)->GetPlayerController());

								if (CurrentController) {
									auto it = std::find_if(AllControllers.begin(), AllControllers.end(), [CurrentController, Connection](const auto& pair)
										{
											return pair.first == CurrentController && pair.second == Connection;
										});

									if (it == AllControllers.end()) {
										AllControllers.push_back({ CurrentController, Connection });
									}
								}
							}

							ImGui::Text(("Players Connected: " + std::to_string(AllControllers.size())).c_str());

							for (int i = 0; i < AllControllers.size(); i++)
							{
								auto& CurrentPair = AllControllers.at(i);
								auto CurrentPlayerState = CurrentPair.first->GetPlayerState();

								if (!CurrentPlayerState)
								{
									std::cout << "tf!\n";
									continue;
								}

								FString NameFStr;

								auto Connection = CurrentPair.second;
								auto RequestURL = *GetRequestURL(Connection);

								if (RequestURL.Data.Data && RequestURL.Data.ArrayNum)
								{
									auto RequestURLStr = RequestURL.ToString();

									std::size_t pos = RequestURLStr.find("Name=");

									if (pos != std::string::npos) {
										std::size_t end_pos = RequestURLStr.find('?', pos);

										if (end_pos != std::string::npos)
											RequestURLStr = RequestURLStr.substr(pos + 5, end_pos - pos - 5);
									}

									auto RequestURLCStr = RequestURLStr.c_str();

									if (ImGui::Button(RequestURLCStr))
									{
										PlayerTab = i;
									}
								}
							}
						}
					}
				}
			}
		}

		else if (Tab == EVENT_TAB)
		{
			if (ImGui::Button(std::format("Start {}", GetEventName()).c_str()))
			{
				StartEvent();
			}

			if (Fortnite_Version == 18.40)
			{
				if (ImGui::Button("Remove Storm Effect"))
				{
					auto ClientConnections = GetWorld()->GetNetDriver()->GetClientConnections();

					for (int i = 0; i < ClientConnections.Num(); i++)
					{
						auto CurrentController = (AFortPlayerControllerAthena*)ClientConnections.At(i)->GetPlayerController();

						static auto StormEffectClass = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");
						auto PlayerState = CurrentController->GetPlayerStateAthena();
						PlayerState->GetAbilitySystemComponent()->RemoveActiveGameplayEffectBySourceEffect(StormEffectClass, 1, PlayerState->GetAbilitySystemComponent());
					}
				}
			}

			if (Fortnite_Version == 8.51)
			{
				if (ImGui::Button("Unvault DrumGun"))
				{
					static auto SetUnvaultItemNameFn = FindObject<UFunction>(L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.SetUnvaultItemName");
					auto EventScripting = GetEventScripting();

					if (EventScripting)
					{
						FName Name = UKismetStringLibrary::Conv_StringToName(L"DrumGun");
						EventScripting->ProcessEvent(SetUnvaultItemNameFn, &Name);

						static auto PillarsConcludedFn = FindObject<UFunction>(L"/Game/Athena/Prototype/Blueprints/White/BP_SnowScripting.BP_SnowScripting_C.PillarsConcluded");
						EventScripting->ProcessEvent(PillarsConcludedFn, &Name);
					}
				}
			}
		}

		else if (Tab == CALENDAR_TAB)
		{
			if (Calendar::HasSnowModification())
			{
				static bool bFirst = false;

				static float FullSnowValue = Calendar::GetFullSnowMapValue();
				static float NoSnowValue = 0.0f;
				static float SnowValue = 0.0f;

				ImGui::SliderFloat(("Snow Level"), &SnowValue, 0, FullSnowValue);

				if (ImGui::Button("Set Snow Level"))
				{
					Calendar::SetSnow(SnowValue);
				}

				if (ImGui::Button("Toggle Full Snow Map"))
				{
					bFirst ? Calendar::SetSnow(NoSnowValue) : Calendar::SetSnow(FullSnowValue);

					bFirst = !bFirst;
				}
			}

			if (Calendar::HasNYE())
			{
				if (ImGui::Button("Start New Years Eve Event"))
				{
					Calendar::StartNYE();
				}
			}

			if (std::floor(Fortnite_Version) == 13)
			{
				static UObject* WL = FindObject("/Game/Athena/Apollo/Maps/Apollo_POI_Foundations.Apollo_POI_Foundations.PersistentLevel.Apollo_WaterSetup_2");

				if (WL)
				{
					static auto MaxWaterLevelOffset = WL->GetOffset("MaxWaterLevel");

					static int MaxWaterLevel = WL->Get<int>(MaxWaterLevelOffset);
					static int WaterLevel = 0;

					ImGui::SliderInt("WaterLevel", &WaterLevel, 0, MaxWaterLevel);

					if (ImGui::Button("Set Water Level"))
					{
						Calendar::SetWaterLevel(WaterLevel);
					}
				}
			}
		}

		else if (Tab == UNBAN_TAB)
		{

		}
		else if (Tab == FUN_TAB)
		{
			static std::string ItemToGrantEveryone;
			static int AmountToGrantEveryone = 1;

			ImGui::InputFloat("Starting Shield", &StartingShield);
			ImGui::InputText("Item to Give", &ItemToGrantEveryone);
			ImGui::InputInt("Amount to Give", &AmountToGrantEveryone);

			if (ImGui::Button("Destroy all player builds"))
			{
				bShouldDestroyAllPlayerBuilds = true;
			}

			if (ImGui::Button("Give Item to Everyone"))
			{
				auto ItemDefinition = FindObject<UFortItemDefinition>(ItemToGrantEveryone, nullptr, ANY_PACKAGE);

				if (ItemDefinition)
				{
					static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
					auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
					auto& ClientConnections = WorldNetDriver->GetClientConnections();

					for (int i = 0; i < ClientConnections.Num(); i++)
					{
						auto PlayerController = Cast<AFortPlayerController>(ClientConnections.at(i)->GetPlayerController());

						if (!PlayerController->IsValidLowLevel())
							continue;

						auto WorldInventory = PlayerController->GetWorldInventory();

						if (!WorldInventory->IsValidLowLevel())
							continue;

						bool bShouldUpdate = false;
						WorldInventory->AddItem(ItemDefinition, &bShouldUpdate, AmountToGrantEveryone);

						if (bShouldUpdate)
							WorldInventory->Update();
					}
				}
				else
				{
					ItemToGrantEveryone = "";
					LOG_WARN(LogUI, "Invalid Item Definition!");
				}
			}

			auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());

			if (GameState)
			{
				static auto DefaultGliderRedeployCanRedeployOffset = FindOffsetStruct("/Script/FortniteGame.FortGameStateAthena", "DefaultGliderRedeployCanRedeploy", false);
				static auto DefaultParachuteDeployTraceForGroundDistanceOffset = GameState->GetOffset("DefaultParachuteDeployTraceForGroundDistance", false);

				if (Globals::bStartedListening) // it resets accordingly to ProHenis b4 this
				{
					if (DefaultParachuteDeployTraceForGroundDistanceOffset != -1)
					{
						ImGui::InputFloat("Automatic Parachute Pullout Distance", GameState->GetPtr<float>(DefaultParachuteDeployTraceForGroundDistanceOffset));
					}
				}

				if (DefaultGliderRedeployCanRedeployOffset != -1)
				{
					bool EnableGliderRedeploy = (bool)GameState->Get<float>(DefaultGliderRedeployCanRedeployOffset);

					if (ImGui::Checkbox("Enable Glider Redeploy", &EnableGliderRedeploy))
					{
						GameState->Get<float>(DefaultGliderRedeployCanRedeployOffset) = EnableGliderRedeploy;
					}
				}

				GET_PLAYLIST(GameState);

				if (CurrentPlaylist)
				{
					bool bRespawning = CurrentPlaylist->GetRespawnType() == EAthenaRespawnType::InfiniteRespawn || CurrentPlaylist->GetRespawnType() == EAthenaRespawnType::InfiniteRespawnExceptStorm;

					if (ImGui::Checkbox("Respawning", &bRespawning))
					{
						CurrentPlaylist->GetRespawnType() = (EAthenaRespawnType)bRespawning;
					}
				}
			}
		}
		else if (Tab == DEVELOPER_TAB)
		{
			static std::string ClassNameToDump;
			static std::string FunctionNameToDump;
			static std::string ObjectToDump;
			static std::string FileNameToSaveTo;
			static bool bExcludeUnhandled = true;

			ImGui::Checkbox("Handle Death", &bHandleDeath);
			ImGui::Checkbox("Fill Vending Machines", &Globals::bFillVendingMachines);
			ImGui::Checkbox("Enable Bot Tick", &bEnableBotTick);
			ImGui::Checkbox("Enable Rebooting", &bEnableRebooting);
			ImGui::Checkbox("Enable Combine Pickup", &bEnableCombinePickup);
			ImGui::Checkbox("Exclude unhandled", &bExcludeUnhandled);
			ImGui::InputInt("Amount To Subtract Index", &AmountToSubtractIndex);
			ImGui::InputText("Class Name to mess with", &ClassNameToDump);
			ImGui::InputText("Object to dump", &ObjectToDump);
			ImGui::InputText("File to save to", &FileNameToSaveTo);

			ImGui::InputText("Function Name to mess with", &FunctionNameToDump);

			if (ImGui::Button("Print Gamephase Step"))
			{
				auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());

				if (GameState)
				{
					LOG_INFO(LogDev, "GamePhaseStep: {}", (int)GameState->GetGamePhaseStep());
				}
			}

			if (ImGui::Button("Dump Object Info"))
			{
				ObjectViewer::DumpContentsToFile(ObjectToDump, FileNameToSaveTo, bExcludeUnhandled);
			}

			if (ImGui::Button("Print all instances of class"))
			{
				auto ClassToScuff = FindObject<UClass>(ClassNameToDump);

				if (ClassToScuff)
				{
					auto ObjectNum = ChunkedObjects ? ChunkedObjects->Num() : UnchunkedObjects ? UnchunkedObjects->Num() : 0;

					for (int i = 0; i < ObjectNum; i++)
					{
						auto CurrentObject = GetObjectByIndex(i);

						if (!CurrentObject)
							continue;

						if (!CurrentObject->IsA(ClassToScuff))
							continue;

						LOG_INFO(LogDev, "Object Name: {}", CurrentObject->GetPathName());
					}
				}
			}

			if (ImGui::Button("Load BGA Class"))
			{
				static auto BlueprintGeneratedClassClass = FindObject<UClass>(L"/Script/Engine.BlueprintGeneratedClass");
				auto Class = LoadObject(ClassNameToDump, BlueprintGeneratedClassClass);

				LOG_INFO(LogDev, "New Class: {}", __int64(Class));
			}

			if (ImGui::Button("Find all classes that inherit"))
			{
				auto ClassToScuff = FindObject<UClass>(ClassNameToDump);

				if (ClassToScuff)
				{
					auto ObjectNum = ChunkedObjects ? ChunkedObjects->Num() : UnchunkedObjects ? UnchunkedObjects->Num() : 0;

					for (int i = 0; i < ObjectNum; i++)
					{
						auto CurrentObject = GetObjectByIndex(i);

						if (!CurrentObject || CurrentObject == ClassToScuff)
							continue;

						if (!CurrentObject->IsA(ClassToScuff))
							continue;

						LOG_INFO(LogDev, "Class Name: {}", CurrentObject->GetPathName());
					}
				}
			}

			if (ImGui::Button("Print Class VFT"))
			{
				auto Class = FindObject<UClass>(ClassNameToDump);

				if (Class)
				{
					auto ClassToDump = Class->CreateDefaultObject();

					if (ClassToDump)
					{
						LOG_INFO(LogDev, "{} VFT: 0x{:x}", ClassToDump->GetName(), __int64(ClassToDump->VFTable) - __int64(GetModuleHandleW(0)));
					}
				}
			}

			if (ImGui::Button("Print Function Exec Addr"))
			{
				auto Function = FindObject<UFunction>(FunctionNameToDump);

				if (Function)
				{
					LOG_INFO(LogDev, "{} Exec: 0x{:x}", Function->GetName(), __int64(Function->GetFunc()) - __int64(GetModuleHandleW(0)));
				}
			}

			/* if (ImGui::Button("Load BGA Class (and spawn so no GC)"))
			{
				static auto BGAClass = FindObject<UClass>("/Script/Engine.BlueprintGeneratedClass");
				auto Class = LoadObject<UClass>(ClassNameToDump, BGAClass);

				if (Class)
				{
					GetWorld()->SpawnActor<AActor>(Class, FVector());
				}
			} */

			/*
			ImGui::Text(std::format("Amount of hooks {}", AllFunctionHooks.size()).c_str());

			for (auto& FunctionHook : AllFunctionHooks)
			{
				if (ImGui::Button(std::format("{} {} (0x{:x})", (FunctionHook.IsHooked ? "Unhook" : "Hook"), FunctionHook.Name, (__int64(FunctionHook.Original) - __int64(GetModuleHandleW(0)))).c_str()))
				{
					if (FunctionHook.IsHooked)
					{
						if (!FunctionHook.VFT || FunctionHook.Index == -1)
						{
							Hooking::MinHook::Unhook(FunctionHook.Original);
						}
						else
						{
							VirtualSwap(FunctionHook.VFT, FunctionHook.Index, FunctionHook.Original);
						}
					}
					else
					{
						Hooking::MinHook::Hook(FunctionHook.Original, FunctionHook.Detour, nullptr, FunctionHook.Name);
					}

					FunctionHook.IsHooked = !FunctionHook.IsHooked;
				}
			}
			*/
		}
		else if (Tab == SETTINGS_TAB)
		{
			// ImGui::Checkbox("Use custom lootpool (from Win64/lootpool.txt)", &Defines::bCustomLootpool);
		}
	}
	else if (PlayerTab != 2435892 && bLoaded)
	{
		auto World = GetWorld();
		auto NetDriver = World->GetNetDriver();

		if (NetDriver)
		{
			static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
			auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
			auto& ClientConnections = WorldNetDriver->GetClientConnections();

			for (int i = 0; i < ClientConnections.Num(); i++) {
				auto Connection = ClientConnections.At(i);

				if (!Connection)
					continue;

				auto CurrentController = Cast<AFortPlayerControllerAthena>(ClientConnections.at(i)->GetPlayerController());

				if (CurrentController) {
					auto it = std::find_if(AllControllers.begin(), AllControllers.end(), [CurrentController, Connection](const auto& pair)
						{
							return pair.first == CurrentController && pair.second == Connection;
						});

					if (it == AllControllers.end()) {
						AllControllers.push_back({ CurrentController, Connection });
					}
				}
			}
		}

		if (PlayerTab < AllControllers.size())
		{
			PlayerTabs();

			auto& CurrentPair = AllControllers.at(PlayerTab);
			auto CurrentController = CurrentPair.first;
			auto CurrentPawn = CurrentController->GetMyFortPawn();
			auto CurrentPlayerState = CurrentController->GetPlayerState();

			if (CurrentPlayerState)
			{
				FString NameFStr;

				auto Connection = CurrentPair.second;
				auto RequestURL = *GetRequestURL(Connection); // If it works don't fix it

				if (RequestURL.Data.Data)
				{
					auto RequestURLStr = RequestURL.ToString();

					std::size_t pos = RequestURLStr.find("Name=");

					if (pos != std::string::npos)
					{
						std::size_t end_pos = RequestURLStr.find('?', pos);

						if (end_pos != std::string::npos)
							RequestURLStr = RequestURLStr.substr(pos + 5, end_pos - pos - 5);
					}

					auto RequestURLCStr = RequestURLStr.c_str();
					ImGui::Text(("Viewing " + RequestURLStr).c_str());

					if (playerTabTab == MAIN_PLAYERTAB)
					{
						static std::string KickReason = "You have been kicked!";

						ImGui::InputText("Kick Reason", &KickReason);

						if (ImGui::Button("Kick"))
						{
							std::wstring wstr = std::wstring(KickReason.begin(), KickReason.end());
							FString Reason;
							Reason.Set(wstr.c_str());

							static auto ClientReturnToMainMenu = FindObject<UFunction>("/Script/Engine.PlayerController.ClientReturnToMainMenu");
							CurrentController->ProcessEvent(ClientReturnToMainMenu, &Reason);
						}

						if (ImGui::Button("DBNO"))
						{
							CurrentPawn->SetDBNO(true);
						}

						if (ImGui::Button("Troll"))
						{
							CurrentPawn->SetHealth(500);
							CurrentPawn->SetMaxHealth(500);
						}

						if (ImGui::Button("Rape EXPERIMENTAL"))
						{
							auto Cast = (AFortPlayerStateAthena*)CurrentPlayerState;

							auto NetWork = CurrentPlayerState->GetSavedNetworkAddress();
							LOG_INFO(LogDev, "NetWork Ip: {}", NetWork.ToString());
							auto GameMode = (AFortGameModeAthena*)GetWorld()->GetGameMode();
							GameMode->ChangeName(CurrentController, NetWork, true);
						}
					}
				}
			}

		}
		ImGui::NewLine();

		if (ImGui::Button("Back"))
		{
			PlayerTab = -1;
		}
	}
}

static inline void PregameUI()
{
	StaticUI();

	if (Engine_Version >= 422/* && Engine_Version < 424*/)
	{
		ImGui::Checkbox("Creative", &Globals::bCreative);
	}

	if (Addresses::SetZoneToIndex)
	{
		bool bWillBeLategame = Globals::bLateGame.load();
		ImGui::Checkbox("Lategame", &bWillBeLategame);
		SetIsLategame(bWillBeLategame);
	}

	if (HasEvent())
	{
		ImGui::Checkbox("Play Event", &Globals::bGoingToPlayEvent);
	}

	if (!bSwitchedInitialLevel)
	{
		// ImGui::Checkbox("Use Custom Map", &bUseCustomMap);

		if (bUseCustomMap)
		{
			// ImGui::InputText("Custom Map", &CustomMapName);
		}

		ImGui::SliderInt("Seconds until load into map", &SecondsUntilTravel, 1, 100);
	}

	if (!Globals::bCreative)
		ImGui::InputText("Playlist", &PlaylistName);
}

static inline HICON LoadIconFromMemory(const char* bytes, int bytes_size, const wchar_t* IconName) {
	HANDLE hMemory = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bytes_size, IconName);
	if (hMemory == NULL) {
		return NULL;
	}

	LPVOID lpBuffer = MapViewOfFile(hMemory, FILE_MAP_READ, 0, 0, bytes_size);

	if (lpBuffer == NULL) {
		CloseHandle(hMemory);
		return NULL;
	}

	ICONINFO icon_info;

	if (!GetIconInfo((HICON)lpBuffer, &icon_info)) {
		UnmapViewOfFile(lpBuffer);
		CloseHandle(hMemory);
		return NULL;
	}

	HICON hIcon = CreateIconIndirect(&icon_info);
	UnmapViewOfFile(lpBuffer);
	CloseHandle(hMemory);
	return hIcon;
}

static inline DWORD WINAPI GuiThread(LPVOID)
{
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"RebootClass", NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindowExW(0L, wc.lpszClassName, L"12.41", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX), 100, 100, Width, Height, NULL, NULL, wc.hInstance, NULL);

	if (false) // idk why this dont work
	{
		auto hIcon = LoadIconFromMemory((const char*)reboot_icon_data, strlen((const char*)reboot_icon_data), L"RebootIco");
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}

	// SetWindowLongPtrW(hwnd, GWL_STYLE, WS_POPUP); // Disables windows title bar at the cost of dragging and some quality

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.IniFilename = NULL; // Disable imgui.ini generation.
	io.DisplaySize = ImGui::GetMainViewport()->Size;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	InitFont();
	InitStyle();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImFontConfig config;
	config.MergeMode = true;
	config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
	// static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	// io.Fonts->AddFontFromFileTTF("Reboot Resources/fonts/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges);

	bool done = false;

	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				// done = true;
				break;
			}
		}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		auto WindowSize = ImGui::GetMainViewport()->Size;
		// ImGui::SetNextWindowPos(ImVec2(WindowSize.x * 0.5f, WindowSize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f)); // Center
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);

		tagRECT rect;

		if (GetWindowRect(hwnd, &rect))
		{
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
		}

		if (!ImGui::IsWindowCollapsed())
		{
			ImGui::Begin("12.41", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

			Globals::bInitializedPlaylist ? MainUI() : PregameUI();

			ImGui::End();
		}

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}

		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

		// Handle loss of D3D9 device
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Helper functions

static inline bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

static inline void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

static inline void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static inline LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// my implementation of window dragging..
	/* {
		static int dababy = 0;
		if (dababy > 100) // wait until gui is initialized ig?
		{
			if (ImGui::IsMouseDragging(ImGuiMouseButton(0)))
			{
				// if (LOWORD(lParam) > 255 && HIWORD(lParam) > 255)
				{
					POINT p;
					GetCursorPos(&p);

					SetWindowPos(hWnd, nullptr, p.x, p.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
				}
			}
		}
		dababy++;
	} */

	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
