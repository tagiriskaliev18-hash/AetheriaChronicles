// ============================================================================
// src/ui.js — Complete Fantasy RPG HUD, Compass, Minimap, Quests, Dialogue & Touch
// ============================================================================
import * as THREE from 'three';
import { LANDMARKS, WORLD, getTerrainHeight, isInLake, isOnBridge, isInsideDungeon } from './config.js';

export function createUI(callbacks) {
  const root = document.getElementById('ui-root');
  root.innerHTML = `
    <!-- Floating 3D-Projected Labels Layer -->
    <div id="world-labels" class="world-labels"></div>

    <!-- Subtle Vignette & Damage Flash -->
    <div id="damage-vignette" class="damage-vignette"></div>

    <!-- Center Aim Reticle -->
    <div id="reticle" class="reticle hidden">
      <div class="reticle-dot"></div>
      <div class="reticle-ring"></div>
    </div>

    <!-- Interaction Prompt -->
    <div id="interact-prompt" class="interact-prompt hidden">
      <span class="key-badge" id="interact-key">E</span>
      <span id="interact-text">Speak</span>
    </div>

    <!-- Toast Notifications -->
    <div id="toast-container" class="toast-container"></div>

    <!-- TOP CENTER: Ornate Compass + Boss Bar -->
    <div id="hud-top" class="hud-top hidden">
      <div class="compass-frame">
        <div class="compass-caret"></div>
        <div class="compass-viewport">
          <div id="compass-strip" class="compass-strip"></div>
          <div id="compass-markers" class="compass-markers"></div>
        </div>
      </div>

      <div id="boss-bar-wrap" class="boss-bar-wrap hidden">
        <div class="boss-header">
          <span id="boss-name" class="boss-name">MALAKOR, THE GRAVE SOVEREIGN</span>
          <span id="boss-pct" class="boss-pct">100%</span>
        </div>
        <div class="boss-track">
          <div id="boss-lag" class="boss-lag"></div>
          <div id="boss-fill" class="boss-fill"></div>
        </div>
        <div id="boss-warning" class="boss-warning hidden">CRIMSON CLEAVE</div>
      </div>
    </div>

    <!-- TOP RIGHT: Gold, Time, Quick Actions & Tracked Quest -->
    <div id="hud-top-right" class="hud-top-right hidden">
      <div class="hud-pills-row">
        <div class="hud-pill gold-pill" title="Gold">
          <svg class="coin-icon" viewBox="0 0 24 24" aria-hidden="true">
            <circle cx="12" cy="12" r="9.5" fill="url(#goldGrad)" stroke="#ffe599" stroke-width="1.4"/>
            <defs><linearGradient id="goldGrad" x1="0" y1="0" x2="1" y2="1"><stop offset="0%" stop-color="#ffe082"/><stop offset="100%" stop-color="#b8771e"/></linearGradient></defs>
            <path d="M12 6.8l2.2 5.2L12 17.2 9.8 12z" fill="#fff3c4" opacity="0.85"/>
          </svg>
          <span id="hud-gold" class="mono-num">0</span>
        </div>
        <button id="btn-time" class="hud-pill hud-btn" type="button" title="Cycle Time of Day [T]">
          <span id="hud-time-icon">☀</span>
          <span id="hud-time-label">Sunset</span>
        </button>
        <button id="btn-quests" class="hud-pill hud-btn" type="button" title="Quest Log [J]">
          <span>Quests</span><kbd>J</kbd>
        </button>
        <button id="btn-mute" class="hud-pill hud-btn icon-only" type="button" title="Toggle Sound [M]">🔊</button>
      </div>
      <div id="tracked-quest-card" class="tracked-quest-card">
        <div class="tq-label">ACTIVE QUEST</div>
        <div id="tq-title" class="tq-title">Speak to Elder Aldric</div>
        <div id="tq-progress" class="tq-progress">Oakhaven Village</div>
      </div>
    </div>

    <!-- BOTTOM LEFT: Health, Stamina & Dragon Fire Bars -->
    <div id="hud-bottom-left" class="hud-bottom-left hidden">
      <div id="zone-indicator" class="zone-indicator">OAKHAVEN VALLEY</div>
      <div class="bars-panel">
        <div class="bar-row">
          <span class="bar-label">HP</span>
          <div class="bar-track hp-track">
            <div id="hp-lag" class="bar-lag"></div>
            <div id="hp-fill" class="bar-fill hp-fill"></div>
          </div>
          <span id="hp-text" class="bar-num mono-num">100</span>
        </div>
        <div class="bar-row">
          <span class="bar-label">SP</span>
          <div class="bar-track sp-track">
            <div id="sp-fill" class="bar-fill sp-fill"></div>
          </div>
          <span id="sp-text" class="bar-num mono-num">100</span>
        </div>
        <div id="dragon-bar-row" class="bar-row hidden">
          <span class="bar-label fire-lbl">🔥</span>
          <div class="bar-track fire-track">
            <div id="fire-fill" class="bar-fill fire-fill"></div>
          </div>
          <span id="fire-text" class="bar-num mono-num">100</span>
        </div>
      </div>
      <div id="controls-strip" class="controls-strip">
        <span><b>LMB</b> Slash</span>
        <span><b>RMB</b> Block</span>
        <span><b>Q</b> Roll</span>
        <span><b>E</b> Talk</span>
        <span><b>F</b> Dragon</span>
        <span><b>J</b> Quests</span>
        <span><b>T</b> Time</span>
      </div>
      <div id="dragon-controls-strip" class="controls-strip hidden">
        <span><b>W/S</b> Speed</span>
        <span><b>Mouse</b> Steer</span>
        <span><b>Space</b> Up</span>
        <span><b>Shift</b> Down</span>
        <span><b>LMB</b> Fire Breath</span>
        <span><b>F</b> Land</span>
      </div>
    </div>

    <!-- BOTTOM RIGHT: Round Minimap -->
    <div id="hud-bottom-right" class="hud-bottom-right hidden">
      <div class="minimap-shell">
        <canvas id="minimap-canvas" width="160" height="160"></canvas>
        <div class="minimap-n">N</div>
      </div>
    </div>

    <!-- MOBILE TOUCH CONTROLS -->
    <div id="touch-controls" class="touch-controls hidden">
      <div id="touch-stick-zone" class="touch-stick-zone">
        <div class="touch-stick-base">
          <div id="touch-stick-knob" class="touch-stick-knob"></div>
        </div>
      </div>
      <div class="touch-actions">
        <button id="t-btn-interact" class="t-btn t-btn-gold hidden" type="button">E</button>
        <button id="t-btn-dragon" class="t-btn t-btn-crimson" type="button">🐉</button>
        <button id="t-btn-roll" class="t-btn" type="button">Roll</button>
        <button id="t-btn-block" class="t-btn" type="button">Block</button>
        <button id="t-btn-jump" class="t-btn" type="button">Jump</button>
        <button id="t-btn-attack" class="t-btn t-btn-primary" type="button">⚔</button>
      </div>
    </div>

    <!-- DIALOGUE BOX (Villagers) -->
    <div id="dialogue-modal" class="dialogue-modal hidden">
      <div class="dialogue-card">
        <div class="dialogue-head">
          <div>
            <div id="dlg-name" class="dlg-name">Elder Aldric</div>
            <div id="dlg-title" class="dlg-title">Village Elder</div>
          </div>
          <div id="dlg-reward" class="dlg-reward">Reward: 120 Gold</div>
        </div>
        <p id="dlg-body" class="dlg-body"></p>
        <div class="dialogue-actions">
          <button id="dlg-primary" class="rpg-btn rpg-btn-gold" type="button">Accept Quest [E]</button>
          <button id="dlg-close" class="rpg-btn rpg-btn-ghost" type="button">Leave [Esc]</button>
        </div>
      </div>
    </div>

    <!-- QUEST LOG MODAL [J] -->
    <div id="quest-modal" class="overlay-modal hidden">
      <div class="modal-panel">
        <div class="modal-head">
          <div>
            <div class="modal-eyebrow">CHRONICLE OF OAKHAVEN</div>
            <h2 class="modal-title">Quest Log</h2>
          </div>
          <button id="btn-close-quests" class="modal-close" type="button" aria-label="Close">✕</button>
        </div>
        <div id="quest-list" class="quest-list"></div>
      </div>
    </div>

    <!-- TITLE SCREEN OVER LIVE 3D WORLD -->
    <div id="title-screen" class="title-screen">
      <div class="title-content">
        <div class="title-crest">AETHELGARD CHRONICLES</div>
        <h1 class="title-main">ASHES OF THE DRAKE</h1>
        <p class="title-sub">An open-world 3D fantasy realm forged entirely in code — ride the crimson dragon, brave the lava crypt, and vanquish Highkeep’s Grave Sovereign.</p>
        <div class="title-actions">
          <button id="btn-start" class="rpg-btn rpg-btn-hero" type="button">Enter the Realm</button>
        </div>
        <div class="title-keys">
          <span><b>WASD</b> Move</span>
          <span><b>Mouse</b> Look</span>
          <span><b>Space</b> Jump</span>
          <span><b>LMB / RMB / Q</b> Combat</span>
          <span><b>F</b> Ride Dragon</span>
          <span><b>E / J</b> Quests</span>
        </div>
      </div>
    </div>

    <!-- GAME OVER / RESPAWN SCREEN -->
    <div id="gameover-screen" class="overlay-modal hidden">
      <div class="gameover-panel">
        <div class="go-eyebrow">THE FLAME FADES</div>
        <h2 class="go-title">YOU HAVE FALLEN</h2>
        <div id="go-stats" class="go-stats"></div>
        <button id="btn-respawn" class="rpg-btn rpg-btn-hero" type="button">Respawn at Oakhaven [R]</button>
      </div>
    </div>
  `;

  // Element cache
  const el = {
    worldLabels: root.querySelector('#world-labels'),
    damageVig: root.querySelector('#damage-vignette'),
    reticle: root.querySelector('#reticle'),
    interactPrompt: root.querySelector('#interact-prompt'),
    interactKey: root.querySelector('#interact-key'),
    interactText: root.querySelector('#interact-text'),
    toasts: root.querySelector('#toast-container'),
    hudTop: root.querySelector('#hud-top'),
    compassStrip: root.querySelector('#compass-strip'),
    compassMarkers: root.querySelector('#compass-markers'),
    bossWrap: root.querySelector('#boss-bar-wrap'),
    bossName: root.querySelector('#boss-name'),
    bossPct: root.querySelector('#boss-pct'),
    bossFill: root.querySelector('#boss-fill'),
    bossLag: root.querySelector('#boss-lag'),
    bossWarning: root.querySelector('#boss-warning'),
    hudTopRight: root.querySelector('#hud-top-right'),
    hudGold: root.querySelector('#hud-gold'),
    btnTime: root.querySelector('#btn-time'),
    timeIcon: root.querySelector('#hud-time-icon'),
    timeLabel: root.querySelector('#hud-time-label'),
    btnQuests: root.querySelector('#btn-quests'),
    btnMute: root.querySelector('#btn-mute'),
    tqTitle: root.querySelector('#tq-title'),
    tqProgress: root.querySelector('#tq-progress'),
    hudBottomLeft: root.querySelector('#hud-bottom-left'),
    zoneIndicator: root.querySelector('#zone-indicator'),
    hpFill: root.querySelector('#hp-fill'),
    hpLag: root.querySelector('#hp-lag'),
    hpText: root.querySelector('#hp-text'),
    spFill: root.querySelector('#sp-fill'),
    spText: root.querySelector('#sp-text'),
    dragonBarRow: root.querySelector('#dragon-bar-row'),
    fireFill: root.querySelector('#fire-fill'),
    fireText: root.querySelector('#fire-text'),
    controlsStrip: root.querySelector('#controls-strip'),
    dragonControlsStrip: root.querySelector('#dragon-controls-strip'),
    hudBottomRight: root.querySelector('#hud-bottom-right'),
    minimapCanvas: root.querySelector('#minimap-canvas'),
    touchControls: root.querySelector('#touch-controls'),
    touchStickZone: root.querySelector('#touch-stick-zone'),
    touchStickKnob: root.querySelector('#touch-stick-knob'),
    tBtnInteract: root.querySelector('#t-btn-interact'),
    tBtnDragon: root.querySelector('#t-btn-dragon'),
    tBtnRoll: root.querySelector('#t-btn-roll'),
    tBtnBlock: root.querySelector('#t-btn-block'),
    tBtnJump: root.querySelector('#t-btn-jump'),
    tBtnAttack: root.querySelector('#t-btn-attack'),
    dialogueModal: root.querySelector('#dialogue-modal'),
    dlgName: root.querySelector('#dlg-name'),
    dlgTitle: root.querySelector('#dlg-title'),
    dlgReward: root.querySelector('#dlg-reward'),
    dlgBody: root.querySelector('#dlg-body'),
    dlgPrimary: root.querySelector('#dlg-primary'),
    dlgClose: root.querySelector('#dlg-close'),
    questModal: root.querySelector('#quest-modal'),
    questList: root.querySelector('#quest-list'),
    btnCloseQuests: root.querySelector('#btn-close-quests'),
    titleScreen: root.querySelector('#title-screen'),
    btnStart: root.querySelector('#btn-start'),
    gameoverScreen: root.querySelector('#gameover-screen'),
    goStats: root.querySelector('#go-stats'),
    btnRespawn: root.querySelector('#btn-respawn'),
  };

  // Build repeating compass ticks (-360..720 degrees so wrap is seamless)
  const cardinals = { 0: 'N', 45: 'NE', 90: 'E', 135: 'SE', 180: 'S', 225: 'SW', 270: 'W', 315: 'NW' };
  const PX_PER_DEG = 2.6;
  let stripHtml = '';
  for (let deg = -180; deg <= 540; deg += 15) {
    const norm = ((deg % 360) + 360) % 360;
    const label = cardinals[norm];
    const x = (deg + 180) * PX_PER_DEG;
    if (label) {
      const major = norm % 90 === 0;
      stripHtml += `<span class="c-dir ${major ? 'c-major' : ''}" style="left:${x}px">${label}</span>`;
    } else {
      stripHtml += `<span class="c-tick" style="left:${x}px"></span>`;
    }
  }
  el.compassStrip.innerHTML = stripHtml;

  // Pre-create pooled compass markers
  const compassMarkerEls = [];
  for (let i = 0; i < 6; i++) {
    const m = document.createElement('div');
    m.className = 'c-marker hidden';
    el.compassMarkers.appendChild(m);
    compassMarkerEls.push(m);
  }

  // Pre-create pooled world floating labels (villagers + enemies + dmg numbers)
  const villagerLabels = [];
  for (let i = 0; i < 6; i++) {
    const d = document.createElement('div');
    d.className = 'npc-label hidden';
    d.innerHTML = `<span class="npc-badge"></span><span class="npc-name"></span>`;
    el.worldLabels.appendChild(d);
    villagerLabels.push({ el: d, badge: d.querySelector('.npc-badge'), name: d.querySelector('.npc-name') });
  }

  const enemyBars = [];
  for (let i = 0; i < 14; i++) {
    const d = document.createElement('div');
    d.className = 'enemy-bar hidden';
    d.innerHTML = `<div class="eb-name"></div><div class="eb-track"><div class="eb-fill"></div></div>`;
    el.worldLabels.appendChild(d);
    enemyBars.push({ el: d, name: d.querySelector('.eb-name'), fill: d.querySelector('.eb-fill') });
  }

  const dmgEls = [];
  for (let i = 0; i < 18; i++) {
    const d = document.createElement('div');
    d.className = 'dmg-num hidden';
    el.worldLabels.appendChild(d);
    dmgEls.push(d);
  }

  // Pre-render static Minimap terrain basemap offscreen (fast 240x240 lookup)
  const mapRes = 240;
  const mapOffscreen = document.createElement('canvas');
  mapOffscreen.width = mapRes;
  mapOffscreen.height = mapRes;
  const mOffCtx = mapOffscreen.getContext('2d');
  const imgData = mOffCtx.createImageData(mapRes, mapRes);
  for (let py = 0; py < mapRes; py++) {
    for (let px = 0; px < mapRes; px++) {
      const wx = ((px / mapRes) - 0.5) * WORLD.SIZE;
      const wz = ((py / mapRes) - 0.5) * WORLD.SIZE;
      const idx = (py * mapRes + px) * 4;
      let r = 34, g = 62, b = 38;
      if (isInsideDungeon(wx, wz)) {
        r = 52; g = 24; b = 22;
      } else if (isOnBridge(wx, wz)) {
        r = 165; g = 152; b = 128;
      } else if (isInLake(wx, wz)) {
        r = 26; g = 78; b = 112;
      } else {
        const h = getTerrainHeight(wx, wz);
        if (Math.hypot(wx - LANDMARKS.castle.x, wz - LANDMARKS.castle.z) < 30) {
          r = 98; g = 102; b = 112;
        } else if (Math.hypot(wx - LANDMARKS.village.x, wz - LANDMARKS.village.z) < 22) {
          r = 92; g = 84; b = 66;
        } else if (h > 22) {
          r = 68; g = 72; b = 78;
        } else {
          const shade = Math.max(0, Math.min(1, (h - 1) / 16));
          r = Math.round(30 + shade * 30);
          g = Math.round(58 + shade * 38);
          b = Math.round(36 + shade * 18);
        }
      }
      imgData.data[idx] = r;
      imgData.data[idx + 1] = g;
      imgData.data[idx + 2] = b;
      imgData.data[idx + 3] = 255;
    }
  }
  mOffCtx.putImageData(imgData, 0, 0);
  const minimapCtx = el.minimapCanvas.getContext('2d');

  // Wire buttons
  el.btnStart.addEventListener('click', () => callbacks.onStart());
  el.btnTime.addEventListener('click', () => callbacks.onCycleTime());
  el.btnQuests.addEventListener('click', () => callbacks.onToggleQuestLog());
  el.btnCloseQuests.addEventListener('click', () => callbacks.onToggleQuestLog(false));
  el.btnMute.addEventListener('click', () => {
    const muted = callbacks.onToggleMute();
    el.btnMute.textContent = muted ? '🔇' : '🔊';
  });
  el.btnRespawn.addEventListener('click', () => callbacks.onRespawn());
  el.dlgClose.addEventListener('click', () => callbacks.onCloseDialogue());

  // Touch virtual joystick & button state
  const touchState = { moveX: 0, moveY: 0, attackHeld: false, blockHeld: false, upHeld: false };
  let stickId = null;
  let stickOriginX = 0;
  let stickOriginY = 0;

  el.touchStickZone.addEventListener('pointerdown', (e) => {
    e.stopPropagation();
    stickId = e.pointerId;
    const rect = el.touchStickZone.getBoundingClientRect();
    stickOriginX = rect.left + rect.width * 0.5;
    stickOriginY = rect.top + rect.height * 0.5;
    el.touchStickZone.setPointerCapture(e.pointerId);
  });
  el.touchStickZone.addEventListener('pointermove', (e) => {
    if (e.pointerId !== stickId) return;
    e.stopPropagation();
    const maxR = 40;
    let dx = e.clientX - stickOriginX;
    let dy = e.clientY - stickOriginY;
    const len = Math.hypot(dx, dy);
    if (len > maxR) {
      dx = (dx / len) * maxR;
      dy = (dy / len) * maxR;
    }
    el.touchStickKnob.style.transform = `translate(${dx}px, ${dy}px)`;
    touchState.moveX = dx / maxR;
    touchState.moveY = -dy / maxR;
  });
  const resetStick = (e) => {
    if (e.pointerId !== stickId) return;
    stickId = null;
    el.touchStickKnob.style.transform = 'translate(0px, 0px)';
    touchState.moveX = 0;
    touchState.moveY = 0;
  };
  el.touchStickZone.addEventListener('pointerup', resetStick);
  el.touchStickZone.addEventListener('pointercancel', resetStick);

  el.tBtnAttack.addEventListener('pointerdown', (e) => { e.stopPropagation(); touchState.attackHeld = true; callbacks.onAttackTap(); });
  el.tBtnAttack.addEventListener('pointerup', () => { touchState.attackHeld = false; });
  el.tBtnAttack.addEventListener('pointercancel', () => { touchState.attackHeld = false; });
  el.tBtnBlock.addEventListener('pointerdown', (e) => { e.stopPropagation(); touchState.blockHeld = true; });
  el.tBtnBlock.addEventListener('pointerup', () => { touchState.blockHeld = false; });
  el.tBtnBlock.addEventListener('pointercancel', () => { touchState.blockHeld = false; });
  el.tBtnJump.addEventListener('pointerdown', (e) => { e.stopPropagation(); touchState.upHeld = true; callbacks.onJumpTap(); });
  el.tBtnJump.addEventListener('pointerup', () => { touchState.upHeld = false; });
  el.tBtnJump.addEventListener('pointercancel', () => { touchState.upHeld = false; });
  el.tBtnRoll.addEventListener('pointerdown', (e) => { e.stopPropagation(); callbacks.onRollTap(); });
  el.tBtnDragon.addEventListener('pointerdown', (e) => { e.stopPropagation(); callbacks.onDragonTap(); });
  el.tBtnInteract.addEventListener('pointerdown', (e) => { e.stopPropagation(); callbacks.onInteractTap(); });

  let displayedHpLag = 100;
  let displayedBossLag = 100;
  let damageFlashAlpha = 0;
  let dialogueActionCb = null;
  let lastZone = '';
  const projVec = new THREE.Vector3();

  el.dlgPrimary.addEventListener('click', () => {
    if (dialogueActionCb) dialogueActionCb();
  });

  function showToast(title, subtitle, type = 'quest') {
    const t = document.createElement('div');
    t.className = `rpg-toast toast-${type}`;
    t.innerHTML = `<div class="toast-title">${title}</div>${subtitle ? `<div class="toast-sub">${subtitle}</div>` : ''}`;
    el.toasts.appendChild(t);
    setTimeout(() => t.classList.add('show'), 20);
    setTimeout(() => {
      t.classList.remove('show');
      setTimeout(() => t.remove(), 420);
    }, 3600);
  }

  function showGameHUD(show, isTouch) {
    const m = show ? 'remove' : 'add';
    el.hudTop.classList[m]('hidden');
    el.hudTopRight.classList[m]('hidden');
    el.hudBottomLeft.classList[m]('hidden');
    el.hudBottomRight.classList[m]('hidden');
    el.reticle.classList[m]('hidden');
    if (show && isTouch) el.touchControls.classList.remove('hidden');
    else el.touchControls.classList.add('hidden');
    if (show) el.titleScreen.classList.add('hidden');
  }

  function openDialogue(villager, quest, hero, onAction) {
    el.dlgName.textContent = villager.name;
    el.dlgTitle.textContent = villager.title;
    let body = quest.dialogue.offer;
    let btnLabel = 'Accept Quest [E]';
    let showPrimary = true;

    if (quest.status === 'available') {
      body = quest.dialogue.offer;
      btnLabel = 'Accept Quest [E]';
    } else if (quest.status === 'active') {
      body = `${quest.dialogue.active} (${quest.progress}/${quest.target})`;
      showPrimary = false;
    } else if (quest.status === 'ready') {
      body = quest.dialogue.turnin;
      btnLabel = `Claim ${quest.rewardGold} Gold [E]`;
    } else {
      body = quest.dialogue.after;
      showPrimary = false;
    }

    el.dlgReward.textContent = quest.status === 'completed'
      ? 'Completed'
      : `Reward: ${quest.rewardGold} Gold${quest.bonusText ? ' · ' + quest.bonusText : ''}`;
    el.dlgBody.textContent = body;
    el.dlgPrimary.textContent = btnLabel;
    el.dlgPrimary.classList.toggle('hidden', !showPrimary);
    dialogueActionCb = showPrimary ? onAction : null;
    el.dialogueModal.classList.remove('hidden');
  }

  function closeDialogue() {
    el.dialogueModal.classList.add('hidden');
    dialogueActionCb = null;
  }

  function renderQuestLog(quests) {
    el.questList.innerHTML = quests.list.map((q) => {
      const statusLabel = q.status === 'completed'
        ? 'Completed'
        : q.status === 'ready'
          ? 'Ready to Turn In'
          : q.status === 'active'
            ? `Active · ${q.progress}/${q.target}`
            : 'Available';
      const tracked = quests.trackedId === q.id && q.status !== 'completed';
      return `
        <div class="q-item q-status-${q.status} ${tracked ? 'q-tracked' : ''}">
          <div class="q-item-top">
            <div>
              <div class="q-item-title">${q.title}</div>
              <div class="q-item-giver">Given by ${q.giverName}</div>
            </div>
            <span class="q-badge q-badge-${q.status}">${statusLabel}</span>
          </div>
          <p class="q-item-desc">${q.summary}</p>
          <div class="q-item-foot">
            <span class="q-obj">${q.objectiveText} (${Math.min(q.progress, q.target)}/${q.target})</span>
            <div class="q-foot-right">
              <span class="q-gold">${q.rewardGold}g${q.bonusText ? ` + ${q.bonusText}` : ''}</span>
              ${q.status !== 'completed' ? `<button class="q-track-btn" data-qid="${q.id}" type="button">${tracked ? 'Tracked' : 'Track'}</button>` : ''}
            </div>
          </div>
        </div>
      `;
    }).join('');
    el.questList.querySelectorAll('.q-track-btn').forEach((btn) => {
      btn.addEventListener('click', () => {
        quests.setTrackedQuest(btn.dataset.qid);
        renderQuestLog(quests);
      });
    });
  }

  function toggleQuestLog(show, quests) {
    if (show) {
      renderQuestLog(quests);
      el.questModal.classList.remove('hidden');
    } else {
      el.questModal.classList.add('hidden');
    }
  }

  function showGameOver(show, hero, quests) {
    if (show) {
      const done = quests.list.filter((q) => q.status === 'completed').length;
      el.goStats.innerHTML = `
        <div class="go-stat"><span>Gold</span><strong>${hero.gold}</strong></div>
        <div class="go-stat"><span>Foes Slain</span><strong>${hero.slainCount}</strong></div>
        <div class="go-stat"><span>Quests</span><strong>${done} / ${quests.list.length}</strong></div>
      `;
      el.gameoverScreen.classList.remove('hidden');
    } else {
      el.gameoverScreen.classList.add('hidden');
    }
  }

  function triggerDamageFlash() {
    damageFlashAlpha = 0.72;
  }

  function getZoneName(x, z, mounted) {
    if (mounted) return 'SKIES OF AETHELGARD';
    if (isInsideDungeon(x, z)) return 'EMBER CRYPT · DARK DUNGEON';
    if (Math.hypot(x - LANDMARKS.castle.x, z - LANDMARKS.castle.z) < 35) return 'HIGHKEEP CASTLE';
    if (isOnBridge(x, z)) return 'STONE BRIDGE';
    if (Math.hypot(x - LANDMARKS.village.x, z - LANDMARKS.village.z) < 30) return 'OAKHAVEN VILLAGE';
    if (Math.hypot(x - LANDMARKS.ruins.x, z - LANDMARKS.ruins.z) < 20) return 'ANCIENT RUNE CIRCLE';
    if (Math.hypot(x - LANDMARKS.dragonPerch.x, z - LANDMARKS.dragonPerch.z) < 18) return 'DRAGON AERIE';
    return 'BLUE-LEAF VALLEY';
  }

  function getTimePhase(t) {
    if (t >= 0.21 && t < 0.34) return { icon: '🌅', label: 'Dawn' };
    if (t >= 0.34 && t < 0.66) return { icon: '☀', label: 'Day' };
    if (t >= 0.66 && t < 0.81) return { icon: '🌇', label: 'Sunset' };
    return { icon: '🌕', label: 'Night' };
  }

  function update(dt, state) {
    const { hero, dragon, villagers, combat, quests, camera, camYaw, timeOfDay, interactTarget, world } = state;
    const focusPos = dragon.mounted ? dragon.position : hero.position;
    const width = window.innerWidth;
    const height = window.innerHeight;

    // 1. Damage vignette fade
    if (damageFlashAlpha > 0.005) {
      damageFlashAlpha = Math.max(0, damageFlashAlpha - dt * 1.9);
      el.damageVig.style.opacity = damageFlashAlpha.toFixed(3);
    } else {
      el.damageVig.style.opacity = '0';
    }

    // 2. Pop quest notifications
    const notes = quests.popNotifications();
    for (let i = 0; i < notes.length; i++) {
      showToast(notes[i].title, notes[i].subtitle, notes[i].type);
    }

    // 3. Health, Stamina, Dragon Fire, Gold, Time & Zone
    const hpPct = (hero.hp / hero.maxHp) * 100;
    displayedHpLag += (hpPct - displayedHpLag) * Math.min(1, dt * 4.5);
    el.hpFill.style.width = `${hpPct.toFixed(1)}%`;
    el.hpLag.style.width = `${Math.max(hpPct, displayedHpLag).toFixed(1)}%`;
    el.hpText.textContent = Math.ceil(hero.hp);

    const spPct = (hero.stamina / hero.maxStamina) * 100;
    el.spFill.style.width = `${spPct.toFixed(1)}%`;
    el.spText.textContent = Math.ceil(hero.stamina);

    el.dragonBarRow.classList.toggle('hidden', !dragon.mounted);
    el.controlsStrip.classList.toggle('hidden', dragon.mounted);
    el.dragonControlsStrip.classList.toggle('hidden', !dragon.mounted);
    el.reticle.classList.toggle('dragon-mode', dragon.mounted);
    if (dragon.mounted) {
      el.fireFill.style.width = `${dragon.fireEnergy.toFixed(1)}%`;
      el.fireText.textContent = Math.ceil(dragon.fireEnergy);
    }

    el.hudGold.textContent = hero.gold;
    const phase = getTimePhase(timeOfDay);
    el.timeIcon.textContent = phase.icon;
    el.timeLabel.textContent = phase.label;

    const zone = getZoneName(focusPos.x, focusPos.z, dragon.mounted);
    if (zone !== lastZone) {
      lastZone = zone;
      el.zoneIndicator.textContent = zone;
    }

    // Tracked quest mini-card
    const tq = quests.getTrackedQuest();
    if (tq) {
      el.tqTitle.textContent = tq.title;
      el.tqProgress.textContent = tq.status === 'available'
        ? `Talk to ${tq.giverName}`
        : tq.status === 'ready'
          ? `Return to ${tq.giverName}`
          : `${tq.objectiveText} (${tq.progress}/${tq.target})`;
    } else {
      el.tqTitle.textContent = 'All Quests Complete';
      el.tqProgress.textContent = 'Champion of Aethelgard';
    }

    // 4. Compass scrolling & markers
    // Entity forward convention: (+sin(yaw), +cos(yaw)) so yaw=PI -> -Z (North = 0 deg), yaw=PI/2 -> +X (East = 90 deg)
    const forwardX = Math.sin(camYaw);
    const forwardZ = Math.cos(camYaw);
    let headingDeg = (Math.atan2(forwardX, -forwardZ) * 180) / Math.PI;
    headingDeg = ((headingDeg % 360) + 360) % 360;
    const viewportHalfW = 160; // 320px compass width / 2
    const offsetPx = viewportHalfW - (headingDeg + 180) * PX_PER_DEG;
    el.compassStrip.style.transform = `translate3d(${offsetPx.toFixed(1)}px, 0, 0)`;

    const activeMarker = quests.getActiveMarker(villagers, world.skyRings);
    const distToVillage = Math.hypot(focusPos.x - LANDMARKS.village.x, focusPos.z - LANDMARKS.village.z);
    const markers = [
      activeMarker ? { x: activeMarker.x, z: activeMarker.z, glyph: '◆', cls: 'cm-quest' } : null,
      !dragon.mounted ? { x: dragon.position.x, z: dragon.position.z, glyph: '🐉', cls: 'cm-dragon' } : null,
      { x: LANDMARKS.castle.x, z: LANDMARKS.castle.z, glyph: '♛', cls: 'cm-castle' },
      { x: LANDMARKS.dungeonEntrance.x, z: LANDMARKS.dungeonEntrance.z, glyph: '🔥', cls: 'cm-dungeon' },
      distToVillage > 36 ? { x: LANDMARKS.village.x, z: LANDMARKS.village.z, glyph: '⌂', cls: 'cm-village' } : null,
    ];
    for (let i = 0; i < compassMarkerEls.length; i++) {
      const m = markers[i];
      const mEl = compassMarkerEls[i];
      if (!m) {
        mEl.classList.add('hidden');
        continue;
      }
      const dx = m.x - focusPos.x;
      const dz = m.z - focusPos.z;
      let targetDeg = (Math.atan2(dx, -dz) * 180) / Math.PI;
      let diff = ((targetDeg - headingDeg + 540) % 360) - 180;
      if (Math.abs(diff) > 68) {
        mEl.classList.add('hidden');
      } else {
        mEl.classList.remove('hidden');
        mEl.className = `c-marker ${m.cls}`;
        mEl.textContent = m.glyph;
        mEl.style.left = `${(viewportHalfW + diff * PX_PER_DEG).toFixed(1)}px`;
      }
    }

    // 5. Boss Bar (only while engaged AND inside/near Highkeep Castle!)
    const boss = combat.boss;
    const bossDist = Math.hypot(focusPos.x - LANDMARKS.bossArena.x, focusPos.z - LANDMARKS.bossArena.z);
    const showBoss = boss.alive && boss.engaged && bossDist < 58;
    el.bossWrap.classList.toggle('hidden', !showBoss);
    if (showBoss) {
      const bPct = Math.max(0, (boss.hp / boss.maxHp) * 100);
      displayedBossLag += (bPct - displayedBossLag) * Math.min(1, dt * 3.5);
      el.bossName.textContent = boss.name.toUpperCase();
      el.bossPct.textContent = `${Math.ceil(bPct)}%`;
      el.bossFill.style.width = `${bPct.toFixed(1)}%`;
      el.bossLag.style.width = `${Math.max(bPct, displayedBossLag).toFixed(1)}%`;
      if (boss.attackState === 'telegraph') {
        el.bossWarning.classList.remove('hidden');
        el.bossWarning.textContent = boss.currentAttack === 'cleave'
          ? '⚠ CRIMSON CLEAVE'
          : boss.currentAttack === 'slam'
            ? '⚠ SEISMIC SLAM — JUMP OR ROLL!'
            : '⚠ ABYSSAL FIRE VOLLEY';
      } else {
        el.bossWarning.classList.add('hidden');
      }
    }

    // 6. Interaction prompt
    if (interactTarget) {
      el.interactPrompt.classList.remove('hidden');
      el.interactKey.textContent = interactTarget.key;
      el.interactText.textContent = interactTarget.label;
      el.tBtnInteract.classList.remove('hidden');
      el.tBtnInteract.textContent = interactTarget.key;
    } else {
      el.interactPrompt.classList.add('hidden');
      el.tBtnInteract.classList.add('hidden');
    }

    // 7. Screen-projected Villager Nameplates
    for (let i = 0; i < villagerLabels.length; i++) {
      const slot = villagerLabels[i];
      const v = villagers[i];
      if (!v) {
        slot.el.classList.add('hidden');
        continue;
      }
      const dist = v.position.distanceTo(focusPos);
      if (dist > 34) {
        slot.el.classList.add('hidden');
        continue;
      }
      projVec.set(v.position.x, v.position.y + 2.45, v.position.z).project(camera);
      if (projVec.z > 1 || projVec.z < -1 || Math.abs(projVec.x) > 0.96 || Math.abs(projVec.y) > 0.96) {
        slot.el.classList.add('hidden');
        continue;
      }
      const sx = Math.max(76, Math.min(width - 76, (projVec.x * 0.5 + 0.5) * width));
      const sy = (-projVec.y * 0.5 + 0.5) * height;
      const marker = quests.getVillagerMarkerState(v.id);
      slot.el.classList.remove('hidden');
      slot.el.style.transform = `translate3d(${sx.toFixed(1)}px, ${sy.toFixed(1)}px, 0) translate(-50%, -100%)`;
      slot.name.textContent = v.name;
      if (marker) {
        slot.badge.textContent = marker;
        slot.badge.className = `npc-badge badge-${marker === '?' ? 'ready' : marker === '!' ? 'avail' : 'active'}`;
        slot.badge.classList.remove('hidden');
      } else {
        slot.badge.classList.add('hidden');
      }
    }

    // 8. Screen-projected Enemy Health Bars
    const visEnemies = combat.getVisibleEnemies(focusPos, 32);
    for (let i = 0; i < enemyBars.length; i++) {
      const slot = enemyBars[i];
      const e = visEnemies[i];
      if (!e) {
        slot.el.classList.add('hidden');
        continue;
      }
      projVec.set(e.position.x, e.position.y + e.height + 0.55, e.position.z).project(camera);
      if (projVec.z > 1 || projVec.z < -1 || Math.abs(projVec.x) > 1.04 || Math.abs(projVec.y) > 1.04) {
        slot.el.classList.add('hidden');
        continue;
      }
      const sx = (projVec.x * 0.5 + 0.5) * width;
      const sy = (-projVec.y * 0.5 + 0.5) * height;
      slot.el.classList.remove('hidden');
      slot.el.style.transform = `translate3d(${sx.toFixed(1)}px, ${sy.toFixed(1)}px, 0) translate(-50%, -100%)`;
      slot.name.textContent = e.name;
      slot.fill.style.width = `${Math.max(0, (e.hp / e.maxHp) * 100).toFixed(1)}%`;
    }

    // 9. Screen-projected Floating Damage / Loot Numbers
    for (let i = 0; i < dmgEls.length; i++) {
      const dEl = dmgEls[i];
      const dn = combat.damageNumbers[i];
      if (!dn) {
        dEl.classList.add('hidden');
        continue;
      }
      projVec.copy(dn.worldPos).project(camera);
      if (projVec.z > 1 || projVec.z < -1) {
        dEl.classList.add('hidden');
        continue;
      }
      const sx = (projVec.x * 0.5 + 0.5) * width;
      const sy = (-projVec.y * 0.5 + 0.5) * height;
      dEl.classList.remove('hidden');
      dEl.textContent = dn.text;
      dEl.style.color = dn.color;
      dEl.style.opacity = Math.min(1, (dn.life / dn.maxLife) * 1.6).toFixed(2);
      dEl.style.transform = `translate3d(${sx.toFixed(1)}px, ${sy.toFixed(1)}px, 0) translate(-50%, -50%)`;
    }

    // 10. Round Minimap Canvas
    this._drawMinimap(focusPos, camYaw, dragon, villagers, combat, quests, activeMarker);
  }

  function _drawMinimap(focusPos, camYaw, dragon, villagers, combat, quests, activeMarker) {
    const ctx = minimapCtx;
    const size = el.minimapCanvas.width;
    const half = size * 0.5;
    const viewSpan = 190; // meters visible across minimap diameter
    const scale = size / viewSpan;

    ctx.clearRect(0, 0, size, size);
    ctx.save();
    ctx.beginPath();
    ctx.arc(half, half, half - 2, 0, Math.PI * 2);
    ctx.clip();

    // Draw cached terrain centered on player
    const px = ((focusPos.x / WORLD.SIZE) + 0.5) * mapRes;
    const pz = ((focusPos.z / WORLD.SIZE) + 0.5) * mapRes;
    const srcSpan = (viewSpan / WORLD.SIZE) * mapRes;
    ctx.fillStyle = '#121822';
    ctx.fillRect(0, 0, size, size);
    ctx.drawImage(
      mapOffscreen,
      px - srcSpan * 0.5,
      pz - srcSpan * 0.5,
      srcSpan,
      srcSpan,
      0,
      0,
      size,
      size,
    );

    const toMap = (wx, wz) => ({
      x: half + (wx - focusPos.x) * scale,
      y: half + (wz - focusPos.z) * scale,
    });

    // Subtle radar rings
    ctx.strokeStyle = 'rgba(216, 178, 104, 0.16)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(half, half, half * 0.5, 0, Math.PI * 2);
    ctx.stroke();

    // Enemies
    for (let i = 0; i < combat.enemies.length; i++) {
      const e = combat.enemies[i];
      if (!e.alive) continue;
      const p = toMap(e.position.x, e.position.z);
      ctx.fillStyle = e.kind === 'troll' ? '#ff6a33' : '#e05252';
      ctx.beginPath();
      ctx.arc(p.x, p.y, e.kind === 'troll' ? 3.2 : 2.3, 0, Math.PI * 2);
      ctx.fill();
    }

    // Boss
    if (combat.boss.alive) {
      const bp = toMap(combat.boss.position.x, combat.boss.position.z);
      ctx.fillStyle = '#ff2020';
      ctx.strokeStyle = '#ffd580';
      ctx.lineWidth = 1.4;
      ctx.beginPath();
      ctx.arc(bp.x, bp.y, 4.8, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
    }

    // Villagers
    for (let i = 0; i < villagers.length; i++) {
      const v = villagers[i];
      const vp = toMap(v.position.x, v.position.z);
      const st = quests.getVillagerMarkerState(v.id);
      ctx.fillStyle = st === '?' ? '#5df2a8' : st === '!' ? '#ffd15c' : '#8ab6ff';
      ctx.beginPath();
      ctx.arc(vp.x, vp.y, 3.0, 0, Math.PI * 2);
      ctx.fill();
    }

    // Red Dragon
    if (!dragon.mounted) {
      const dp = toMap(dragon.position.x, dragon.position.z);
      ctx.fillStyle = '#ff3838';
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 1.2;
      ctx.beginPath();
      ctx.moveTo(dp.x, dp.y - 5);
      ctx.lineTo(dp.x + 4.5, dp.y + 3.5);
      ctx.lineTo(dp.x, dp.y + 1.2);
      ctx.lineTo(dp.x - 4.5, dp.y + 3.5);
      ctx.closePath();
      ctx.fill();
      ctx.stroke();
    }

    // Active Quest Target Ring (clamped to minimap edge if far)
    if (activeMarker) {
      let qx = half + (activeMarker.x - focusPos.x) * scale;
      let qy = half + (activeMarker.z - focusPos.z) * scale;
      const d = Math.hypot(qx - half, qy - half);
      if (d > half - 9) {
        qx = half + ((qx - half) / d) * (half - 9);
        qy = half + ((qy - half) / d) * (half - 9);
      }
      ctx.strokeStyle = '#ffd966';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(qx, qy, 5.2, 0, Math.PI * 2);
      ctx.stroke();
    }

    // Player heading cone + arrow in center
    const fx = Math.sin(camYaw);
    const fz = Math.cos(camYaw);
    const ang = Math.atan2(fx, -fz);
    ctx.save();
    ctx.translate(half, half);
    ctx.rotate(ang);
    ctx.fillStyle = 'rgba(255, 240, 190, 0.18)';
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.arc(0, 0, 26, -Math.PI * 0.5 - 0.45, -Math.PI * 0.5 + 0.45);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = '#ffffff';
    ctx.strokeStyle = '#090b12';
    ctx.lineWidth = 1.4;
    ctx.beginPath();
    ctx.moveTo(0, -6.5);
    ctx.lineTo(4.8, 5);
    ctx.lineTo(0, 2.2);
    ctx.lineTo(-4.8, 5);
    ctx.closePath();
    ctx.fill();
    ctx.stroke();
    ctx.restore();

    ctx.restore();
  }

  return {
    touchState,
    showGameHUD,
    showToast,
    openDialogue,
    closeDialogue,
    toggleQuestLog,
    showGameOver,
    triggerDamageFlash,
    update,
    _drawMinimap,
  };
}
