#pragma once

#include "Object.h"
#include "GameplayAbilitySpec.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

struct PadHex10 { char Pad[0x10]; };
struct PadHex18 { char Pad[0x18]; };
struct PadHexA8 { char Pad[0xA8]; };
struct PadHexB0 { char Pad[0xB0]; };

// using FPredictionKey = PadHex18;
// using FGameplayEventData = PadHexA8;

// using FPredictionKey = PadHex10; 
using FGameplayEventData = PadHexB0;

// using FGameplayEventData = __int64;

struct FPredictionKey // todo move
{
	// __int64 real;

	static UStruct* GetStruct()
	{
		static auto Struct = FindObject<UStruct>("/Script/GameplayAbilities.PredictionKey");
		return Struct;
	}

	static int GetStructSize() { return GetStruct()->GetPropertiesSize(); }
};

struct FGameplayAbilityActivationInfo // TODO Move
{
	static UStruct* GetStruct()
	{
		static auto Struct = FindObject<UStruct>("/Script/GameplayAbilities.GameplayAbilityActivationInfo");
		return Struct;
	}

	static int GetStructSize() { return GetStruct()->GetPropertiesSize(); }

	FPredictionKey* GetPredictionKeyWhenActivated()
	{
		static auto PredictionKeyWhenActivatedOffset = FindOffsetStruct("/Script/GameplayAbilities.GameplayAbilityActivationInfo", "PredictionKeyWhenActivated");
		return *(FPredictionKey**)(__int64(this) + PredictionKeyWhenActivatedOffset);
	}
};

struct FGameplayEffectContextHandle
{
	unsigned char                                      UnknownData00[0x18];                                      // 0x0000(0x0018) MISSED OFFSET
};

struct FActiveGameplayEffectHandle
{
	int                                                Handle;                                                   // 0x0000(0x0004) (ZeroConstructor, IsPlainOldData)
	bool                                               bPassedFiltersAndWasExecuted;                             // 0x0004(0x0001) (ZeroConstructor, IsPlainOldData)
	unsigned char                                      UnknownData00[0x3];                                       // 0x0005(0x0003) MISSED OFFSET
};

struct FGameplayAbilitySpecContainer : public FFastArraySerializer
{
	TArray<FGameplayAbilitySpec>& GetItems()
	{
		static auto ItemsOffset = FindOffsetStruct("/Script/GameplayAbilities.GameplayAbilitySpecContainer", "Items");
		return *(TArray<FGameplayAbilitySpec>*)(__int64(this) + ItemsOffset);
	}
};

struct FGameplayEventData2
{
public:
	FGameplayTag EventTag;                                          // 0x0(0x8)(Edit, BlueprintVisible, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	AActor* Instigator;                                        // 0x8(0x8)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	AActor* Target;                                            // 0x10(0x8)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	UObject* OptionalObject;                                    // 0x18(0x8)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	UObject* OptionalObject2;                                   // 0x20(0x8)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	FGameplayEffectContextHandle ContextHandle;                                     // 0x28(0x18)(Edit, BlueprintVisible, NativeAccessSpecifierPublic)
	FGameplayTagContainer InstigatorTags;                                    // 0x40(0x20)(Edit, BlueprintVisible, NativeAccessSpecifierPublic)
	FGameplayTagContainer TargetTags;                                        // 0x60(0x20)(Edit, BlueprintVisible, NativeAccessSpecifierPublic)
	float EventMagnitude;                                    // 0x80(0x4)(Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	uint8 Pad_9A5[0x4];                                      // Fixing Size After Last Property  [ Dumper-7 ]
	// FGameplayAbilityTargetDataHandle TargetData;                                        // 0x88(0x28)(Edit, BlueprintVisible, NativeAccessSpecifierPublic)
};

class UFortGameplayAbility_Sprint : public UObject // UFortGameplayAbility_Action
{
public:
	static UClass* StaticClass()
	{
		static auto Class = FindObject<UClass>(L"/Script/FortniteGame.FortGameplayAbility_Sprint");
		return Class;
	}
};

