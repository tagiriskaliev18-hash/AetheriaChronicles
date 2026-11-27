# Steam Release Guide & Store Page — Aethelgard: Ashes of the Drake

Полное пошаговое руководство по упаковке HTML/Three.js-игры в нативное приложение для **Windows и macOS через Electron**, публикации в **Steam**, а также готовые тексты страницы магазина, план трейлера из 8 шотов и стратегия ценообразования.

---

## Часть 1. Пошаговая сборка Windows и macOS приложения на Electron для Steam

Вся инфраструктура уже создана в папке проекта:
- `package.json` — конфигурация `electron` и `electron-builder` (Windows NSIS + распакованная папка для SteamPipe, macOS Universal `.dmg`/`.zip`).
- `electron/main.cjs` — главное окно Electron (полноэкранный режим, `F11` / `Alt+Enter`, аппаратный WebGL, single-instance lock, заготовка под `steamworks.js` и Steam Overlay).
- `electron/preload.cjs` — безопасный IPC-мост (`contextIsolation: true`, `sandbox: true`).
- `electron/build-desktop.mjs` — собирает **100% офлайн-версию** игры в `dist-desktop/` (локальные `three.module.js` и `three.core.js`, без обращений к CDN и Google Fonts — обязательное требование Valve и Steam Deck).
- `assets/icon.png` и `assets/icon.ico` — иконки для macOS и Windows.

### Шаг 1. Установка зависимостей
```bash
npm install
```

### Шаг 2. Локальная сборка и запуск Desktop-версии
```bash
npm run desktop
```
Команда выполнит `node build.mjs` → `node electron/build-desktop.mjs` (создаст полностью автономный `dist-desktop/`) → откроет игру в полноэкранном окне Electron.
- `F11` или `Alt+Enter` — переключение полноэкранного/оконного режима.
- `START_WINDOWED=1 npm run desktop` — запуск в окне для отладки.

### Шаг 3. Сборка дистрибутивов под Windows и macOS
```bash
# Windows (создаст release/win-unpacked/ для SteamPipe + .exe инсталлятор)
npm run dist:win

# macOS Universal Intel + Apple Silicon (выполнять на Mac или в GitHub Actions macos-latest)
npm run dist:mac
```
> **Важно про macOS:** для публичной раздачи вне Steam и прохождения Apple Gatekeeper требуется Developer ID Certificate + нотаризация (`APPLE_ID`, `APPLE_APP_SPECIFIC_PASSWORD`, `APPLE_TEAM_ID`). Через Steam файлы запускаются напрямую из Steam-библиотеки, но для macOS 13+ Apple всё равно рекомендует подпись и нотаризацию.

### Шаг 4. Подключение Steamworks SDK (оверлей + достижения)
1. Зарегистрируйтесь в **Steamworks Partner** (разовый возвратный взнос Steam Direct — \$100 на игру) и получите свой **AppID** (например, `3456780`).
2. Установите обёртку Steamworks для Node/Electron:
   ```bash
   npm install steamworks.js
   ```
3. В файле `electron/main.cjs` уже встроен `initSteamworksIfInstalled()`. Укажите ваш реальный AppID в переменной `STEAM_APP_ID` или замените дефолтное `480` в `electron/main.cjs`.
4. Внутри игры вызывайте `window.desktopAPI?.unlockAchievement('ACH_SLAY_MALAKOR')` при победе над боссом, пролёте через кольца дракона и выполнении всех 5 квестов.

### Шаг 5. Загрузка билда в Steam через SteamPipe (`steamcmd`)
1. Скачайте **Steamworks SDK** с портала партнёра и откройте `tools/ContentBuilder`.
2. Отредактируйте шаблоны `steam/app_build.vdf`, `steam/depot_win.vdf`, `steam/depot_mac.vdf` (подставьте ваши реальные `AppID` и `DepotID`).
3. Соберите `release/win-unpacked` через `npm run dist:win` и загрузите депо командой:
   ```bash
   steamcmd +login YOUR_STEAM_BUILDER_ACCOUNT +run_app_build_http C:\Users\user\Desktop\Fantasy\steam\app_build.vdf +quit
   ```
4. В Steamworks Dashboard → **SteamPipe → Builds** назначьте загруженный билд на ветку `default` (или `demo` для бесплатной демки).
5. Настройте **Launch Options**:
   - Windows: `Aethelgard - Ashes of the Drake.exe`
   - macOS: `Aethelgard - Ashes of the Drake.app/Contents/MacOS/Aethelgard - Ashes of the Drake`
