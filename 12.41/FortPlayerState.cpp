#include "FortPlayerState.h"

void AFortPlayerState::UpdateScoreStatChanged()
{
	static auto fn = FindObject<UFunction>("/Script/FortniteGame.FortPlayerState.UpdateScoreStatChanged");
	this->ProcessEvent(fn);
}

void AFortPlayerState::EndDBNOAbilities()
{
	static auto GAB_AthenaDBNOClass = FindObject<UClass>(L"/Game/Abilities/NPC/Generic/GAB_AthenaDBNO.Default__GAB_AthenaDBNO_C");

	auto ASC = this->GetAbilitySystemComponent();

	if (!ASC)
		return;

	FGameplayAbilitySpec* DBNOSpec = nullptr;

	UObject* ClassToFind = GAB_AthenaDBNOClass->ClassPrivate;

	auto compareAbilities = [&DBNOSpec, &ClassToFind](FGameplayAbilitySpec* Spec) {
		auto CurrentAbility = Spec->GetAbility();

		if (CurrentAbility->ClassPrivate == ClassToFind)
		{
			DBNOSpec = Spec;
			return;
		}
	};

	LoopSpecs(ASC, compareAbilities);

	if (!DBNOSpec)
		return;

	ASC->ClientCancelAbility(DBNOSpec->GetHandle(), DBNOSpec->GetActivationInfo());
	ASC->ClientEndAbility(DBNOSpec->GetHandle(), DBNOSpec->GetActivationInfo());
	ASC->ServerEndAbility(DBNOSpec->GetHandle(), DBNOSpec->GetActivationInfo(), nullptr);
}

void AFortPlayerState::SiphonEffect()
{
	static auto BGAClass = FindObject<UClass>(L"/Script/Engine.BlueprintGeneratedClass");
	UClass* GE_Class = LoadObject<UClass>(L"/Game/Athena/Items/Consumables/ChillBronco/GE_Athena_ChillBronco_Shields.GE_Athena_ChillBronco_Shields_C", BGAClass);
	GetAbilitySystemComponent()->ApplyGameplayEffectToSelf(GE_Class, -1.f);
}

bool AFortPlayerState::AreUniqueIDsIdentical(FUniqueNetIdRepl* A, FUniqueNetIdRepl* B)
{
	return A->IsIdentical(B);
}