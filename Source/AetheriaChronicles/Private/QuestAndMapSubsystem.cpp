// ============================================================================
// QuestAndMapSubsystem.cpp — 6-Quest Story Progression & 4km x 4km Fog-of-War
// Open-World Exploration Map Subsystem Implementation
// ============================================================================
#include "QuestAndMapSubsystem.h"

#include "Engine/GameInstance.h"
#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"

// ============================================================================
// UQuestSubsystem
// ============================================================================

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Quests.Reset();
	NotificationQueue.Reset();

	PopulateStoryQuests();
	TrackedQuestId = FName(TEXT("Q_AwakenExort"));

	UE_LOG(LogChronicles, Log, TEXT("UQuestSubsystem initialized with %d story quests (Tracked: %s)."),
		Quests.Num(), *TrackedQuestId.ToString());
}

void UQuestSubsystem::Deinitialize()
{
	NotificationQueue.Reset();
	Quests.Reset();
	TrackedQuestId = NAME_None;

	Super::Deinitialize();
}

void UQuestSubsystem::PopulateStoryQuests()
{
	Quests.Reserve(6);

	// 1. Ashes of the Hearth -> Unlocks Exort (EEE Solar Cataclysm)
	{
		FQuestDef Q;
		Q.QuestId = FName(TEXT("Q_AwakenExort"));
		Q.GiverId = FName(TEXT("ElderAldric"));
		Q.GiverName = NSLOCTEXT("AetheriaQuests", "Giver_Aldric", "Elder Aldric");
		Q.Title = NSLOCTEXT("AetheriaQuests", "Title_AwakenExort", "Ashes of the Hearth");
		Q.Summary = NSLOCTEXT("AetheriaQuests", "Summary_AwakenExort",
			"A corrupted seal smothers Oakhaven's ancient hearth. Shatter the darkened barrier and reignite the two Oakhaven Runic Beacons to rekindle the Primal Sphere of Exort.");
		Q.ObjectiveText = NSLOCTEXT("AetheriaQuests", "Obj_AwakenExort",
			"Shatter the corrupted barrier & reignite the Oakhaven Runic Beacons");
		Q.OfferDialogue = NSLOCTEXT("AetheriaQuests", "Offer_AwakenExort",
			"Oakhaven's ward-flame flickers on the edge of ruin. Break the shadow barrier beyond the square and relight both Runic Beacons — the embers will awaken your first Primal Sphere: Exort.");
		Q.TurnInDialogue = NSLOCTEXT("AetheriaQuests", "TurnIn_AwakenExort",
			"Golden fire answers the hearth once more! Feel the Solar Flame bind to your palm — invoke EEE to call down Solar Cataclysm upon any foe or barrier.");
		Q.State = EQuestState::Available;
		Q.Progress = 0;
		Q.TargetCount = 2;
		Q.RewardGold = 120;
		Q.RewardXP = 200;
		Q.UnlockOrbReward = EChronicleOrb::Exort;
		Q.bUnlocksDragonMount = false;
		Q.ObjectiveWorldLocation = FVector(0.f, 4200.f, 450.f);
		Quests.Add(MoveTemp(Q));
	}

	// 2. Taming the Crimson Wing -> Unlocks Dragon Mount (Ignis)
	{
		FQuestDef Q;
		Q.QuestId = FName(TEXT("Q_TameDragon"));
		Q.GiverId = FName(TEXT("ScholarLyra"));
		Q.GiverName = NSLOCTEXT("AetheriaQuests", "Giver_Lyra", "Scholar Lyra");
		Q.Title = NSLOCTEXT("AetheriaQuests", "Title_TameDragon", "Taming the Crimson Wing");
		Q.Summary = NSLOCTEXT("AetheriaQuests", "Summary_TameDragon",
			"The war-drake Ignis stands shackled atop the western cliffs of Dragon Aerie. Shatter the two runic bind-crystals with EEE Solar Cataclysm and earn the Crimson Wing's allegiance.");
		Q.ObjectiveText = NSLOCTEXT("AetheriaQuests", "Obj_TameDragon",
			"Break the bind-crystals at Dragon Aerie with EEE Solar Strike & bond with Ignis");
		Q.OfferDialogue = NSLOCTEXT("AetheriaQuests", "Offer_TameDragon",
			"From Mirror Lake I have watched Ignis strain against Malakor's binding pylons on the Aerie cliffs. Strike both bind-crystals with your Solar Cataclysm, and the Crimson Wing will carry you across all of Aetheria.");
		Q.TurnInDialogue = NSLOCTEXT("AetheriaQuests", "TurnIn_TameDragon",
			"The chains are ash and the drake answers your call! The skies of Aetheria belong to you — summon Ignis whenever the road grows too steep.");
		Q.State = EQuestState::Available;
		Q.Progress = 0;
		Q.TargetCount = 2;
		Q.RewardGold = 180;
		Q.RewardXP = 260;
		Q.UnlockOrbReward = EChronicleOrb::None;
		Q.bUnlocksDragonMount = true;
		Q.ObjectiveWorldLocation = FVector(-3400.f, 2200.f, 740.f);
		Quests.Add(MoveTemp(Q));
	}

	// 3. Spires of the Tempest -> Unlocks Wex (expands spells 1 -> 4)
	{
		FQuestDef Q;
		Q.QuestId = FName(TEXT("Q_StormSpires"));
		Q.GiverId = FName(TEXT("BlacksmithTorin"));
		Q.GiverName = NSLOCTEXT("AetheriaQuests", "Giver_Torin", "Blacksmith Torin");
		Q.Title = NSLOCTEXT("AetheriaQuests", "Title_StormSpires", "Spires of the Tempest");
		Q.Summary = NSLOCTEXT("AetheriaQuests", "Summary_StormSpires",
			"Lightning tears through the Sapphire Whispering Woods. Align all three Storm Conductors atop the forest monoliths and fell the Tempest Stalker to master the Sphere of Wex.");
		Q.ObjectiveText = NSLOCTEXT("AetheriaQuests", "Obj_StormSpires",
			"Align 3 Storm Conductors in Blue-Leaf Whispering Woods & defeat the Tempest Stalker");
		Q.OfferDialogue = NSLOCTEXT("AetheriaQuests", "Offer_StormSpires",
			"My forge-bellows spark with wild static rolling out of the Blue-Leaf Woods. Climb the three monoliths, lock their Storm Conductors into phase, and put down the Tempest Stalker guarding the canopy.");
		Q.TurnInDialogue = NSLOCTEXT("AetheriaQuests", "TurnIn_StormSpires",
			"Ha! Hear that thunder hum in tune? You've harnessed Wex — weave it with Exort to hurl Chaos Meteors, surge with Tempest Alacrity, or unleash a Plasma EMP Nova!");
		Q.State = EQuestState::Available;
		Q.Progress = 0;
		Q.TargetCount = 3;
		Q.RewardGold = 240;
		Q.RewardXP = 340;
		Q.UnlockOrbReward = EChronicleOrb::Wex;
		Q.bUnlocksDragonMount = false;
		Q.ObjectiveWorldLocation = FVector(6400.f, 3200.f, 620.f);
		Quests.Add(MoveTemp(Q));
	}

	// 4. Embers of the Deep -> Sunstone Relic inside Ember Crypt
	{
		FQuestDef Q;
		Q.QuestId = FName(TEXT("Q_EmberCrypt"));
		Q.GiverId = FName(TEXT("WardenKael"));
		Q.GiverName = NSLOCTEXT("AetheriaQuests", "Giver_Kael", "Warden Kael");
		Q.Title = NSLOCTEXT("AetheriaQuests", "Title_EmberCrypt", "Embers of the Deep");
		Q.Summary = NSLOCTEXT("AetheriaQuests", "Summary_EmberCrypt",
			"Deep beneath the Ash Caldera lies the Sunstone Relic behind an ancient obsidian bulkhead. Descend into the molten crypt, fracture the obsidian seal with EEW Chaos Meteor, and claim the relic.");
		Q.ObjectiveText = NSLOCTEXT("AetheriaQuests", "Obj_EmberCrypt",
			"Fracture the obsidian seal wall with EEW Chaos Meteor & claim the Sunstone Relic");
		Q.OfferDialogue = NSLOCTEXT("AetheriaQuests", "Offer_EmberCrypt",
			"No mortal steel can scratch the obsidian bulkhead deep inside Ember Crypt. Bring down an EEW Chaos Meteor inside the magma vault to shatter the seal and recover the Sunstone Relic before the Sovereign's cultists do.");
		Q.TurnInDialogue = NSLOCTEXT("AetheriaQuests", "TurnIn_EmberCrypt",
			"The Sunstone Relic burns bright in your hands! Its radiance will shield you against the biting wards of the northern glacier and Highkeep's dark citadel.");
		Q.State = EQuestState::Available;
		Q.Progress = 0;
		Q.TargetCount = 1;
		Q.RewardGold = 300;
		Q.RewardXP = 420;
		Q.UnlockOrbReward = EChronicleOrb::None;
		Q.bUnlocksDragonMount = false;
		Q.ObjectiveWorldLocation = FVector(9400.f, -2400.f, 560.f);
		Quests.Add(MoveTemp(Q));
	}

	// 5. Silence of the Crystal Glacier -> Unlocks Quas (all 10 Invoker spells!)
	{
		FQuestDef Q;
		Q.QuestId = FName(TEXT("Q_GlacierSilence"));
		Q.GiverId = FName(TEXT("SeerVaelis"));
		Q.GiverName = NSLOCTEXT("AetheriaQuests", "Giver_Vaelis", "Seer Vaelis");
		Q.Title = NSLOCTEXT("AetheriaQuests", "Title_GlacierSilence", "Silence of the Crystal Glacier");
		Q.Summary = NSLOCTEXT("AetheriaQuests", "Summary_GlacierSilence",
			"Time itself freezes atop the Crystal Glacier Peaks. Navigate the plateau bonfires and vanquish the Frost Warden to unlock the final Primal Sphere: Quas.");
		Q.ObjectiveText = NSLOCTEXT("AetheriaQuests", "Obj_GlacierSilence",
			"Cross the frozen plateau bonfires & defeat the Frost Warden to master Chrono-Frost");
		Q.OfferDialogue = NSLOCTEXT("AetheriaQuests", "Offer_GlacierSilence",
			"Keep close to the ridge bonfires as you ascend the glacier plateau. Beyond the frozen arch waits the Frost Warden — overcome its stillness, and the final sphere of Chrono-Frost, Quas, will awaken.");
		Q.TurnInDialogue = NSLOCTEXT("AetheriaQuests", "TurnIn_GlacierSilence",
			"Exort, Wex, and Quas now orbit in perfect trinity. All ten archaic invocations lie at your fingertips — you are ready to face the Grave Sovereign.");
		Q.State = EQuestState::Available;
		Q.Progress = 0;
		Q.TargetCount = 1;
		Q.RewardGold = 360;
		Q.RewardXP = 520;
		Q.UnlockOrbReward = EChronicleOrb::Quas;
		Q.bUnlocksDragonMount = false;
		Q.ObjectiveWorldLocation = FVector(-8200.f, -6800.f, 1380.f);
		Quests.Add(MoveTemp(Q));
	}

	// 6. Fall of the Grave Sovereign -> Highkeep Citadel Finale (Solo or 1-6 Co-op)
	{
		FQuestDef Q;
		Q.QuestId = FName(TEXT("Q_FallOfSovereign"));
		Q.GiverId = FName(TEXT("CaptainRowena"));
		Q.GiverName = NSLOCTEXT("AetheriaQuests", "Giver_Rowena", "Captain Rowena");
		Q.Title = NSLOCTEXT("AetheriaQuests", "Title_FallOfSovereign", "Fall of the Grave Sovereign");
		Q.Summary = NSLOCTEXT("AetheriaQuests", "Summary_FallOfSovereign",
			"Malakor, Sovereign of the Shattered Seal, commands the ruin from Highkeep's obsidian throne. Storm Highkeep Citadel alone or with up to six allies and end his reign.");
		Q.ObjectiveText = NSLOCTEXT("AetheriaQuests", "Obj_FallOfSovereign",
			"Storm Highkeep Citadel & vanquish Malakor, Sovereign of the Shattered Seal");
		Q.OfferDialogue = NSLOCTEXT("AetheriaQuests", "Offer_FallOfSovereign",
			"The Highkeep Gate stands breached, Mage. Whether you march alone or rally a full warband of six, climb to the Shattered Throne and cast down Malakor before the Seal fractures forever!");
		Q.TurnInDialogue = NSLOCTEXT("AetheriaQuests", "TurnIn_FallOfSovereign",
			"Malakor has fallen and the Shattered Seal is whole! Bards from Oakhaven to the Crystal Peaks will sing of the Archmage who saved Aetheria!");
		Q.State = EQuestState::Available;
		Q.Progress = 0;
		Q.TargetCount = 1;
		Q.RewardGold = 650;
		Q.RewardXP = 900;
		Q.UnlockOrbReward = EChronicleOrb::None;
		Q.bUnlocksDragonMount = false;
		Q.ObjectiveWorldLocation = FVector(0.f, -9600.f, 1750.f);
		Quests.Add(MoveTemp(Q));
	}
}

