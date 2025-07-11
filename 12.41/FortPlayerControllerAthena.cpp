#include "FortPlayerControllerAthena.h"
#include "FortPlayerPawn.h"
#include "FortKismetLibrary.h"

#include "SoftObjectPtr.h"
#include "globals.h"
#include "GameplayStatics.h"
#include "hooking.h"
#include "FortAthenaMutator_GiveItemsAtGamePhaseStep.h"
#include "DataTableFunctionLibrary.h"
#include "AthenaResurrectionComponent.h"
#include "FortAthenaMutator_InventoryOverride.h"
#include "FortGadgetItemDefinition.h"
#include "gui.h"
#include "FortAthenaMutator_GG.h"
#include "FortAthenaMutator_DropZone.h"

void AFortPlayerControllerAthena::GiveXP(AFortPlayerControllerAthena* PC, int CombatXP, int SurvivalXP, int BonusMedalXP, int ChallengeXP, int MatchXP)
{
	UFortPlayerControllerAthenaXPComponent* XPComponent = PC->GetXPComponent();

	if (XPComponent)
	{
		XPComponent->GetCombatXp() += CombatXP;
		XPComponent->GetSurvivalXp() += SurvivalXP;
		XPComponent->GetMedalBonusXP() += BonusMedalXP;
		XPComponent->GetChallengeXp() += ChallengeXP;
		XPComponent->GetMatchXp() += MatchXP;
		XPComponent->GetTotalXpEarned() += (CombatXP + SurvivalXP + BonusMedalXP + ChallengeXP + MatchXP);
		XPComponent->GetCachedLevelInfo().GetLevelXp() += XPComponent->GetTotalXpEarned();
		XPComponent->GetCachedLevelInfo().GetBookLevelXp() += XPComponent->GetTotalXpEarned();
		XPComponent->OnXpUpdated(CombatXP, SurvivalXP, BonusMedalXP, ChallengeXP, MatchXP, XPComponent->GetTotalXpEarned());
	}
}

void AFortPlayerControllerAthena::ProgressQuest(AFortPlayerControllerAthena* PC, UFortQuestItemDefinition* QuestDef, FName BackendName)
{
	if (Fortnite_Version < 16)
	{
		PC->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, nullptr, true, false);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->GetPlayerState();
		for (size_t i = 0; i < PlayerState->GetPlayerTeam()->GetTeamMembers().Num(); i++)
		{
			auto pc = (AFortPlayerControllerAthena*)PlayerState->GetPlayerTeam()->GetTeamMembers()[i];
			if (pc && pc != PC)
			{
				pc->GetQuestManager(ESubGame::Athena)->SelfCompletedUpdatedQuest(PC, QuestDef, BackendName, 1, 1, PlayerState, true, false);
			}
		}
	}

	auto QuestItem = PC->GetQuestManager(ESubGame::Athena)->GetQuestWithDefinition(QuestDef);

	FXPEventEntry XPEventEntry{};
	FXPEventInfo XPEventInfo{};

	XPEventEntry.EventXpValue = 5000;//still skunked i will make it so it gets real xp value from datatable
	XPEventInfo.EventXpValue = 5000;
	XPEventEntry.QuestDef = QuestDef;
	XPEventInfo.QuestDef = QuestDef;
	XPEventEntry.Time = UGameplayStatics::GetTimeSeconds(GetWorld());
	//XPEventInfo.PlayerController = PC;
	XPEventInfo.Priority = EXPEventPriorityType::EXPEventPriorityType__NearReticle;
	XPEventInfo.SimulatedText = UKismetTextLibrary::Conv_StringToText(L"Challenge Completed!");
	//XPEventInfo.SimulatedXpSize = EFortSimulatedXPSize::EFortSimulatedXPSize__VeryLarge;
	PC->GetXPComponent()->GetChallengeXp() += XPEventEntry.EventXpValue;
	PC->GetXPComponent()->GetTotalXpEarned() += XPEventEntry.EventXpValue;
	XPEventEntry.TotalXpEarnedInMatch = PC->GetXPComponent()->GetTotalXpEarned();
	XPEventEntry.SimulatedXpEvent = UKismetTextLibrary::Conv_StringToText(L"Challenge Completed!");
	PC->GetXPComponent()->GetRestXP() += XPEventEntry.EventXpValue;
	PC->GetXPComponent()->GetInMatchProfileVer()++;
	PC->GetXPComponent()->OnInMatchProfileUpdate(PC->GetXPComponent()->GetInMatchProfileVer());
	PC->GetXPComponent()->OnProfileUpdated();
	PC->GetXPComponent()->OnXpUpdated(PC->GetXPComponent()->GetCombatXp(), PC->GetXPComponent()->GetSurvivalXp(), PC->GetXPComponent()->GetMedalBonusXP(), PC->GetXPComponent()->GetChallengeXp(), PC->GetXPComponent()->GetMatchXp(), PC->GetXPComponent()->GetTotalXpEarned());
	PC->GetXPComponent()->GetWaitingQuestXp().Add(XPEventEntry);

	if (Fortnite_Version < 19)
		PC->GetXPComponent()->HighPrioXPEvent(XPEventEntry);
	else
		PC->GetXPComponent()->OnXpEvent1(XPEventInfo);
}