6. Пройдите проверку Valve (обычно 3–5 рабочих дней для страницы магазина и 3–5 дней для билда). **Страницу магазина открывайте минимум за 60 дней до релиза**, чтобы копить вишлисты.

---

## Часть 2. Готовая страница Steam (Copy-Paste в Steamworks)

### Название игры
**Aethelgard: Ashes of the Drake**

### Short Description (EN — 268 / 300 символов, лимит Steam)
> Soar across a sunlit fantasy valley on a fire-breathing red dragon, explore an ancient lava-cracked crypt, and challenge the Grave Sovereign atop Highkeep Castle in this third-person open-world action RPG forged with sword, shield, and flame.

### Short Description (RU — 249 / 300 символов)
> Взмывайте в небо на огнедышащем красном драконе, исследуйте лавовое подземелье и бросьте вызов Повелителю Могил в замке Хайкип в экшен-RPG от третьего лица с динамическим циклом дня и ночи, квестами и боёвкой на мечах.

---

### Long Description (готовый Steam BBCode)

```bbcode
[h2]RIDE THE FLAME. RECLAIM THE VALLEY.[/h2]
The northern roads of Oakhaven are overrun. Skeletons prowl the Stone Bridge, Forest Trolls stalk the Blue-Leaf Woods, and inside the fortress of Highkeep waits [b]Malakor, the Grave Sovereign[/b]. 

[b]Aethelgard: Ashes of the Drake[/b] is a focused, third-person open-world fantasy action RPG built for immediate adventure — seamless exploration on foot, exhilarating mounted dragon flight, crisp melee combat, and atmospheric exploration from golden sunset to moonlit night.

[h2]MOUNT A FIRE-BREATHING RED DRAGON[/h2]
No fast-travel menus — take to the skies yourself. Approach [b]Ignis[/b] on the western ridge, press a single key to leap onto the saddle, and soar above emerald valleys, Mirror Lake, and Highkeep's towers.
[list]
[*]Fluid aerial flight with speed boosts, banking turns, altitude dives, and gliding.
[*]Unleash roaring streams of dragonfire from above to incinerate trolls, skeletons, and castle defenders.
[*]Weave through ancient Sky Rune Rings suspended over the valley and land anywhere in seconds.
[/list]

[h2]RESPONSIVE SWORD, SHIELD & DODGE COMBAT[/h2]
Face skeletal raiders, hulking forest trolls, and an unforgiving castle boss with tactile third-person combat:
[list]
[*]Chain 3-hit rune-sword combos, raise your shield to block or timed-parry incoming blows, and dodge-roll with invulnerability frames.
[*]Battle [b]Malakor, the Grave Sovereign[/b] in a multi-attack boss encounter — read his crimson telegraph glow before every Crimson Cleave, Seismic Ground Slam, and Abyssal Fire Volley.
[*]Collect gold and restorative drops from defeated foes, and visit Blacksmith Torin to temper your blade.
[/list]

[h2]FIVE HANDCRAFTED QUESTS & A LIVING REALM[/h2]
[list]
[*]Meet five distinct villagers across Oakhaven, the Ancient Rune Circle, the crypt crags, and Highkeep Gate — track every objective via your Quest Log, Compass, and live Minimap.
[*]Descend seamlessly into the [b]Ember Crypt[/b], a pitch-dark subterranean dungeon lit only by flickering wall torches and pulsing molten lava cracks, to claim the Sunstone Relic.
[*]Experience a full [b]day-to-night cycle[/b] — golden-hour sun shafts through blue-leaf canopies, drifting valley fog, wind-swept grass, glowing cottage windows, twinkling stars, and a luminous full moon.
[/list]

[b]Wishlist now[/b] and prepare to ride the flame.
```

---

### 5 Steam-тегов (по приоритету алгоритма рекомендаций)
1. **Open World** *(высокий объём поискового трафика и связка с фанатами Skyrim / BotW / Valheim)*
2. **Action RPG** *(основной жанровый якорь для правильной аудитории)*
3. **Dragons** *(сильнейший нишевый триггер-дифференциатор с высокой кликабельностью)*
4. **Fantasy** *(ключевой тематический кластер)*
5. **Third Person** *(точно отсекает нецелевых игроков и совпадает с 2D/isometric-фильтром)*

---

### 8 шотов для трейлера (хронометраж ~65 секунд, темп: 1 удар монтажа = 1 новая механика)

