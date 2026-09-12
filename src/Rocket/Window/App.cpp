#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <algorithm>
#include <Rocket/Base/Time.hpp>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App_Private.hpp>

namespace Rocket {
namespace App {

struct _Timeout {
    std::function<void()> callback;
    double timeValue;
    double invokeTime;
    bool isInvoked;
    std::atomic<bool> isCancelled;
};

static auto _lastTime = 0.0;
static auto _deltaTime = 0.0;
static auto _timeouts = std::vector<std::shared_ptr<_Timeout>>();
static auto _intervals = std::vector<std::shared_ptr<_Timeout>>();
static auto _mutex = std::mutex();

Pub<AppEvent const&> OnEvent = {};

static void _UpdateDeltaTime() {
    PROFILE

    auto nowTime = GetTime();
    _deltaTime = (nowTime - _lastTime);
    _lastTime = nowTime;
}

static void _UpdateTimeouts() {
    PROFILE

    auto const nowTime = GetTime();

    auto timeouts = std::vector<std::shared_ptr<_Timeout>>();
    {
        auto lock = std::scoped_lock(_mutex);
        timeouts = _timeouts;
    }

    for (auto& timeout : timeouts) {
        if (timeout->isCancelled) {
            continue;
        }

        if (timeout->invokeTime <= nowTime) {
            timeout->callback();
            timeout->isInvoked = true;
        }
    }

    {
        auto lock = std::scoped_lock(_mutex);
        _timeouts.erase(
            std::remove_if(
                _timeouts.begin(),
                _timeouts.end(),
                [](auto& timeout) {
                    return (timeout->isCancelled)
                        || (timeout->isInvoked);
                }
            ),
            _timeouts.end()
        );
    }
}

static void _UpdateIntervals() {
    PROFILE

    auto const nowTime = GetTime();

    auto intervals = std::vector<std::shared_ptr<_Timeout>>();
    {
        auto lock = std::scoped_lock(_mutex);
        intervals = _intervals;
    }

    for (auto& interval : intervals) {
        if (interval->isCancelled) {
            continue;
        }

        if (interval->invokeTime <= nowTime) {
            interval->callback();
            interval->isInvoked = true;
            interval->invokeTime += interval->timeValue;
        }
    }

    {
        auto lock = std::scoped_lock(_mutex);
        _intervals.erase(
            std::remove_if(
                _intervals.begin(),
                _intervals.end(),
                [](auto& interval) {
                    return interval->isCancelled.load();
                }
            ),
            _intervals.end()
        );
    }
}

std::string const& GetExecPath() {
    PROFILE

    return __GetExecPath();
}

std::string const& GetExecDir() {
    PROFILE

    return __GetExecDir();
}

std::string const& GetWorkDir() {
    PROFILE

    return __GetWorkDir();
}

std::string const& GetUserDir() {
    PROFILE

    return __GetUserDir();
}

std::function<void()> SetTimeout(std::function<void()> const& callback, double timeout) {
    PROFILE

    auto record = std::make_shared<_Timeout>();
    record->callback = callback;
    record->timeValue = timeout;
    record->invokeTime = (GetTime() + timeout);
    record->isCancelled = false;
    record->isInvoked = false;

    {
        auto lock = std::scoped_lock(_mutex);
        _timeouts.push_back(record);
    }

    return [record]() {
        record->isCancelled = true;
    };
}

std::function<void()> SetInterval(std::function<void()> const& callback, double interval) {
    PROFILE

    auto record = std::make_shared<_Timeout>();
    record->callback = callback;
    record->timeValue = interval;
    record->invokeTime = (GetTime() + interval);
    record->isCancelled = false;
    record->isInvoked = false;

    {
        auto lock = std::scoped_lock(_mutex);
        _intervals.push_back(record);
    }

    return [record]() {
        record->isCancelled = true;
    };
}

void Run(int argc, char const** argv) {
    // PROFILE

    _lastTime = GetTime();
    _deltaTime = 0.0;

    OnEvent.publish(ReadyAppEvent{});

    __Run(argc, argv);

    OnEvent.publish(ExitAppEvent{});
}

void Exit() {
    PROFILE

    __Exit();
}

void _Update() {
    PROFILE

    _UpdateDeltaTime();
    _UpdateTimeouts();
    _UpdateIntervals();

    OnEvent.publish(UpdateAppEvent{});
}

void _Close() {
    PROFILE

    OnEvent.publish(CloseAppEvent{});
}

} /* namespace App */
} /* namespace Rocket */