void AFortPlayerControllerAthena::GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* Accolade)
{
	if (!PC || !Accolade)
		return;

	FXPEventInfo Info{};

	Info.Accolade = UKismetSystemLibrary::GetPrimaryAssetIdFromObject(Accolade);
	float XpValue = Accolade->GetAccoladeXpValue();

		XpValue = UDataTableFunctionLibrary::EvaluateCurveTableRow(Accolade->GetXpRewardAmount().GetCurve().CurveTable, Accolade->GetXpRewardAmount().GetCurve().RowName, Accolade->GetXpRewardAmount().GetValue());
	if (XpValue == 0)
	{
		//XpValue = UDataTableFunctionLibrary::EvaluateCurveTableRow(Accolade->GetXpRewardAmount().GetCurve().CurveTable, UKismetStringLibrary::Conv_StringToName(TEXT("Default_Medal")), Accolade->GetXpRewardAmount().GetValue());
	}

	if (XpValue == 0)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dist(1, 150);
		int randomNumber = dist(gen);
		XpValue = Accolade->GetAccoladeXpValue() + randomNumber;
	}

	Info.EventXpValue = int32(XpValue);
	Info.RestedValuePortion = Info.EventXpValue;
	Info.RestedXPRemaining = Info.EventXpValue;
	Info.TotalXpEarnedInMatch = Info.EventXpValue + PC->GetXPComponent()->GetTotalXpEarned();
	Info.Priority = Accolade->GetPriority();
	Info.SimulatedText = UKismetTextLibrary::Conv_StringToText(L"Test");
	Info.EventName = Accolade->GetFName();
	Info.SeasonBoostValuePortion = 0;

	PC->GetXPComponent()->GetMatchXp() += Info.EventXpValue;
	PC->GetXPComponent()->GetTotalXpEarned() += Info.EventXpValue;

	PC->GetXPComponent()->OnXPEvent2(Info);
}

void AFortPlayerControllerAthena::StartGhostModeHook(UObject* Context, FFrame* Stack, void* Ret)
{
	LOG_INFO(LogDev, __FUNCTION__);

	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	UFortWorldItemDefinition* ItemProvidingGhostMode = nullptr;

	Stack->StepCompiledIn(&ItemProvidingGhostMode);

	if (!ItemProvidingGhostMode)
	{
		LOG_INFO(LogDev, "Null item!");
		return StartGhostModeOriginal(Context, Stack, Ret);
	}

	if (!PlayerController->HasAuthority()) // for real
		return StartGhostModeOriginal(Context, Stack, Ret);
	
	LOG_INFO(LogDev, "Attempting to give item {}", ItemProvidingGhostMode->IsValidLowLevel() ? ItemProvidingGhostMode->GetFullName() : "BadRead");

	auto GhostModeRepData = PlayerController->GetGhostModeRepData();

	if (GhostModeRepData->IsInGhostMode())
	{
		LOG_INFO(LogDev, "Player is already in ghost mode!");
		return StartGhostModeOriginal(Context, Stack, Ret);
	}

	auto WorldInventory = PlayerController->GetWorldInventory();
	
	if (!WorldInventory)
		return StartGhostModeOriginal(Context, Stack, Ret);

	auto PickaxeInstance = WorldInventory->GetPickaxeInstance();

	// LOG_INFO(LogDev, "PickaxeInstance: {}", __int64(PickaxeInstance));

	if (PickaxeInstance)
	{
		WorldInventory->RemoveItem(PickaxeInstance->GetItemEntry()->GetItemGuid(), nullptr, REMOVE_ALL_ITEMS, true);
		// WorldInventory->Update();
	}

	bool bShouldUpdate = false;
	auto NewAndModifiedInstances = WorldInventory->AddItem(ItemProvidingGhostMode, &bShouldUpdate, 1);
	auto GhostModeItemInstance = NewAndModifiedInstances.first[0];

	if (!GhostModeItemInstance)
		return StartGhostModeOriginal(Context, Stack, Ret);

	// if (bShouldUpdate)
	WorldInventory->Update();

	PlayerController->AddPickaxeToInventory();
	WorldInventory->Update();

	PlayerController->ServerExecuteInventoryItemHook(PlayerController, GhostModeItemInstance->GetItemEntry()->GetItemGuid());
	PlayerController->ClientEquipItem(GhostModeItemInstance->GetItemEntry()->GetItemGuid(), true);
	LOG_INFO(LogDev, "Finished!");

	return StartGhostModeOriginal(Context, Stack, Ret);
}

