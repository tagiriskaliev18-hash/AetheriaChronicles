// ============================================================================
// src/main.js — Renderer, Camera, Input, Day/Night Cycle & Master Game Loop
// ============================================================================
import * as THREE from 'three';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { LANDMARKS, getWalkableHeight, isInsideDungeon } from './config.js';
import { createProceduralTextures } from './textures.js';
import { SoundEngine } from './audio.js';
import { createWorld } from './world.js';
import { createHero, createDragon, createVillagers } from './entities.js';
import { createCombatSystem } from './combat.js';
import { createQuestSystem } from './quests.js';
import { createUI } from './ui.js';

const canvas = document.getElementById('game-canvas');
const isMobile = window.matchMedia('(max-width: 768px)').matches || ('ontouchstart' in window && navigator.maxTouchPoints > 0);
const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

// Renderer (Three.js r186)
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  powerPreference: 'high-performance',
  preserveDrawingBuffer: true,
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, isMobile ? 1.5 : 1.75));
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.toneMapping = THREE.AgXToneMapping;
renderer.toneMappingExposure = 1.06;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.25, 620);

// Image-based lighting from a neutral studio room — gives metals/gems/wet
// surfaces real specular life instead of the flat "plasticine" read.
const pmrem = new THREE.PMREMGenerator(renderer);
scene.environment = pmrem.fromScene(new RoomEnvironment(), 0.04).texture;
scene.environmentIntensity = 0.55;
pmrem.dispose();

// Core Systems
const textures = createProceduralTextures();
const sound = new SoundEngine();
const world = createWorld(scene, textures, isMobile);
const hero = createHero(scene, textures);
const dragon = createDragon(scene, textures);
const villagers = createVillagers(scene, textures);
const combat = createCombatSystem(scene, textures, sound);
const quests = createQuestSystem(sound);

// Game State
let gameStarted = false;
let questLogOpen = false;
let activeDialogueVillager = null;
let gameOverShown = false;

// Day-to-night cycle: 0.73 = warm golden sunset (default hero opening!)
// Full day takes ~240s, plus user can press [T] or click HUD clock to jump between Sunset / Night / Dawn / Day
let timeOfDay = 0.73;
let targetTimeOfDay = null;
const TIME_PRESETS = [0.73, 0.02, 0.27, 0.50]; // Sunset, Starry Full-Moon Night, Dawn, Day
let timePresetIdx = 0;

// Camera state
let camYaw = Math.PI; // Facing North (-Z) toward Village, Lake, Bridge & Castle
let camPitch = 0.24;
let camTransition = 0; // Smooth sweep from title flyover into third-person
let camShake = 0;
let prevHeroHp = hero.hp;

const camPosSmooth = new THREE.Vector3(0, 24, 92);
const camLookSmooth = new THREE.Vector3(0, 8, -10);
const desiredCamPos = new THREE.Vector3();
const desiredCamLook = new THREE.Vector3();

// Input state
const keys = {};
const input = {
  forward: 0,
  right: 0,
  jump: false,
  up: false,
  down: false,
  attackHeld: false,
  blockHeld: false,
};
const mouseDelta = { x: 0, y: 0 };
let pointerLocked = false;
let mouseDragging = false;
let touchLookId = null;
let touchLookX = 0;
let touchLookY = 0;
let jumpQueued = false;

// UI callbacks
function requestLock() {
  if (isMobile || questLogOpen || activeDialogueVillager || hero.dead) return;
  try {
    canvas.requestPointerLock?.();
  } catch {
    // Pointer lock optional
  }
}

function releaseLock() {
  if (document.pointerLockElement) {
    try { document.exitPointerLock(); } catch {}
  }
}

function handleStartGame() {
  if (gameStarted) return;
  sound.unlock();
  gameStarted = true;
  camTransition = 1.0;
  ui.showGameHUD(true, isMobile);
  ui.showToast('Welcome to Aethelgard', 'Speak to Elder Aldric [E] or mount the Red Dragon [F]', 'quest');
  requestLock();
}

