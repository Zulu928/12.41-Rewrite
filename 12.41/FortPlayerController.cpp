#include "FortPlayerController.h"

#include "Rotator.h"
#include "BuildingSMActor.h"
#include "FortGameModeAthena.h"

#include "FortPlayerState.h"
#include "BuildingWeapons.h"

#include "ActorComponent.h"
#include "FortPlayerStateAthena.h"
#include "globals.h"
#include "FortPlayerControllerAthena.h"
#include "BuildingContainer.h"
#include "FortLootPackage.h"
#include "FortPickup.h"
#include "FortPlayerPawn.h"
#include <memcury.h>
#include "KismetStringLibrary.h"
#include "FortGadgetItemDefinition.h"
#include "FortAbilitySet.h"
#include "vendingmachine.h"
#include "KismetSystemLibrary.h"
#include "gui.h"
#include "FortAthenaMutator_InventoryOverride.h"
#include "FortAthenaMutator_TDM.h"
#include "FortAthenaMutator_GG.h"

void AFortPlayerController::ClientReportDamagedResourceBuilding(ABuildingSMActor* BuildingSMActor, EFortResourceType PotentialResourceType, int PotentialResourceCount, bool bDestroyed, bool bJustHitWeakspot)
{
	static auto fn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ClientReportDamagedResourceBuilding");

	struct { ABuildingSMActor* BuildingSMActor; EFortResourceType PotentialResourceType; int PotentialResourceCount; bool bDestroyed; bool bJustHitWeakspot; }
	AFortPlayerController_ClientReportDamagedResourceBuilding_Params{BuildingSMActor, PotentialResourceType, PotentialResourceCount, bDestroyed, bJustHitWeakspot};

	this->ProcessEvent(fn, &AFortPlayerController_ClientReportDamagedResourceBuilding_Params);
}

void AFortPlayerController::ClientEquipItem(const FGuid& ItemGuid, bool bForceExecution)
{
	static auto ClientEquipItemFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerAthena.ClientEquipItem") 
		? FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerControllerAthena.ClientEquipItem") 
		: FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerController.ClientEquipItem");

	if (ClientEquipItemFn)
	{
		struct
		{
			FGuid                                       ItemGuid;                                                 // (ConstParm, Parm, ZeroConstructor, ReferenceParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			bool                                               bForceExecution;                                          // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		} AFortPlayerController_ClientEquipItem_Params{ ItemGuid, bForceExecution };

		this->ProcessEvent(ClientEquipItemFn, &AFortPlayerController_ClientEquipItem_Params);
	}
}

bool AFortPlayerController::DoesBuildFree()
{
	if (Globals::bInfiniteMaterials)
		return true;

	static auto bBuildFreeOffset = GetOffset("bBuildFree");
	static auto bBuildFreeFieldMask = GetFieldMask(GetProperty("bBuildFree"));
	return ReadBitfieldValue(bBuildFreeOffset, bBuildFreeFieldMask);
}

void AFortPlayerController::DropAllItems(const std::vector<UFortItemDefinition*>& IgnoreItemDefs, bool bIgnoreSecondaryQuickbar, bool bRemoveIfNotDroppable, bool RemovePickaxe)
{
	auto Pawn = this->GetMyFortPawn();

	if (!Pawn)
		return;

	auto WorldInventory = this->GetWorldInventory();

	if (!WorldInventory)
		return;

	auto& ItemInstances = WorldInventory->GetItemList().GetItemInstances();
	auto Location = Pawn->GetActorLocation();

	std::vector<std::pair<FGuid, int>> GuidAndCountsToRemove;

	auto PickaxeInstance = WorldInventory->GetPickaxeInstance();

	for (int i = 0; i < ItemInstances.Num(); ++i)
	{
		auto ItemInstance = ItemInstances.at(i);

		if (!ItemInstance)
			continue;

		auto ItemEntry = ItemInstance->GetItemEntry();

		if (RemovePickaxe && ItemInstance == PickaxeInstance)
		{
			GuidAndCountsToRemove.push_back({ ItemEntry->GetItemGuid(), ItemEntry->GetCount() });
			continue;
		}

		auto WorldItemDefinition = Cast<UFortWorldItemDefinition>(ItemEntry->GetItemDefinition());

		if (!WorldItemDefinition || std::find(IgnoreItemDefs.begin(), IgnoreItemDefs.end(), WorldItemDefinition) != IgnoreItemDefs.end())
			continue;

		if (bIgnoreSecondaryQuickbar && !IsPrimaryQuickbar(WorldItemDefinition))
			continue;

		if (!bRemoveIfNotDroppable && !WorldItemDefinition->CanBeDropped())
			continue;

		GuidAndCountsToRemove.push_back({ ItemEntry->GetItemGuid(), ItemEntry->GetCount() });

		if (bRemoveIfNotDroppable && !WorldItemDefinition->CanBeDropped())
			continue;
	
		PickupCreateData CreateData;
		CreateData.ItemEntry = ItemEntry;
		CreateData.SpawnLocation = Location;
		CreateData.SourceType = EFortPickupSourceTypeFlag::GetPlayerValue();

		AFortPickup::SpawnPickup(CreateData);
	}

	for (auto& Pair : GuidAndCountsToRemove)
	{
		WorldInventory->RemoveItem(Pair.first, nullptr, Pair.second, true);
	}

	WorldInventory->Update();
}

void AFortPlayerController::ApplyCosmeticLoadout()
{
	auto PlayerStateAsFort = Cast<AFortPlayerStateAthena>(GetPlayerState());

	if (!PlayerStateAsFort)
		return;

	auto PawnAsFort = Cast<AFortPlayerPawn>(GetMyFortPawn());

	if (!PawnAsFort)
		return;

	static auto UpdatePlayerCustomCharacterPartsVisualizationFn = FindObject<UFunction>(L"/Script/FortniteGame.FortKismetLibrary.UpdatePlayerCustomCharacterPartsVisualization");

	if (!UpdatePlayerCustomCharacterPartsVisualizationFn)
	{
		if (Addresses::ApplyCharacterCustomization)
		{
			static void* (*ApplyCharacterCustomizationOriginal)(AFortPlayerState* a1, AFortPawn* a3) = decltype(ApplyCharacterCustomizationOriginal)(Addresses::ApplyCharacterCustomization);
			ApplyCharacterCustomizationOriginal(PlayerStateAsFort, PawnAsFort);

			PlayerStateAsFort->ForceNetUpdate();
			PawnAsFort->ForceNetUpdate();
			this->ForceNetUpdate();

			return;
		}

		auto CosmeticLoadout = GetCosmeticLoadoutOffset() != -1 ? this->GetCosmeticLoadout() : nullptr;

		if (CosmeticLoadout)
		{
			/* static auto Pawn_CosmeticLoadoutOffset = PawnAsFort->GetOffset("CosmeticLoadout");

			if (Pawn_CosmeticLoadoutOffset != -1)
			{
				CopyStruct(PawnAsFort->GetPtr<__int64>(Pawn_CosmeticLoadoutOffset), CosmeticLoadout, FFortAthenaLoadout::GetStructSize());
			} */

			auto Character = CosmeticLoadout->GetCharacter();

			// LOG_INFO(LogDev, "Character: {}", __int64(Character));
			// LOG_INFO(LogDev, "Character Name: {}", Character ? Character->GetFullName() : "InvalidObject");

			if (PawnAsFort)
			{
				ApplyCID(PawnAsFort, Character, false);

				auto Backpack = CosmeticLoadout->GetBackpack();

				if (Backpack)
				{
					static auto CharacterPartsOffset = Backpack->GetOffset("CharacterParts");

					if (CharacterPartsOffset != -1)
					{
						auto& BackpackCharacterParts = Backpack->Get<TArray<UObject*>>(CharacterPartsOffset);

						for (int i = 0; i < BackpackCharacterParts.Num(); ++i)
						{
							auto BackpackCharacterPart = BackpackCharacterParts.at(i);

							if (!BackpackCharacterPart)
								continue;

							PawnAsFort->ServerChoosePart(EFortCustomPartType::Backpack, BackpackCharacterPart);
						}

						// UFortKismetLibrary::ApplyCharacterCosmetics(GetWorld(), BackpackCharacterParts, PlayerStateAsFort, &aa);
					}
				}
			}
		}
		else
		{
			static auto HeroTypeOffset = PlayerStateAsFort->GetOffset("HeroType");
			ApplyHID(PawnAsFort, PlayerStateAsFort->Get(HeroTypeOffset));
		}

		PlayerStateAsFort->ForceNetUpdate();
		PawnAsFort->ForceNetUpdate();
		this->ForceNetUpdate();

		return;
	}

	UFortKismetLibrary::StaticClass()->ProcessEvent(UpdatePlayerCustomCharacterPartsVisualizationFn, &PlayerStateAsFort);

	PlayerStateAsFort->ForceNetUpdate();
	PawnAsFort->ForceNetUpdate();
	this->ForceNetUpdate();
}

void AFortPlayerController::ServerLoadingScreenDroppedHook(UObject* Context, FFrame* Stack, void* Ret)
{
	LOG_INFO(LogDev, "ServerLoadingScreenDroppedHook!");

	auto PlayerController = (AFortPlayerController*)Context;
	auto PlayerState = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState());
	auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
	auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
	auto WorldInventory = PlayerController->GetWorldInventory();

	PlayerController->ApplyCosmeticLoadout();

	if (Fortnite_Version >= 11)
	{
		LOG_INFO(LogDev, "Putting XP bar.");

		PlayerController->GetXPComponent()->IsRegisteredWithQuestManager() = true;
		PlayerController->GetXPComponent()->OnRep_bRegisteredWithQuestManager();
		PlayerState->GetSeasonLevelUIDisplay() = PlayerController->GetXPComponent()->GetCurrentLevel();
		PlayerState->OnRep_SeasonLevelUIDisplay();
	}

	static bool First = false;

	if (!First) // 1:1 fr
	{
		First = true;
		LettersClass = FindObject<UClass>("/Game/Athena/Items/QuestInteractables/FortnightLetters/FortniteLettersBPs/Prop_QuestInteractable_Letters_Parent.Prop_QuestInteractable_Letters_Parent_C");
		LetterQuestItem = FindObject<UProperty>("/Game/Athena/Items/QuestInteractables/Generic/Prop_QuestInteractable_Parent.Prop_QuestInteractable_Parent_C.QuestItem");
		BackendNameProp = FindObject<UProperty>("/Game/Athena/Items/QuestInteractables/Generic/Prop_QuestInteractable_Parent.Prop_QuestInteractable_Parent_C.QuestBackendObjectiveName");

		if (std::floor(Fortnite_Version) == 13)
		{
			TSubclassOf<AActor> FishingHoleClass = FindObject<UClass>(L"/Game/Athena/Items/EnvironmentalItems/FlopperSpawn/BGA_Athena_FlopperSpawn_World.BGA_Athena_FlopperSpawn_World_C");

			auto AllFishingHoles = UGameplayStatics::GetAllActorsOfClass(GetWorld(), FishingHoleClass);

			LOG_INFO(LogDev, "AllFishingHoles.Num(): {}", AllFishingHoles.Num());

			for (int i = 0; i < AllFishingHoles.Num(); i++)
			{
				auto FishingHole = AllFishingHoles.at(i);

				FishingHole->K2_DestroyActor();

				LOG_INFO(LogDev, "Destroyed Fishing Hole {}", FishingHole->GetFullName());
			}
		}
	}

	return ServerLoadingScreenDroppedOriginal(Context, Stack, Ret);
}

