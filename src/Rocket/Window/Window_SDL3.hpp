#pragma once

#include <SDL3/SDL.h>
#include <Rocket/Window/Window.hpp>

namespace Rocket {

/** Initializes SDL's video subsystem, with the hints the backend relies on; safe to call any number of times. */
void __InitVideo();

/**
 * Runs one tick of the run loop: waits for events until the next frame is due,
 * handles every pending event, and paints the windows once a frame.
 *
 * App::Run ticks with `update` set; the loops that block inside a tick of their
 * own (a modal window, a file dialog) tick without it, so App timeouts,
 * intervals and UpdateAppEvent do not run while they block, as on macOS.
 *
 * @param update True to run App::_Update in this tick.
 */
void __Tick(bool update);

} /* namespace Rocket */
