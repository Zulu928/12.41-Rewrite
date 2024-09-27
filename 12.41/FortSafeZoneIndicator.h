#pragma once

#include "Actor.h"
#include "FortPlayerStateAthena.h"
#include "Vector2D.h"
#include "Vector.h"
#include "NameTypes.h"
#include "KismetSystemLibrary.h"

enum class EFortSafeZoneState : uint8_t
{
	None = 0,
	Starting = 1,
	Holding = 2,
	Shrinking = 3,
	EFortSafeZoneState_MAX = 4
};

class AFortSafeZoneIndicator : public AActor
{
public:
	class UFortMiniMapComponent* MinimapComp;                                              // 0x0218(0x0008) (ExportObject, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              LastRadius;                                               // 0x0220(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              NextRadius;                                               // 0x0224(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              NextNextRadius;                                           // 0x0228(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct FVector_NetQuantize100                      LastCenter;                                               // 0x022C(0x000C) (BlueprintVisible, BlueprintReadOnly, Net, NoDestructor, NativeAccessSpecifierPublic)
	struct FVector_NetQuantize100                      NextCenter;                                               // 0x0238(0x000C) (BlueprintVisible, BlueprintReadOnly, Net, NoDestructor, NativeAccessSpecifierPublic)
	struct FVector_NetQuantize100                      NextNextCenter;                                           // 0x0244(0x000C) (BlueprintVisible, BlueprintReadOnly, Net, NoDestructor, NativeAccessSpecifierPublic)
	float                                              SafeZoneStartShrinkTime;                                  // 0x0250(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              SafeZoneFinishShrinkTime;                                 // 0x0254(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	bool                                               bSafezoneEventDriven;                                     // 0x0258(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	bool                                               bPaused;                                                  // 0x0259(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	bool                                               bPausedForPreview;                                        // 0x025A(0x0001) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	bool                                               bPausedForPreview_Previous;                               // 0x025B(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct FGameplayTag                                MegaStormGameplayCueTag;                                  // 0x025C(0x0008) (Edit, DisableEditOnInstance, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	int                                                NextNextMegaStormGridCellThickness;                       // 0x0264(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	int                                                NextMegaStormGridCellThickness;                           // 0x0268(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              MegaStormDelayTimeBeforeDestruction;                      // 0x026C(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, Transient, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	int                                                NumActiveMegaStormCircles;                                // 0x0270(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, Transient, IsPlainOldData, RepNotify, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	int                                                ActiveMegaStormCircleGridCellCountFromEdge;               // 0x0274(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, Transient, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	unsigned char                                      UnknownData00[0x10];                                      // 0x0278(0x0010) MISSED OFFSET                                 // 0x0310(0x0088) (Edit, DisableEditOnInstance, NativeAccessSpecifierPublic)
	unsigned char                                      UnknownData01[0x4];                                       // 0x0398(0x0004) MISSED OFFSET
	float                                              Radius;                                                   // 0x039C(0x0004) (BlueprintVisible, BlueprintReadOnly, Net, ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash, NativeAccessSpecifierProtected)                             // 0x0448(0x0018) MISSED OFFSET

	static UClass* StaticClass()
	{
		static auto Class = FindObject<UClass>("Class FortniteGame.FortSafeZoneIndicator");
		return Class;
	}

	static inline void (*OnSafeZoneStateChangeOriginal)(AFortSafeZoneIndicator* SafeZoneIndicator, EFortSafeZoneState NewState, bool bInitial);

	float& GetSafeZoneStartShrinkTime()
	{
		static auto SafeZoneStartShrinkTimeOffset = GetOffset("SafeZoneStartShrinkTime");
		return Get<float>(SafeZoneStartShrinkTimeOffset);
	}

	float& GetSafeZoneFinishShrinkTime()
	{
		static auto SafeZoneFinishShrinkTimeOffset = GetOffset("SafeZoneFinishShrinkTime");
		return Get<float>(SafeZoneFinishShrinkTimeOffset);
	}

	FVector_NetQuantize100& GetNextCenter()
	{
		static auto NextCenterOffset = GetOffset("NextCenter");
		return Get<FVector_NetQuantize100>(NextCenterOffset);
	}

	FVector_NetQuantize100& GetNextNextCenter()
	{
		static auto NextNextCenterOffset = GetOffset("NextNextCenter");
		return Get<FVector_NetQuantize100>(NextNextCenterOffset);
	}

	void SkipShrinkSafeZone();

	static void OnSafeZoneStateChangeHook(AFortSafeZoneIndicator* SafeZoneIndicator, EFortSafeZoneState NewState, bool bInitial);
};

template<class TEnum>
class TEnumAsByte
{
public:
	inline TEnumAsByte()
	{
	}

	inline TEnumAsByte(TEnum _value)
		: value(static_cast<uint8_t>(_value))
	{
	}

	explicit inline TEnumAsByte(int32_t _value)
		: value(static_cast<uint8_t>(_value))
	{
	}

	explicit inline TEnumAsByte(uint8_t _value)
		: value(_value)
	{
	}

