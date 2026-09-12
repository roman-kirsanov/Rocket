#pragma once

#include <string>
#include <Rocket/Base/Object.hpp>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Window/AppEvent.hpp>

namespace Rocket {
namespace App {

/** Application lifecycle event source (see the AppEvent subclasses). */
extern Pub<AppEvent const&> OnEvent;

/** Returns the absolute path of the running executable (cached at startup). */
std::string const& GetExecPath();

/** Returns the directory containing the running executable (cached at startup). */
std::string const& GetExecDir();

/** Returns the process's current working directory (re-queried on every call). */
std::string const& GetWorkDir();

/**
 * Returns the per-user directory for application settings and data
 * (macOS: ~/Library/Application Support; intended for Windows: %APPDATA%;
 * Linux: $XDG_CONFIG_HOME or ~/.config — only the macOS backend exists at
 * present), cached on first call.
 *
 * The directory is per-user, not per-application: append your own
 * application folder and create it before writing.
 */
std::string const& GetUserDir();

/**
 * Schedules a callback to run once on the run loop after a delay.
 *
 * Safe to call from any thread, as is the returned cancel function; the
 * callback itself always runs on the main run loop.
 *
 * @param callback The function to invoke.
 * @param timeout  Delay in milliseconds.
 * @return A function that cancels the timeout when called before it fires.
 */
std::function<void()> SetTimeout(std::function<void()> const& callback, double timeout);

/**
 * Schedules a callback to run repeatedly on the run loop.
 *
 * The next invocation is scheduled relative to the previous one's due time,
 * not to when the callback actually ran. Missed periods are not skipped: at
 * most one invocation runs per run-loop tick, so a backlog drains one call
 * per tick.
 *
 * Safe to call from any thread, as is the returned cancel function; the
 * callback itself always runs on the main run loop.
 *
 * @param callback The function to invoke.
 * @param interval Period in milliseconds.
 * @return A function that stops the interval when called.
 */
std::function<void()> SetInterval(std::function<void()> const& callback, double interval);

/**
 * Runs the application event loop until Exit is called.
 *
 * Publishes ReadyAppEvent before the loop starts, UpdateAppEvent on each
 * tick, and ExitAppEvent after the loop ends. Timeouts and intervals only
 * fire while the loop is running. Call once, from the main thread.
 *
 * @param argc Argument count, as passed to main(); not currently used by the macOS backend.
 * @param argv Argument vector, as passed to main(); not currently used by the macOS backend.
 */
void Run(int argc, char const** argv);

/** Ends the run loop at the end of the current tick; App::Run then publishes ExitAppEvent and returns. Safe to call from event handlers. */
void Exit();

} /* namespace App */
} /* namespace Rocket */