FQuestDef* UQuestSubsystem::FindQuest(FName QuestId)
{
	if (QuestId.IsNone())
	{
		return nullptr;
	}

	for (FQuestDef& Quest : Quests)
	{
		if (Quest.QuestId == QuestId)
		{
			return &Quest;
		}
	}
	return nullptr;
}

const FQuestDef* UQuestSubsystem::FindQuest(FName QuestId) const
{
	if (QuestId.IsNone())
	{
		return nullptr;
	}

	for (const FQuestDef& Quest : Quests)
	{
		if (Quest.QuestId == QuestId)
		{
			return &Quest;
		}
	}
	return nullptr;
}

const FQuestDef* UQuestSubsystem::FindQuestForGiver(FName GiverId) const
{
	if (GiverId.IsNone())
	{
		return nullptr;
	}

	// Prioritize ReadyToTurnIn > Available > Active > Completed if a giver ever owns multiple quests.
	const FQuestDef* BestMatch = nullptr;
	auto StatePriority = [](EQuestState State) -> int32
	{
		switch (State)
		{
		case EQuestState::ReadyToTurnIn: return 4;
		case EQuestState::Available:     return 3;
		case EQuestState::Active:        return 2;
		case EQuestState::Completed:     return 1;
		default:                         return 0;
		}
	};

	for (const FQuestDef& Quest : Quests)
	{
		if (Quest.GiverId == GiverId)
		{
			if (!BestMatch || StatePriority(Quest.State) > StatePriority(BestMatch->State))
			{
				BestMatch = &Quest;
			}
		}
	}
	return BestMatch;
}

