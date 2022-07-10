#ifndef UVCLS_LOOP_INCLUDE_H
#define UVCLS_LOOP_INCLUDE_H

#include <chrono>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <uv.h>
#include "emitter.hpp"

namespace uvcls {

// 枚举定义 loop 的三种运行模式。uv_run_mode 是枚举
enum class UVRunMode : std::underlying_type_t<uv_run_mode> {
    DEFAULT = UV_RUN_DEFAULT,
    ONCE = UV_RUN_ONCE,
    NOWAIT = UV_RUN_NOWAIT
};

enum class UVLoopOption : std::underlying_type_t<uv_loop_option> {
    BLOCK_SIGNAL = UV_LOOP_BLOCK_SIGNAL,
};

class Loop final: public Emitter<Loop>, public std::enable_shared_from_this<Loop> {
    // 释放 uv_loop_t 占的空间
    using Deleter = void (*)(uv_loop_t *);

    template<typename, typename>
    friend class Resource;

public:
    // 获取 Loop 类默认实例
    static std::shared_ptr<Loop> getDefault();

    Loop(std::unique_ptr<uv_loop_t, Deleter> ptr) noexcept;

    ~Loop() noexcept;

    void close();

    template<UVRunMode mode = UVRunMode::DEFAULT>
    bool run() noexcept;

    void stop() noexcept;

private:
    std::unique_ptr<uv_loop_t, Deleter> loop;
    std::shared_ptr<void> userData{nullptr};
};

// 获取默认的 Loop
UVCLS_INLINE std::shared_ptr<Loop> Loop::getDefault() {
    static std::weak_ptr<Loop> ref;
    std::shared_ptr<Loop> loop;

    if(ref.expired()) {
        auto def = uv_default_loop();
        if(def) {
            auto ptr = std::unique_ptr<uv_loop_t, Deleter>(def, [](uv_loop_t *) {});
            loop = std::shared_ptr<Loop>{new Loop{std::move(ptr)}};
        }
        ref = loop;
    } else {
        loop = ref.lock();
    }

    return loop;
}

template<UVRunMode mode>
bool Loop::run() noexcept {
    auto utm = static_cast<std::underlying_type_t<UVRunMode>>(mode);
    auto uvrm = static_cast<uv_run_mode>(utm);
    return (uv_run(loop.get(), uvrm) == 0);
}

UVCLS_INLINE void Loop::stop() noexcept {
    uv_stop(loop.get());
}

UVCLS_INLINE void Loop::close() {
    auto err = uv_loop_close(loop.get());
    return err ? publish(ErrorEvent{err}) : loop.reset();
}

UVCLS_INLINE Loop::Loop(std::unique_ptr<uv_loop_t, Deleter> ptr) noexcept
    : loop{std::move(ptr)} {}

UVCLS_INLINE Loop::~Loop() noexcept {
    if(loop) {
        close();
    }
}

}

#endif