function handleCycleTime() {
  timePresetIdx = (timePresetIdx + 1) % TIME_PRESETS.length;
  targetTimeOfDay = TIME_PRESETS[timePresetIdx];
  const labels = ['Golden Sunset', 'Starry Full Moon', 'Morning Dawn', 'High Noon'];
  ui.showToast(labels[timePresetIdx], 'Dynamic day-to-night sky', 'quest');
}

function handleToggleQuestLog(force) {
  if (!gameStarted || hero.dead) return;
  if (activeDialogueVillager) handleCloseDialogue();
  questLogOpen = typeof force === 'boolean' ? force : !questLogOpen;
  ui.toggleQuestLog(questLogOpen, quests);
  if (questLogOpen) releaseLock();
  else requestLock();
}

function handleCloseDialogue() {
  activeDialogueVillager = null;
  ui.closeDialogue();
  if (gameStarted && !questLogOpen && !hero.dead) requestLock();
}

function handleRespawn() {
  hero.respawn();
  combat.resetBoss();
  prevHeroHp = hero.hp;
  gameOverShown = false;
  ui.showGameOver(false, hero, quests);
  camYaw = Math.PI;
  camPitch = 0.24;
  ui.showToast('Returned to Oakhaven', 'Health and Stamina restored', 'quest');
  requestLock();
}

function findNearestInteractable() {
  if (hero.dead) return null;
  if (dragon.mounted) {
    return null; // Dragon flight controls are shown cleanly in bottom-left HUD strip
  }
  // Check nearest villager within 4.4m
  let bestV = null;
  let bestDist = 4.4;
  for (let i = 0; i < villagers.length; i++) {
    const d = villagers[i].position.distanceTo(hero.position);
    if (d < bestDist) {
      bestDist = d;
      bestV = villagers[i];
    }
  }
  if (bestV) {
    return { type: 'villager', villager: bestV, key: 'E', label: `Speak to ${bestV.name}` };
  }
  // Check Red Dragon within 9.5m
  const dragDist = dragon.position.distanceTo(hero.position);
  if (dragDist < 9.5) {
    return { type: 'mount', key: 'F', label: 'Mount Red Dragon' };
  }
  return null;
}

function handleInteract() {
  if (!gameStarted || hero.dead || questLogOpen) return;
  sound.unlock();
  if (activeDialogueVillager) {
    const q = quests.getQuestForVillager(activeDialogueVillager.id);
    if (q && q.status === 'available') {
      quests.acceptQuest(q.id);
      handleCloseDialogue();
    } else if (q && q.status === 'ready') {
      quests.turnInQuest(q.id, hero);
      handleCloseDialogue();
    } else {
      handleCloseDialogue();
    }
    return;
  }
  const target = findNearestInteractable();
  if (target && target.type === 'villager') {
    const v = target.villager;
    const q = quests.getQuestForVillager(v.id);
    if (!q) return;
    activeDialogueVillager = v;
    releaseLock();
    ui.openDialogue(v, q, hero, () => {
      if (q.status === 'available') quests.acceptQuest(q.id);
      else if (q.status === 'ready') quests.turnInQuest(q.id, hero);
      handleCloseDialogue();
    });
  }
}

function handleToggleDragon() {
  if (!gameStarted || hero.dead || questLogOpen || activeDialogueVillager) return;
  sound.unlock();
  if (dragon.mounted) {
    dragon.dismount(hero, sound);
    camYaw = dragon.yaw;
    camPitch = 0.24;
    ui.showToast('Landed Safely', 'On foot in Aethelgard', 'quest');
  } else {
    const dist = dragon.position.distanceTo(hero.position);
    if (dist < 11.5) {
      dragon.mount(hero, sound);
      ui.showToast('Dragon Mounted!', 'W Boost · Mouse Steer · Space/Shift Altitude · Click Fire', 'quest');
    } else {
      ui.showToast('Too Far from Dragon', 'Head to the Dragon Aerie (west of village)', 'quest');
    }
  }
}