const FQuestDef* UQuestSubsystem::GetTrackedQuest() const
{
	if (const FQuestDef* Tracked = FindQuest(TrackedQuestId))
	{
		return Tracked;
	}

	// Fallback: return first incomplete quest so HUD compass always has guidance when available.
	for (const FQuestDef& Quest : Quests)
	{
		if (Quest.State != EQuestState::Completed)
		{
			return &Quest;
		}
	}
	return nullptr;
}

void UQuestSubsystem::SetTrackedQuest(FName QuestId)
{
	if (QuestId.IsNone())
	{
		TrackedQuestId = NAME_None;
		return;
	}

	if (FindQuest(QuestId) != nullptr)
	{
		TrackedQuestId = QuestId;
		UE_LOG(LogChronicles, Log, TEXT("Tracked quest set to '%s'."), *TrackedQuestId.ToString());
	}
}

bool UQuestSubsystem::AcceptQuest(FName QuestId)
{
	FQuestDef* Quest = FindQuest(QuestId);
	if (!Quest || Quest->State != EQuestState::Available)
	{
		return false;
	}

	if (Quest->Progress >= Quest->TargetCount)
	{
		Quest->Progress = Quest->TargetCount;
		Quest->State = EQuestState::ReadyToTurnIn;
	}
	else
	{
		Quest->State = EQuestState::Active;
	}

	TrackedQuestId = Quest->QuestId;

	PushNotification(FString::Printf(TEXT("QUEST ACCEPTED: %s"), *Quest->Title.ToString()));
	UE_LOG(LogChronicles, Log, TEXT("Accepted quest '%s' (%s)."),
		*Quest->QuestId.ToString(), *Quest->Title.ToString());
	return true;
}

