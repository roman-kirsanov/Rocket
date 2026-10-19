#include <string>
#include <Rocket/Window/Input.hpp>

namespace Rocket {

/**
 * Whether the key types a character: the digit row, the letters, the
 * punctuation keys and Space. These are the keys whose keycode follows the
 * layout; every other key keeps its name.
 *
 * @param scancode The physical key.
 */
bool __IsCharacterScancode(Scancode scancode);

/**
 * Platform implementation of GetScancodeLabel: the keycode the scancode has
 * on the current layout, without modifiers.
 *
 * @param scancode The physical key.
 */
std::string __GetLayoutKeycode(Scancode scancode);

} /* namespace Rocket */
