# Fab Assets Shopping List — Crimson Desert & Wukong Visual Upgrade (100% Free on Epic Fab)

Все перечисленные ассеты **бесплатны в Fab** (внутри UE 5.8 Editor откройте вкладку **Window → Fab**, войдите в ваш аккаунт Epic Games и нажмите **Add to Project**). После импорта перенесите или оставьте их в соответствующих папках `Content/FabImports/...` (или запустите `Scripts/import_fab_assets.py` в Python-консоли UE 5.8) — C++ классы автоматически подхватят высокополигональные скелетные модели с 70–150+ костями!

## 1. Герой (кино-скелет, броня, физика плаща, анимации меча и магии)
- **Paragon: Gideon** (маг-рыцарь с плащом, сферами и анимациями каста — идеально для Exort/Wex/Quas!)
- **Paragon: Greystone** или **Paragon: Kwang** (тяжёлая рыцарская броня и мощные комбо мечом в духе *Crimson Desert*)
- **Game Animation Sample Project (Motion Matching / Locomotion)** от Epic Games (бесплатно в Fab — 500+ motion-captured анимаций бега, прыжков, перекатов и разворотов!)

## 2. Кино-боссы с большим количеством костей и множеством атак
- **Paragon: Sevarog** (огромный мрачный жнец с гигантским молотом и многостадийной бронёй — идеальный **Malakor, Sovereign of the Shattered Seal**)
- **Paragon: Grux** или **Paragon: Rampage** (массивные звероподобные боссы для лесного и горного биомов)
- **Paragon: Khaimera / Feng Mao / Aurora** (быстрые дуэльные боссы в духе *Black Myth: Wukong*, особенно Aurora для Хранителя Ледника!)

## 3. Красный Дракон
- **Quadruped Fantasy Creatures / Mountain Dragon** (бесплатные/Epic-паки в Fab) — поместите SkeletalMesh дракона в `/Game/FabImports/Dragon/` (наш `AAetheriaDragonActor` автоматически переключится с процедурной сборки на скелетного дракона с крыльями и хвостом!).

## 4. Пейзажи уровня Crimson Desert (Nanite + Quixel Megascans)
- **Quixel Megascans — Nordic Coastal Cliffs / Icelandic Mossy Rocks / Alpine Meadow Grass** (бесплатно через встроенный Quixel Bridge / Fab)
- **Medieval Village / Infinity Blade: Grass Lands & Fire Lands & Ice Lands** (бесплатные коллекции Epic Games — замки, каменные мосты, деревня, лавовые пещеры, ледяные руины)
- **Electric Dreams Environment** (от Epic Games — эталонная PCG-растительность и Nanite-камни для UE 5.8!)

## 5. NVIDIA DLSS 4 + Ray Reconstruction (30 секунд установки)
1. Откройте [developer.nvidia.com/rtx/dlss/get-started](https://developer.nvidia.com/rtx/dlss/get-started), примите NVIDIA EULA и скачайте **UE 5.8 DLSS / Streamline Plugin**.
2. Распакуйте папки `DLSS` и `Streamline` внутрь `D:\AetheriaChronicles\Plugins\`.
3. Запустите `BUILD_AETHERIA_CHRONICLES.bat` — наш `UGraphicsRTXSubsystem` автоматически обнаружит `WITH_AETHERIA_DLSS=1` и будет включать **RTX + DLSS Quality + Ray Reconstruction + Reflex** сразу при запуске игры (а на видеокартах без RTX по-прежнему безопасно откатится в упрощённый режим!).