void AFortPlayerControllerAthena::EndGhostModeHook(AFortPlayerControllerAthena* PlayerController)
{
	// I believe there are a lot of other places we should remove it (go to XREFs of K2_RemoveItemFromPlayer on a version like 6.21, and there will be something checking ghost stuff).

	LOG_INFO(LogDev, __FUNCTION__);

	if (!PlayerController->HasAuthority()) // for real
		return EndGhostModeOriginal(PlayerController);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return EndGhostModeOriginal(PlayerController);

	FGhostModeRepData* GhostModeRepData = PlayerController->GetGhostModeRepData();
	UFortWorldItemDefinition* GhostModeItemDef = GhostModeRepData->GetGhostModeItemDef();

	LOG_INFO(LogDev, "GhostModeItemDef: {}", GhostModeItemDef->IsValidLowLevel() ? GhostModeItemDef->GetFullName() : "BadRead");

	if (!GhostModeItemDef) // bro IDFK
	{
		GhostModeItemDef = FindObject<UFortWorldItemDefinition>("/Game/Athena/Items/Gameplay/SpookyMist/AGID_SpookyMist.AGID_SpookyMist");
	}

	if (!GhostModeItemDef)
		return EndGhostModeOriginal(PlayerController);

	auto GhostModeItemInstance = WorldInventory->FindItemInstance(GhostModeItemDef);

	LOG_INFO(LogDev, "GhostModeItemInstance: {}", GhostModeItemInstance->IsValidLowLevel() ? GhostModeItemInstance->GetFullName() : "BadRead");

	if (!GhostModeItemInstance)
		return EndGhostModeOriginal(PlayerController);

	auto PickaxeInstance = PlayerController->AddPickaxeToInventory();

	WorldInventory->ForceNetUpdate();
	PlayerController->ForceNetUpdate();

	bool bShouldUpdate = false;
	int Count = GhostModeItemInstance->GetItemEntry()->GetCount(); // 1
	bool bForceRemoval = true; // false
	WorldInventory->RemoveItem(GhostModeItemInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, Count, bForceRemoval);

	// if (bShouldUpdate)
	WorldInventory->Update();

	if (PickaxeInstance)
	{
		PlayerController->ClientEquipItem(PickaxeInstance->GetItemEntry()->GetItemGuid(), true);
	}

	return EndGhostModeOriginal(PlayerController);
}

