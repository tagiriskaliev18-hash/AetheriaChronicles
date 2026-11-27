// ============================================================================
// src/quests.js — 5-Quest RPG System, Background Progress & Dynamic Markers
// ============================================================================
import { LANDMARKS } from './config.js';

const EMPTY_NOTIFICATIONS = Object.freeze([]);

const ACTIVE_MARKER_LABELS = {
  q_bridge: 'Stone Bridge',
  q_trolls: 'Blue-Leaf Woods',
  q_dragon: LANDMARKS.dragonPerch.label,
  q_dungeon: LANDMARKS.dungeonRelic.label,
  q_boss: LANDMARKS.bossArena.label,
};

function extractXZ(obj, out) {
  if (!obj) return false;
  if (obj.group && obj.group.position) {
    out.x = obj.group.position.x;
    out.z = obj.group.position.z;
    return true;
  }
  if (obj.position && typeof obj.position.x === 'number') {
    out.x = obj.position.x;
    out.z = obj.position.z;
    return true;
  }
  if (obj.pos && typeof obj.pos.x === 'number') {
    out.x = obj.pos.x;
    out.z = obj.pos.z;
    return true;
  }
  if (obj.mesh && obj.mesh.position) {
    out.x = obj.mesh.position.x;
    out.z = obj.mesh.position.z;
    return true;
  }
  if (typeof obj.x === 'number' && typeof obj.z === 'number') {
    out.x = obj.x;
    out.z = obj.z;
    return true;
  }
  return false;
}

function findVillagerCoords(villagers, giverId, out) {
  if (villagers) {
    const arr = Array.isArray(villagers)
      ? villagers
      : Array.isArray(villagers.list)
        ? villagers.list
        : Array.isArray(villagers.npcs)
          ? villagers.npcs
          : null;

    if (arr) {
      for (let i = 0; i < arr.length; i++) {
        const v = arr[i];
        if (v && (v.id === giverId || v.giverId === giverId)) {
          if (extractXZ(v, out)) return true;
        }
      }
    } else if (villagers[giverId]) {
      if (extractXZ(villagers[giverId], out)) return true;
    }
  }
  out.x = LANDMARKS.village.x;
  out.z = LANDMARKS.village.z;
  return false;
}