| # | Таймкод | Кадр / Сцена | Титр / Звук |
|---|---|---|---|
| **1** | `0:00–0:07` | **Крючок (Hook):** герой бежит к западному утёсу на золотом закате, нажимает `F`, запрыгивает на красного дракона — мощный взмах крыльев, взлёт над деревней и каменным мостом. | Глубокий вдох крыльев + нарастающий оркестровый рог. Титр: *«TAKE TO THE SKIES»* |
| **2** | `0:07–0:15` | **Дыхание огнём в полёте:** камера за драконом над Mirror Lake и голубым лесом; дракон пикирует и заливает огненной струёй патруль скелетов и тролля. | Рёв драконьего пламени, треск углей. |
| **3** | `0:15–0:23` | **Живой открытый мир (таймлапс):** плавный пролёт над Oakhaven Village, арочным мостом и кругом рун — закатные лучи солнца переходят в звёздную ночь с полной луной и зажигающимися окнами домов. | Контраст тишины и ночного ветра. Титр: *«EXPLORE AETHELGARD»* |
| **4** | `0:23–0:31` | **Квесты и NPC:** разговор с Elder Aldric и Blacksmith Torin в деревне, принятие квеста, вспышка обновлённого компаса и открытие журнала квестов `J`. | Звон квеста, короткий срез 2–3 сцен. Титр: *«FIVE QUESTS · ONE REALM»* |
| **5** | `0:31–0:40` | **Наземная боёвка:** схватка в голубом лесу с огромным лесным троллем — блок щитом с искрами парирования, перекат `Q` в сторону, серия из 3 ударов мечом и выпадение золотых монет. | Металлический лязг, хруст ударов. Титр: *«STEEL, SHIELD & ROLL»* |
| **6** | `0:40–0:48` | **Тёмное подземелье Ember Crypt:** шаг в обсидиановую арку — свет заката гаснет, вспыхивают настенные факелы, под ногами пульсируют лавовые трещины, из темноты выходят криптовые скелеты с голубыми глазами. | Гул подземелья, треск факелов. Титр: *«BRAVE THE EMBER CRYPT»* |
| **7** | `0:48–0:58` | **Кульминация — Босс в замке:** вход в ворота Highkeep Castle, появление полосы **MALAKOR, THE GRAVE SOVEREIGN**; алая вспышка телеграфа, ударная волна Seismic Slam, герой перепрыгивает кольцо и контратакует. | Удар барабана, боевой горн босса. Титр: *«DEFEAT THE GRAVE SOVEREIGN»* |
| **8** | `0:58–1:05` | **Финальный пэкшот:** дракон пролетает сквозь небесное руническое кольцо на фоне силуэта замка, резкий уход в логотип **AETHELGARD: ASHES OF THE DRAKE** + **WISHLIST NOW ON STEAM** + **FREE DEMO AVAILABLE**. | Финальный аккорд + звук крыльев. |

---

## Часть 3. Рекомендуемая стартовая цена и обоснование

### Рекомендуемая цена: **\$14.99** (со стартовой скидкой **−15% = \$12.74** в первую неделю)

**Почему именно \$14.99 — идеальная точка для этой игры:**
1. **Психология восприятия ценности (избегаем ловушки \$4.99–\$9.99):** в Steam цены ниже \$10 часто считываются аудиторией как «мини-игра на 30 минут» или asset-flip. \$14.99 ставит игру в проверенный класс качественных соло-инди action-RPG (рядом с *A Short Hike+*, *Lil Gator Game*, *Dread Delusion* на скидках, *Gedonia* / *Valheim*-tier impulse buy).
2. **Импульсный порог покупки:** при скидке запуска −15% игрок видит **\$12.74** — это ниже психологического барьера \$15, поэтому высокий процент вишлистов конвертируется сразу в релизную неделю (критично для попадания в *New & Trending*).
3. **Запас на сезонные распродажи Steam:** с базой \$14.99 можно комфортно давать −20% (через 2 месяца), −33% (Summer/Autumn Sale, ровно \$9.99!) и −50% через год (\$7.49), не обесценивая игру.
4. **Региональные цены (Valve Recommended Matrix):** обязательно включите региональное ценообразование в Steamworks (примерно ~550–650 ₽ / ~\$7.99 CIS / ~\$8.49 LATAM / ~14.79 € EU) — это добавляет +20–30% к объёму продаж в копиях без каннибализации США/ЕС.
5. **Условие для \$14.99 перед платным релизом:** для сильного рейтинга *Very Positive* на старте при \$14.99 важно дополнить базовую сборку до 4–6 часов контента (больше квестов/лута/прокачки/ещё 1–2 босса и сохранения `localStorage`/Steam Cloud). Если выпускать текущую компактную версию как первую главу/аркадную RPG — оптимальна цена **\$7.99–\$9.99** или бесплатный пролог + платный сиквел/расширение за \$14.99.