const ui = createUI({
  onStart: handleStartGame,
  onCycleTime: handleCycleTime,
  onToggleQuestLog: handleToggleQuestLog,
  onToggleMute: () => {
    sound.setMuted(!sound.muted);
    return sound.muted;
  },
  onCloseDialogue: handleCloseDialogue,
  onRespawn: handleRespawn,
  onAttackTap: () => {
    sound.unlock();
    if (gameStarted && !dragon.mounted && !questLogOpen && !activeDialogueVillager) {
      hero.startAttack(sound);
    }
  },
  onJumpTap: () => { jumpQueued = true; },
  onRollTap: () => {
    sound.unlock();
    if (gameStarted && !dragon.mounted && !questLogOpen && !activeDialogueVillager) {
      hero.startRoll(input, camYaw, sound);
    }
  },
  onDragonTap: handleToggleDragon,
  onInteractTap: handleInteract,
});

// Keyboard Events
window.addEventListener('keydown', (e) => {
  const code = e.code;
  keys[code] = true;

  if (!gameStarted) {
    if (code === 'Enter' || code === 'Space') handleStartGame();
    return;
  }

  if (code === 'Escape') {
    if (activeDialogueVillager) handleCloseDialogue();
    else if (questLogOpen) handleToggleQuestLog(false);
    return;
  }
  if (hero.dead) {
    if (code === 'KeyR') handleRespawn();
    return;
  }

  if (code === 'KeyJ') {
    handleToggleQuestLog();
  } else if (code === 'KeyE') {
    handleInteract();
  } else if (code === 'KeyF') {
    handleToggleDragon();
  } else if (code === 'KeyQ' && !dragon.mounted && !questLogOpen && !activeDialogueVillager) {
    sound.unlock();
    hero.startRoll(input, camYaw, sound);
  } else if (code === 'Space') {
    jumpQueued = true;
  } else if (code === 'KeyT') {
    handleCycleTime();
  } else if (code === 'KeyM') {
    sound.setMuted(!sound.muted);
  }
});

window.addEventListener('keyup', (e) => {
  keys[e.code] = false;
});

// Mouse & Pointer Lock Events
document.addEventListener('pointerlockchange', () => {
  pointerLocked = document.pointerLockElement === canvas;
});

canvas.addEventListener('contextmenu', (e) => e.preventDefault());

canvas.addEventListener('mousedown', (e) => {
  if (!gameStarted || questLogOpen || activeDialogueVillager || hero.dead) return;
  sound.unlock();
  if (!pointerLocked && !isMobile) requestLock();
  if (e.button === 0) {
    input.attackHeld = true;
    if (!dragon.mounted) hero.startAttack(sound);
  } else if (e.button === 2) {
    input.blockHeld = true;
  } else if (e.button === 1) {
    mouseDragging = true;
  }
});

window.addEventListener('mouseup', (e) => {
  if (e.button === 0) input.attackHeld = false;
  if (e.button === 2) input.blockHeld = false;
  if (e.button === 1) mouseDragging = false;
});

window.addEventListener('mousemove', (e) => {
  if (!gameStarted || questLogOpen || activeDialogueVillager || hero.dead) return;
  if (pointerLocked || mouseDragging) {
    const dx = e.movementX || 0;
    const dy = e.movementY || 0;
    mouseDelta.x += dx;
    mouseDelta.y += dy;
    if (!dragon.mounted) {
      camYaw -= dx * 0.0024;
      camPitch = THREE.MathUtils.clamp(camPitch + dy * 0.002, -0.32, 0.92);
    }
  }
});

// Touch camera look on right side of screen
canvas.addEventListener('pointerdown', (e) => {
  if (!gameStarted || !isMobile || touchLookId !== null) return;
  if (e.clientX > window.innerWidth * 0.36) {
    touchLookId = e.pointerId;
    touchLookX = e.clientX;
    touchLookY = e.clientY;
  }
});
canvas.addEventListener('pointermove', (e) => {
  if (e.pointerId !== touchLookId) return;
  const dx = e.clientX - touchLookX;
  const dy = e.clientY - touchLookY;
  touchLookX = e.clientX;
  touchLookY = e.clientY;
  mouseDelta.x += dx * 1.4;
  mouseDelta.y += dy * 1.4;
  if (!dragon.mounted) {
    camYaw -= dx * 0.0042;
    camPitch = THREE.MathUtils.clamp(camPitch + dy * 0.0034, -0.25, 0.85);
  }
});
const clearTouchLook = (e) => {
  if (e.pointerId === touchLookId) touchLookId = null;
};
canvas.addEventListener('pointerup', clearTouchLook);
canvas.addEventListener('pointercancel', clearTouchLook);

