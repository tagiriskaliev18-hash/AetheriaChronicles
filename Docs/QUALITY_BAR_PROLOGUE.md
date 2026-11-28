# Quality bar: prologue (sage meeting → ledge collapse → fall)

Target look: between **Crimson Desert** (realistic open world, volumetric light and fog, dense nature),
**Black Myth: Wukong** (photoscanned rock and foliage, cinematic boss/NPC staging, painterly colour) and
**Dark Souls** (dark, readable silhouettes, quiet menace, light used to guide the player).

A shot passes only if every line below holds. "Close" means within 5 m of the camera.

## 1. Geometry and materials
- Close rock, cave walls and ledge are photoscan or sculpted assets (50k+ tris per close hero piece), never
  tiled bricks, boxes or visibly faceted silhouettes.
- Characters on screen during dialogue are 30k+ tris with cloth folds, a readable face/hood and a beard or
  equivalent detail for the sage. No stand-in hero model as the sage.
- All materials are PBR with albedo, normal and mask/roughness; no flat colour, no stretched UVs, no visible
  texture repetition at a glance. Wet rock near the waterfall is darker and glossier.
- Imported textures are BC-compressed and sized for an 8 GB GPU (RTX 4060): close assets 2K, background 1K.

## 2. Light and atmosphere
- One motivated key: daylight from the cave mouth. Inside the cave it falls off to near-black; the player
  reads the path from that contrast, not from flat ambient light.
- Volumetric light shafts and dust in the mouth; the sage is rim-lit by them.
- Outside, the vista shows depth layers: foreground rock, mid-ground forest and lake, distant mountains in
  aerial perspective. Never a featureless fog wall or empty void.
- Colour: warm key, cool shadows, no oversaturated orange cast over the whole frame.

## 3. Characters and animation
- No T/A-pose, foot sliding, floating, ground clipping or limbs through props in any shot.
- The sage idles (breathing, staff weight shift) and gestures on each line; the hero turns to face him.
- The sage's entrance is a light effect (embers, glow), not a scale pop.
- During the fall the hero tumbles or braces with a fall animation, not a walk/idle pose sliding along a path.

## 4. Camera and staging
- Dialogue: letterbox, over-the-shoulder shot/reverse-shot, both faces readable, rule of thirds, shallow
  depth of field on the speaker.
- Collapse: camera at the hero's eye level or slightly below, looking along the cliff so the drop is
  readable; cracks propagate, debris and dust burst, one beat of slow motion, then the plunge.
- Fall: the camera follows behind/above the hero down the waterfall with spray and mist in front of the
  lens; it cuts to black on impact. Never a top-down view of the ground where the player cannot tell
  where they are.

## 5. Integrity and performance
- Ground and collision everywhere the camera looks; nothing floats.
- 60 fps at 1080p on an RTX 4060 8 GB with DLSS, with no GPU memory overflow (the D3D12 "device removed" crash).
- `Aetheria.exe -verify-demo` passes all checks.

## Current state (2026-11-28, build from PR 1 branch)

| Shot | Verdict | Main gaps |
|---|---|---|
| 01–02 cave wake / toward light | partial | Cave is now rock and lit from the mouth (good); the hero material reads as plastic, the floor is flat. |
| 03–04 sage appears / talks | fail | The sage is a small stand-in hero model in orange armour, not an old sage; it is far away and faceless; no shot/reverse-shot; the whole frame has an orange cast. |
| 05 vista | fail | Earlier build showed fog void; to recheck after the terrain fix. |
| 06 ledge collapse | fail | Top-down camera over a slope: the player cannot tell what is happening; no visible crack/debris/drop. |
| 07–10 fall, splash, glade | blocked | Game crashed from GPU memory overflow before these shots; being fixed. |
