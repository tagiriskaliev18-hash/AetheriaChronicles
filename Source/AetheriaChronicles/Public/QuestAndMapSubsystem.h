// ============================================================================
// QuestAndMapSubsystem.h — Story Quest Progression (6 Non-Repetitive Quests
// Unlocking Exort -> Dragon Mount -> Wex -> Sunstone Relic -> Quas -> Highkeep
// Raid) & 4km x 4km Fog-of-War Open-World Exploration Map Subsystems
// ============================================================================
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChroniclesTypes.h"
#include "QuestAndMapSubsystem.generated.h"

/**
 * Manages the 6 distinct story quests across Aetheria, supporting out-of-order
 * world exploration, orb/mount progression rewards, and a FIFO banner queue for Slate HUD.
 */
UCLASS()
class AETHERIACHRONICLES_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Quests")
	const TArray<FQuestDef>& GetAllQuests() const { return Quests; }

	FQuestDef* FindQuest(FName QuestId);
	const FQuestDef* FindQuest(FName QuestId) const;
	const FQuestDef* FindQuestForGiver(FName GiverId) const;
	const FQuestDef* GetTrackedQuest() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Quests")
	FName GetTrackedQuestId() const { return TrackedQuestId; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Quests")
	void SetTrackedQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Quests")
	bool AcceptQuest(FName QuestId);

	/** Advances quest progress (auto-activates Available quests so out-of-order exploration never softlocks). */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Quests")
	bool AdvanceQuestProgress(FName QuestId, int32 Delta = 1);

	/** Turns in a ReadyToTurnIn quest, writing all rewards to out parameters and advancing tracked quest. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Quests")
	bool TurnInQuest(
		FName QuestId,
		int32& OutGold,
		int32& OutXP,
		EChronicleOrb& OutUnlockedOrb,
		bool& bOutUnlockedDragon);

	/** Enqueues a HUD banner notification into the FIFO queue. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Quests")
	void PushNotification(const FString& Message);

	/** Pops the oldest pending banner notification (FIFO) for Slate HUD display, or empty string if none. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Quests")
	FString PopLatestNotification();

	UFUNCTION(BlueprintPure, Category = "Aetheria|Quests")
	bool HasPendingNotifications() const { return NotificationQueue.Num() > 0; }

private:
	void PopulateStoryQuests();
	void SelectNextTrackedQuest();

	static constexpr int32 MAX_NOTIFICATION_QUEUE = 32;

	UPROPERTY()
	TArray<FQuestDef> Quests;

	UPROPERTY()
	FName TrackedQuestId = NAME_None;

	UPROPERTY()
	TArray<FString> NotificationQueue;
};

/**
 * Manages the seamless 4 km x 4 km ([-200000..+200000] UU) Fog-of-War exploration
 * grid (64x64 = 4096 cells), biome spatial queries, and landmark / fast-travel pins.
 */
UCLASS()
class AETHERIACHRONICLES_API UFogOfWarMapSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Half-extent of the 4 km x 4 km seamless open world in Unreal Units (cm): [-200000..+200000] UU. */
	static constexpr float WORLD_EXTENT_UU = 200000.f;

	/** Resolution of the Fog-of-War grid along each axis (64 x 64 = 4096 discovery cells). */
	static constexpr int32 GRID_SIZE = 64;

	/** World-space size of a single exploration cell in Unreal Units (6250 UU = 62.5 m). */
	static constexpr float CELL_SIZE_UU = (2.f * WORLD_EXTENT_UU) / static_cast<float>(GRID_SIZE);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Reveals all fog cells intersecting RevealRadiusUU around WorldLoc and discovers nearby map pins. */
	UFUNCTION(BlueprintCallable, Category = "Aetheria|Map")
	void RevealAroundWorldLocation(const FVector& WorldLoc, float RevealRadiusUU = 5500.f);

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	bool IsCellExplored(int32 CellX, int32 CellY) const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	FIntPoint WorldToGrid(const FVector& WorldLoc) const;

	/** Converts a world-space location to normalized [0..1] map UV coordinates for Minimap & Fullscreen Map. */
	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	FVector2D WorldToNormalizedUV(const FVector& WorldLoc) const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	float GetTotalExplorationPercent() const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	float GetBiomeExplorationPercent(EChronicleBiome Biome) const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	EChronicleBiome GetBiomeAtLocation(const FVector& WorldLoc) const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	FText GetBiomeDisplayName(EChronicleBiome Biome) const;

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	const TArray<FMapPinInfo>& GetMapPins() const { return MapPins; }

	UFUNCTION(BlueprintPure, Category = "Aetheria|Map")
	const TArray<uint8>& GetRawFogGrid() const { return ExploredGrid; }

	UFUNCTION(BlueprintCallable, Category = "Aetheria|Map")
	void LoadRawFogGrid(const TArray<uint8>& InGrid);

private:
	static constexpr int32 TOTAL_CELLS = GRID_SIZE * GRID_SIZE;
	static constexpr int32 BIOME_COUNT = 5;

	void SeedMapPins();
	void BuildBiomeGridCache();
	void RebuildExplorationCounts();
	FVector GetCellWorldCenter(int32 CellX, int32 CellY) const;

	UPROPERTY()
	TArray<uint8> ExploredGrid;

	UPROPERTY()
	TArray<FMapPinInfo> MapPins;

	TArray<EChronicleBiome> CellBiomeCache;
	int32 BiomeTotalCells[BIOME_COUNT] = { 0, 0, 0, 0, 0 };
	int32 BiomeExploredCells[BIOME_COUNT] = { 0, 0, 0, 0, 0 };
	int32 TotalExploredCells = 0;
	bool bIsInitializing = false;
};