export function createQuestSystem(sound) {
  // Background tracking ensures zero softlocks if objectives are cleared out of order
  const bg = {
    skeletonKills: 0,
    trollKills: 0,
    ringsPassed: new Set(),
    relicFound: false,
    bossDefeated: false,
  };

  let dragonMounted = false;
  const notifications = [];

  // Preallocated scratch objects to guarantee zero per-frame allocations
  const activeMarker = { x: 0, z: 0, label: '' };
  const tmpCoords = { x: 0, z: 0 };

  const list = [
    {
      id: 'q_bridge',
      giverId: 'aldric',
      giverName: 'Elder Aldric',
      title: 'Bridge of Bones',
      summary: 'Skeletons have risen along the northern bank of the Stone Bridge, cutting off Oakhaven from Highkeep.',
      objectiveText: 'Defeat Skeletons near the Stone Bridge',
      status: 'available',
      progress: 0,
      target: 4,
      rewardGold: 120,
      bonusText: '',
      marker: { x: 0, z: -38 },
      dialogue: {
        offer: 'Thank the sun you still bear steel, traveler. Skeletons have clawed up along the northern bank of the Stone Bridge, cutting Oakhaven off from Highkeep. Cut down four of the wretched dead so our scouts may cross once more.',
        active: 'The rattling dead still prowl the northern bank past the Stone Bridge. Keep your shield ready and strike cleanly — four fallen skeletons will break their hold.',
        turnin: 'The Stone Bridge stands open once more! Take this gold from Oakhaven\'s chest, brave blade — Aethelgard has dire need of hands like yours.',
        after: 'Highkeep\'s bells have hung silent since Malakor claimed the hill. Speak with Captain Rowena when your steel is ready for the Grave Sovereign.',
      },
    },
    {
      id: 'q_trolls',
      giverId: 'torin',
      giverName: 'Blacksmith Torin',
      title: 'Trollbane Ore',
      summary: 'Forest Trolls stalk the Blue-Leaf Woods carrying star-iron clubs. Slay them and I will temper your blade.',
      objectiveText: 'Slay Forest Trolls in the Blue Woods',
      status: 'available',
      progress: 0,
      target: 3,
      rewardGold: 180,
      bonusText: 'Tempered Blade (+25% Sword Damage)',
      marker: { x: -60, z: -18 },
      dialogue: {
        offer: 'See this anvil? Half-cold without star-iron. Three hulking Forest Trolls prowl the Blue-Leaf Woods to the west swinging clubs forged from the stuff. Fell three of the brutes and I\'ll temper your sword free of charge.',
        active: 'Those moss-backed trolls hit like falling oaks — roll past their heavy swings in the western Blue-Leaf Woods. Bring word once three lie still.',
        turnin: 'Hah! Sparks and cinders, you brought down all three! Pass me your blade... there — star-iron edge, balanced to cleave bone and plate alike. A quarter more bite on every swing, plus your gold!',
        after: 'Keep that tempered edge keen and hungry. Even a Grave Sovereign bleeds when star-iron finds his ribs.',
      },
    },
    {
      id: 'q_dragon',
      giverId: 'lyra',
      giverName: 'Scholar Lyra',
      title: 'Wings Over Aethelgard',
      summary: 'Mount the Red Dragon at the western ridge [F] and soar through the three Ancient Sky Rune Rings above the valley.',
      objectiveText: 'Fly through Sky Rune Rings on the Dragon',
      status: 'available',
      progress: 0,
      target: 3,
      rewardGold: 200,
      bonusText: '',
      marker: { x: LANDMARKS.dragonPerch.x, z: LANDMARKS.dragonPerch.z },
      dialogue: {
        offer: 'Look toward the western ridge — the Red Dragon rests upon the Dragon Aerie, awaiting a fearless rider. Approach the drake and press [F] to mount, then soar through the three Ancient Sky Rune Rings hovering above our valley.',
        active: 'Climb to the western ridge, mount the Red Dragon with [F], and steer through all three glowing Sky Rune Rings high above the valley. Let the mountain winds lift you!',
        turnin: 'Magnificent! The ancient runes sang across the valley as you pierced every ring! Accept the scholars\' stipend — few mortals in a century have flown beside a true drake.',
        after: 'The Sky Rune Rings still hum in your wake. The Red Dragon awaits you at the western ridge whenever the clouds call.',
      },
    },
    {
      id: 'q_dungeon',
      giverId: 'kael',
      giverName: 'Warden Kael',
      title: 'Embers of the Deep',
      summary: 'Beneath the eastern crags lies the Ember Crypt, lit only by torches and cracked magma. Claim the Sunstone Relic within.',
      objectiveText: 'Retrieve the Sunstone Relic in the Dark Dungeon',
      status: 'available',
      progress: 0,
      target: 1,
      rewardGold: 250,
      bonusText: '',
      marker: { x: LANDMARKS.dungeonRelic.x, z: LANDMARKS.dungeonRelic.z },
      dialogue: {
        offer: 'Beyond the eastern crags yawns the Ember Crypt, a dark vault lit only by guttering torches and veins of cracked magma. Within rests the Sunstone Relic. Brave the deep and claim it before Malakor\'s shadow snuffs its fire.',
        active: 'Follow the eastern trail into the crags to reach the Ember Crypt archway. Tread carefully past the lava fissures and lift the Sunstone Relic from its inner altar.',
        turnin: 'By the old wards, the Sunstone still burns pure! Its ember light will guard Oakhaven through the darkest nights to come. Take this bounty — you have earned every coin.',
        after: 'The warmth of the Sunstone already steadies our hearths. May its light hold firm when you march upon Highkeep.',
      },
    },
    {
      id: 'q_boss',
      giverId: 'rowena',
      giverName: 'Captain Rowena',
      title: 'Fall of the Grave Lord',
      summary: 'Malakor, the Grave Sovereign, holds the hilltop courtyard of Highkeep Castle. End his reign — watch for his crimson glow before each strike.',
      objectiveText: 'Defeat Malakor inside Highkeep Castle',
      status: 'available',
      progress: 0,
      target: 1,
      rewardGold: 500,
      bonusText: 'Champion of Aethelgard',
      marker: { x: LANDMARKS.bossArena.x, z: LANDMARKS.bossArena.z },
      dialogue: {
        offer: 'Past the Stone Bridge, the winding hill road climbs to Highkeep Castle. Malakor, the Grave Sovereign, holds the throne courtyard above. End his reign — and heed me well: watch for the crimson glow before each strike, then dodge or block!',
        active: 'Highkeep\'s hilltop gate stands open to the north. Watch Malakor\'s crimson telegraph before every sweeping cleave, strike in the opening, and free Aethelgard from his curse.',
        turnin: 'It is done... the storm over Highkeep breaks at last! Take the royal purse and bear the title Champion of Aethelgard — songs of this victory will outlive the stones of the keep!',
        after: 'The banner of Aethelgard flies proud over Highkeep once more, Champion. The realm owes its dawn to your steel.',
      },
    },
  ];

  const byId = Object.create(null);
  const byGiver = Object.create(null);
  for (let i = 0; i < list.length; i++) {
    byId[list[i].id] = list[i];
    byGiver[list[i].giverId] = list[i];
  }

  function pushToast(title, subtitle, type = 'quest') {
    notifications.push({ title, subtitle, type });
  }

  function playQuestSound() {
    if (sound && typeof sound.playQuest === 'function') {
      sound.playQuest();
    }
  }

  function findNextSkyRingIndex(skyRings) {
    const rings = Array.isArray(skyRings) && skyRings.length > 0 ? skyRings : LANDMARKS.skyRings;
    for (let i = 0; i < rings.length; i++) {
      const r = rings[i];
      const done = bg.ringsPassed.has(i) || Boolean(r && (r.collected || r.passed));
      if (!done) return i;
    }
    return -1;
  }

  function countCollectedSkyRings(skyRings) {
    const rings = Array.isArray(skyRings) && skyRings.length > 0 ? skyRings : LANDMARKS.skyRings;
    let count = 0;
    for (let i = 0; i < rings.length; i++) {
      const r = rings[i];
      if (bg.ringsPassed.has(i) || Boolean(r && (r.collected || r.passed))) {
        count++;
      }
    }
    return Math.max(count, bg.ringsPassed.size);
  }

  function syncDragonMarker(skyRings, isMounted = false) {
    const q = byId.q_dragon;
    const rings = Array.isArray(skyRings) && skyRings.length > 0 ? skyRings : LANDMARKS.skyRings;
    const collected = countCollectedSkyRings(rings);
    const nextIdx = findNextSkyRingIndex(rings);
    const mounted = Boolean(dragonMounted || isMounted || (skyRings && skyRings.mounted));

    if ((mounted || collected > 0) && nextIdx !== -1) {
      const ringObj = rings[nextIdx];
      if (extractXZ(ringObj, tmpCoords)) {
        q.marker.x = tmpCoords.x;
        q.marker.z = tmpCoords.z;
      } else if (LANDMARKS.skyRings[nextIdx]) {
        q.marker.x = LANDMARKS.skyRings[nextIdx].x;
        q.marker.z = LANDMARKS.skyRings[nextIdx].z;
      }
      return nextIdx;
    }

    q.marker.x = LANDMARKS.dragonPerch.x;
    q.marker.z = LANDMARKS.dragonPerch.z;
    return -1;
  }

  function syncProgressFromBackground(q) {
    if (!q || q.status === 'completed') return;
    if (q.id === 'q_bridge') {
      q.progress = Math.min(q.target, bg.skeletonKills);
    } else if (q.id === 'q_trolls') {
      q.progress = Math.min(q.target, bg.trollKills);
    } else if (q.id === 'q_dragon') {
      q.progress = Math.min(q.target, bg.ringsPassed.size);
      syncDragonMarker(null, dragonMounted);
    } else if (q.id === 'q_dungeon') {
      q.progress = bg.relicFound ? 1 : 0;
    } else if (q.id === 'q_boss') {
      q.progress = bg.bossDefeated ? 1 : 0;
    }
  }

  function advanceTrackedId() {
    for (let i = 0; i < list.length; i++) {
      if (list[i].status === 'ready') {
        api.trackedId = list[i].id;
        return;
      }
    }
    for (let i = 0; i < list.length; i++) {
      if (list[i].status === 'active') {
        api.trackedId = list[i].id;
        return;
      }
    }
    for (let i = 0; i < list.length; i++) {
      if (list[i].status === 'available') {
        api.trackedId = list[i].id;
        return;
      }
    }
    api.trackedId = null;
  }

  const api = {
    list,
    trackedId: 'q_bridge',

    getQuest(id) {
      return byId[id] || null;
    },

    getQuestForVillager(villagerId) {
      return byGiver[villagerId] || null;
    },

    getVillagerMarkerState(villagerId) {
      const q = byGiver[villagerId];
      if (!q) return null;
      if (q.status === 'ready') return '?';
      if (q.status === 'available') return '!';
      if (q.status === 'active') return '...';
      return null;
    },

    acceptQuest(id) {
      const q = byId[id];
      if (!q || q.status !== 'available') return false;

      syncProgressFromBackground(q);
      q.status = q.progress >= q.target ? 'ready' : 'active';
      api.trackedId = q.id;

      playQuestSound();
      if (q.status === 'ready') {
        pushToast(
          `Quest Ready: ${q.title}`,
          `Objective already fulfilled! Speak with ${q.giverName} to claim your reward.`,
          'quest'
        );
      } else {
        pushToast(`Quest Accepted: ${q.title}`, q.objectiveText, 'quest');
      }
      return true;
    },

    turnInQuest(id, hero) {
      const q = byId[id];
      if (!q || q.status !== 'ready') return false;

      q.status = 'completed';
      q.progress = q.target;

      if (hero) {
        const currentGold = Number.isFinite(hero.gold) ? hero.gold : 0;
        hero.gold = currentGold + q.rewardGold;
        if (q.id === 'q_trolls') {
          const currentBonus = Number.isFinite(hero.damageBonus) ? hero.damageBonus : 1.0;
          hero.damageBonus = Math.max(currentBonus, 1.25);
        }
      }

      if (api.trackedId === q.id || !api.getTrackedQuest()) {
        advanceTrackedId();
      }

      playQuestSound();

      const rewardSubtitle = q.bonusText
        ? `+${q.rewardGold} Gold • ${q.bonusText}`
        : `+${q.rewardGold} Gold awarded`;
      pushToast(`Quest Complete: ${q.title}`, rewardSubtitle, 'gold');

      let allCompleted = true;
      for (let i = 0; i < list.length; i++) {
        if (list[i].status !== 'completed') {
          allCompleted = false;
          break;
        }
      }

      if (allCompleted) {
        pushToast(
          'Victory: Legend of Aethelgard!',
          'All five trials are fulfilled — the entire realm hails its eternal Champion!',
          'victory'
        );
      } else if (q.id === 'q_boss') {
        pushToast(
          'Victory: Champion of Aethelgard!',
          'Malakor the Grave Sovereign has fallen and Highkeep stands free!',
          'victory'
        );
      }

      return true;
    },

    setTrackedQuest(id) {
      const q = byId[id];
      if (!q || q.status === 'completed') return false;
      api.trackedId = q.id;
      return true;
    },

    getTrackedQuest() {
      if (api.trackedId) {
        const q = byId[api.trackedId];
        if (q && q.status !== 'completed') return q;
      }
      advanceTrackedId();
      return api.trackedId ? byId[api.trackedId] || null : null;
    },

    setDragonMounted(flag) {
      dragonMounted = Boolean(flag);
      syncDragonMarker(null, dragonMounted);
    },

    getActiveMarker(villagers, skyRings, mountedOrHero = false) {
      const q = api.getTrackedQuest();
      if (!q) return null;

      if (q.status === 'available' || q.status === 'ready') {
        findVillagerCoords(villagers, q.giverId, tmpCoords);
        activeMarker.x = tmpCoords.x;
        activeMarker.z = tmpCoords.z;
        activeMarker.label = q.status === 'ready'
          ? `Return to ${q.giverName}`
          : `Speak to ${q.giverName}`;
        return activeMarker;
      }

      if (q.id === 'q_dragon') {
        const isMounted = typeof mountedOrHero === 'boolean'
          ? mountedOrHero
          : Boolean(
              mountedOrHero &&
              (mountedOrHero.mounted || mountedOrHero.isMounted || mountedOrHero.onDragon)
            );
        const nextRingIdx = syncDragonMarker(skyRings, isMounted);
        activeMarker.x = q.marker.x;
        activeMarker.z = q.marker.z;
        activeMarker.label = nextRingIdx !== -1
          ? `Sky Rune Ring ${nextRingIdx + 1}`
          : LANDMARKS.dragonPerch.label;
        return activeMarker;
      }

      activeMarker.x = q.marker.x;
      activeMarker.z = q.marker.z;
      activeMarker.label = ACTIVE_MARKER_LABELS[q.id] || q.title;
      return activeMarker;
    },

    onEnemyKilled(kind) {
      const normalized = typeof kind === 'string' ? kind.toLowerCase() : '';
      let q = null;
      let count = 0;

      if (normalized === 'skeleton') {
        bg.skeletonKills += 1;
        q = byId.q_bridge;
        count = bg.skeletonKills;
      } else if (normalized === 'troll') {
        bg.trollKills += 1;
        q = byId.q_trolls;
        count = bg.trollKills;
      } else {
        return;
      }

      if (q.status === 'completed') return;

      const prev = q.progress;
      q.progress = Math.min(q.target, count);

      if (q.status === 'active') {
        if (q.progress >= q.target) {
          q.status = 'ready';
          playQuestSound();
          pushToast(
            `Objective Complete: ${q.title}`,
            `Return to ${q.giverName} in Oakhaven`,
            'quest'
          );
        } else if (q.progress > prev) {
          pushToast(
            q.title,
            `${q.objectiveText} (${q.progress}/${q.target})`,
            'quest'
          );
        }
      }
    },

    onSkyRingPassed(index) {
      if (typeof index !== 'number' || index < 0 || bg.ringsPassed.has(index)) {
        return;
      }
      bg.ringsPassed.add(index);

      const q = byId.q_dragon;
      syncDragonMarker(null, true);
      if (q.status === 'completed') return;

      q.progress = Math.min(q.target, bg.ringsPassed.size);

      if (q.status === 'active') {
        if (q.progress >= q.target) {
          q.status = 'ready';
          playQuestSound();
          pushToast(
            `Objective Complete: ${q.title}`,
            `Return to ${q.giverName} in Oakhaven`,
            'quest'
          );
        } else {
          pushToast(
            q.title,
            `Sky Rune Rings flown (${q.progress}/${q.target})`,
            'quest'
          );
        }
      } else if (q.status === 'available') {
        pushToast(
          `Sky Rune Ring Awakened (${q.progress}/${q.target})`,
          q.progress >= q.target
            ? `Speak to ${q.giverName} in Oakhaven to claim your reward`
            : 'Ancient draconic runes resonate across the valley',
          'quest'
        );
      }
    },

    onRelicCollected() {
      if (bg.relicFound) return;
      bg.relicFound = true;

      const q = byId.q_dungeon;
      if (q.status === 'completed') return;

      q.progress = 1;

      if (q.status === 'active') {
        q.status = 'ready';
        playQuestSound();
        pushToast(
          `Objective Complete: ${q.title}`,
          `Return the Sunstone Relic to ${q.giverName}`,
          'quest'
        );
      } else if (q.status === 'available') {
        playQuestSound();
        pushToast(
          'Sunstone Relic Claimed',
          `Bring the relic to ${q.giverName} in Oakhaven`,
          'gold'
        );
      }
    },

    onBossKilled() {
      if (bg.bossDefeated) return;
      bg.bossDefeated = true;

      const q = byId.q_boss;
      if (q.status === 'completed') return;

      q.progress = 1;

      if (q.status === 'active') {
        q.status = 'ready';
        playQuestSound();
        pushToast(
          'Malakor Vanquished!',
          `Return to ${q.giverName} to claim your reward`,
          'boss'
        );
      } else if (q.status === 'available') {
        playQuestSound();
        pushToast(
          'Malakor Vanquished!',
          `Speak to ${q.giverName} in Oakhaven to claim the Champion's reward`,
          'boss'
        );
      }
    },

    popNotifications() {
      if (notifications.length === 0) return EMPTY_NOTIFICATIONS;
      const out = notifications.slice();
      notifications.length = 0;
      return out;
    },
  };

  return api;
}
