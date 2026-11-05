#pragma once

// Water on flat ground that mirrors the hero, under the Enhanced look: the world drawn a second time from a mirrored
// view. It shows that an extra pass costs drawing time only, since drawing no longer changes the game.
namespace Render::PuddleReflection
{
[[nodiscard]] bool IsOn();
void SetOn(bool on);

// How much of the surface shows at this far from the middle, in radii: full inside, fading to nothing at the rim.
[[nodiscard]] float EdgeAlpha(float dx, float dy);

// The hero's reflection, into its own target; before the frame's world pass.
void CaptureReflection();
// The water, on the ground drawn so far.
void DrawPuddles();
// True while the reflection is being drawn: what's cast on the ground has no place in it.
[[nodiscard]] bool IsCapturing();
} // namespace Render::PuddleReflection