void AFortPlayerControllerAthena::EnterAircraftHook(UObject* PC, AActor* Aircraft)
{
	auto PlayerController = Cast<AFortPlayerController>(Engine_Version < 424 ? PC : ((UActorComponent*)PC)->GetOwner());

	if (!PlayerController)
		return;

	LOG_INFO(LogDev, "EnterAircraftHook");

	EnterAircraftOriginal(PC, Aircraft);

	// TODO Check if the player successfully got in the aircraft.

	auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
	auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	std::vector<std::pair<FGuid, int>> GuidAndCountsToRemove;

	auto& InventoryList = WorldInventory->GetItemList();

	auto& ItemInstances = InventoryList.GetItemInstances();

	for (int i = 0; i < ItemInstances.Num(); ++i)
	{
		auto ItemEntry = ItemInstances.at(i)->GetItemEntry();
		auto ItemDefinition = Cast<UFortWorldItemDefinition>(ItemEntry->GetItemDefinition());

		if (!ItemDefinition)
			continue;

		if (!ItemDefinition->CanBeDropped())
			continue;

		GuidAndCountsToRemove.push_back({ ItemEntry->GetItemGuid(), ItemEntry->GetCount() });
	}

	for (auto& Pair : GuidAndCountsToRemove)
	{
		WorldInventory->RemoveItem(Pair.first, nullptr, Pair.second, true);
	}

	std::vector<std::pair<AFortAthenaMutator*, UFunction*>> FunctionsToCall;
	LoopMutators([&](AFortAthenaMutator* Mutator) { FunctionsToCall.push_back(std::make_pair(Mutator, Mutator->FindFunction("OnGamePhaseStepChanged"))); });

	auto HandleGiveItemsAtGamePhaseStepMutator = [&](AFortAthenaMutator* Mutator) {
		if (auto GiveItemsAtGamePhaseStepMutator = Cast<AFortAthenaMutator_GiveItemsAtGamePhaseStep>(Mutator))
		{
			auto PhaseToGive = GiveItemsAtGamePhaseStepMutator->GetPhaseToGiveItems();
			auto& ItemsToGive = GiveItemsAtGamePhaseStepMutator->GetItemsToGive();

			LOG_INFO(LogDev, "PhaseToGiveItems: {} ItemsToGive.Num(): {}", (int)PhaseToGive, ItemsToGive.Num());

			if (PhaseToGive <= 5) // Flying or lower
			{
				for (int j = 0; j < ItemsToGive.Num(); j++)
				{
					auto ItemToGive = ItemsToGive.AtPtr(j, FItemsToGive::GetStructSize());

					if (!ItemToGive->GetItemToDrop())
						continue;

					float Out2 = 0;

					if (!IsBadReadPtr(ItemToGive->GetNumberToGive().GetCurve().CurveTable, 8) && ItemToGive->GetNumberToGive().GetCurve().RowName.IsValid())
					{
						Out2 = UDataTableFunctionLibrary::EvaluateCurveTableRow(ItemToGive->GetNumberToGive().GetCurve().CurveTable, ItemToGive->GetNumberToGive().GetCurve().RowName, 0.f);
					}

					LOG_INFO(LogDev, "[{}] Out2: {} ItemToGive.ItemToDrop: {}", j, Out2, ItemToGive->GetItemToDrop()->IsValidLowLevel() ? ItemToGive->GetItemToDrop()->GetFullName() : "BadRead");

					if (!Out2) // ?
						Out2 = 0;

					WorldInventory->AddItem(ItemToGive->GetItemToDrop(), nullptr, Out2);
				}
			}
		}
		};

	LoopMutators(HandleGiveItemsAtGamePhaseStepMutator);

	/* if (auto GGMutator = Cast<AFortAthenaMutator_GG>(Mutator))
	{
		auto& WeaponEntries = GGMutator->GetWeaponEntries();

		LOG_INFO(LogDev, "[{}] WeaponEntries.Num(): {}", i, WeaponEntries.Num());

		for (int j = 0; j < WeaponEntries.Num(); j++)
		{
			WorldInventory->AddItem(WeaponEntries.at(j).Weapon, nullptr, 1);
		}
	} */

	auto PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState());

	auto AddInventoryOverrideTeamLoadouts = [&](AFortAthenaMutator* Mutator)
		{
			if (auto InventoryOverride = Cast<AFortAthenaMutator_InventoryOverride>(Mutator))
			{
				auto TeamIndex = PlayerStateAthena->GetTeamIndex();
				auto LoadoutTeam = InventoryOverride->GetLoadoutTeamForTeamIndex(TeamIndex);

				if (LoadoutTeam.UpdateOverrideType == EAthenaInventorySpawnOverride::AircraftPhaseOnly)
				{
					auto LoadoutContainer = InventoryOverride->GetLoadoutContainerForTeamIndex(TeamIndex);

					for (int i = 0; i < LoadoutContainer.Loadout.Num(); ++i)
					{
						auto& ItemAndCount = LoadoutContainer.Loadout.at(i);
						WorldInventory->AddItem(ItemAndCount.GetItem(), nullptr, ItemAndCount.GetCount());
					}
				}
			}
		};

	LoopMutators(AddInventoryOverrideTeamLoadouts);
	{
		/* auto PickaxeInstance = WorldInventory->GetPickaxeInstance();

		if (!PickaxeInstance)
			return;

		AFortPlayerController::ServerExecuteInventoryItemHook(PlayerController, PickaxeInstance->GetItemEntry()->GetItemGuid()); */
	}
}