void AFortPlayerController::ServerRepairBuildingActorHook(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair)
{
	if (!BuildingActorToRepair 
		// || !BuildingActorToRepair->GetWorld()
		)
		return;

	if (BuildingActorToRepair->GetEditingPlayer())
	{
		// ClientSendMessage
		return;
	}

	float BuildingHealthPercent = BuildingActorToRepair->GetHealthPercent();

	// todo not hardcode these

	float BuildingCost = 10;
	float RepairCostMultiplier = 0.75;

	float BuildingHealthPercentLost = 1.0 - BuildingHealthPercent;
	float RepairCostUnrounded = (BuildingCost * BuildingHealthPercentLost) * RepairCostMultiplier;
	float RepairCost = std::floor(RepairCostUnrounded > 0 ? RepairCostUnrounded < 1 ? 1 : RepairCostUnrounded : 0);

	if (RepairCost < 0)
		return;

	auto ResourceItemDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActorToRepair->GetResourceType());

	if (!ResourceItemDefinition)
		return;

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	if (!PlayerController->DoesBuildFree())
	{
		auto ResourceInstance = WorldInventory->FindItemInstance(ResourceItemDefinition);

		if (!ResourceInstance)
			return;

		bool bShouldUpdate = false;

		if (!WorldInventory->RemoveItem(ResourceInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, RepairCost))
			return;

		if (bShouldUpdate)
			WorldInventory->Update();
	}

	struct { AFortPlayerController* RepairingController; int ResourcesSpent; } ABuildingSMActor_RepairBuilding_Params{ PlayerController, RepairCost };

	static auto RepairBuildingFn = FindObject<UFunction>(L"/Script/FortniteGame.BuildingSMActor.RepairBuilding");
	BuildingActorToRepair->ProcessEvent(RepairBuildingFn, &ABuildingSMActor_RepairBuilding_Params);
	// PlayerController->FortClientPlaySoundAtLocation(PlayerController->StartRepairSound, BuildingActorToRepair->K2_GetActorLocation(), 0, 0);
}

void AFortPlayerController::ServerExecuteInventoryItemHook(AFortPlayerController* PlayerController, FGuid ItemGuid)
{
	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	auto ItemInstance = WorldInventory->FindItemInstance(ItemGuid);
	auto Pawn = Cast<AFortPlayerPawn>(PlayerController->GetPawn());

	if (!ItemInstance || !Pawn)
		return;

	FGuid OldGuid = Pawn->GetCurrentWeapon() ? Pawn->GetCurrentWeapon()->GetItemEntryGuid() : FGuid(-1, -1, -1, -1);
	UFortItem* OldInstance = OldGuid == FGuid(-1, -1, -1, -1) ? nullptr : WorldInventory->FindItemInstance(OldGuid);
	auto ItemDefinition = ItemInstance->GetItemEntry()->GetItemDefinition();

	if (!ItemDefinition)
		return;

	static auto FortDecoTool_ContextTrapStaticClass = FindObject<UClass>(L"/Script/FortniteGame.FortDecoTool_ContextTrap");

	if (Fortnite_Version >= 18)
	{
		if (Pawn->GetCurrentWeapon() && Pawn->GetCurrentWeapon()->IsA(FortDecoTool_ContextTrapStaticClass))
		{
			LOG_INFO(LogDev, "Should unequip trap!");

			LOG_INFO(LogDev, "Pawn->GetCurrentWeapon()->GetItemEntryGuid(): {}", Pawn->GetCurrentWeapon()->GetItemEntryGuid().ToString());
			LOG_INFO(LogDev, "ItemGuid: {}", ItemGuid.ToString());
			LOG_INFO(LogDev, "ItemDefinition: {}", ItemDefinition->GetFullName());
			LOG_INFO(LogDev, "ItemInstance->GetItemEntry()->GetItemGuid(): {}", ItemInstance->GetItemEntry()->GetItemGuid().ToString());

			Pawn->GetCurrentWeapon()->GetItemEntryGuid() = ItemGuid;
			Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemDefinition, ItemInstance->GetItemEntry()->GetItemGuid());

			LOG_INFO(LogDev, "Pawn->GetCurrentWeapon()->GetItemEntryGuid(): {}", Pawn->GetCurrentWeapon()->GetItemEntryGuid().ToString());
			LOG_INFO(LogDev, "Pawn->GetCurrentWeapon()->GetFullName(): {}", Pawn->GetCurrentWeapon()->GetFullName());
		}
	}

	// LOG_INFO(LogDev, "Equipping ItemDefinition: {}", ItemDefinition->GetFullName());

	static auto FortGadgetItemDefinitionClass = FindObject<UClass>(L"/Script/FortniteGame.FortGadgetItemDefinition");

	UFortGadgetItemDefinition* GadgetItemDefinition = Cast<UFortGadgetItemDefinition>(ItemDefinition);

	if (GadgetItemDefinition)
	{
		static auto GetWeaponItemDefinition = FindObject<UFunction>(L"/Script/FortniteGame.FortGadgetItemDefinition.GetWeaponItemDefinition");

		if (GetWeaponItemDefinition)
		{
			ItemDefinition->ProcessEvent(GetWeaponItemDefinition, &ItemDefinition);
		}
		else
		{
			static auto GetDecoItemDefinition = FindObject<UFunction>(L"/Script/FortniteGame.FortGadgetItemDefinition.GetDecoItemDefinition");
			ItemDefinition->ProcessEvent(GetDecoItemDefinition, &ItemDefinition);
		}

		// LOG_INFO(LogDev, "Equipping Gadget: {}", ItemDefinition->GetFullName());
	}

	if (auto DecoItemDefinition = Cast<UFortDecoItemDefinition>(ItemDefinition))
	{
		Pawn->PickUpActor(nullptr, DecoItemDefinition); // todo check ret value? // I checked on 1.7.2 and it only returns true if the new weapon is a FortDecoTool
		Pawn->GetCurrentWeapon()->GetItemEntryGuid() = ItemGuid;

		if (Pawn->GetCurrentWeapon()->IsA(FortDecoTool_ContextTrapStaticClass))
		{
			LOG_INFO(LogDev, "Pawn->GetCurrentWeapon()->IsA(FortDecoTool_ContextTrapStaticClass)!");

			static auto ContextTrapItemDefinitionOffset = Pawn->GetCurrentWeapon()->GetOffset("ContextTrapItemDefinition");
			Pawn->GetCurrentWeapon()->Get<UObject*>(ContextTrapItemDefinitionOffset) = DecoItemDefinition;

			if (Fortnite_Version >= 18)
			{
				static auto SetContextTrapItemDefinitionFn = FindObject<UFunction>(L"/Script/FortniteGame.FortDecoTool_ContextTrap.SetContextTrapItemDefinition");
				Pawn->GetCurrentWeapon()->ProcessEvent(SetContextTrapItemDefinitionFn, &DecoItemDefinition);

				auto& ReplicatedEntries = WorldInventory->GetItemList().GetReplicatedEntries();

				for (int i = 0; i < ReplicatedEntries.Num(); i++)
				{
					auto ReplicatedEntry = ReplicatedEntries.AtPtr(i, FFortItemEntry::GetStructSize());

					if (ReplicatedEntry->GetItemGuid() == ItemGuid)
					{
						auto Instance = WorldInventory->FindItemInstance(ItemGuid);

						WorldInventory->GetItemList().MarkItemDirty(ReplicatedEntry);
						WorldInventory->GetItemList().MarkItemDirty(Instance->GetItemEntry());
						WorldInventory->HandleInventoryLocalUpdate();

						Pawn->EquipWeaponDefinition(DecoItemDefinition, ReplicatedEntry->GetItemGuid());
					}
				}
			}
		}

		return;
	}

	if (!ItemDefinition)
		return;

	if (auto Weapon = Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemDefinition, ItemInstance->GetItemEntry()->GetItemGuid()))
	{
		if (Engine_Version < 420)
		{
			static auto FortWeap_BuildingToolClass = FindObject<UClass>(L"/Script/FortniteGame.FortWeap_BuildingTool");

			if (!Weapon->IsA(FortWeap_BuildingToolClass))
				return;

			auto BuildingTool = Weapon;

			using UBuildingEditModeMetadata = UObject;
			using UFortBuildingItemDefinition = UObject;

			static auto OnRep_DefaultMetadataFn = FindObject<UFunction>(L"/Script/FortniteGame.FortWeap_BuildingTool.OnRep_DefaultMetadata");
			static auto DefaultMetadataOffset = BuildingTool->GetOffset("DefaultMetadata");

			static auto RoofPiece = FindObject<UFortBuildingItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_RoofS.BuildingItemData_RoofS");
			static auto FloorPiece = FindObject<UFortBuildingItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Floor.BuildingItemData_Floor");
			static auto WallPiece = FindObject<UFortBuildingItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Wall.BuildingItemData_Wall");
			static auto StairPiece = FindObject<UFortBuildingItemDefinition>(L"/Game/Items/Weapons/BuildingTools/BuildingItemData_Stair_W.BuildingItemData_Stair_W");

			UBuildingEditModeMetadata* OldMetadata = nullptr; // Newer versions
			OldMetadata = BuildingTool->Get<UBuildingEditModeMetadata*>(DefaultMetadataOffset);

			if (ItemDefinition == RoofPiece)
			{
				static auto RoofMetadata = FindObject<UBuildingEditModeMetadata>(L"/Game/Building/EditModePatterns/Roof/EMP_Roof_RoofC.EMP_Roof_RoofC");
				BuildingTool->Get<UBuildingEditModeMetadata*>(DefaultMetadataOffset) = RoofMetadata;
			}
			else if (ItemDefinition == StairPiece)
			{
				static auto StairMetadata = FindObject<UBuildingEditModeMetadata>(L"/Game/Building/EditModePatterns/Stair/EMP_Stair_StairW.EMP_Stair_StairW");
				BuildingTool->Get<UBuildingEditModeMetadata*>(DefaultMetadataOffset) = StairMetadata;
			}
			else if (ItemDefinition == WallPiece)
			{
				static auto WallMetadata = FindObject<UBuildingEditModeMetadata>(L"/Game/Building/EditModePatterns/Wall/EMP_Wall_Solid.EMP_Wall_Solid");
				BuildingTool->Get<UBuildingEditModeMetadata*>(DefaultMetadataOffset) = WallMetadata;
			}
			else if (ItemDefinition == FloorPiece)
			{
				static auto FloorMetadata = FindObject<UBuildingEditModeMetadata>(L"/Game/Building/EditModePatterns/Floor/EMP_Floor_Floor.EMP_Floor_Floor");
				BuildingTool->Get<UBuildingEditModeMetadata*>(DefaultMetadataOffset) = FloorMetadata;
			}

			BuildingTool->ProcessEvent(OnRep_DefaultMetadataFn, &OldMetadata);
		}
	}
}