bool UQuestSubsystem::AdvanceQuestProgress(FName QuestId, int32 Delta)
{
	if (Delta <= 0)
	{
		return false;
	}

	FQuestDef* Quest = FindQuest(QuestId);
	if (!Quest)
	{
		return false;
	}

	// Completed or already ReadyToTurnIn quests do not advance further.
	if (Quest->State == EQuestState::Completed || Quest->State == EQuestState::ReadyToTurnIn)
	{
		return false;
	}

	// Out-of-order exploration support: auto-activate Available quests so players never softlock!
	const bool bAutoDiscovered = (Quest->State == EQuestState::Available);
	if (bAutoDiscovered)
	{
		Quest->State = EQuestState::Active;
		if (TrackedQuestId.IsNone())
		{
			TrackedQuestId = Quest->QuestId;
		}
		else if (const FQuestDef* CurrentTracked = FindQuest(TrackedQuestId))
		{
			if (CurrentTracked->State == EQuestState::Completed)
			{
				TrackedQuestId = Quest->QuestId;
			}
		}
	}

	const int32 SafeTarget = FMath::Max(1, Quest->TargetCount);
	const int32 OldProgress = Quest->Progress;
	Quest->Progress = FMath::Clamp(Quest->Progress + Delta, 0, SafeTarget);

	if (Quest->Progress == OldProgress && !bAutoDiscovered)
	{
		return false;
	}

	if (Quest->Progress >= SafeTarget)
	{
		Quest->State = EQuestState::ReadyToTurnIn;
		PushNotification(FString::Printf(TEXT("QUEST READY: %s — Return to %s"),
			*Quest->Title.ToString(), *Quest->GiverName.ToString()));
	}
	else if (bAutoDiscovered)
	{
		PushNotification(FString::Printf(TEXT("QUEST DISCOVERED: %s (%d/%d)"),
			*Quest->Title.ToString(), Quest->Progress, SafeTarget));
	}
	else
	{
		PushNotification(FString::Printf(TEXT("QUEST UPDATED: %s (%d/%d)"),
			*Quest->Title.ToString(), Quest->Progress, SafeTarget));
	}

	UE_LOG(LogChronicles, Log, TEXT("Quest '%s' progress advanced to %d/%d (State=%d, AutoDiscovered=%d)."),
		*Quest->QuestId.ToString(), Quest->Progress, SafeTarget,
		static_cast<int32>(Quest->State), bAutoDiscovered ? 1 : 0);
	return true;
}

bool UQuestSubsystem::TurnInQuest(
	FName QuestId,
	int32& OutGold,
	int32& OutXP,
	EChronicleOrb& OutUnlockedOrb,
	bool& bOutUnlockedDragon)
{
	OutGold = 0;
	OutXP = 0;
	OutUnlockedOrb = EChronicleOrb::None;
	bOutUnlockedDragon = false;

	FQuestDef* Quest = FindQuest(QuestId);
	if (!Quest || Quest->State != EQuestState::ReadyToTurnIn)
	{
		return false;
	}

	Quest->Progress = FMath::Max(1, Quest->TargetCount);
	Quest->State = EQuestState::Completed;

	OutGold = Quest->RewardGold;
	OutXP = Quest->RewardXP;
	OutUnlockedOrb = Quest->UnlockOrbReward;
	bOutUnlockedDragon = Quest->bUnlocksDragonMount;

	PushNotification(FString::Printf(TEXT("QUEST COMPLETED: %s (+%d Gold, +%d XP)"),
		*Quest->Title.ToString(), OutGold, OutXP));

	switch (OutUnlockedOrb)
	{
	case EChronicleOrb::Exort:
		PushNotification(TEXT("PRIMAL SPHERE AWAKENED: [E] EXORT — Solar Cataclysm (EEE) Unlocked!"));
		break;
	case EChronicleOrb::Wex:
		PushNotification(TEXT("PRIMAL SPHERE AWAKENED: [W] WEX — 4 Tempest & Solar Spells Unlocked!"));
		break;
	case EChronicleOrb::Quas:
		PushNotification(TEXT("PRIMAL SPHERE AWAKENED: [Q] QUAS — All 10 Invoker Spells Mastered!"));
		break;
	default:
		break;
	}

	if (bOutUnlockedDragon)
	{
		PushNotification(TEXT("DRAGON MOUNT UNLOCKED: Ignis, the Crimson Wing — Summon at will!"));
	}

	if (Quest->QuestId == FName(TEXT("Q_EmberCrypt")))
	{
		PushNotification(TEXT("RELIC CLAIMED: Sunstone Relic secured from Ember Crypt!"));
	}
	else if (Quest->QuestId == FName(TEXT("Q_FallOfSovereign")))
	{
		PushNotification(TEXT("HIGHKEEP CITADEL LIBERATED: Malakor, Sovereign of the Shattered Seal Vanquished!"));
	}

	SelectNextTrackedQuest();

	UE_LOG(LogChronicles, Log, TEXT("Turned in quest '%s': +%d Gold, +%d XP, Orb=%d, Dragon=%d. Next Tracked='%s'."),
		*Quest->QuestId.ToString(), OutGold, OutXP,
		static_cast<int32>(OutUnlockedOrb), bOutUnlockedDragon ? 1 : 0, *TrackedQuestId.ToString());
	return true;
}

