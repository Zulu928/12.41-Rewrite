#include "BuildingActor.h"
#include "FortWeapon.h"
#include "BuildingSMActor.h"
#include "FortPlayerControllerAthena.h"
#include "FortPawn.h"
#include "FortWeaponMeleeItemDefinition.h"
#include "CurveTable.h"
#include "DataTable.h"
#include "FortResourceItemDefinition.h"
#include "FortKismetLibrary.h"
#include "DataTableFunctionLibrary.h"
#include "FortGameStateAthena.h"
#include "GameplayStatics.h"
#include "BuildingContainer.h"
#include "bots.h"

void ABuildingActor::OnDamageServerHook(ABuildingActor* BuildingActor, float Damage, FGameplayTagContainer DamageTags,
	FVector Momentum, /* FHitResult */ __int64 HitInfo, APlayerController* InstigatedBy, AActor* DamageCauser,
	/* FGameplayEffectContextHandle */ __int64 EffectContext)
{

	auto BuildingSMActor = Cast<ABuildingSMActor>(BuildingActor);
	auto PlayerController = Cast<AFortPlayerControllerAthena>(InstigatedBy);

	if (!BuildingSMActor) {

	}

	if (auto Container = Cast<ABuildingContainer>(BuildingActor))
	{
		if ((BuildingSMActor->GetHealth() <= 0 || BuildingActor->GetHealth() <= 0) && !Container->IsAlreadySearched())
		{
			LOG_INFO(LogDev, "It's a me, a buildingcontainer!");
			PlayerController->GiveAccolade(PlayerController, GetDefFromEvent(EAccoladeEvent::Search, Damage, BuildingActor));
			Container->SpawnLoot();
		}
	}
	AFortPlayerStateAthena* PS = PlayerController->GetPlayerStateAthena();
	auto AttachedBuildingActors = BuildingSMActor->GetAttachedBuildingActors();
	for (int i = 0; i < AttachedBuildingActors.Num(); ++i)
	{

		auto CurrentBuildingActor = AttachedBuildingActors.at(i);
		auto CurrentActor = Cast<ABuildingActor>(CurrentBuildingActor);
		if (BuildingSMActor->GetHealth() <= 0 || BuildingActor->GetHealth() <= 0)
		{
			if (auto Container = Cast<ABuildingContainer>(CurrentActor))
			{
				if (!Container->IsAlreadySearched())
				{
					PlayerController->GiveAccolade(PlayerController, GetDefFromEvent(EAccoladeEvent::Search, Damage, CurrentActor));
					Container->SpawnLoot();
					LOG_INFO(LogDev, "spawnloott");
				}
			}
		}
	}

	
	auto Weapon = Cast<AFortWeapon>(DamageCauser);
	if (!BuildingSMActor)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);


	if (BuildingSMActor->IsDestroyed())
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	if (!PlayerController || !Weapon)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto WeaponData = Cast<UFortWeaponMeleeItemDefinition>(Weapon->GetWeaponData());

	if (!WeaponData)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	UFortResourceItemDefinition* ItemDef = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->GetResourceType());

	if (!ItemDef)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	static auto BuildingResourceAmountOverrideOffset = BuildingSMActor->GetOffset("BuildingResourceAmountOverride");
	auto& BuildingResourceAmountOverride = BuildingSMActor->Get<FCurveTableRowHandle>(BuildingResourceAmountOverrideOffset);

	int ResourceCount = 0;


	if (BuildingResourceAmountOverride.RowName.IsValid())
	{
		UCurveTable* CurveTable = nullptr;

		if (!CurveTable)
			CurveTable = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

		{

			float Out = UDataTableFunctionLibrary::EvaluateCurveTableRow(CurveTable, BuildingResourceAmountOverride.RowName, 0.f);

			const float DamageThatWillAffect = Damage;

			float skid = Out / (BuildingActor->GetMaxHealth() / DamageThatWillAffect);

			ResourceCount = round(skid);
		}
	}

	if (ResourceCount <= 0)
	{
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	bool bIsWeakspot = Damage == 100.0f;
	if (bIsWeakspot)
	{
		PlayerController->GiveAccolade(PlayerController, FindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_066_WeakSpotsInARow.AccoladeId_066_WeakSpotsInARow"));
	}
	PlayerController->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->GetResourceType(), ResourceCount, false, bIsWeakspot);
	bool bShouldUpdate = false;
	int MaximumMaterial = Globals::MaxMats;
	auto MatDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->GetResourceType());
	auto MatInstance = WorldInventory->FindItemInstance(MatDefinition);
	if (!MatInstance) {
		WorldInventory->AddItem(ItemDef, &bShouldUpdate, ResourceCount);
		if (bShouldUpdate)
			WorldInventory->Update();
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}
	auto MatsAfterUpdate = MatInstance->GetItemEntry()->GetCount() + ResourceCount;
	FGuid MatsGUID = MatInstance->GetItemEntry()->GetItemGuid();
	WorldInventory->AddItem(ItemDef, &bShouldUpdate, ResourceCount);
	if (MatsAfterUpdate > MaximumMaterial)
	{
		auto Pawn = PlayerController->GetMyFortPawn();
		PickupCreateData CreateData;
		CreateData.ItemEntry = FFortItemEntry::MakeItemEntry(ItemDef, MatsAfterUpdate - MaximumMaterial, -1, MAX_DURABILITY);
		CreateData.SpawnLocation = Pawn->GetActorLocation();
		CreateData.PawnOwner = Cast<AFortPawn>(Pawn);
		CreateData.SourceType = EFortPickupSourceTypeFlag::GetPlayerValue();
		CreateData.bShouldFreeItemEntryWhenDeconstructed = true;

		AFortPickup::SpawnPickup(CreateData);
		WorldInventory->FindReplicatedEntry(MatsGUID)->GetCount() = MaximumMaterial;
	}

	if (bShouldUpdate)
		WorldInventory->Update();

	return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

UClass* ABuildingActor::StaticClass()
{
	static auto Class = FindObject<UClass>(L"/Script/FortniteGame.BuildingActor");
	return Class;
}