void AFortPlayerControllerAthena::ServerRequestSeatChangeHook(AFortPlayerControllerAthena* PlayerController, int TargetSeatIndex)
{
	auto Pawn = Cast<AFortPlayerPawn>(PlayerController->GetPawn());
	
	if (!Pawn)
		return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);

	auto Vehicle = Pawn->GetVehicle();

	if (!Vehicle)
		return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);

	auto OldVehicleWeaponDefinition = Pawn->GetVehicleWeaponDefinition(Vehicle);
	
	LOG_INFO(LogDev, "OldVehicleWeaponDefinition: {}", OldVehicleWeaponDefinition ? OldVehicleWeaponDefinition->GetFullName() : "BadRead");

	if (!OldVehicleWeaponDefinition)
		return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);

	auto OldVehicleWeaponInstance = WorldInventory->FindItemInstance(OldVehicleWeaponDefinition);

	if (OldVehicleWeaponInstance)
	{
		bool bShouldUpdate = false;
		WorldInventory->RemoveItem(OldVehicleWeaponInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, OldVehicleWeaponInstance->GetItemEntry()->GetCount(), true);

		if (bShouldUpdate)
			WorldInventory->Update();
	}

	auto RequestingVehicleWeaponDefinition = Vehicle->GetVehicleWeaponForSeat(TargetSeatIndex);

	if (!RequestingVehicleWeaponDefinition)
	{
		auto PickaxeInstance = WorldInventory->GetPickaxeInstance();

		if (!PickaxeInstance)
			return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);

		AFortPlayerController::ServerExecuteInventoryItemHook(PlayerController, PickaxeInstance->GetItemEntry()->GetItemGuid()); // Bad, we should equip the last weapon.
		return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);
	}

	LOG_INFO(LogVehicles, "Equipping RequestingVehicleWeaponDefinition: {}", RequestingVehicleWeaponDefinition->GetFullName());

	auto NewAndModifiedInstances = WorldInventory->AddItem(RequestingVehicleWeaponDefinition, nullptr);
	auto RequestedVehicleInstance = NewAndModifiedInstances.first[0];

	if (!RequestedVehicleInstance)
		return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);

	auto& ReplicatedEntries = WorldInventory->GetItemList().GetReplicatedEntries();

	for (int i = 0; i < ReplicatedEntries.Num(); i++)
	{
		auto ReplicatedEntry = ReplicatedEntries.AtPtr(i, FFortItemEntry::GetStructSize());

		if (ReplicatedEntry->GetItemGuid() == RequestedVehicleInstance->GetItemEntry()->GetItemGuid())
		{
			WorldInventory->GetItemList().MarkItemDirty(ReplicatedEntry);
			WorldInventory->GetItemList().MarkItemDirty(RequestedVehicleInstance->GetItemEntry());
			WorldInventory->HandleInventoryLocalUpdate();

			Pawn->EquipWeaponDefinition(RequestingVehicleWeaponDefinition, ReplicatedEntry->GetItemGuid());
		}
	}

	return ServerRequestSeatChangeOriginal(PlayerController, TargetSeatIndex);
}

void AFortPlayerControllerAthena::ServerRestartPlayerHook(AFortPlayerControllerAthena* Controller)
{
	static auto FortPlayerControllerZoneDefault = FindObject<UClass>(L"/Script/FortniteGame.Default__FortPlayerControllerZone");
	static auto ServerRestartPlayerFn = FindObject<UFunction>(L"/Script/Engine.PlayerController.ServerRestartPlayer");
	static auto ZoneServerRestartPlayer = __int64(FortPlayerControllerZoneDefault->VFTable[GetFunctionIdxOrPtr(ServerRestartPlayerFn) / 8]);
	static void (*ZoneServerRestartPlayerOriginal)(AFortPlayerController*) = decltype(ZoneServerRestartPlayerOriginal)(__int64(ZoneServerRestartPlayer));
	
	LOG_INFO(LogDev, "ServerRestartPlayerHook Call 0x{:x} returning with 0x{:x}!", ZoneServerRestartPlayer - __int64(_ReturnAddress()), __int64(ZoneServerRestartPlayerOriginal) - __int64(GetModuleHandleW(0)));
	return ZoneServerRestartPlayerOriginal(Controller);
}

void AFortPlayerControllerAthena::ServerGiveCreativeItemHook(AFortPlayerControllerAthena* Controller, FFortItemEntry CreativeItem)
{
	// Don't worry, the validate has a check if it is a creative enabled mode or not, but we need to add a volume check and permission check I think.

	LOG_INFO(LogDev, "ServerGiveCreativeItemHook!");

	auto CreativeItemPtr = &CreativeItem;
	auto ItemDefinition = CreativeItemPtr->GetItemDefinition();

	if (!ItemDefinition)
		return;

	bool bShouldUpdate = false;
	auto LoadedAmmo = -1; // CreativeItemPtr->GetLoadedAmmo()
	Controller->GetWorldInventory()->AddItem(ItemDefinition, &bShouldUpdate, CreativeItemPtr->GetCount(), LoadedAmmo, false);

	if (bShouldUpdate)
		Controller->GetWorldInventory()->Update(Controller);
}