void AFortPlayerController::ServerAttemptInteractHook(UObject* Context, FFrame* Stack, void* Ret)
{
	static std::map<AFortPlayerControllerAthena*, int> ChestsSearched{};
	// static auto LlamaClass = FindObject<UClass>(L"/Game/Athena/SupplyDrops/Llama/AthenaSupplyDrop_Llama.AthenaSupplyDrop_Llama_C");
	static auto FortAthenaSupplyDropClass = FindObject<UClass>(L"/Script/FortniteGame.FortAthenaSupplyDrop");
	static auto BuildingItemCollectorActorClass = FindObject<UClass>(L"/Script/FortniteGame.BuildingItemCollectorActor");
	static auto AthenaQuestBGAClass = FindObject<UClass>("/Game/Athena/Items/QuestInteractablesV2/Parents/AthenaQuest_BGA.AthenaQuest_BGA_C");
	static auto BP_Athena_PropQuestActor_ParentClass = FindObject<UClass>("/Game/Athena/Items/QuestParents/PropQuestActor/BP_Athena_PropQuestActor_Parent.BP_Athena_PropQuestActor_Parent_C");
	static auto BP_StationProp_ParentClass = FindObject<UClass>("/Game/Building/ActorBlueprints/Stations/BP_StationProp_Parent.BP_StationProp_Parent_C");

	LOG_INFO(LogInteraction, "ServerAttemptInteract!");

	auto Params = Stack->Locals;

	static bool bIsUsingComponent = FindObject<UClass>(L"/Script/FortniteGame.FortControllerComponent_Interaction");

	AFortPlayerControllerAthena* PlayerController = bIsUsingComponent ? Cast<AFortPlayerControllerAthena>(((UActorComponent*)Context)->GetOwner()) :
		Cast<AFortPlayerControllerAthena>(Context);

	if (!PlayerController)
		return;

	std::string StructName = bIsUsingComponent ? "/Script/FortniteGame.FortControllerComponent_Interaction.ServerAttemptInteract" : "/Script/FortniteGame.FortPlayerController.ServerAttemptInteract";

	static auto ReceivingActorOffset = FindOffsetStruct(StructName, "ReceivingActor");
	auto ReceivingActor = *(AActor**)(__int64(Params) + ReceivingActorOffset);

	static auto InteractionBeingAttemptedOffset = FindOffsetStruct(StructName, "InteractionBeingAttempted");
	auto InteractionBeingAttempted = *(EInteractionBeingAttempted*)(__int64(Params) + InteractionBeingAttemptedOffset);

	// LOG_INFO(LogInteraction, "ReceivingActor: {}", __int64(ReceivingActor));

	if (!ReceivingActor)
		return;

	LOG_INFO(LogInteraction, "ReceivingActor Name: {}", ReceivingActor->GetFullName());

	FVector LocationToSpawnLoot = ReceivingActor->GetActorLocation() + ReceivingActor->GetActorRightVector() * 70.f + FVector{ 0, 0, 50 };

	static auto FortAthenaVehicleClass = FindObject<UClass>(L"/Script/FortniteGame.FortAthenaVehicle");
	static auto SearchAnimationCountOffset = FindOffsetStruct("/Script/FortniteGame.FortSearchBounceData", "SearchAnimationCount");

	if (auto BuildingContainer = Cast<ABuildingContainer>(ReceivingActor))
	{
		static auto bAlreadySearchedOffset = BuildingContainer->GetOffset("bAlreadySearched");
		static auto SearchBounceDataOffset = BuildingContainer->GetOffset("SearchBounceData");
		static auto bAlreadySearchedFieldMask = GetFieldMask(BuildingContainer->GetProperty("bAlreadySearched"));

		auto SearchBounceData = BuildingContainer->GetPtr<void>(SearchBounceDataOffset);

		if (BuildingContainer->ReadBitfieldValue(bAlreadySearchedOffset, bAlreadySearchedFieldMask))
			return;

		// LOG_INFO(LogInteraction, "bAlreadySearchedFieldMask: {}", bAlreadySearchedFieldMask);

		BuildingContainer->SpawnLoot(PlayerController->GetMyFortPawn());

		AFortPlayerStateAthena* PS = PlayerController->GetPlayerStateAthena();
		if (!ReceivingActor || !ReceivingActor->ClassPrivate->GetName().contains("Ammo"))
		{
			PS->Get<int>(MemberOffsets::FortPlayerStateAthena::ChestsSearched)++;
		}
		auto Searched = PS->Get<int>(MemberOffsets::FortPlayerStateAthena::ChestsSearched);
		PlayerController->GiveAccolade(PlayerController, GetDefFromEvent(EAccoladeEvent::Search, Searched, ReceivingActor));

		BuildingContainer->SetBitfieldValue(bAlreadySearchedOffset, bAlreadySearchedFieldMask, true);
		(*(int*)(__int64(SearchBounceData) + SearchAnimationCountOffset))++;
		BuildingContainer->BounceContainer();

		BuildingContainer->ForceNetUpdate(); // ?

		static auto OnRep_bAlreadySearchedFn = FindObject<UFunction>(L"/Script/FortniteGame.BuildingContainer.OnRep_bAlreadySearched");
		// BuildingContainer->ProcessEvent(OnRep_bAlreadySearchedFn);

		// if (BuildingContainer->ShouldDestroyOnSearch())
			// BuildingContainer->K2_DestroyActor();
	}
	else if (ReceivingActor->IsA(FortAthenaVehicleClass))
	{
		auto Vehicle = (AFortAthenaVehicle*)ReceivingActor;
		ServerAttemptInteractOriginal(Context, Stack, Ret);

		if (!AreVehicleWeaponsEnabled())
			return;

		auto Pawn = (AFortPlayerPawn*)PlayerController->GetMyFortPawn();

		if (!Pawn)
			return;

		auto VehicleWeaponDefinition = Pawn->GetVehicleWeaponDefinition(Vehicle);

		if (!VehicleWeaponDefinition)
		{
			LOG_INFO(LogDev, "Invalid VehicleWeaponDefinition!");
			return;
		}

		LOG_INFO(LogDev, "Equipping {}", VehicleWeaponDefinition->GetFullName());

		auto WorldInventory = PlayerController->GetWorldInventory();

		if (!WorldInventory)
			return;

		auto NewAndModifiedInstances = WorldInventory->AddItem(VehicleWeaponDefinition, nullptr, 1, 9999);

		auto NewVehicleInstance = NewAndModifiedInstances.first[0];

		if (!NewVehicleInstance)
			return;

		PlayerController->GetSwappingItemDefinition() = Pawn->GetCurrentWeapon()->GetWeaponData();

		auto& ReplicatedEntries = WorldInventory->GetItemList().GetReplicatedEntries();

		for (int i = 0; i < ReplicatedEntries.Num(); i++)
		{
			auto ReplicatedEntry = ReplicatedEntries.AtPtr(i, FFortItemEntry::GetStructSize());

			if (ReplicatedEntry->GetItemGuid() == NewVehicleInstance->GetItemEntry()->GetItemGuid())
			{
				WorldInventory->GetItemList().MarkItemDirty(ReplicatedEntry);
				WorldInventory->GetItemList().MarkItemDirty(NewVehicleInstance->GetItemEntry());
				WorldInventory->HandleInventoryLocalUpdate();

				Pawn->EquipWeaponDefinition(VehicleWeaponDefinition, ReplicatedEntry->GetItemGuid());
			}
		}

		return;
	}
	else if (ReceivingActor->IsA(BuildingItemCollectorActorClass))
	{
		if (Engine_Version >= 424 && /*Fortnite_Version < 15 && */ReceivingActor->GetFullName().contains("Wumba"))
		{
			bool bIsSidegrading = InteractionBeingAttempted == EInteractionBeingAttempted::SecondInteraction ? true : false;

			LOG_INFO(LogDev, "bIsSidegrading: {}", (bool)bIsSidegrading);

			struct FWeaponUpgradeItemRow
			{
				void* FTableRowBaseInheritance;
				UFortWeaponItemDefinition* CurrentWeaponDef;                                  // 0x8(0x8)(Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				UFortWeaponItemDefinition* UpgradedWeaponDef;                                 // 0x10(0x8)(Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				EFortWeaponUpgradeCosts           WoodCost;                                          // 0x18(0x1)(Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				EFortWeaponUpgradeCosts           MetalCost;                                         // 0x19(0x1)(Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				EFortWeaponUpgradeCosts           BrickCost;                                         // 0x1A(0x1)(Edit, ZeroConstructor, DisableEditOnInstance, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				EFortWeaponUpgradeDirection       Direction;
			};

			static auto WumbaDataTable = FindObject<UDataTable>("/Game/Items/Datatables/AthenaWumbaData.AthenaWumbaData");

			static auto LootPackagesRowMap = WumbaDataTable->GetRowMap();

			auto Pawn = Cast<AFortPawn>(PlayerController->GetPawn());
			auto CurrentHeldWeapon = Pawn->GetCurrentWeapon();
			auto CurrentHeldWeaponDef = CurrentHeldWeapon->GetWeaponData();

			auto Direction = bIsSidegrading ? EFortWeaponUpgradeDirection::Horizontal : EFortWeaponUpgradeDirection::Vertical;

			LOG_INFO(LogDev, "Direction: {}", (int)Direction);

			FWeaponUpgradeItemRow* FoundRow = nullptr;

			for (int i = 0; i < LootPackagesRowMap.Pairs.Elements.Data.Num(); i++)
			{
				auto& Pair = LootPackagesRowMap.Pairs.Elements.Data.at(i).ElementData.Value;
				auto First = Pair.First;
				auto Row = (FWeaponUpgradeItemRow*)Pair.Second;

				if (Row->CurrentWeaponDef == CurrentHeldWeaponDef && Row->Direction == Direction)
				{
					FoundRow = Row;
					break;
				}
			}

			if (!FoundRow)
			{
				LOG_WARN(LogGame, "Failed to find row!");
				return;
			}

			auto NewDefinition = FoundRow->UpgradedWeaponDef;

			LOG_INFO(LogDev, "UpgradedWeaponDef: {}", NewDefinition->GetFullName());

			int WoodToRemove = Fortnite_Version < 12 ? -50 : 0;
			int StoneToRemove = Fortnite_Version < 12 ? 350 : 400;
			int MetalToRemove = Fortnite_Version < 12 ? 150 : 200;

			int WoodCost = (int)FoundRow->WoodCost * 50 - WoodToRemove;
			int StoneCost = (int)FoundRow->BrickCost * 50 - StoneToRemove;
			int MetalCost = (int)FoundRow->MetalCost * 50 - MetalToRemove;

			if (bIsSidegrading)
			{
				WoodCost = 20;
				StoneCost = 20;
				MetalCost = 20;
			}

			static auto WoodItemData = FindObject<UFortItemDefinition>("/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
			static auto StoneItemData = FindObject<UFortItemDefinition>("/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
			static auto MetalItemData = FindObject<UFortItemDefinition>("/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

			auto WorldInventory = PlayerController->GetWorldInventory();

			auto WoodInstance = WorldInventory->FindItemInstance(WoodItemData);
			auto WoodCount = WoodInstance->GetItemEntry()->GetCount();

			auto StoneInstance = WorldInventory->FindItemInstance(StoneItemData);
			auto StoneCount = StoneInstance->GetItemEntry()->GetCount();

			auto MetalInstance = WorldInventory->FindItemInstance(MetalItemData);
			auto MetalCount = MetalInstance->GetItemEntry()->GetCount();

			if (WoodCount < WoodCost || StoneCount < StoneCost || MetalCount < MetalCost)
			{
				return ServerAttemptInteractOriginal(Context, Stack, Ret);
			}

			bool bShouldUpdate = false;

			WorldInventory->RemoveItem(WoodInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, WoodCost);
			WorldInventory->RemoveItem(StoneInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, StoneCost);
			WorldInventory->RemoveItem(MetalInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, MetalCost);

			WorldInventory->RemoveItem(CurrentHeldWeapon->GetItemEntryGuid(), &bShouldUpdate, 1, true);

			WorldInventory->AddItem(NewDefinition, &bShouldUpdate);

			if (bShouldUpdate)
				WorldInventory->Update();
		}
		else
		{
			auto WorldInventory = PlayerController->GetWorldInventory();

			if (!WorldInventory)
				return ServerAttemptInteractOriginal(Context, Stack, Ret);

			auto ItemCollector = ReceivingActor;
			static auto ActiveInputItemOffset = ItemCollector->GetOffset("ActiveInputItem");
			auto CurrentMaterial = ItemCollector->Get<UFortWorldItemDefinition*>(ActiveInputItemOffset); // InteractType->OptionalObjectData

			if (!CurrentMaterial)
				return ServerAttemptInteractOriginal(Context, Stack, Ret);

			int Index = 0;

			// this is a weird way of getting the current item collection we are on.

			static auto StoneItemData = FindObject<UFortResourceItemDefinition>(L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
			static auto MetalItemData = FindObject<UFortResourceItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

			if (CurrentMaterial == StoneItemData)
				Index = 1;
			else if (CurrentMaterial == MetalItemData)
				Index = 2;

			static auto ItemCollectionsOffset = ItemCollector->GetOffset("ItemCollections");
			auto& ItemCollections = ItemCollector->Get<TArray<FCollectorUnitInfo>>(ItemCollectionsOffset);

			auto ItemCollection = ItemCollections.AtPtr(Index, FCollectorUnitInfo::GetPropertiesSize());

			if (Fortnite_Version < 8.10)
			{
				auto Cost = ItemCollection->GetInputCount()->GetValue();

				if (!CurrentMaterial)
					return ServerAttemptInteractOriginal(Context, Stack, Ret);

				auto MatInstance = WorldInventory->FindItemInstance(CurrentMaterial);

				if (!MatInstance)
					return ServerAttemptInteractOriginal(Context, Stack, Ret);

				bool bShouldUpdate = false;

				if (!WorldInventory->RemoveItem(MatInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, Cost, true))
					return ServerAttemptInteractOriginal(Context, Stack, Ret);

				if (bShouldUpdate)
					WorldInventory->Update();
			}

			for (int z = 0; z < ItemCollection->GetOutputItemEntry()->Num(); z++)
			{
				auto Entry = ItemCollection->GetOutputItemEntry()->AtPtr(z, FFortItemEntry::GetStructSize());

				PickupCreateData CreateData;
				CreateData.ItemEntry = FFortItemEntry::MakeItemEntry(Entry->GetItemDefinition(), Entry->GetCount(), Entry->GetLoadedAmmo(), MAX_DURABILITY, Entry->GetLevel());
				CreateData.SpawnLocation = LocationToSpawnLoot;
				CreateData.PawnOwner = PlayerController->GetMyFortPawn(); // hmm
				CreateData.bShouldFreeItemEntryWhenDeconstructed = true;

				AFortPickup::SpawnPickup(CreateData);
			}

			static auto bCurrentInteractionSuccessOffset = ItemCollector->GetOffset("bCurrentInteractionSuccess", false);

			if (bCurrentInteractionSuccessOffset != -1)
			{
				static auto bCurrentInteractionSuccessFieldMask = GetFieldMask(ItemCollector->GetProperty("bCurrentInteractionSuccess"));
				ItemCollector->SetBitfieldValue(bCurrentInteractionSuccessOffset, bCurrentInteractionSuccessFieldMask, true); // idek if this is needed
			}

			static auto DoVendDeath = FindObject<UFunction>(L"/Game/Athena/Items/Gameplay/VendingMachine/B_Athena_VendingMachine.B_Athena_VendingMachine_C.DoVendDeath");

			if (DoVendDeath)
			{
				ItemCollector->ProcessEvent(DoVendDeath);
				ItemCollector->K2_DestroyActor();
			}
		}
	}
	else if (ReceivingActor->IsA(BP_StationProp_ParentClass))
	{
		LOG_INFO(LogDev, "S15+ UI BP!");

		if (ReceivingActor->GetFullName().contains("Bounty"))
		{
			static auto ConversationComponentOffset = ReceivingActor->GetOffset("NonPlayerConversationComponent");
			auto ConversationComponent = ReceivingActor->Get<UObject*>(ConversationComponentOffset);

			static auto EntryTagOffset = ReceivingActor->GetOffset("ConversationEntryTag");
			auto EntryTag = ReceivingActor->Get<FGameplayTag>(EntryTagOffset);

			static auto StartConversationFn = FindObject<UFunction>("/Script/FortniteGame.FortNonPlayerConversationParticipantComponent.StartConversation");

			struct
			{
				FGameplayTag InConversationEntryTag;
				AActor* Instigator;
				AActor* Target;
			}UFortNonPlayerConversationParticipantComponent_StartConversation_Params{ EntryTag , PlayerController , ReceivingActor };

			ConversationComponent->ProcessEvent(StartConversationFn, &UFortNonPlayerConversationParticipantComponent_StartConversation_Params);
		}
	}
	else if (ReceivingActor->IsA(LettersClass))
	{
		LOG_INFO(LogGame, "Letter!");

		FName BackendName = *(FName*)(__int64(ReceivingActor) + BackendNameProp->Offset);
		UFortQuestItemDefinition* QuestDef = *(UFortQuestItemDefinition**)(__int64(ReceivingActor) + LetterQuestItem->Offset);
		PlayerController->ProgressQuest(PlayerController, QuestDef, BackendName);
	}
	else if (ReceivingActor->IsA(AthenaQuestBGAClass))
	{
		LOG_INFO(LogGame, "Quest!");
		ReceivingActor->ProcessEvent(FindObject<UFunction>("/Game/Athena/Items/QuestInteractablesV2/Parents/AthenaQuest_BGA.AthenaQuest_BGA_C.BindToQuestManagerForQuestUpdate"), &PlayerController);

		static auto QuestsRequiredOnProfileOffset = ReceivingActor->GetOffset("QuestsRequiredOnProfile");
		static auto Primary_BackendNameOffset = ReceivingActor->GetOffset("Primary_BackendName");
		TArray<UFortQuestItemDefinition*>& QuestsRequiredOnProfile = *(TArray<UFortQuestItemDefinition*>*)(__int64(ReceivingActor) + QuestsRequiredOnProfileOffset);
		FName& Primary_BackendName = *(FName*)(__int64(ReceivingActor) + Primary_BackendNameOffset);

		PlayerController->ProgressQuest(PlayerController, QuestsRequiredOnProfile[0], Primary_BackendName);
	}
	/*
	else if (ReceivingActor->GetFullName().contains("QuestInteractable"))
	{
		LOG_INFO(LogGame, "Old quest so bad code wjasfhuaeguj");

		static auto QuestInteractable_GEN_VARIABLEOffset = ReceivingActor->GetOffset("QuestInteractable");
		static auto PCsOnQuestOffset = ReceivingActor->GetOffset("PCsOnQuest");
		static auto PCsThatCompletedQuest_ServerOffset = ReceivingActor->GetOffset("PCsThatCompletedQuest_Server");
		UQuestInteractableComponent* QuestComp = *(UQuestInteractableComponent**)(__int64(ReceivingActor) + QuestInteractable_GEN_VARIABLEOffset);
		TArray<AFortPlayerControllerAthena*>& PCsOnQuest = *(TArray<AFortPlayerControllerAthena*>*)(__int64(ReceivingActor) + PCsThatCompletedQuest_ServerOffset);
		TArray<AFortPlayerControllerAthena*>& PCsThatCompletedQuest_Server = *(TArray<AFortPlayerControllerAthena*>*)(__int64(ReceivingActor) + PCsThatCompletedQuest_ServerOffset);
		QuestComp->IsReady() = true;
		QuestComp->OnRep_Ready();
		auto QuestManager = PlayerController->GetQuestManager(ESubGame::Athena);
		auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());

		PCsOnQuest.Add(PlayerController);
		PCsThatCompletedQuest_Server.Add(PlayerController);
		QuestComp->OnPlaylistDataReady(GameState, GameState->GetCurrentPlaylist(), *(FGameplayTagContainer*)(__int64(GameState->GetCurrentPlaylist()) + GameState->GetCurrentPlaylist()->GetOffset("GameplayTagContainer")));

		PlayerController->ProgressQuest(PlayerController, QuestComp->GetQuestItemDefinition(), QuestComp->GetObjectiveBackendName());

		QuestComp->OnCalendarUpdated();
	}
	*/
	else if (ReceivingActor->IsA(BP_Athena_PropQuestActor_ParentClass))
	{
		LOG_INFO(LogGame, "Quest!");

		static auto ObjBackendNameOffset = ReceivingActor->GetOffset("ObjBackendName");
		static auto QuestItemDefinitionOffset = ReceivingActor->GetOffset("QuestItemDefinition");
		FName& ObjBackendName = *(FName*)(__int64(ReceivingActor) + ObjBackendNameOffset);
		UFortQuestItemDefinition* QuestItemDefinition = *(UFortQuestItemDefinition**)(__int64(ReceivingActor) + QuestItemDefinitionOffset);

		PlayerController->ProgressQuest(PlayerController, QuestItemDefinition, ObjBackendName);
	}
	else if (ReceivingActor->ClassPrivate->GetName().contains("Tiered_"))
	{
		ChestsSearched[PlayerController]++;
		PlayerController->GiveAccolade(PlayerController, GetDefFromEvent(EAccoladeEvent::Search, ChestsSearched[PlayerController], ReceivingActor));
	}

	return ServerAttemptInteractOriginal(Context, Stack, Ret);
}

void AFortPlayerController::ServerAttemptAircraftJumpHook(AFortPlayerController* PC, FRotator ClientRotation)
{
	auto PlayerController = Cast<AFortPlayerControllerAthena>(Engine_Version < 424 ? PC : ((UActorComponent*)PC)->GetOwner());

	if (Engine_Version < 424 && !Globals::bLateGame.load())
		return ServerAttemptAircraftJumpOriginal(PC, ClientRotation);

	if (!PlayerController)
		return ServerAttemptAircraftJumpOriginal(PC, ClientRotation);

	// if (!PlayerController->bInAircraft) 
		// return;

	LOG_INFO(LogDev, "ServerAttemptAircraftJumpHook!");

	auto GameMode = (AFortGameModeAthena*)GetWorld()->GetGameMode();
	auto GameState = GameMode->GetGameStateAthena();

	AActor* AircraftToJumpFrom = nullptr;

	static auto AircraftsOffset = GameState->GetOffset("Aircrafts", false);

	if (AircraftsOffset == -1)
	{
		static auto AircraftOffset = GameState->GetOffset("Aircraft");
		AircraftToJumpFrom = GameState->Get<AActor*>(AircraftOffset);
	}
	else
	{
		auto Aircrafts = GameState->GetPtr<TArray<AActor*>>(AircraftsOffset);
		AircraftToJumpFrom = Aircrafts->Num() > 0 ? Aircrafts->at(0) : nullptr; // skunky
	}

	if (!AircraftToJumpFrom)
		return ServerAttemptAircraftJumpOriginal(PC, ClientRotation);

	if (false)
	{
		auto NewPawn = GameMode->SpawnDefaultPawnForHook(GameMode, (AController*)PlayerController, AircraftToJumpFrom);
		PlayerController->Possess(NewPawn);
	}
	else
	{
		if (false)
		{
			// honestly idk why this doesnt work

			auto NAME_Inactive = UKismetStringLibrary::Conv_StringToName(L"NAME_Inactive");

			LOG_INFO(LogDev, "name Comp: {}", NAME_Inactive.ComparisonIndex.Value);

			PlayerController->GetStateName() = NAME_Inactive;
			PlayerController->SetPlayerIsWaiting(true);
			PlayerController->ServerRestartPlayer();
		}
		else
		{
			GameMode->RestartPlayer(PlayerController);
		}

		// we are supposed to do some skydivign stuff here but whatever
	}

	auto NewPawnAsFort = PlayerController->GetMyFortPawn();

	if (Fortnite_Version >= 18) // TODO (Milxnor) Find a better fix and move this
	{
		static auto StormEffectClass = FindObject<UClass>(L"/Game/Athena/SafeZone/GE_OutsideSafeZoneDamage.GE_OutsideSafeZoneDamage_C");
		auto PlayerState = PlayerController->GetPlayerStateAthena();

		PlayerState->GetAbilitySystemComponent()->RemoveActiveGameplayEffectBySourceEffect(StormEffectClass, 1, PlayerState->GetAbilitySystemComponent());
	}

	if (NewPawnAsFort)
	{
		NewPawnAsFort->SetHealth(100); // needed with server restart player?
		
		if (Globals::LateGame)
		{
			NewPawnAsFort->SetShield(100);

			NewPawnAsFort->TeleportTo(AircraftToJumpFrom->GetActorLocation(), FRotator());
		}
	}

	// PlayerController->ServerRestartPlayer();
	// return ServerAttemptAircraftJumpOriginal(PC, ClientRotation);
}

void AFortPlayerController::ServerSuicideHook(AFortPlayerController* PlayerController)
{
	LOG_INFO(LogDev, "Suicide!");

	auto Pawn = PlayerController->GetPawn();

	if (!Pawn)
		return;

	// theres some other checks here idk

	if (!Pawn->IsA(AFortPlayerPawn::StaticClass())) // Why FortPlayerPawn? Ask Fortnite
		return;

	// suicide doesn't actually call force kill but its basically the same function

	static auto ForceKillFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPawn.ForceKill"); // exists on 1.2 and 19.10 with same params so I assume it's the same on every other build.

	FGameplayTag DeathReason; // unused on 1.7.2
	AActor* KillerActor = nullptr; // its just 0 in suicide (not really but easiest way to explain it)

	struct { FGameplayTag DeathReason; AController* KillerController; AActor* KillerActor; } AFortPawn_ForceKill_Params{ DeathReason, PlayerController, KillerActor };

	Pawn->ProcessEvent(ForceKillFn, &AFortPawn_ForceKill_Params);

	//PlayerDeathReport->ServerTimeForRespawn && PlayerDeathReport->ServerTimeForResurrect = 0? // I think this is what they do on 1.7.2 I'm too lazy to double check though.
}

void AFortPlayerController::ServerDropAllItemsHook(AFortPlayerController* PlayerController, UFortItemDefinition* IgnoreItemDef)
{
	LOG_INFO(LogDev, "DropAllItems!");
	PlayerController->DropAllItems({ IgnoreItemDef });
}

void AFortPlayerController::ServerCreateBuildingActorHook(UObject* Context, FFrame* Stack, void* Ret)
{
	auto PlayerController = (AFortPlayerController*)Context;

	if (!PlayerController) // ??
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);

	auto PlayerStateAthena = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState());

	if (!PlayerStateAthena)
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);

	UClass* BuildingClass = nullptr;
	FVector BuildLocation;
	FRotator BuildRotator;
	bool bMirrored;

	if (Fortnite_Version >= 8.30)
	{
		struct FCreateBuildingActorData { uint32_t BuildingClassHandle; FVector BuildLoc; FRotator BuildRot; bool bMirrored; };
		auto CreateBuildingData = (FCreateBuildingActorData*)Stack->Locals;

		BuildLocation = CreateBuildingData->BuildLoc;
		BuildRotator = CreateBuildingData->BuildRot;
		bMirrored = CreateBuildingData->bMirrored;

		static auto BroadcastRemoteClientInfoOffset = PlayerController->GetOffset("BroadcastRemoteClientInfo");
		auto BroadcastRemoteClientInfo = PlayerController->Get(BroadcastRemoteClientInfoOffset);

		static auto RemoteBuildableClassOffset = BroadcastRemoteClientInfo->GetOffset("RemoteBuildableClass");
		BuildingClass = BroadcastRemoteClientInfo->Get<UClass*>(RemoteBuildableClassOffset);
	}
	else
	{
		struct FBuildingClassData { UClass* BuildingClass; int PreviousBuildingLevel; int UpgradeLevel; };
		struct SCBAParams { FBuildingClassData BuildingClassData; FVector BuildLoc; FRotator BuildRot; bool bMirrored; };

		auto Params = (SCBAParams*)Stack->Locals;

		BuildingClass = Params->BuildingClassData.BuildingClass;
		BuildLocation = Params->BuildLoc;
		BuildRotator = Params->BuildRot;
		bMirrored = Params->bMirrored;
	}

	// LOG_INFO(LogDev, "BuildingClass {}", __int64(BuildingClass));

	if (!BuildingClass)
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);

	auto GameState = Cast<AFortGameStateAthena>(((AFortGameMode*)GetWorld()->GetGameMode())->GetGameState());

	auto StructuralSupportSystem = GameState->GetStructuralSupportSystem();

	if (StructuralSupportSystem)
	{
		if (!StructuralSupportSystem->IsWorldLocValid(BuildLocation))
		{
			return ServerCreateBuildingActorOriginal(Context, Stack, Ret);
		}
	}

	if (!GameState->IsPlayerBuildableClass(BuildingClass))
	{
		LOG_INFO(LogDev, "Cheater most likely.");
		// PlayerController->GetAnticheatComponent().AddAndCheck(Severity::HIGH);
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);
	}

	TArray<ABuildingSMActor*> ExistingBuildings;
	char idk;
	static __int64 (*CantBuild)(UObject*, UObject*, FVector, FRotator, char, TArray<ABuildingSMActor*>*, char*) = decltype(CantBuild)(Addresses::CantBuild);
	bool bCanBuild = !CantBuild(GetWorld(), BuildingClass, BuildLocation, BuildRotator, bMirrored, &ExistingBuildings, &idk);

	if (!bCanBuild)
	{
		ExistingBuildings.Free();
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);
	}

	FTransform Transform{};
	Transform.Translation = BuildLocation;
	Transform.Rotation = BuildRotator.Quaternion();
	Transform.Scale3D = { 1, 1, 1 };

	auto BuildingActor = GetWorld()->SpawnActor<ABuildingSMActor>(BuildingClass, Transform);

	if (!BuildingActor)
	{
		ExistingBuildings.Free();
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);
	}

	auto MatDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActor->GetResourceType());

	auto MatInstance = WorldInventory->FindItemInstance(MatDefinition);

	bool bBuildFree = PlayerController->DoesBuildFree();

	// LOG_INFO(LogDev, "MatInstance->GetItemEntry()->GetCount(): {}", MatInstance->GetItemEntry()->GetCount());

	int MinimumMaterial = 10;
	bool bShouldDestroy = MatInstance && MatInstance->GetItemEntry() ? MatInstance->GetItemEntry()->GetCount() < MinimumMaterial : true;

	if (bShouldDestroy && !bBuildFree)
	{
		ExistingBuildings.Free();
		BuildingActor->SilentDie();
		return ServerCreateBuildingActorOriginal(Context, Stack, Ret);
	}

	for (int i = 0; i < ExistingBuildings.Num(); ++i)
	{
		auto ExistingBuilding = ExistingBuildings.At(i);

		ExistingBuilding->K2_DestroyActor();
	}

	ExistingBuildings.Free();

	BuildingActor->SetPlayerPlaced(true);
	BuildingActor->InitializeBuildingActor(PlayerController, BuildingActor, true);
	BuildingActor->SetTeam(PlayerStateAthena->GetTeamIndex()); // required?

	if (!bBuildFree)
	{
		bool bShouldUpdate = false;
		WorldInventory->RemoveItem(MatInstance->GetItemEntry()->GetItemGuid(), &bShouldUpdate, 10);

		if (bShouldUpdate)
			WorldInventory->Update();
	}

	/*

	GET_PLAYLIST(GameState);

	if (CurrentPlaylist)
	{
		// CurrentPlaylist->ApplyModifiersToActor(BuildingActor); // seems automatic
	} */

	return ServerCreateBuildingActorOriginal(Context, Stack, Ret);
}