void UQuestSubsystem::SelectNextTrackedQuest()
{
	// 1. Prefer any quest ready to turn in
	for (const FQuestDef& Quest : Quests)
	{
		if (Quest.State == EQuestState::ReadyToTurnIn)
		{
			TrackedQuestId = Quest.QuestId;
			return;
		}
	}

	// 2. Next prefer an active quest
	for (const FQuestDef& Quest : Quests)
	{
		if (Quest.State == EQuestState::Active)
		{
			TrackedQuestId = Quest.QuestId;
			return;
		}
	}

	// 3. Next follow canonical story order for available quests
	for (const FQuestDef& Quest : Quests)
	{
		if (Quest.State == EQuestState::Available)
		{
			TrackedQuestId = Quest.QuestId;
			return;
		}
	}

	// 4. All 6 story quests completed!
	TrackedQuestId = NAME_None;
	PushNotification(TEXT("CHRONICLES COMPLETE: Peace returns to all realms of Aetheria!"));
}

void UQuestSubsystem::PushNotification(const FString& Message)
{
	if (Message.IsEmpty())
	{
		return;
	}

	if (NotificationQueue.Num() >= MAX_NOTIFICATION_QUEUE)
	{
		NotificationQueue.RemoveAt(0);
	}
	NotificationQueue.Add(Message);
}

FString UQuestSubsystem::PopLatestNotification()
{
	if (NotificationQueue.Num() == 0)
	{
		return FString();
	}

	// FIFO queue: pop the oldest queued banner first
	FString NextMessage = MoveTemp(NotificationQueue[0]);
	NotificationQueue.RemoveAt(0);
	return NextMessage;
}

// ============================================================================
// UFogOfWarMapSubsystem
// ============================================================================

void UFogOfWarMapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bIsInitializing = true;

	ExploredGrid.Init(0, TOTAL_CELLS);
	BuildBiomeGridCache();
	SeedMapPins();
	RebuildExplorationCounts();

	// Uncover starting Oakhaven Valley village footprint so the hero's home hub starts explored.
	RevealAroundWorldLocation(FVector(0.f, 4200.f, 450.f), 5500.f);

	bIsInitializing = false;

	UE_LOG(LogChronicles, Log, TEXT("UFogOfWarMapSubsystem initialized (%dx%d grid, %.1fkm x %.1fkm world, %d map pins, %.2f%% initial explored)."),
		GRID_SIZE, GRID_SIZE,
		(2.f * WORLD_EXTENT_UU) / 100000.f,
		(2.f * WORLD_EXTENT_UU) / 100000.f,
		MapPins.Num(),
		GetTotalExplorationPercent());
}

void UFogOfWarMapSubsystem::Deinitialize()
{
	ExploredGrid.Reset();
	MapPins.Reset();
	CellBiomeCache.Reset();
	TotalExploredCells = 0;
	for (int32 i = 0; i < BIOME_COUNT; ++i)
	{
		BiomeTotalCells[i] = 0;
		BiomeExploredCells[i] = 0;
	}

	Super::Deinitialize();
}

