#pragma once

#include <string>
#include <thread>
#include <optional>
#include <functional>
#include <Rocket/Base/PubSub.hpp>
#include <Rocket/Base/Profile.hpp>
#include <Rocket/Window/App.hpp>

namespace Rocket {

/**
 * A restartable background task: run() executes the work function on a
 * background thread and publishes through onResult on the app run loop.
 * If the work function throws, onError publishes the exception message
 * instead. Calling run() while a run is in flight aborts it (latest
 * wins); an aborted or outlived run publishes nothing.
 *
 * run(), abort() and destruction must happen on the main thread — the
 * completion callback runs there too, which is what makes the raw
 * back-pointer handshake below safe.
 */
template<typename OptionsType, typename ResultType>
class Task {
public:
    /** Published on the app run loop when a run completes successfully. */
    Pub<ResultType const&> onResult;

    /** Published on the app run loop with the exception message when a run throws. */
    Pub<std::string const&> onError;

    ~Task();
    Task(std::function<ResultType(OptionsType const&)> const&);

    Task(Task &&) = delete;
    Task(Task const&) = delete;
    Task& operator=(Task &&) = delete;
    Task& operator=(Task const&) = delete;

    /** Returns true while a run is in flight. */
    bool isRunning() const;

    /** Starts the work on a background thread, aborting any run still in flight. */
    void run(OptionsType const&);

    /** Detaches from a run in flight; its result is discarded when it finishes. */
    void abort();
private:
    struct _Run {
        std::optional<ResultType> result;
        std::optional<std::string> error;
        Task<OptionsType, ResultType>* self;
    };

    std::function<ResultType(OptionsType const&)> _fn;
    std::shared_ptr<_Run> _run;
};

template<typename OptionsType, typename ResultType>
inline Task<OptionsType, ResultType>::~Task() {
    PROFILE

    abort();
}

template<typename OptionsType, typename ResultType>
inline Task<OptionsType, ResultType>::Task(std::function<ResultType(OptionsType const&)> const& fn)
    : _fn(fn)
    , _run(nullptr)
{
    PROFILE
}

template<typename OptionsType, typename ResultType>
inline bool Task<OptionsType, ResultType>::isRunning() const {
    PROFILE

    return (_run != nullptr);
}

template<typename OptionsType, typename ResultType>
inline void Task<OptionsType, ResultType>::run(OptionsType const& options) {
    PROFILE

    abort();

    _run = std::make_shared<_Run>();
    _run->self = this;

    std::thread([options, fn = _fn, run = _run] {
        try {
            run->result = fn(options);
        } catch (std::exception const& exception) {
            run->error = exception.what();
        } catch (...) {
            run->error = "unknown error";
        }

        App::SetTimeout([run]{
            if (run->self != nullptr) {
                auto self = run->self;
                run->self = nullptr;
                self->_run = nullptr;

                if (run->error.has_value()) {
                    self->onError.publish(*run->error);
                } else {
                    self->onResult.publish(*run->result);
                }
            }
        }, 0);
    }).detach();
}

template<typename OptionsType, typename ResultType>
inline void Task<OptionsType, ResultType>::abort() {
    PROFILE

    if (_run != nullptr) {
        _run->self = nullptr;
        _run = nullptr;
    }
}

} /* namespace Rocket */