void AFortPlayerControllerAthena::ServerTeleportToPlaygroundLobbyIslandHook(AFortPlayerControllerAthena* Controller)
{
	LOG_INFO(LogDev, "ServerTeleportToPlaygroundLobbyIslandHook!");

	auto Pawn = Controller->GetMyFortPawn();

	if (!Pawn)
		return;

	auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());

	if (!GameState)
		return;

	if (GameState->IsTeleportToCreativeHubAllowed())
	{
		LOG_INFO(LogDev, "GameState->IsTeleportToCreativeHubAllowed()!");

		static auto FortPlayerStartCreativeClass = FindObject<UClass>(L"/Script/FortniteGame.FortPlayerStartCreative");
		auto AllCreativePlayerStarts = UGameplayStatics::GetAllActorsOfClass(GetWorld(), FortPlayerStartCreativeClass);

		for (int i = 0; i < AllCreativePlayerStarts.Num(); ++i)
		{
			auto CurrentPlayerStart = AllCreativePlayerStarts.at(i);

			static auto PlayerStartTagsOffset = CurrentPlayerStart->GetOffset("PlayerStartTags");
			auto bHasSpawnTag = CurrentPlayerStart->Get<FGameplayTagContainer>(PlayerStartTagsOffset).Contains("Playground.LobbyIsland.Spawn");

			if (!bHasSpawnTag)
				continue;

			auto PlayerController = Cast<AFortPlayerControllerAthena>(Pawn->GetController());

			static auto FortEditToolItemDefinitionClass = FindObject<UClass>(L"/Script/FortniteGame.FortEditToolItemDefinition");
			static auto FortBuildingItemDefinitionClass = FindObject<UClass>(L"/Script/FortniteGame.FortBuildingItemDefinition");
			const auto& ItemInstances = PlayerController->GetWorldInventory()->GetItemList().GetItemInstances();
			auto PickaxeInstance = PlayerController->GetWorldInventory()->GetPickaxeInstance();
			std::vector<std::pair<FGuid, int>> GuidsAndCountsToRemove;

			if (PlayerController->GetWorldInventory())
			{
				for (int i = 0; i < ItemInstances.Num(); ++i)
				{
					auto ItemInstance = ItemInstances.at(i);
					const auto ItemDefinition = Cast<UFortWorldItemDefinition>(ItemInstance->GetItemEntry()->GetItemDefinition());

					if (ItemDefinition->CanBeDropped() ||
						(!ItemDefinition->IsA(FortBuildingItemDefinitionClass) &&
							!ItemDefinition->IsA(FortEditToolItemDefinitionClass) &&
							ItemInstance != PickaxeInstance))
					{
						GuidsAndCountsToRemove.push_back({ ItemInstance->GetItemEntry()->GetItemGuid(), ItemInstance->GetItemEntry()->GetCount() });
					}
				}

				for (auto& [Guid, Count] : GuidsAndCountsToRemove)
				{
					PlayerController->GetWorldInventory()->RemoveItem(Guid, nullptr, Count, true);
				}

				PlayerController->GetWorldInventory()->Update();
			}

			Pawn->TeleportTo(CurrentPlayerStart->GetActorLocation(), Pawn->GetActorRotation());
			break;
		}

		AllCreativePlayerStarts.Free();

		LOG_INFO(LogCreative, "Teleported and freed!");
	}
}

void AFortPlayerControllerAthena::ServerAcknowledgePossessionHook(APlayerController* Controller, APawn* Pawn)
{
	static auto AcknowledgedPawnOffset = Controller->GetOffset("AcknowledgedPawn");

	const APawn* OldAcknowledgedPawn = Controller->Get<APawn*>(AcknowledgedPawnOffset);
	Controller->Get<APawn*>(AcknowledgedPawnOffset) = Pawn;

	auto ControllerAsFort = Cast<AFortPlayerController>(Controller);
	auto PawnAsFort = Cast<AFortPlayerPawn>(Pawn);
	auto PlayerStateAsFort = Cast<AFortPlayerState>(Pawn->GetPlayerState());

	if (!PawnAsFort)
		return;

	if (OldAcknowledgedPawn != PawnAsFort)
	{
		if (Globals::bStartedBus == true)
		{
			PawnAsFort->SetShield(StartingShield);
		}
	}

	if (Globals::bNoMCP)
	{
		static auto CustomCharacterPartClass = FindObject<UClass>("/Script/FortniteGame.CustomCharacterPart");
		static auto backpackPart = LoadObject("/Game/Characters/CharacterParts/Backpacks/NoBackpack.NoBackpack", CustomCharacterPartClass);

		// PawnAsFort->ServerChoosePart(EFortCustomPartType::Backpack, backpackPart);

		return;
	}

	ControllerAsFort->ApplyCosmeticLoadout();
}