void UFogOfWarMapSubsystem::SeedMapPins()
{
	MapPins.Reset();
	MapPins.Reserve(12);

	auto AddPin = [this](
		const TCHAR* Id,
		const FText& Label,
		const FVector& Loc,
		EChronicleBiome Biome,
		const FLinearColor& Color,
		bool bInitiallyDiscovered,
		bool bIsFastTravel)
	{
		FMapPinInfo Pin;
		Pin.PinId = FName(Id);
		Pin.Label = Label;
		Pin.WorldLocation = Loc;
		Pin.Biome = Biome;
		Pin.Color = Color;
		Pin.bDiscovered = bInitiallyDiscovered;
		Pin.bFastTravel = bIsFastTravel;
		MapPins.Add(MoveTemp(Pin));
	};

	// --- Major Landmarks ---
	AddPin(
		TEXT("Pin_OakhavenVillage"),
		NSLOCTEXT("AetheriaMap", "Pin_OakhavenVillage", "Oakhaven Village"),
		FVector(0.f, 4200.f, 450.f),
		EChronicleBiome::OakhavenValley,
		FLinearColor(0.98f, 0.80f, 0.32f, 1.0f),
		true,
		true);

	AddPin(
		TEXT("Pin_DragonAerie"),
		NSLOCTEXT("AetheriaMap", "Pin_DragonAerie", "Dragon Aerie"),
		FVector(-3400.f, 2200.f, 740.f),
		EChronicleBiome::OakhavenValley,
		FLinearColor(0.96f, 0.30f, 0.14f, 1.0f),
		false,
		false);

	AddPin(
		TEXT("Pin_MirrorLake"),
		NSLOCTEXT("AetheriaMap", "Pin_MirrorLake", "Mirror Lake & Stone Bridge"),
		FVector(3400.f, 1200.f, 520.f),
		EChronicleBiome::OakhavenValley,
		FLinearColor(0.35f, 0.78f, 0.98f, 1.0f),
		false,
		false);

	AddPin(
		TEXT("Pin_BlueLeafWoods"),
		NSLOCTEXT("AetheriaMap", "Pin_BlueLeafWoods", "Blue-Leaf Woods"),
		FVector(6400.f, 3200.f, 620.f),
		EChronicleBiome::BlueLeafWoods,
		FLinearColor(0.42f, 0.56f, 1.00f, 1.0f),
		false,
		false);

	AddPin(
		TEXT("Pin_EmberCrypt"),
		NSLOCTEXT("AetheriaMap", "Pin_EmberCrypt", "Ember Crypt Lava Dungeon"),
		FVector(9400.f, -2400.f, 560.f),
		EChronicleBiome::AshCalderaCrypt,
		FLinearColor(1.00f, 0.42f, 0.08f, 1.0f),
		false,
		false);

	AddPin(
		TEXT("Pin_CrystalGlacier"),
		NSLOCTEXT("AetheriaMap", "Pin_CrystalGlacier", "Crystal Glacier Peaks"),
		FVector(-8200.f, -6800.f, 1380.f),
		EChronicleBiome::CrystalGlacier,
		FLinearColor(0.58f, 0.92f, 1.00f, 1.0f),
		false,
		false);

	AddPin(
		TEXT("Pin_HighkeepCitadel"),
		NSLOCTEXT("AetheriaMap", "Pin_HighkeepCitadel", "Highkeep Citadel"),
		FVector(0.f, -9600.f, 1750.f),
		EChronicleBiome::HighkeepCitadel,
		FLinearColor(0.86f, 0.18f, 0.28f, 1.0f),
		false,
		false);

	// --- Shrine Fast-Travel Waypoints (one per biome) ---
	const FLinearColor ShrineColor(0.20f, 0.95f, 0.90f, 1.0f);

	AddPin(
		TEXT("Shrine_OakhavenForge"),
		NSLOCTEXT("AetheriaMap", "Shrine_OakhavenForge", "Oakhaven Hearth Shrine"),
		FVector(-1400.f, 4800.f, 450.f),
		EChronicleBiome::OakhavenValley,
		ShrineColor,
		true,
		true);

	AddPin(
		TEXT("Shrine_WhisperingSpire"),
		NSLOCTEXT("AetheriaMap", "Shrine_WhisperingSpire", "Whispering Spire Shrine"),
		FVector(5400.f, 2400.f, 580.f),
		EChronicleBiome::BlueLeafWoods,
		ShrineColor,
		false,
		true);

	AddPin(
		TEXT("Shrine_CalderaWatch"),
		NSLOCTEXT("AetheriaMap", "Shrine_CalderaWatch", "Caldera Obsidian Shrine"),
		FVector(8500.f, -1900.f, 540.f),
		EChronicleBiome::AshCalderaCrypt,
		ShrineColor,
		false,
		true);

	AddPin(
		TEXT("Shrine_GlacierTrail"),
		NSLOCTEXT("AetheriaMap", "Shrine_GlacierTrail", "Glacier Trail Bonfire Shrine"),
		FVector(-6800.f, -5600.f, 1100.f),
		EChronicleBiome::CrystalGlacier,
		ShrineColor,
		false,
		true);

	AddPin(
		TEXT("Shrine_HighkeepGate"),
		NSLOCTEXT("AetheriaMap", "Shrine_HighkeepGate", "Highkeep Gate Shrine"),
		FVector(0.f, -7600.f, 1450.f),
		EChronicleBiome::HighkeepCitadel,
		ShrineColor,
		false,
		true);
}

void UFogOfWarMapSubsystem::BuildBiomeGridCache()
{
	CellBiomeCache.SetNumUninitialized(TOTAL_CELLS);
	for (int32 i = 0; i < BIOME_COUNT; ++i)
	{
		BiomeTotalCells[i] = 0;
	}

	for (int32 CellY = 0; CellY < GRID_SIZE; ++CellY)
	{
		for (int32 CellX = 0; CellX < GRID_SIZE; ++CellX)
		{
			const int32 Index = CellY * GRID_SIZE + CellX;
			const EChronicleBiome Biome = GetBiomeAtLocation(GetCellWorldCenter(CellX, CellY));
			CellBiomeCache[Index] = Biome;

			const int32 BiomeIdx = static_cast<int32>(Biome);
			if (BiomeIdx >= 0 && BiomeIdx < BIOME_COUNT)
			{
				++BiomeTotalCells[BiomeIdx];
			}
		}
	}
}

void UFogOfWarMapSubsystem::RebuildExplorationCounts()
{
	TotalExploredCells = 0;
	for (int32 i = 0; i < BIOME_COUNT; ++i)
	{
		BiomeExploredCells[i] = 0;
	}

	if (ExploredGrid.Num() != TOTAL_CELLS || CellBiomeCache.Num() != TOTAL_CELLS)
	{
		return;
	}

	for (int32 Index = 0; Index < TOTAL_CELLS; ++Index)
	{
		if (ExploredGrid[Index] != 0)
		{
			++TotalExploredCells;
			const int32 BiomeIdx = static_cast<int32>(CellBiomeCache[Index]);
			if (BiomeIdx >= 0 && BiomeIdx < BIOME_COUNT)
			{
				++BiomeExploredCells[BiomeIdx];
			}
		}
	}
}

