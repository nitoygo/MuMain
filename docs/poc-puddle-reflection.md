# Proof of concept: a puddle that reflects the hero

This branch tests one idea from the D series (#914): with drawing no longer changing game state, the world can safely be
drawn a second time. It's a proof of concept and isn't meant to merge. This page is for whoever picks up graphics work
next: what was built, what it showed, and what to do differently.

## What it does

Under the Enhanced look, one hand-placed puddle sits on flat town ground in Lorencia, around tile (142, 118).

1. Before the world pass, `PuddleReflection::CaptureReflection` draws the hero into its own render target through a
   mirrored view (`CameraProjection::ApplyMirroredView`). It only does so when the hero is near a visible puddle.
2. After the map objects, `PuddleReflection::DrawPuddles` draws Lorencia's `TileWater01` as a soft-edged surface, then
   the render target on top. Each vertex finds its place in the target from where it lands on screen.

`$puddle on` and `$puddle off` compare it, in builds with the control socket.

## What it showed

- **Drawing twice works.** With the reflection on, `drawpurity` reported no game state changes, `drawspawns` no spawns,
  and `drawparity` 0 differing calls out of about 2,970, at once and drawn later.
- **A second camera needed no renderer change.** Render targets and `CameraProjection::ApplyView` were already there,
  and the skinned and basic shaders take their position from the renderer's matrices, so they pick up the mirror.
- **The reflection looked right the first time it ran,** wings included.
- **The water didn't look like a puddle.** Tiling a strong caustic texture over the surface reads as ocean waves. A
  plain grey-blue tint read as a grey patch, and neither looks like a shallow puddle.

The frame cost wasn't measured.

## Renderer facts worth knowing

| Fact | Why it matters |
|---|---|
| `SetFrontFace` isn't overridden by the SDL_GPU renderer, so it does nothing there | A mirror reverses triangle winding, and this can't correct it. The capture turns the image upside down as well, which restores the winding, and the surface flips `v` when it samples |
| `EnableAlphaBlend` is additive glow (`BlendMode::Glow`). `EnableAlphaBlend3` is plain alpha blending | The first can only brighten, so a dark surface needs the second |
| `DisableTexture` turns depth writing back on | Call `DisableDepthMask` after it, not before |
| `EnableTexture2D` skips the `TextureEnable` tracking and isn't recorded for draws drawn later | Use the legacy calls that do both, such as `EnableAlphaBlend3` |
| A capture sets its own viewport and scissor when replayed, and replays before the main pass samples it | The target can be sampled in the same frame's world pass |
| `gluPerspective` here multiplies the current matrix and writes no globals | A flip applied before it survives. `gluPerspective2` does write the camera's globals |
| `BITMAP_WATER` is only loaded on some maps. `BITMAP_MAPTILE + 5` is each map's `TileWater01` | Use the second for water on any map |
| Hidden characters are decided by `IsCharacterDrawn` | A second pass needs that check too, or a hidden game master shows in the water |

## Known limits

- **Point lights are wrong on the reflection.** `PackVertexToWorldRows` works out world position as
  `worldFromView * modelView`. With the mirror in the model-view, the shader sees reflected vertices below the ground,
  so torches and spell lights land wrongly. Main lighting is calculated in model space and isn't affected.
- **`worldFromView` is one frame old during the capture,** because the capture runs before the scene sets its lighting.
- **Only the hero reflects.** There's no sky, map or other characters, and sprite glows and trails aren't included
  because they're drawn in a separate pass.
- **No clipping at the water plane,** which is fine for a hero standing on it, but needed for anything that crosses it.
- **The puddle is hard-coded,** and has no `Look.json` value or Options entry.
- **The target is full viewport size.** Half size is common and would cost less.

## How to do it properly

- **Use a water shader,** so the look can be per pixel: viewing-angle strength (Fresnel), and a normal map that disturbs
  the reflection. MU's camera looks down steeply, so physically correct water would reflect very little. Expect to
  choose the strength by eye.
- **For a puddle, drop the visible water texture.** Darken and slightly wet the ground beneath, and let the reflection
  and small ripples carry the effect. Keep ripples small and slow, as large ones read as open water.
- **Reflect the sky colour at minimum,** so that the water isn't only dark where the hero isn't.
- **Choose spots by hand.** Lorencia's light map dark spots are mostly baked shadows beside walls and buildings. Some
  are walkable, like the one around (142, 118), but others are blocked or have objects standing in them.

Standard planar reflection, as in Source engine water or Unreal's planar reflections, is the right reference. Screen-space
reflections can only show what's already on screen, and cube maps are too coarse for a sharp character reflection.