AActor* AFortPlayerController::SpawnToyInstanceHook(UObject* Context, FFrame* Stack, AActor** Ret)
{
	LOG_INFO(LogDev, "SpawnToyInstance!");

	auto PlayerController = Cast<AFortPlayerController>(Context);

	UClass* ToyClass = nullptr;
	FTransform SpawnPosition;

	Stack->StepCompiledIn(&ToyClass);
	Stack->StepCompiledIn(&SpawnPosition);

	SpawnToyInstanceOriginal(Context, Stack, Ret);

	if (!ToyClass)
		return nullptr;

	auto Params = CreateSpawnParameters(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, false, PlayerController);
	auto NewToy = GetWorld()->SpawnActor<AActor>(ToyClass, SpawnPosition, Params);
	// free(Params); // ?

	static auto ActiveToyInstancesOffset = PlayerController->GetOffset("ActiveToyInstances");
	auto& ActiveToyInstances = PlayerController->Get<TArray<AActor*>>(ActiveToyInstancesOffset);
	
	static auto ToySummonCountsOffset = PlayerController->GetOffset("ToySummonCounts");
	auto& ToySummonCounts = PlayerController->Get<TMap<UClass*, int>>(ToySummonCountsOffset);

	// ActiveToyInstances.Add(NewToy);

	*Ret = NewToy;
	return *Ret;
}