FVector UFogOfWarMapSubsystem::GetCellWorldCenter(int32 CellX, int32 CellY) const
{
	const double WorldX = -static_cast<double>(WORLD_EXTENT_UU) + (static_cast<double>(CellX) + 0.5) * static_cast<double>(CELL_SIZE_UU);
	const double WorldY = -static_cast<double>(WORLD_EXTENT_UU) + (static_cast<double>(CellY) + 0.5) * static_cast<double>(CELL_SIZE_UU);
	return FVector(WorldX, WorldY, 0.0);
}

void UFogOfWarMapSubsystem::RevealAroundWorldLocation(const FVector& WorldLoc, float RevealRadiusUU)
{
	if (ExploredGrid.Num() != TOTAL_CELLS)
	{
		return;
	}

	const double EffectiveRadius = static_cast<double>(FMath::Max(RevealRadiusUU, 1.f));
	const double RadiusSq = EffectiveRadius * EffectiveRadius;
	const double ExtentD = static_cast<double>(WORLD_EXTENT_UU);
	const double CellSizeD = static_cast<double>(CELL_SIZE_UU);

	// Compute tight grid-cell search bounds covering the reveal circle
	const FIntPoint MinCell = WorldToGrid(FVector(WorldLoc.X - EffectiveRadius, WorldLoc.Y - EffectiveRadius, 0.0));
	const FIntPoint MaxCell = WorldToGrid(FVector(WorldLoc.X + EffectiveRadius, WorldLoc.Y + EffectiveRadius, 0.0));

	for (int32 CellY = MinCell.Y; CellY <= MaxCell.Y; ++CellY)
	{
		const double BoxMinY = -ExtentD + static_cast<double>(CellY) * CellSizeD;
		const double BoxMaxY = BoxMinY + CellSizeD;
		const double ClosestY = FMath::Clamp(WorldLoc.Y, BoxMinY, BoxMaxY);
		const double DeltaY = WorldLoc.Y - ClosestY;
		const double DeltaYSq = DeltaY * DeltaY;

		for (int32 CellX = MinCell.X; CellX <= MaxCell.X; ++CellX)
		{
			const int32 Index = CellY * GRID_SIZE + CellX;
			if (ExploredGrid[Index] != 0)
			{
				continue;
			}

			const double BoxMinX = -ExtentD + static_cast<double>(CellX) * CellSizeD;
			const double BoxMaxX = BoxMinX + CellSizeD;
			const double ClosestX = FMath::Clamp(WorldLoc.X, BoxMinX, BoxMaxX);
			const double DeltaX = WorldLoc.X - ClosestX;

			if ((DeltaX * DeltaX + DeltaYSq) <= RadiusSq)
			{
				ExploredGrid[Index] = 1;
				++TotalExploredCells;

				if (CellBiomeCache.IsValidIndex(Index))
				{
					const int32 BiomeIdx = static_cast<int32>(CellBiomeCache[Index]);
					if (BiomeIdx >= 0 && BiomeIdx < BIOME_COUNT)
					{
						++BiomeExploredCells[BiomeIdx];
					}
				}
			}
		}
	}

	// Mark any map pins within RevealRadiusUU as discovered
	for (FMapPinInfo& Pin : MapPins)
	{
		if (Pin.bDiscovered)
		{
			continue;
		}

		if (FVector::DistSquared2D(WorldLoc, Pin.WorldLocation) <= RadiusSq)
		{
			Pin.bDiscovered = true;
			UE_LOG(LogChronicles, Log, TEXT("Discovered map landmark: '%s' (%s)."),
				*Pin.PinId.ToString(), *Pin.Label.ToString());

			if (!bIsInitializing)
			{
				if (UGameInstance* GameInstance = GetGameInstance())
				{
					if (UQuestSubsystem* QuestSys = GameInstance->GetSubsystem<UQuestSubsystem>())
					{
						QuestSys->PushNotification(FString::Printf(
							TEXT("LANDMARK DISCOVERED: %s"), *Pin.Label.ToString()));
					}
				}
			}
		}
	}
}

bool UFogOfWarMapSubsystem::IsCellExplored(int32 CellX, int32 CellY) const
{
	if (CellX < 0 || CellX >= GRID_SIZE || CellY < 0 || CellY >= GRID_SIZE)
	{
		return false;
	}

	const int32 Index = CellY * GRID_SIZE + CellX;
	return ExploredGrid.IsValidIndex(Index) && ExploredGrid[Index] != 0;
}

FVector2D UFogOfWarMapSubsystem::WorldToNormalizedUV(const FVector& WorldLoc) const
{
	const double ExtentD = static_cast<double>(WORLD_EXTENT_UU);
	const double InvWorldSpan = 1.0 / (2.0 * ExtentD);
	const double U = FMath::Clamp((WorldLoc.X + ExtentD) * InvWorldSpan, 0.0, 1.0);
	const double V = FMath::Clamp((WorldLoc.Y + ExtentD) * InvWorldSpan, 0.0, 1.0);
	return FVector2D(U, V);
}

FIntPoint UFogOfWarMapSubsystem::WorldToGrid(const FVector& WorldLoc) const
{
	const FVector2D UV = WorldToNormalizedUV(WorldLoc);
	const int32 CellX = FMath::Clamp(FMath::FloorToInt32(UV.X * static_cast<double>(GRID_SIZE)), 0, GRID_SIZE - 1);
	const int32 CellY = FMath::Clamp(FMath::FloorToInt32(UV.Y * static_cast<double>(GRID_SIZE)), 0, GRID_SIZE - 1);
	return FIntPoint(CellX, CellY);
}