window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});

// Debug & Automated Verification Hook
window.__GAME_DEBUG__ = {
  startGame: handleStartGame,
  setTimeOfDay(val) {
    timeOfDay = ((val % 1) + 1) % 1;
    targetTimeOfDay = null;
  },
  teleport(x, z, yaw = Math.PI) {
    if (dragon.mounted) dragon.dismount(hero, sound);
    hero.position.set(x, getWalkableHeight(x, z, 20), z);
    hero.velocity.set(0, 0, 0);
    hero.yaw = yaw;
    camYaw = yaw;
    camPitch = 0.22;
    camTransition = 0;
  },
  mountDragonAndFly(x = -8, y = 24, z = 12, yaw = Math.PI, breathe = true) {
    if (!dragon.mounted) dragon.mount(hero, sound);
    dragon.position.set(x, y, z);
    dragon.yaw = yaw;
    dragon.pitch = -0.06;
    dragon.speed = 26;
    dragon.fireEnergy = 100;
    input.attackHeld = breathe;
    camYaw = yaw;
    camTransition = 0;
  },
  setFireHeld(flag) {
    input.attackHeld = !!flag;
  },
  enterBossFight() {
    if (dragon.mounted) dragon.dismount(hero, sound);
    hero.position.set(0, LANDMARKS.castle.floorY, -97);
    hero.velocity.set(0, 0, 0);
    hero.yaw = Math.PI;
    camYaw = Math.PI;
    camPitch = 0.2;
    combat.boss.engaged = true;
    combat.boss.hp = Math.round(combat.boss.maxHp * 0.72);
    camTransition = 0;
  },
  enterDungeon() {
    if (dragon.mounted) dragon.dismount(hero, sound);
    hero.position.set(112, LANDMARKS.dungeonCenter.floorY, -24);
    hero.velocity.set(0, 0, 0);
    hero.yaw = Math.PI * 0.5;
    camYaw = Math.PI * 0.5;
    camPitch = 0.14;
    camTransition = 0;
  },
  openQuestLog(open = true) {
    handleToggleQuestLog(open);
  },
  openDialogueWith(villagerId = 'aldric') {
    if (dragon.mounted) dragon.dismount(hero, sound);
    const v = villagers.find((item) => item.id === villagerId);
    if (!v) return;
    hero.position.set(v.position.x, v.position.y, v.position.z + 3.2);
    hero.yaw = Math.PI;
    camYaw = Math.PI;
    camPitch = 0.2;
    activeDialogueVillager = v;
    releaseLock();
    const q = quests.getQuestForVillager(v.id);
    ui.openDialogue(v, q, hero, () => {
      if (q.status === 'available') quests.acceptQuest(q.id);
      else if (q.status === 'ready') quests.turnInQuest(q.id, hero);
      handleCloseDialogue();
    });
  },
  triggerGameOver() {
    hero.hp = 0;
    hero.dead = true;
    gameOverShown = true;
    releaseLock();
    ui.showGameOver(true, hero, quests);
  },
  respawn: handleRespawn,
  getStats() {
    return {
      calls: renderer.info.render.calls,
      triangles: renderer.info.render.triangles,
      geometries: renderer.info.memory.geometries,
      textures: renderer.info.memory.textures,
      heroPos: hero.position.toArray(),
      heroHp: hero.hp,
      gold: hero.gold,
      mounted: dragon.mounted,
      bossEngaged: combat.boss.engaged,
      bossHp: combat.boss.hp,
      timeOfDay,
      quests: quests.list.map((q) => ({ id: q.id, status: q.status, progress: q.progress })),
    };
  },
};