void AFortPlayerControllerAthena::ServerPlaySquadQuickChatMessageHook(AFortPlayerControllerAthena* PlayerController, __int64 ChatEntry, __int64 SenderID)
{
	using UAthenaEmojiItemDefinition = UFortItemDefinition;

	auto PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState());

	if (!PlayerStateAthena)
		return;

	static auto IndexOffset = FindOffsetStruct("/Script/FortniteGame.AthenaQuickChatActiveEntry", "Index");
	auto Index = *(int8*)(__int64(ChatEntry) + IndexOffset);

	LOG_INFO(LogDev, "Index: {}", (int)Index);

	uint8 NewTeamMemberState = 0;

	switch (Index)
	{
	case 0:
		NewTeamMemberState = 8;
		break;
	case 1:
		NewTeamMemberState = 9;
		break;
	case 2:
		NewTeamMemberState = 11;
		break;
	case 3:
		NewTeamMemberState = 10;
		break;
	case 4:
		NewTeamMemberState = 12;
		break;
	case 5:
		NewTeamMemberState = 3;
		break;
	case 6:
		NewTeamMemberState = 4;
		break;
	case 7:
		NewTeamMemberState = 2;
		break;
	case 8:
		NewTeamMemberState = 5;
		break;
	case 9:
		NewTeamMemberState = 6;
		break;
	default:
		break;
	}

	NewTeamMemberState -= AmountToSubtractIndex;

	PlayerStateAthena->Get<uint8_t>("ReplicatedTeamMemberState") = NewTeamMemberState;
	PlayerStateAthena->Get<uint8_t>("TeamMemberState") = NewTeamMemberState; // pretty sure unneeded

	static auto EmojiComm = FindObject<UAthenaEmojiItemDefinition>(L"/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");
	PlayerController->ServerPlayEmoteItemHook(PlayerController, EmojiComm);

	static auto OnRep_ReplicatedTeamMemberStateFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerStateAthena.OnRep_ReplicatedTeamMemberState");
	PlayerStateAthena->ProcessEvent(OnRep_ReplicatedTeamMemberStateFn);
}

void AFortPlayerControllerAthena::GetPlayerViewPointHook(AFortPlayerControllerAthena* PlayerController, FVector& Location, FRotator& Rotation)
{
	// I don't know why but GetActorEyesViewPoint only works on some versions.
	/* static auto GetActorEyesViewPointFn = FindObject<UFunction>(L"/Script/Engine.Actor.GetActorEyesViewPoint");
	static auto GetActorEyesViewPointIndex = GetFunctionIdxOrPtr(GetActorEyesViewPointFn) / 8;

	void (*GetActorEyesViewPointOriginal)(AActor* Actor, FVector* OutLocation, FRotator* OutRotation) = decltype(GetActorEyesViewPointOriginal)(PlayerController->VFTable[GetActorEyesViewPointIndex]);
	return GetActorEyesViewPointOriginal(PlayerController, &Location, &Rotation); */

	if (auto MyFortPawn = PlayerController->GetMyFortPawn())
	{
		Location = MyFortPawn->GetActorLocation();
		Rotation = PlayerController->GetControlRotation();
		return;
	}

	return AFortPlayerControllerAthena::GetPlayerViewPointOriginal(PlayerController, Location, Rotation);
}

void AFortPlayerControllerAthena::ServerReadyToStartMatchHook(AFortPlayerControllerAthena* PlayerController)
{
	auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
	auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());

	LOG_INFO(LogDev, "ServerReadyToStartMatch!");

	ReviveCounts[PlayerController] = 0;

	if (Fortnite_Version <= 2.5) // techinally we should do this at the end of OnReadyToStartMatch
	{
		static auto QuickBarsOffset = PlayerController->GetOffset("QuickBars", false);

		if (QuickBarsOffset != -1)
		{
			auto& QuickBars = PlayerController->Get<AActor*>(QuickBarsOffset);

			// LOG_INFO(LogDev, "QuickBarsOld: {}", __int64(QuickBars));

			if (QuickBars)
				return ServerReadyToStartMatchOriginal(PlayerController);

			static auto FortQuickBarsClass = FindObject<UClass>("/Script/FortniteGame.FortQuickBars");

			QuickBars = GetWorld()->SpawnActor<AActor>(FortQuickBarsClass);

			// LOG_INFO(LogDev, "QuickBarsNew: {}", __int64(QuickBars));

			if (!QuickBars)
				return ServerReadyToStartMatchOriginal(PlayerController);

			PlayerController->Get<AActor*>(QuickBarsOffset)->SetOwner(PlayerController);
		}
	}

	return ServerReadyToStartMatchOriginal(PlayerController);
}