void AFortPlayerController::DropSpecificItemHook(UObject* Context, FFrame& Stack, void* Ret)
{
	UFortItemDefinition* DropItemDef = nullptr;

	Stack.StepCompiledIn(&DropItemDef);

	if (!DropItemDef)
		return;

	auto PlayerController = Cast<AFortPlayerController>(Context);

	if (!PlayerController)
		return DropSpecificItemOriginal(Context, Stack, Ret);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return DropSpecificItemOriginal(Context, Stack, Ret);

	auto ItemInstance = WorldInventory->FindItemInstance(DropItemDef);

	if (!ItemInstance)
		return DropSpecificItemOriginal(Context, Stack, Ret);

	PlayerController->ServerAttemptInventoryDropHook(PlayerController, ItemInstance->GetItemEntry()->GetItemGuid(), ItemInstance->GetItemEntry()->GetCount());

	return DropSpecificItemOriginal(Context, Stack, Ret);
}

void AFortPlayerController::ServerAttemptInventoryDropHook(AFortPlayerController* PlayerController, FGuid ItemGuid, int Count)
{

	LOG_INFO(LogDev, "ServerAttemptInventoryDropHook Dropping: {}", Count);

	auto Pawn = PlayerController->GetMyFortPawn();

	if (Count < 0 || !Pawn)
		return;

	if (auto PlayerControllerAthena = Cast<AFortPlayerControllerAthena>(PlayerController))
	{
		if (PlayerControllerAthena->IsInGhostMode())
			return;
	}

	// TODO If the player is in a vehicle and has a vehicle weapon, don't let them drop.

	auto WorldInventory = PlayerController->GetWorldInventory();
	auto ReplicatedEntry = WorldInventory->FindReplicatedEntry(ItemGuid);

	if (!ReplicatedEntry || ReplicatedEntry->GetCount() < Count)
		return;

	auto ItemDefinition = Cast<UFortWorldItemDefinition>(ReplicatedEntry->GetItemDefinition());

	if (!ItemDefinition || !ItemDefinition->CanBeDropped())
		return;

	static auto DropBehaviorOffset = ItemDefinition->GetOffset("DropBehavior", false);

	EWorldItemDropBehavior DropBehavior = DropBehaviorOffset != -1 ? ItemDefinition->GetDropBehavior() : EWorldItemDropBehavior::EWorldItemDropBehavior_MAX;

	if (!ItemDefinition->ShouldIgnoreRespawningOnDrop() && DropBehavior != EWorldItemDropBehavior::DestroyOnDrop)
	{
		PickupCreateData CreateData;
		CreateData.ItemEntry = ReplicatedEntry;
		CreateData.SpawnLocation = Pawn->GetActorLocation();
		CreateData.bToss = true;
		CreateData.OverrideCount = Count;
		CreateData.PawnOwner = Pawn;
		CreateData.bRandomRotation = true;
		CreateData.SourceType = EFortPickupSourceTypeFlag::GetPlayerValue();
		CreateData.bShouldFreeItemEntryWhenDeconstructed = false;

		auto Pickup = AFortPickup::SpawnPickup(CreateData);

		if (!Pickup)
			return;
	}

	bool bShouldUpdate = false;

	if (!WorldInventory->RemoveItem(ItemGuid, &bShouldUpdate, Count, true, DropBehavior == EWorldItemDropBehavior::DropAsPickupDestroyOnEmpty))
		return;

	if (bShouldUpdate)
		WorldInventory->Update();
}

