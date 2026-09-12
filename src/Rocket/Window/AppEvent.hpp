/*
 * Copyright (c) 2026 Roman Kirsanov
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <Rocket/Base/Object.hpp>

namespace Rocket {

/** Base class for application lifecycle events published on App::OnEvent. */
class AppEvent : public Object {};

/** Published once after the run loop ends, just before App::Run returns. */
class ExitAppEvent : public AppEvent {};

/** Published once at the start of App::Run, before the loop begins. */
class ReadyAppEvent : public AppEvent {};

/** Published when the user asks the application to quit (e.g. the Quit menu item); call App::Exit to honor it. */
class CloseAppEvent : public AppEvent {};

/** Published on every run-loop tick, after timeouts and intervals have run. */
class UpdateAppEvent : public AppEvent {};

} /* namespace Rocket */