void AFortPlayerControllerAthena::UpdateTrackedAttributesHook(AFortPlayerControllerAthena* PlayerController)
{
	LOG_INFO(LogDev, "UpdateTrackedAttributesHook Return: 0x{:x}", __int64(_ReturnAddress()) - __int64(GetModuleHandleW(0)));

	// IDK IF GADGET IS A PARAM OR WHAT

	auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState()); // really we only need zone

	if (!PlayerState)
		return;

	auto ASC = PlayerState->GetAbilitySystemComponent();

	if (!ASC)
		return;

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	auto& ItemInstances = WorldInventory->GetItemList().GetItemInstances();

	std::vector<UFortItem*> ItemInstancesToRemove;

	for (int i = 0; i < ItemInstances.Num(); ++i)
	{
		auto ItemInstance = ItemInstances.at(i);
		auto GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(ItemInstance->GetItemEntry()->GetItemDefinition());

		if (!GadgetItemDefinition)
			continue;

		if (!GadgetItemDefinition->ShouldDestroyGadgetWhenTrackedAttributesIsZero())
			continue;

		bool bIsTrackedAttributesZero = true;

		for (int i = 0; i < GadgetItemDefinition->GetTrackedAttributes().Num(); ++i)
		{
			auto& CurrentTrackedAttribute = GadgetItemDefinition->GetTrackedAttributes().at(i);

			int CurrentAttributeValue = -1;

			for (int i = 0; i < ASC->GetSpawnedAttributes().Num(); ++i)
			{
				auto CurrentSpawnedAttribute = ASC->GetSpawnedAttributes().at(i);

				if (CurrentSpawnedAttribute->IsA(CurrentTrackedAttribute.AttributeOwner))
				{
					auto PropertyOffset = CurrentSpawnedAttribute->GetOffset(CurrentTrackedAttribute.GetAttributePropertyName());

					if (PropertyOffset != -1)
					{
						if (CurrentSpawnedAttribute->GetPtr<FFortGameplayAttributeData>(PropertyOffset)->GetCurrentValue() > 0)
						{
							bIsTrackedAttributesZero = false;
							break; // hm
						}
					}
				}
			}
		}

		if (bIsTrackedAttributesZero)
		{
			ItemInstancesToRemove.push_back(ItemInstance);
		}
	}

	for (auto ItemInstanceToRemove : ItemInstancesToRemove)
	{
		auto GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(ItemInstanceToRemove->GetItemEntry()->GetItemDefinition());

		WorldInventory->RemoveItem(ItemInstanceToRemove->GetItemEntry()->GetItemGuid(), nullptr, ItemInstanceToRemove->GetItemEntry()->GetCount(), true);

		static auto MulticastTriggerOnGadgetTrackedAttributeDestroyedFXFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerStateZone.MulticastTriggerOnGadgetTrackedAttributeDestroyedFX");
		PlayerState->ProcessEvent(MulticastTriggerOnGadgetTrackedAttributeDestroyedFXFn, &GadgetItemDefinition);
	}

	if (ItemInstancesToRemove.size() > 0)
		WorldInventory->Update();
}

void AFortPlayerControllerAthena::ServerCreativeSetFlightSpeedIndexHook(UObject* Context, FFrame& Stack, void* Ret)
{
	LOG_INFO(LogCreative, "ServerCreativeSetFlightSpeedIndexHook!");

	auto PlayerController = (AFortPlayerControllerAthena*)Context;

	int Index;
	Stack.StepCompiledIn(&Index);

	LOG_INFO(LogCreative, "Index: {}", Index);

	if (!PlayerController)
	{
		return ServerCreativeSetFlightSpeedIndexOriginal(Context, Stack, Ret);
	}

	auto Pawn = Cast<AFortPlayerPawnAthena>(PlayerController->GetPawn());

	if (!Pawn)
	{
		return ServerCreativeSetFlightSpeedIndexOriginal(Context, Stack, Ret);
	}

	auto CharacterMovement = Pawn->GetCharacterMovement();
	CharacterMovement->GetMaxFlySpeed() = Index;

	return ServerCreativeSetFlightSpeedIndexOriginal(Context, Stack, Ret);
}