// Master Animation Loop (Three.js r186 Timer)
const timer = new THREE.Timer();
timer.connect(document);

function updateCamera(dt, elapsed) {
  if (!gameStarted) {
    // Cinematic Golden Hour Valley Flyover on Title Screen
    const angle = elapsed * 0.095;
    const cx = Math.sin(angle) * 58;
    const cz = 32 + Math.cos(angle) * 34;
    const cy = Math.max(getWalkableHeight(cx, cz, 25) + 12, 22 + Math.sin(elapsed * 0.25) * 4);
    camera.position.set(cx, cy, cz);
    camera.lookAt(0, 8, -32);
    camPosSmooth.copy(camera.position);
    camLookSmooth.set(0, 8, -32);
    return;
  }

  let shakeX = 0;
  let shakeY = 0;
  if (camShake > 0.005 && !reducedMotion) {
    shakeX = (Math.random() - 0.5) * camShake;
    shakeY = (Math.random() - 0.5) * camShake;
    camShake = Math.max(0, camShake - dt * 2.6);
  }

  if (dragon.mounted) {
    // Follow behind dragon slightly higher & angled so forward fire breath streams visibly into view
    camYaw = dragon.yaw;
    const cosP = Math.cos(dragon.pitch);
    const fwdX = Math.sin(dragon.yaw) * cosP;
    const fwdY = Math.sin(dragon.pitch);
    const fwdZ = Math.cos(dragon.yaw) * cosP;
    const rightX = -Math.cos(dragon.yaw);
    const rightZ = Math.sin(dragon.yaw);

    const dist = 15.2 + (dragon.speed - 18) * 0.12;
    desiredCamPos.set(
      dragon.position.x - fwdX * dist + rightX * 1.4,
      dragon.position.y - fwdY * dist + 6.4,
      dragon.position.z - fwdZ * dist + rightZ * 1.4,
    );
    const minGround = getWalkableHeight(desiredCamPos.x, desiredCamPos.z, desiredCamPos.y) + 2.0;
    if (desiredCamPos.y < minGround) desiredCamPos.y = minGround;

    desiredCamLook.set(
      dragon.position.x + fwdX * 18,
      dragon.position.y + fwdY * 18 + 0.8,
      dragon.position.z + fwdZ * 18,
    );

    const targetFov = 62 + Math.max(0, dragon.speed - 18) * 0.42;
    if (Math.abs(camera.fov - targetFov) > 0.05) {
      camera.fov = THREE.MathUtils.lerp(camera.fov, targetFov, Math.min(1, dt * 4));
      camera.updateProjectionMatrix();
    }
  } else {
    // Third-person shoulder/back follow camera behind hero
    const inDung = isInsideDungeon(hero.position.x, hero.position.z);
    const dist = inDung ? 5.6 : 6.8;
    const horiz = Math.cos(camPitch) * dist;
    const vert = Math.sin(camPitch) * dist;
    const fwdX = Math.sin(camYaw);
    const fwdZ = Math.cos(camYaw);

    desiredCamPos.set(
      hero.position.x - fwdX * horiz,
      hero.position.y + 2.15 + vert,
      hero.position.z - fwdZ * horiz,
    );

    const groundFloor = getWalkableHeight(desiredCamPos.x, desiredCamPos.z, desiredCamPos.y) + 0.65;
    if (desiredCamPos.y < groundFloor) desiredCamPos.y = groundFloor;
    if (inDung && desiredCamPos.y > LANDMARKS.dungeonCenter.ceilingY - 1.2) {
      desiredCamPos.y = LANDMARKS.dungeonCenter.ceilingY - 1.2;
    }

    desiredCamLook.set(
      hero.position.x + fwdX * 2.5,
      hero.position.y + 1.65 - camPitch * 0.8,
      hero.position.z + fwdZ * 2.5,
    );

    if (Math.abs(camera.fov - 60) > 0.05) {
      camera.fov = THREE.MathUtils.lerp(camera.fov, 60, Math.min(1, dt * 5));
      camera.updateProjectionMatrix();
    }
  }

  const followLerp = camTransition > 0 ? Math.min(1, dt * 4.5) : Math.min(1, dt * 11);
  if (camTransition > 0) camTransition = Math.max(0, camTransition - dt * 1.1);

  camPosSmooth.lerp(desiredCamPos, followLerp);
  camLookSmooth.lerp(desiredCamLook, followLerp);
  camera.position.set(camPosSmooth.x + shakeX, camPosSmooth.y + shakeY, camPosSmooth.z);
  camera.lookAt(camLookSmooth);
}

