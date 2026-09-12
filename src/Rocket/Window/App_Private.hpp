#pragma once

#include <Rocket/Window/App.hpp>

namespace Rocket {
namespace App {

/** Advances the frame timer, runs due timeouts and intervals, and publishes UpdateAppEvent; called once per run-loop tick. */
void _Update();

/** Publishes CloseAppEvent; called by the platform layer when the user requests to quit. */
void _Close();

/** Platform implementation of App::GetExecPath. */
std::string const& __GetExecPath();

/** Platform implementation of App::GetExecDir. */
std::string const& __GetExecDir();

/** Platform implementation of App::GetWorkDir. */
std::string const& __GetWorkDir();

/** Platform implementation of App::GetUserDir. */
std::string const& __GetUserDir();

/** Platform implementation of App::Exit. */
void __Exit();

/**
 * Platform implementation of App::Run.
 *
 * @param argc Argument count, as passed to main(); not currently used by the macOS backend.
 * @param argv Argument vector, as passed to main(); not currently used by the macOS backend.
 */
void __Run(int argc, char const** argv);

} /* namespace App */
} /* namespace Rocket */