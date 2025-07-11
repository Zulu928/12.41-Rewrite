#pragma once

#include "OnlineReplStructs.h"
#include "PlayerState.h"
#include "AbilitySystemComponent.h"
#include "FortPlayerPawn.h"

class AFortPlayerState : public APlayerState
{
public:
	AFortTeamInfo* GetPlayerTeam()
	{
		static auto PlayerTeamOffset = GetOffset("PlayerTeam");
		return this->Get<AFortTeamInfo*>(PlayerTeamOffset);
	}

	UAbilitySystemComponent*& GetAbilitySystemComponent()
	{
		static auto AbilitySystemComponentOffset = GetOffset("AbilitySystemComponent");
		return this->Get<UAbilitySystemComponent*>(AbilitySystemComponentOffset);
	}

	int& GetWorldPlayerId()
	{
		static auto WorldPlayerIdOffset = GetOffset("WorldPlayerId");
		return this->Get<int>(WorldPlayerIdOffset);
	}

	AFortPlayerPawn* GetCurrentPawn()
	{
		static auto fn = FindObject<UFunction>("/Script/FortniteGame.FortPlayerState.GetCurrentPawn");

		AFortPlayerPawn* ReturnValue;

		this->ProcessEvent(fn, &ReturnValue);

		return ReturnValue;
	}

	void OnRep_CharacterData()
	{
		static auto fn = FindObject<UFunction>("/Script/FortniteGame.FortPlayerState.OnRep_CharacterData");
		this->ProcessEvent(fn);
	}

	void UpdateScoreStatChanged();

	void EndDBNOAbilities();

	void SiphonEffect();

	static bool AreUniqueIDsIdentical(FUniqueNetIdRepl* A, FUniqueNetIdRepl* B);

	static UClass* StaticClass()
	{
		static auto Class = FindObject<UClass>(L"/Script/FortniteGame.FortPlayerState");
		return Class;
	}
};