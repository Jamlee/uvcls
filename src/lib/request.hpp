#ifndef UVCLS_REQUEST_INCLUDE_H
#define UVCLS_REQUEST_INCLUDE_H

#include <memory>
#include <type_traits>
#include <utility>
#include <uv.h>
#include "handle.hpp"

namespace uvcls {

// Req 类型（另一种是 handle）
template<typename T, typename U>
class Request: public Resource<T, U> {
protected:
    static auto reserve(U *req) {
        auto ptr = static_cast<T *>(req->data)->shared_from_this();
        ptr->reset();
        return ptr;
    }

    template<typename E>
    static void defaultCallback(U *req, int status) {
        if(auto ptr = reserve(req); status) {
            ptr->publish(ErrorEvent{status});
        } else {
            ptr->publish(E{});
        }
    }

    template<typename F, typename... Args>
    auto invoke(F &&f, Args &&...args) {
        if constexpr(std::is_void_v<std::invoke_result_t<F, Args...>>) {
            std::forward<F>(f)(std::forward<Args>(args)...);
            this->leak();
        } else {
            if(auto err = std::forward<F>(f)(std::forward<Args>(args)...); err) {
                Emitter<T>::publish(ErrorEvent{err});
            } else {
                this->leak();
            }
        }
    }

public:
    using Resource<T, U>::Resource;

    bool cancel() {
        return (0 == uv_cancel(this->template get<uv_req_t>()));
    }

    std::size_t size() const noexcept {
        return uv_req_size(this->template get<uv_req_t>()->type);
    }
};

}

#endif