float UFogOfWarMapSubsystem::GetTotalExplorationPercent() const
{
	if (TOTAL_CELLS <= 0)
	{
		return 0.f;
	}
	return FMath::Clamp((static_cast<float>(TotalExploredCells) / static_cast<float>(TOTAL_CELLS)) * 100.f, 0.f, 100.f);
}

float UFogOfWarMapSubsystem::GetBiomeExplorationPercent(EChronicleBiome Biome) const
{
	const int32 BiomeIdx = static_cast<int32>(Biome);
	if (BiomeIdx < 0 || BiomeIdx >= BIOME_COUNT || BiomeTotalCells[BiomeIdx] <= 0)
	{
		return 0.f;
	}

	return FMath::Clamp(
		(static_cast<float>(BiomeExploredCells[BiomeIdx]) / static_cast<float>(BiomeTotalCells[BiomeIdx])) * 100.f,
		0.f,
		100.f);
}

EChronicleBiome UFogOfWarMapSubsystem::GetBiomeAtLocation(const FVector& WorldLoc) const
{
	const double X = WorldLoc.X;
	const double Y = WorldLoc.Y;

	// 1. Highkeep Citadel — Northern mountain spine (-Y central corridor, expanding across the 4km horizon)
	if (Y < -5200.0 && FMath::Abs(X) < FMath::Max(4500.0, FMath::Abs(Y) * 0.55))
	{
		return EChronicleBiome::HighkeepCitadel;
	}

	// 2. Crystal Glacier Peaks — Northwest & Western frozen highlands (Glacier Trail at (-6800, -5600))
	if (X < -4200.0 && Y < 1000.0)
	{
		return EChronicleBiome::CrystalGlacier;
	}

	// 3. Ash Caldera & Ember Crypt — Eastern volcanic badlands (Ember Crypt at (9400, -2400))
	if (X > 4800.0 && Y < 400.0)
	{
		return EChronicleBiome::AshCalderaCrypt;
	}

	// 4. Sapphire Whispering Woods (Blue-Leaf Woods) — Northeastern sapphire forest (Monoliths at (6400, 3200))
	if (X > 4200.0 && Y >= 400.0)
	{
		return EChronicleBiome::BlueLeafWoods;
	}

	// 5. Golden Oakhaven Valley — Central & Southern heartland (Oakhaven, Forge, Dragon Aerie, Mirror Lake)
	return EChronicleBiome::OakhavenValley;
}

FText UFogOfWarMapSubsystem::GetBiomeDisplayName(EChronicleBiome Biome) const
{
	switch (Biome)
	{
	case EChronicleBiome::OakhavenValley:
		return NSLOCTEXT("AetheriaMap", "Biome_OakhavenValley", "Golden Oakhaven Valley");
	case EChronicleBiome::BlueLeafWoods:
		return NSLOCTEXT("AetheriaMap", "Biome_BlueLeafWoods", "Sapphire Whispering Woods");
	case EChronicleBiome::HighkeepCitadel:
		return NSLOCTEXT("AetheriaMap", "Biome_HighkeepCitadel", "Highkeep Citadel");
	case EChronicleBiome::AshCalderaCrypt:
		return NSLOCTEXT("AetheriaMap", "Biome_AshCalderaCrypt", "Ash Caldera & Ember Crypt");
	case EChronicleBiome::CrystalGlacier:
		return NSLOCTEXT("AetheriaMap", "Biome_CrystalGlacier", "Crystal Glacier Peaks");
	default:
		return NSLOCTEXT("AetheriaMap", "Biome_Unknown", "Uncharted Aetheria");
	}
}

void UFogOfWarMapSubsystem::LoadRawFogGrid(const TArray<uint8>& InGrid)
{
	ExploredGrid.Init(0, TOTAL_CELLS);

	if (InGrid.Num() != TOTAL_CELLS && InGrid.Num() > 0)
	{
		UE_LOG(LogChronicles, Warning, TEXT("LoadRawFogGrid received %d bytes (expected %d); clamping/padding safely."),
			InGrid.Num(), TOTAL_CELLS);
	}

	const int32 CopyCount = FMath::Min(InGrid.Num(), TOTAL_CELLS);
	for (int32 i = 0; i < CopyCount; ++i)
	{
		ExploredGrid[i] = (InGrid[i] != 0) ? 1 : 0;
	}

	RebuildExplorationCounts();

	// Synchronize landmark pin discovery states with the loaded fog grid
	for (FMapPinInfo& Pin : MapPins)
	{
		const FIntPoint PinCell = WorldToGrid(Pin.WorldLocation);
		const bool bIsStartingHub = (Pin.PinId == FName(TEXT("Pin_OakhavenVillage"))
			|| Pin.PinId == FName(TEXT("Shrine_OakhavenForge")));
		Pin.bDiscovered = bIsStartingHub || IsCellExplored(PinCell.X, PinCell.Y);
	}

	UE_LOG(LogChronicles, Log, TEXT("Loaded Fog-of-War grid: %d/%d cells explored (%.2f%%)."),
		TotalExploredCells, TOTAL_CELLS, GetTotalExplorationPercent());
}