class UAbilitySystemComponent : public UObject
{
public:
	static inline FGameplayAbilitySpecHandle* (*GiveAbilityOriginal)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, __int64 inSpec);
	static inline bool (*InternalTryActivateAbilityOriginal)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle Handle, PadHex10 InPredictionKey, UObject** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData);
	static inline bool (*InternalTryActivateAbilityOriginal2)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle Handle, PadHex18 InPredictionKey, UObject** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData);

	void ClientActivateAbilityFailed(FGameplayAbilitySpecHandle AbilityToActivate, int16_t PredictionKey)
	{
		struct { FGameplayAbilitySpecHandle AbilityToActivate; int16_t PredictionKey; } UAbilitySystemComponent_ClientActivateAbilityFailed_Params{ AbilityToActivate, PredictionKey };
		static auto fn = FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ClientActivateAbilityFailed");

		this->ProcessEvent(fn, &UAbilitySystemComponent_ClientActivateAbilityFailed_Params);
	}

	void ServerTryActivateAbility(const FGameplayAbilitySpecHandle& AbilityToActivate, bool InputPressed, const FPredictionKey& PredictionKey)
	{
		static auto fn = FindObject<UFunction>(L"/Script/GameplayAbilities.AbilitySystemComponent.ServerTryActivateAbility");

		struct
		{
			FGameplayAbilitySpecHandle  AbilityToActivate;
			bool                               InputPressed;
			FPredictionKey              PredictionKey;
		}params{ AbilityToActivate , InputPressed , PredictionKey };

		this->ProcessEvent(fn, &params);
	}

	TArray<UAttributeSet*>& GetSpawnedAttributes()
	{
		static auto SpawnedAttributesOffset = GetOffset("SpawnedAttributes");
		return Get<TArray<UAttributeSet*>>(SpawnedAttributesOffset);
	}

	FGameplayAbilitySpecContainer* GetActivatableAbilities()
	{
		static auto ActivatableAbilitiesOffset = this->GetOffset("ActivatableAbilities");
		return GetPtr<FGameplayAbilitySpecContainer>(ActivatableAbilitiesOffset);
	}

	UAttributeSet* AddDefaultSubobjectSet(UAttributeSet* Subobject)
	{
		GetSpawnedAttributes().Add(Subobject);
		return Subobject;
	}

	void ServerEndAbility(FGameplayAbilitySpecHandle AbilityToEnd, FGameplayAbilityActivationInfo* ActivationInfo, FPredictionKey* PredictionKey);
	void ClientEndAbility(FGameplayAbilitySpecHandle AbilityToEnd, FGameplayAbilityActivationInfo* ActivationInfo);
	void ClientCancelAbility(FGameplayAbilitySpecHandle AbilityToCancel, FGameplayAbilityActivationInfo* ActivationInfo);
	bool HasAbility(UObject* DefaultAbility);
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(UClass* GameplayEffectClass, float Level, const FGameplayEffectContextHandle& EffectContext = FGameplayEffectContextHandle());
	// FGameplayEffectContextHandle MakeEffectContext();
	void RemoveActiveGameplayEffectBySourceEffect(UClass* GEClass, int StacksToRemove, UAbilitySystemComponent* Instigator);
	void ConsumeAllReplicatedData(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey* AbilityOriginalPredictionKey);
	FGameplayAbilitySpecHandle GiveAbilityEasy(UClass* AbilityClass, UObject* SourceObject = nullptr, bool bDoNotRegive = true);
	FGameplayAbilitySpec* FindAbilitySpecFromHandle(FGameplayAbilitySpecHandle Handle);
	void RemoveActiveGameplayEffectBySourceEffect(UClass* GameplayEffect, UAbilitySystemComponent* InstigatorAbilitySystemComponent, int StacksToRemove);
	void ClearAbility(const FGameplayAbilitySpecHandle& Handle);

	static void InternalServerTryActivateAbilityHook(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey* PredictionKey, const FGameplayEventData* TriggerEventData);
};

void LoopSpecs(UAbilitySystemComponent* AbilitySystemComponent, std::function<void(FGameplayAbilitySpec*)> func);