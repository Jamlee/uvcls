#ifndef UVCLS_IDLE_INCLUDE_H
#define UVCLS_IDLE_INCLUDE_H

#include <uv.h>
#include "handle.hpp"

namespace uvcls {

struct IdleEvent {};

class IdleHandle final: public Handle<IdleHandle, uv_idle_t> {

    static void startCallback(uv_idle_t *handle);

public:

    using Handle::Handle;

    bool init();

    void start();

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
    invoke(&uv_idle_start, get(), &startCallback);
}

UVCLS_INLINE void IdleHandle::stop() {
    invoke(&uv_idle_stop, get());
}

}

#endif