	inline operator TEnum() const
	{
		return (TEnum)value;
	}

	inline TEnum GetValue() const
	{
		return (TEnum)value;
	}

private:
	uint8_t value = 0;
};

enum class ESlateColorStylingMode : uint8_t
{
	UseColor_Specified = 0,
	UseColor_Specified_Link = 1,
	UseColor_Foreground = 2,
	UseColor_Foreground_Subdued = 3,
	UseColor_MAX = 4
};

enum class ESlateBrushDrawType : uint8_t
{
	NoDrawType = 0,
	Box = 1,
	Border = 2,
	Image = 3,
	ESlateBrushDrawType_MAX = 4
};

enum class ESlateBrushTileType : uint8_t
{
	NoTile = 0,
	Horizontal = 1,
	Vertical = 2,
	Both = 3,
	ESlateBrushTileType_MAX = 4
};

enum class ESlateBrushMirrorType : uint8_t
{
	NoMirror = 0,
	Horizontal = 1,
	Vertical = 2,
	Both = 3,
	ESlateBrushMirrorType_MAX = 4
};

enum class ESlateBrushImageType : uint8_t
{
	NoImage = 0,
	FullColor = 1,
	Linear = 2,
	ESlateBrushImageType_MAX = 3
};

struct FSlateColor
{
	struct FLinearColor                                SpecifiedColor;                                           // 0x0000(0x0010) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash, NativeAccessSpecifierProtected)
	TEnumAsByte<ESlateColorStylingMode>                ColorUseRule;                                             // 0x0010(0x0001) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash, NativeAccessSpecifierProtected)
	unsigned char                                      UnknownData00[0x17];                                      // 0x0011(0x0017) MISSED OFFSET
};

struct FMargin
{
	float                                              Left;                                                     // 0x0000(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              Top;                                                      // 0x0004(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              Right;                                                    // 0x0008(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	float                                              Bottom;                                                   // 0x000C(0x0004) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

struct FBox2D
{
	struct FVector2D                                   min;                                                      // 0x0000(0x0008) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct FVector2D                                   max;                                                      // 0x0008(0x0008) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	unsigned char                                      bIsValid;                                                 // 0x0010(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	unsigned char                                      UnknownData00[0x3];                                       // 0x0011(0x0003) MISSED OFFSET
};

struct FRepAttachment
{
	class AActor* AttachParent;                                             // 0x0000(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct FVector_NetQuantize100                      LocationOffset;                                           // 0x0008(0x000C) (NoDestructor, NativeAccessSpecifierPublic)
	struct FVector_NetQuantize100                      RelativeScale3D;                                          // 0x0014(0x000C) (NoDestructor, NativeAccessSpecifierPublic)
	struct FRotator                                    RotationOffset;                                           // 0x0020(0x000C) (ZeroConstructor, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
	struct FName                                       AttachSocket;                                             // 0x002C(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	unsigned char                                      UnknownData00[0x4];                                       // 0x0034(0x0004) MISSED OFFSET
	class USceneComponent* AttachComponent;                                          // 0x0038(0x0008) (ExportObject, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
};

struct FSlateBrush
{
	unsigned char                                      UnknownData00[0x8];                                       // 0x0000(0x0008) MISSED OFFSET
	struct FVector2D                                   ImageSize;                                                // 0x0008(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	struct FMargin                                     Margin;                                                   // 0x0010(0x0010) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
	struct FSlateColor                                 TintColor;                                                // 0x0020(0x0028) (Edit, BlueprintVisible, NativeAccessSpecifierPublic)
	class UObject* ResourceObject;                                           // 0x0048(0x0008) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPrivate)
	struct FName                                       ResourceName;                                             // 0x0050(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, Protected, HasGetValueTypeHash, NativeAccessSpecifierProtected)
	struct FBox2D                                      UVRegion;                                                 // 0x0058(0x0014) (ZeroConstructor, NoDestructor, Protected, NativeAccessSpecifierProtected)
	TEnumAsByte<ESlateBrushDrawType>                   DrawAs;                                                   // 0x006C(0x0001) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	TEnumAsByte<ESlateBrushTileType>                   Tiling;                                                   // 0x006D(0x0001) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	TEnumAsByte<ESlateBrushMirrorType>                 Mirroring;                                                // 0x006E(0x0001) (Edit, BlueprintVisible, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	TEnumAsByte<ESlateBrushImageType>                  ImageType;                                                // 0x006F(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	unsigned char                                      UnknownData01[0x10];                                      // 0x0070(0x0010) MISSED OFFSET
	unsigned char                                      bIsDynamicallyLoaded : 1;                                 // 0x0080(0x0001) (NoDestructor, Protected, HasGetValueTypeHash, NativeAccessSpecifierProtected)
	unsigned char                                      bHasUObject : 1;                                          // 0x0080(0x0001) (Deprecated, NoDestructor, Protected, HasGetValueTypeHash, NativeAccessSpecifierProtected)
	unsigned char                                      UnknownData02[0x7];                                       // 0x0081(0x0007) MISSED OFFSET
};