#include <Rocket/Window/Clipboard.hpp>

namespace Rocket {

/** Platform implementation of GetClipboardString. */
std::string __GetClipboardString();

/**
 * Platform implementation of SetClipboardString.
 *
 * @param string The UTF-8 text to place on the clipboard.
 */
void __SetClipboardString(std::string const& string);

} /* namespace Rocket */