void AFortPlayerController::ServerPlayEmoteItemHook(AFortPlayerController* PlayerController, UObject* EmoteAsset)
{
	auto PlayerState = (AFortPlayerStateAthena*)PlayerController->GetPlayerState();
	auto Pawn = PlayerController->GetPawn();

	if (!EmoteAsset || !PlayerState || !Pawn)
		return;

	auto AbilitySystemComponent = PlayerState->GetAbilitySystemComponent();

	if (!AbilitySystemComponent)
		return;

	UObject* AbilityToUse = nullptr;

	static auto AthenaSprayItemDefinitionClass = FindObject<UClass>(L"/Script/FortniteGame.AthenaSprayItemDefinition");
	static auto AthenaToyItemDefinitionClass = FindObject<UClass>(L"/Script/FortniteGame.AthenaToyItemDefinition");

	if (EmoteAsset->IsA(AthenaSprayItemDefinitionClass))
	{
		static auto SprayGameplayAbilityDefault = FindObject(L"/Game/Abilities/Sprays/GAB_Spray_Generic.Default__GAB_Spray_Generic_C");
		AbilityToUse = SprayGameplayAbilityDefault;
	}

	else if (EmoteAsset->IsA(AthenaToyItemDefinitionClass))
	{
		static auto ToySpawnAbilityOffset = EmoteAsset->GetOffset("ToySpawnAbility");
		auto& ToySpawnAbilitySoft = EmoteAsset->Get<TSoftObjectPtr<UClass>>(ToySpawnAbilityOffset);

		static auto BGAClass = FindObject<UClass>(L"/Script/Engine.BlueprintGeneratedClass");

		auto ToySpawnAbility = ToySpawnAbilitySoft.Get(BGAClass, true);

		if (ToySpawnAbility)
			AbilityToUse = ToySpawnAbility->CreateDefaultObject();
	}

	// LOG_INFO(LogDev, "Before AbilityToUse: {}", AbilityToUse ? AbilityToUse->GetFullName() : "InvalidObject");

	if (!AbilityToUse)
	{
		static auto EmoteGameplayAbilityDefault = FindObject(L"/Game/Abilities/Emotes/GAB_Emote_Generic.Default__GAB_Emote_Generic_C");
		AbilityToUse = EmoteGameplayAbilityDefault;
	}

	if (!AbilityToUse)
		return;

	static auto AthenaDanceItemDefinitionClass = FindObject<UClass>(L"/Script/FortniteGame.AthenaDanceItemDefinition");

	if (EmoteAsset->IsA(AthenaDanceItemDefinitionClass))
	{
		static auto EmoteAsset_bMovingEmoteOffset = EmoteAsset->GetOffset("bMovingEmote", false);
		static auto bMovingEmoteOffset = Pawn->GetOffset("bMovingEmote", false);

		if (bMovingEmoteOffset != -1 && EmoteAsset_bMovingEmoteOffset != -1)
		{
			static auto bMovingEmoteFieldMask = GetFieldMask(Pawn->GetProperty("bMovingEmote"));
			static auto EmoteAsset_bMovingEmoteFieldMask = GetFieldMask(EmoteAsset->GetProperty("bMovingEmote"));
			Pawn->SetBitfieldValue(bMovingEmoteOffset, bMovingEmoteFieldMask, EmoteAsset->ReadBitfieldValue(EmoteAsset_bMovingEmoteOffset, EmoteAsset_bMovingEmoteFieldMask));
		}

		static auto bMoveForwardOnlyOffset = EmoteAsset->GetOffset("bMoveForwardOnly", false);
		static auto bMovingEmoteForwardOnlyOffset = Pawn->GetOffset("bMovingEmoteForwardOnly", false);

		if (bMovingEmoteForwardOnlyOffset != -1 && bMoveForwardOnlyOffset != -1)
		{
			static auto bMovingEmoteForwardOnlyFieldMask = GetFieldMask(Pawn->GetProperty("bMovingEmoteForwardOnly"));
			static auto bMoveForwardOnlyFieldMask = GetFieldMask(EmoteAsset->GetProperty("bMoveForwardOnly"));
			Pawn->SetBitfieldValue(bMovingEmoteOffset, bMovingEmoteForwardOnlyFieldMask, EmoteAsset->ReadBitfieldValue(bMoveForwardOnlyOffset, bMoveForwardOnlyFieldMask));
		}

		static auto WalkForwardSpeedOffset = EmoteAsset->GetOffset("WalkForwardSpeed", false);
		static auto EmoteWalkSpeedOffset = Pawn->GetOffset("EmoteWalkSpeed", false);

		if (EmoteWalkSpeedOffset != -1 && WalkForwardSpeedOffset != -1)
		{
			Pawn->Get<float>(EmoteWalkSpeedOffset) = EmoteAsset->Get<float>(WalkForwardSpeedOffset);
		}
	}

	int outHandle = 0;

	FGameplayAbilitySpec* Spec = MakeNewSpec((UClass*)AbilityToUse, EmoteAsset, true);

	if (!Spec)
		return;

	static unsigned int* (*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ASC, int* outHandle, __int64 Spec, FGameplayEventData* TriggerEventData) = decltype(GiveAbilityAndActivateOnce)(Addresses::GiveAbilityAndActivateOnce); // EventData is only on ue500?

	if (GiveAbilityAndActivateOnce)
	{
		GiveAbilityAndActivateOnce(AbilitySystemComponent, &outHandle, __int64(Spec), nullptr);
	}
}

DWORD WINAPI SpectateThread(LPVOID PC)
{
	auto PlayerController = (UObject*)PC;

	if (!PlayerController->IsValidLowLevel())
		return 0;

	auto SpectatingPC = Cast<AFortPlayerControllerAthena>(PlayerController);

	if (!SpectatingPC)
		return 0;

	Sleep(3000);

	LOG_INFO(LogDev, "bugha!");

	SpectatingPC->SpectateOnDeath();

	return 0;
}