function animate() {
  timer.update();
  const dt = Math.min(timer.getDelta(), 0.05);
  const elapsed = timer.getElapsed();

  // Dynamic Day-to-Night cycle
  if (targetTimeOfDay !== null) {
    let diff = targetTimeOfDay - timeOfDay;
    if (diff > 0.5) diff -= 1;
    if (diff < -0.5) diff += 1;
    if (Math.abs(diff) < 0.004) {
      timeOfDay = targetTimeOfDay;
      targetTimeOfDay = null;
    } else {
      timeOfDay = ((timeOfDay + diff * Math.min(1, dt * 4.5)) % 1 + 1) % 1;
    }
  } else if (gameStarted) {
    // Gentle real-time cycle (~4 minutes per full day)
    timeOfDay = (timeOfDay + dt / 240) % 1;
  }

  // Gather merged keyboard + touch input
  const pausedForUI = !gameStarted || questLogOpen || !!activeDialogueVillager || hero.dead;
  if (pausedForUI) {
    input.forward = 0;
    input.right = 0;
    input.jump = false;
    input.up = false;
    input.down = false;
    input.blockHeld = false;
  } else {
    let fwd = (keys.KeyW || keys.ArrowUp ? 1 : 0) - (keys.KeyS || keys.ArrowDown ? 1 : 0);
    let rgt = (keys.KeyD || keys.ArrowRight ? 1 : 0) - (keys.KeyA || keys.ArrowLeft ? 1 : 0);
    if (Math.abs(ui.touchState.moveY) > 0.05) fwd = ui.touchState.moveY;
    if (Math.abs(ui.touchState.moveX) > 0.05) rgt = ui.touchState.moveX;
    input.forward = fwd;
    input.right = rgt;
    input.jump = jumpQueued;
    input.up = !!(keys.Space || ui.touchState.upHeld);
    input.down = !!(keys.ShiftLeft || keys.ShiftRight || keys.ControlLeft);
    if (isMobile) {
      input.attackHeld = ui.touchState.attackHeld;
      input.blockHeld = ui.touchState.blockHeld;
    }
  }
  jumpQueued = false;

  // Update entities & world
  const focusPos = dragon.mounted ? dragon.position : hero.position;
  hero.update(dt, elapsed, input, camYaw, dragon.mounted, sound);
  dragon.update(dt, elapsed, input, mouseDelta, hero, sound);
  mouseDelta.x = 0;
  mouseDelta.y = 0;

  for (let i = 0; i < villagers.length; i++) {
    villagers[i].update(dt, elapsed, focusPos);
  }

  if (gameStarted) {
    combat.update(dt, elapsed, hero, dragon, quests, world);
  }

  // Trigger hit feedback / camera shake on damage
  if (hero.hp < prevHeroHp) {
    ui.triggerDamageFlash();
    camShake = Math.max(camShake, 0.34);
  }
  prevHeroHp = hero.hp;

  if (hero.dead && !gameOverShown) {
    gameOverShown = true;
    releaseLock();
    ui.showGameOver(true, hero, quests);
  }

  world.update(dt, elapsed, focusPos, timeOfDay);
  updateCamera(dt, elapsed);

  if (gameStarted) {
    ui.update(dt, {
      hero,
      dragon,
      villagers,
      combat,
      quests,
      camera,
      camYaw,
      timeOfDay,
      interactTarget: pausedForUI ? null : findNearestInteractable(),
      world,
    });
  }

  renderer.render(scene, camera);
}

// Pre-compile shaders cleanly before starting loop
renderer.compileAsync(scene, camera).catch(() => {}).finally(() => {
  renderer.setAnimationLoop(animate);
});
