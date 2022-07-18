#ifndef UVCLS_IDLE_INCLUDE_H
#define UVCLS_IDLE_INCLUDE_H

#include <uv.h>

#include "handle.hpp"

namespace uvcls {

struct IdleEvent {};

/*
当 uv_idle_start 时，传入 1 个回调函数。在 idle 阶段运行
*/
class IdleHandle final : public Handle<IdleHandle, uv_idle_t> {
    static void startCallback(uv_idle_t *handle);

   public:
    using Handle::Handle;

    // 初始化 handle。 会创建 uv_idle_handle 结构体。
    bool init();

    // 启动 handle 的含义是 uv_idle_handle 插入到 handle->loop->idle_handles。
    void start();

    // 停止 handle 的含义是从队列中移除，handle->queue 。
    // 1. 修改 handle 的 flags
    // 2. loop->active_handles-- 激活状态的 handles 计数器减去 1
    void stop();
};

UVCLS_INLINE void IdleHandle::startCallback(uv_idle_t *handle) {
    IdleHandle &idle = *(static_cast<IdleHandle *>(handle->data));
    idle.publish(IdleEvent{});
}

UVCLS_INLINE bool IdleHandle::init() {
    return initialize(&uv_idle_init);
}

UVCLS_INLINE void IdleHandle::start() {
    // C++ 里面函数指针标准带上 & 符号（就算不带 C++ 也会隐含带上 &）
    invoke(&uv_idle_start, get(), &startCallback);
}

UVCLS_INLINE void IdleHandle::stop() {
    invoke(&uv_idle_stop, get());
}

}  // namespace uvcls

#endif