uint8 ToDeathCause(const FGameplayTagContainer& TagContainer, bool bWasDBNO = false, AFortPawn* Pawn = nullptr)
{
	static auto ToDeathCauseFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerStateAthena.ToDeathCause");

	if (ToDeathCauseFn)
	{
		struct
		{
			FGameplayTagContainer                       InTags;                                                   // (ConstParm, Parm, OutParm, ReferenceParm, NativeAccessSpecifierPublic)
			bool                                               bWasDBNO;                                                 // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			uint8_t                                        ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		} AFortPlayerStateAthena_ToDeathCause_Params{ TagContainer, bWasDBNO };

		AFortPlayerStateAthena::StaticClass()->ProcessEvent(ToDeathCauseFn, &AFortPlayerStateAthena_ToDeathCause_Params);

		return AFortPlayerStateAthena_ToDeathCause_Params.ReturnValue;
	}

	static bool bHaveFoundAddress = false;

	static uint64 Addr = 0;

	if (!bHaveFoundAddress)
	{
		bHaveFoundAddress = true;

		if (Engine_Version == 419)
			Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 41 0F B6 F8 48 8B DA 48 8B F1 E8 ? ? ? ? 33 ED").Get();
		if (Engine_Version == 420)
			Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 0F B6 FA 48 8B D9 E8 ? ? ? ? 33 F6 48 89 74 24").Get();
		if (Engine_Version == 421) // 5.1
			Addr = Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 0F B6 FA 48 8B D9 E8 ? ? ? ? 33").Get();

		if (!Addr)
		{
			LOG_WARN(LogPlayer, "Failed to find ToDeathCause address!");
			return 0;
		}
	}

	if (!Addr)
	{
		return 0;
	}

	if (Engine_Version == 419)
	{
		static uint8(*sub_7FF7AB499410)(AFortPawn * Pawn, FGameplayTagContainer TagContainer, char bWasDBNOIg) = decltype(sub_7FF7AB499410)(Addr);
		return sub_7FF7AB499410(Pawn, TagContainer, bWasDBNO);
	}

	static uint8(*sub_7FF7AB499410)(FGameplayTagContainer TagContainer, char bWasDBNOIg) = decltype(sub_7FF7AB499410)(Addr);
	return sub_7FF7AB499410(TagContainer, bWasDBNO);
}

DWORD WINAPI RestartThread(LPVOID)
{
	// We should probably use unreal engine's timing system for this.
	// There is no way to restart that I know of without closing the connection to the clients.

	bIsInAutoRestart = true;

	float SecondsBeforeRestart = 10;
	Sleep(SecondsBeforeRestart * 1000);

	LOG_INFO(LogDev, "Auto restarting!");

	Restart();

	bIsInAutoRestart = false;

	return 0;
}

void AFortPlayerController::ClientOnPawnDiedHook(AFortPlayerController* PlayerController, void* DeathReport)
{
	auto GameState = Cast<AFortGameStateAthena>(((AFortGameMode*)GetWorld()->GetGameMode())->GetGameState());
	auto DeadPawn = Cast<AFortPlayerPawn>(PlayerController->GetPawn());
	auto DeadPlayerState = Cast<AFortPlayerStateAthena>(PlayerController->GetPlayerState());
	auto KillerPawn = Cast<AFortPlayerPawn>(*(AFortPawn**)(__int64(DeathReport) + MemberOffsets::DeathReport::KillerPawn));
	auto KillerPlayerState = Cast<AFortPlayerStateAthena>(*(AFortPlayerState**)(__int64(DeathReport) + MemberOffsets::DeathReport::KillerPlayerState));
	static auto World_NetDriverOffset = GetWorld()->GetOffset("NetDriver");
	auto WorldNetDriver = GetWorld()->Get<UNetDriver*>(World_NetDriverOffset);
	auto& ClientConnections = WorldNetDriver->GetClientConnections();

	if (!DeadPawn || !GameState || !DeadPlayerState)
		return ClientOnPawnDiedOriginal(PlayerController, DeathReport);

	auto DeathLocation = DeadPawn->GetActorLocation();

	static auto FallDamageEnumValue = 1;

	uint8_t DeathCause = 0;

	if (Fortnite_Version > 1.8 || Fortnite_Version == 1.11)
	{
		AFortPlayerStateAthena* LastPlayerState = nullptr;
		AFortPlayerController* LastPlayerController = nullptr;

		for (int z = 0; z < ClientConnections.Num(); ++z)
		{
			auto ClientConnection = ClientConnections.at(z);
			auto FortPC = Cast<AFortPlayerController>(ClientConnection->GetPlayerController());

			if (!FortPC)
				continue;

			auto PlayerState = Cast<AFortPlayerStateAthena>(FortPC->GetPlayerState());

			if (PlayerState)
			{
				LastPlayerState = PlayerState;
				LastPlayerController = FortPC;
				break;
			}
		}

		auto DeathInfo = DeadPlayerState->GetDeathInfo(); // Alloc<void>(DeathInfoStructSize);
		DeadPlayerState->ClearDeathInfo();

		auto Tags = MemberOffsets::FortPlayerPawn::CorrectTags == 0 ? FGameplayTagContainer()
			: DeadPawn->Get<FGameplayTagContainer>(MemberOffsets::FortPlayerPawn::CorrectTags);

		DeathCause = ToDeathCause(Tags, false, DeadPawn);

		FGameplayTagContainer CopyTags;

		for (int i = 0; i < Tags.GameplayTags.Num(); ++i)
		{
			CopyTags.GameplayTags.Add(Tags.GameplayTags.at(i));
		}

		for (int i = 0; i < Tags.ParentTags.Num(); ++i)
		{
			CopyTags.ParentTags.Add(Tags.ParentTags.at(i));
		}

		LOG_INFO(LogDev, "DeathCause: {}", (int)DeathCause);
		LOG_INFO(LogDev, "DeadPawn->IsDBNO(): {}", DeadPawn->IsDBNO());
		LOG_INFO(LogDev, "KillerPlayerState: {}", __int64(KillerPlayerState));

		*(bool*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::bDBNO) = DeadPawn->IsDBNO();
		*(uint8*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::DeathCause) = DeathCause;
		*(AActor**)(__int64(DeathInfo) + MemberOffsets::DeathInfo::FinisherOrDowner) = KillerPlayerState ? KillerPlayerState : DeadPlayerState;

		if (MemberOffsets::DeathInfo::DeathLocation != -1)
			*(FVector*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::DeathLocation) = DeathLocation;

		if (MemberOffsets::DeathInfo::DeathTags != -1)
			*(FGameplayTagContainer*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::DeathTags) = CopyTags;

		if (MemberOffsets::DeathInfo::bInitialized != -1)
			*(bool*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::bInitialized) = true;

		if (DeathCause == FallDamageEnumValue)
		{
			if (MemberOffsets::FortPlayerPawnAthena::LastFallDistance != -1)
				*(float*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::Distance) = DeadPawn->Get<float>(MemberOffsets::FortPlayerPawnAthena::LastFallDistance);
		}
		else
		{
			if (MemberOffsets::DeathInfo::Distance != -1)
				*(float*)(__int64(DeathInfo) + MemberOffsets::DeathInfo::Distance) = KillerPawn ? KillerPawn->GetDistanceTo(DeadPawn) : 0;
		}

		if (MemberOffsets::FortPlayerState::PawnDeathLocation != -1)
			DeadPlayerState->Get<FVector>(MemberOffsets::FortPlayerState::PawnDeathLocation) = DeathLocation;

		static auto OnRep_DeathInfoFn = FindObject<UFunction>(L"/Script/FortniteGame.FortPlayerStateAthena.OnRep_DeathInfo");

		if (OnRep_DeathInfoFn)
		{
			DeadPlayerState->ProcessEvent(OnRep_DeathInfoFn);
		}

		if (KillerPlayerState && KillerPlayerState != DeadPlayerState)
		{
			int KillScore;
			int TeamKillScore;

			if (MemberOffsets::FortPlayerStateAthena::KillScore != -1)
			{
				KillerPlayerState->Get<int>(MemberOffsets::FortPlayerStateAthena::KillScore)++;
				KillScore = KillerPlayerState->Get<int>(MemberOffsets::FortPlayerStateAthena::KillScore);
			}

			if (MemberOffsets::FortPlayerStateAthena::TeamKillScore != -1)
			{
				KillerPlayerState->Get<int>(MemberOffsets::FortPlayerStateAthena::TeamKillScore)++;
				TeamKillScore = KillerPlayerState->Get<int>(MemberOffsets::FortPlayerStateAthena::TeamKillScore);
			}

			auto amountOfKills = KillerPlayerState->Get<int>(MemberOffsets::FortPlayerStateAthena::TeamKillScore);
			KillerPlayerState->ClientReportKill(DeadPlayerState);
			KillerPlayerState->ClientReportTeamKill(amountOfKills);

			if (Globals::bEnableScoringSystem)
			{
				static auto ScoreOffset = KillerPlayerState->GetOffset("Score");
				static auto TeamScoreOffset = KillerPlayerState->GetOffset("TeamScore");
				static auto TeamScorePlacementOffset = KillerPlayerState->GetOffset("TeamScorePlacement");
				static auto OldTotalScoreStatOffset = KillerPlayerState->GetOffset("OldTotalScoreStat");
				static auto TotalPlayerScoreOffset = KillerPlayerState->GetOffset("TotalPlayerScore");

				*(float*)(__int64(KillerPlayerState) + ScoreOffset) = KillScore;
				*(int32*)(__int64(KillerPlayerState) + TeamScoreOffset) = KillScore;
				int32& KillerStatePlacement = *(int32*)(__int64(KillerPlayerState) + TeamScorePlacementOffset);

				KillerStatePlacement = 1;
				*(int32*)(__int64(KillerPlayerState) + OldTotalScoreStatOffset) = KillScore;
				*(int32*)(__int64(KillerPlayerState) + TotalPlayerScoreOffset) = KillScore;
				GameState->GetCurrentHighScoreTeam() = 3;
				GameState->GetCurrentHighScore() = KillScore;
				GameState->OnRep_CurrentHighScore();
				GameState->GetWinningScore() = KillScore;
				GameState->GetWinningTeam() = 3;
				GameState->OnRep_WinningTeam();
				GameState->OnRep_WinningScore();
				KillerPlayerState->OnRep_Score();
				KillerPlayerState->OnRep_TeamScore();
				KillerPlayerState->OnRep_TeamScorePlacement();
				KillerPlayerState->OnRep_TotalPlayerScore();
				KillerPlayerState->UpdateScoreStatChanged();
			}

			auto DeadControllerAthena = Cast<AFortPlayerControllerAthena>(PlayerController);
			auto KillerControllerAthena = Cast<AFortPlayerControllerAthena>(KillerPlayerState->GetOwner());

			KillerControllerAthena->GiveAccolade(KillerControllerAthena, GetDefFromEvent(EAccoladeEvent::Kill, KillScore));

			// LOG_INFO(LogDev, "Reported kill.");

			if (Globals::bRefreshMatsOnKill && Globals::Refreshmatsonpawn > 0)
			{
				auto WorldInventory = PlayerController->GetWorldInventory();

				if (!WorldInventory)
					return;

				static auto WoodItemData = FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/WoodItemData.WoodItemData");
				static auto StoneItemData = FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/StoneItemData.StoneItemData");
				static auto MetalItemData = FindObject<UFortItemDefinition>(L"/Game/Items/ResourcePickups/MetalItemData.MetalItemData");

				WorldInventory->AddItem(WoodItemData, nullptr, Globals::Refreshmatsonpawn);
				WorldInventory->AddItem(StoneItemData, nullptr, Globals::Refreshmatsonpawn);
				WorldInventory->AddItem(MetalItemData, nullptr, Globals::Refreshmatsonpawn);

				WorldInventory->Update();
			}

			if (AmountOfHealthSiphon != 0)
			{
				if (KillerPawn && KillerPawn != DeadPawn)
				{
					float Health = KillerPawn->GetHealth();
					float Shield = KillerPawn->GetShield();

					int MaxHealth = 100;
					int MaxShield = 100;
					int AmountGiven = 0;
					/*
					int ShieldGiven = 0;
					int HealthGiven = 0;
					*/

					if ((MaxHealth - Health) > 0)
					{
						int AmountToGive = MaxHealth - Health >= AmountOfHealthSiphon ? AmountOfHealthSiphon : MaxHealth - Health;
						KillerPawn->SetHealth(Health + AmountToGive);
						AmountGiven += AmountToGive;
					}

					if ((MaxShield - Shield) > 0 && AmountGiven < AmountOfHealthSiphon)
					{
						int AmountToGive = MaxShield - Shield >= AmountOfHealthSiphon ? AmountOfHealthSiphon : MaxShield - Shield;
						AmountToGive -= AmountGiven;

						if (AmountToGive > 0)
						{
							KillerPawn->SetShield(Shield + AmountToGive);
							AmountGiven += AmountToGive;
						}
					}

					if (AmountGiven > 0)
					{

					}
				}
			}
		}

		if (KillerPawn)
		{
			auto KillerState = Cast<AFortPlayerStateAthena>(KillerPawn->GetPlayerState());
			if (KillerState)
			{
				KillerState->SiphonEffect();
			}
		}

		if (Fortnite_Version == 19.10)
		{
			if (Globals::bSpawnCrownOnKill && Globals::SpawnCrown > 0)
			{
				if (Globals::AlivePlayers == 1)
				{
					AFortPlayerStateAthena* LastPlayerState = nullptr;
					AFortPlayerController* LastPlayerController = nullptr;

					for (int z = 0; z < ClientConnections.Num(); ++z)
					{
						auto ClientConnection = ClientConnections.at(z);
						auto FortPC = Cast<AFortPlayerController>(ClientConnection->GetPlayerController());

						if (!FortPC)
							continue;

						auto PlayerState = Cast<AFortPlayerStateAthena>(FortPC->GetPlayerState());

						if (PlayerState)
						{
							LastPlayerState = PlayerState;
							LastPlayerController = FortPC;
							break;
						}
					}

					if (LastPlayerController)
					{
						auto WorldInventory = LastPlayerController->GetWorldInventory();

						if (WorldInventory)
						{
							static auto CrownSpawn = FindObject<UFortItemDefinition>(L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");

							WorldInventory->AddItem(CrownSpawn, nullptr, Globals::SpawnCrown);
							KillerPlayerState->GetPlayerName().ToString();
							WorldInventory->Update();

							//LOG_INFO(LogDev, "Crown Given to last player: %s", *LastPlayerState->GetPlayerName().ToString());
						}
					}
				}
			}
		}

		bool bIsRespawningAllowed = GameState->IsRespawningAllowed(DeadPlayerState);

		bool bDropInventory = true;

		LoopMutators([&](AFortAthenaMutator* Mutator)
			{
				if (auto FortAthenaMutator_InventoryOverride = Cast<AFortAthenaMutator_InventoryOverride>(Mutator))
				{
					if (FortAthenaMutator_InventoryOverride->GetDropAllItemsOverride(DeadPlayerState->GetTeamIndex()) == EAthenaLootDropOverride::ForceKeep)
					{
						bDropInventory = false;
					}
				}
			}
		);

		if (bDropInventory
			&& !bIsRespawningAllowed
			)
		{
			auto WorldInventory = PlayerController->GetWorldInventory();

			if (WorldInventory)
			{
				auto& ItemInstances = WorldInventory->GetItemList().GetItemInstances();

				std::vector<std::pair<FGuid, int>> GuidAndCountsToRemove;

				for (int i = 0; i < ItemInstances.Num(); ++i)
				{
					auto ItemInstance = ItemInstances.at(i);

					// LOG_INFO(LogDev, "[{}/{}] CurrentItemInstance {}", i, ItemInstances.Num(), __int64(ItemInstance));

					if (!ItemInstance)
						continue;

					auto ItemEntry = ItemInstance->GetItemEntry();
					auto WorldItemDefinition = Cast<UFortWorldItemDefinition>(ItemEntry->GetItemDefinition());

					// LOG_INFO(LogDev, "[{}/{}] WorldItemDefinition {}", i, ItemInstances.Num(), WorldItemDefinition ? WorldItemDefinition->GetFullName() : "InvalidObject");

					if (!WorldItemDefinition)
						continue;

					auto ShouldBeDropped = WorldItemDefinition->CanBeDropped(); // WorldItemDefinition->ShouldDropOnDeath();

					// LOG_INFO(LogDev, "[{}/{}] ShouldBeDropped {}", i, ItemInstances.Num(), ShouldBeDropped);

					if (!ShouldBeDropped)
						continue;

					PickupCreateData CreateData;
					CreateData.PawnOwner = DeadPawn;
					CreateData.bToss = true;
					CreateData.ItemEntry = ItemEntry;
					CreateData.SourceType = EFortPickupSourceTypeFlag::GetPlayerValue();
					CreateData.Source = EFortPickupSpawnSource::GetPlayerEliminationValue();
					CreateData.SpawnLocation = DeathLocation;

					AFortPickup::SpawnPickup(CreateData);

					GuidAndCountsToRemove.push_back({ ItemEntry->GetItemGuid(), ItemEntry->GetCount() });
					// WorldInventory->RemoveItem(ItemEntry->GetItemGuid(), nullptr, ItemEntry->GetCount());
				}

				for (auto& Pair : GuidAndCountsToRemove)
				{
					WorldInventory->RemoveItem(Pair.first, nullptr, Pair.second, true);
				}

				WorldInventory->Update();
			}
		}

		if (!bIsRespawningAllowed)
		{
			auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());

			LOG_INFO(LogDev, "PlayersLeft: {} IsDBNO: {}", GameState->GetPlayersLeft(), DeadPawn->IsDBNO());

			if (!DeadPawn->IsDBNO())
			{
				if (bHandleDeath)
				{
					if (Fortnite_Version > 1.8 || Fortnite_Version == 1.11)
					{
						static void (*RemoveFromAlivePlayers)(AFortGameModeAthena * GameMode, AFortPlayerController * PlayerController, APlayerState * PlayerState, APawn * FinisherPawn,
							UFortWeaponItemDefinition * FinishingWeapon, uint8_t DeathCause, char a7)
							= decltype(RemoveFromAlivePlayers)(Addresses::RemoveFromAlivePlayers);

						AActor* DamageCauser = *(AActor**)(__int64(DeathReport) + MemberOffsets::DeathReport::DamageCauser);
						UFortWeaponItemDefinition* KillerWeaponDef = nullptr;

						static auto FortProjectileBaseClass = FindObject<UClass>(L"/Script/FortniteGame.FortProjectileBase");

						if (DamageCauser)
						{
							if (DamageCauser->IsA(FortProjectileBaseClass))
							{
								auto Owner = Cast<AFortWeapon>(DamageCauser->GetOwner());
								KillerWeaponDef = Owner->IsValidLowLevel() ? Owner->GetWeaponData() : nullptr; // I just added the IsValidLowLevel check because what if the weapon destroys (idk)?
							}
							if (auto Weapon = Cast<AFortWeapon>(DamageCauser))
							{
								KillerWeaponDef = Weapon->GetWeaponData();
							}
						}

						if (GameMode->GetAlivePlayers().Num() == 50)
						{
							for (int i = 0; i < GameMode->GetAlivePlayers().Num(); i++)
							{
								auto PlayerController = GameMode->GetAlivePlayers().at(i);
								PlayerController->GiveAccolade(PlayerController, FindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_026_Survival_Default_Bronze.AccoladeId_026_Survival_Default_Bronze"));
							}
						}
						if (GameMode->GetAlivePlayers().Num() == 25)
						{
							for (int i = 0; i < GameMode->GetAlivePlayers().Num(); i++)
							{
								auto PlayerController = GameMode->GetAlivePlayers().at(i);
								PlayerController->GiveAccolade(PlayerController, FindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_027_Survival_Default_Silver.AccoladeId_027_Survival_Default_Silver"));
							}
						}
						if (GameMode->GetAlivePlayers().Num() == 10)
						{
							for (int i = 0; i < GameMode->GetAlivePlayers().Num(); i++)
							{
								auto PlayerController = GameMode->GetAlivePlayers().at(i);
								PlayerController->GiveAccolade(PlayerController, FindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_028_Survival_Default_Gold.AccoladeId_028_Survival_Default_Gold"));
							}
						}

						RemoveFromAlivePlayers(GameMode, PlayerController, KillerPlayerState == DeadPlayerState ? nullptr : KillerPlayerState, KillerPawn, KillerWeaponDef, DeathCause, 0);

						LOG_INFO(LogDev, "Removed!");
					}
				}
			}

			if (Fortnite_Version < 6)
			{
				if (GameState->GetGamePhase() > EAthenaGamePhase::Warmup)
				{
					static auto bAllowSpectateAfterDeathOffset = GameMode->GetOffset("bAllowSpectateAfterDeath");
					bool bAllowSpectate = GameMode->Get<bool>(bAllowSpectateAfterDeathOffset);

					LOG_INFO(LogDev, "bAllowSpectate: {}", bAllowSpectate);

					if (bAllowSpectate)
					{
						LOG_INFO(LogDev, "Starting Spectating!");

						static auto PlayerToSpectateOnDeathOffset = PlayerController->GetOffset("PlayerToSpectateOnDeath");
						PlayerController->Get<APawn*>(PlayerToSpectateOnDeathOffset) = KillerPawn;

						UKismetSystemLibrary::K2_SetTimer(PlayerController, L"SpectateOnDeath", 5.f, false); // Soo proper its scary
					}
				}
			}

			if (IsRestartingSupported() && Globals::bAutoRestart && !bIsInAutoRestart)
			{
				// wht

				if (GameState->GetGamePhase() > EAthenaGamePhase::Warmup)
				{
					auto AllPlayerStates = UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFortPlayerStateAthena::StaticClass());

					bool bDidSomeoneWin = AllPlayerStates.Num() == 0;

					for (int i = 0; i < AllPlayerStates.Num(); ++i)
					{
						auto CurrentPlayerState = (AFortPlayerStateAthena*)AllPlayerStates.at(i);

						if (CurrentPlayerState->GetPlace() <= 1)
						{
							bDidSomeoneWin = true;
							break;
						}
					}

					if (bDidSomeoneWin)
					{
						CreateThread(0, 0, RestartThread, 0, 0, 0);
					}
				}
			}
		}

		if (DeadPlayerState->IsBot())
		{
			// AllPlayerBotsToTick.remov3lbah
		}

		DeadPlayerState->EndDBNOAbilities();

		return ClientOnPawnDiedOriginal(PlayerController, DeathReport);
	}
}

void AFortPlayerController::ServerBeginEditingBuildingActorHook(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit)
{
	if (!BuildingActorToEdit || !BuildingActorToEdit->IsPlayerPlaced()) // We need more checks.
		return;

	auto Pawn = PlayerController->GetMyFortPawn();

	if (!Pawn)
		return;

	auto PlayerState = PlayerController->GetPlayerState();

	if (!PlayerState)
		return;

	BuildingActorToEdit->SetEditingPlayer(PlayerState);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	static auto EditToolDef = FindObject<UFortWeaponItemDefinition>(L"/Game/Items/Weapons/BuildingTools/EditTool.EditTool");

	auto EditToolInstance = WorldInventory->FindItemInstance(EditToolDef);

	if (!EditToolInstance)
		return;

	Pawn->EquipWeaponDefinition(EditToolDef, EditToolInstance->GetItemEntry()->GetItemGuid());

	auto EditTool = Cast<AFortWeap_EditingTool>(Pawn->GetCurrentWeapon());

	if (!EditTool)
		return;

	EditTool->GetEditActor() = BuildingActorToEdit;
	EditTool->OnRep_EditActor();
}

void AFortPlayerController::ServerEditBuildingActorHook(UObject* Context, FFrame& Stack, void* Ret)
{
	auto PlayerController = (AFortPlayerController*)Context;

	auto PlayerState = (AFortPlayerState*)PlayerController->GetPlayerState();

	auto Params = Stack.Locals;

	static auto RotationIterationsOffset = FindOffsetStruct("/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor", "RotationIterations");
	static auto NewBuildingClassOffset = FindOffsetStruct("/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor", "NewBuildingClass");
	static auto BuildingActorToEditOffset = FindOffsetStruct("/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor", "BuildingActorToEdit");
	static auto bMirroredOffset = FindOffsetStruct("/Script/FortniteGame.FortPlayerController.ServerEditBuildingActor", "bMirrored");

	auto BuildingActorToEdit = *(ABuildingSMActor**)(__int64(Params) + BuildingActorToEditOffset);
	auto NewBuildingClass = *(UClass**)(__int64(Params) + NewBuildingClassOffset);
	int RotationIterations = Fortnite_Version < 8.30 ? *(int*)(__int64(Params) + RotationIterationsOffset) : (int)(*(uint8*)(__int64(Params) + RotationIterationsOffset));
	auto bMirrored = *(char*)(__int64(Params) + bMirroredOffset);

	// LOG_INFO(LogDev, "RotationIterations: {}", RotationIterations);

	if (!BuildingActorToEdit || !NewBuildingClass || BuildingActorToEdit->IsDestroyed() || BuildingActorToEdit->GetEditingPlayer() != PlayerState)
	{
		// LOG_INFO(LogDev, "Cheater?");
		// LOG_INFO(LogDev, "BuildingActorToEdit->GetEditingPlayer(): {} PlayerState: {} NewBuildingClass: {} BuildingActorToEdit: {}", BuildingActorToEdit ? __int64(BuildingActorToEdit->GetEditingPlayer()) : -1, __int64(PlayerState), __int64(NewBuildingClass), __int64(BuildingActorToEdit));
		return ServerEditBuildingActorOriginal(Context, Stack, Ret);
	}

	// if (!PlayerState || PlayerState->GetTeamIndex() != BuildingActorToEdit->GetTeamIndex()) 
		//return ServerEditBuildingActorOriginal(Context, Frame, Ret);

	BuildingActorToEdit->SetEditingPlayer(nullptr);

	static ABuildingSMActor* (*BuildingSMActorReplaceBuildingActor)(ABuildingSMActor*, __int64, UClass*, int, int, uint8_t, AFortPlayerController*) =
		decltype(BuildingSMActorReplaceBuildingActor)(Addresses::ReplaceBuildingActor);

	if (auto BuildingActor = BuildingSMActorReplaceBuildingActor(BuildingActorToEdit, 1, NewBuildingClass,
		BuildingActorToEdit->GetCurrentBuildingLevel(), RotationIterations, bMirrored, PlayerController))
	{
		BuildingActor->SetPlayerPlaced(true);
	}

	// we should do more things here

	return ServerEditBuildingActorOriginal(Context, Stack, Ret);
}

void AFortPlayerController::ServerEndEditingBuildingActorHook(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToStopEditing)
{
	auto Pawn = PlayerController->GetMyFortPawn();

	if (!BuildingActorToStopEditing || !Pawn
		|| BuildingActorToStopEditing->GetEditingPlayer() != PlayerController->GetPlayerState()
		|| BuildingActorToStopEditing->IsDestroyed())
		return;

	BuildingActorToStopEditing->SetEditingPlayer(nullptr);

	static auto EditToolDef = FindObject<UFortWeaponItemDefinition>(L"/Game/Items/Weapons/BuildingTools/EditTool.EditTool");

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return;

	auto EditToolInstance = WorldInventory->FindItemInstance(EditToolDef);

	if (!EditToolInstance)
		return;

	Pawn->EquipWeaponDefinition(EditToolDef, EditToolInstance->GetItemEntry()->GetItemGuid());

	auto EditTool = Cast<AFortWeap_EditingTool>(Pawn->GetCurrentWeapon());

	BuildingActorToStopEditing->GetEditingPlayer() = nullptr;
	// BuildingActorToStopEditing->OnRep_EditingPlayer();

	if (EditTool)
	{
		EditTool->GetEditActor() = nullptr;
		EditTool->OnRep_EditActor();
	}
}