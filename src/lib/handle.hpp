#ifndef UVCLS_HANDLE_INCLUDE_H
#define UVCLS_HANDLE_INCLUDE_H

#include <memory>
#include "uv.h"
#include "loop.hpp"
#include "emitter.hpp"

namespace uvcls {

struct CloseEvent {};

// UnderlyingType 表示底层的Loop类和libuv的handle, req 资源。
template<typename T, typename U>
class UnderlyingType {

public:
    explicit UnderlyingType(std::shared_ptr<Loop> ref) noexcept
        : pLoop{std::move(ref)}, resource{} {}

    // 取出当前底层资源，例如 uv_tcp_t
    template<typename R = U>
    auto get() noexcept {
        return reinterpret_cast<R *>(&resource);
    }

    // 取出当前底层资源，例如 uv_tcp_t
    template<typename R = U>
    auto get() const noexcept {
        return reinterpret_cast<const R *>(&resource);
    }

    // 获取其他底层资源的指针
    template<typename R, typename... P>
    auto get(UnderlyingType<P...> &other) noexcept {
        return reinterpret_cast<R *>(&other.resource);
    }

    Loop &loop() const noexcept {
        return *pLoop;
    }

private:
    std::shared_ptr<Loop> pLoop;
    U resource; // T 代表 UnderlyingType, U 代表类似 uv_idle_t 类型
};

template<typename T, typename U>
class Resource: public UnderlyingType<T, U>, public Emitter<T>, public std::enable_shared_from_this<T> {

public:
    // req 和 handle 也好都有 data，用于绑定底层资源和上层的封装类关系
    explicit Resource(std::shared_ptr<Loop> ref)
        : UnderlyingType<T, U>{std::move(ref)} {
        this->get()->data = this;
    }

    bool self() const noexcept {
        return static_cast<bool>(sPtr);
    }
    
    // 初始化本身的的“共享指针”。sPtr 意思是 shared ptr
    void leak() noexcept {
        sPtr = this->shared_from_this();
    }

    // 释放资源的所有权（不是释放资源）
    void reset() noexcept {
        sPtr.reset();
    }

    // 资源的父级定义为 loop。因为资源都会属于1个loop
    auto parent() const noexcept {
        return this->loop().loop.get();
    }


private:
    std::shared_ptr<void> userData{nullptr};
    std::shared_ptr<void> sPtr{nullptr};
};

template<typename T, typename U>
class Handle: public Resource<T, U> {

public:
    using Resource<T, U>::Resource;

    // this->template 是因为 Handle 继承了模板类。close 时，事件循环为 close 阶段执行
    void close() noexcept {
        if(!closing()) {
            // 关闭 handle 时调用回调地址, 类的成员函数指针需要 & 符号（不像C 函数名就是指针）
            uv_close(this->template get<uv_handle_t>(), &Handle<T, U>::closeCallback);
        }
    }

    bool closing() const noexcept {
        return !(uv_is_closing(this->template get<uv_handle_t>()) == 0);
    }

protected:
    // 关闭 handle 时调用的回调
    static void closeCallback(uv_handle_t *handle) {
        Handle<T, U> &ref = *(static_cast<T *>(handle->data));
        [[maybe_unused]] auto ptr = ref.shared_from_this();
        ref.reset();
        ref.publish(CloseEvent{});
    }

    static void allocCallback(uv_handle_t *, std::size_t suggested, uv_buf_t *buf) {
        auto size = static_cast<unsigned int>(suggested);
        *buf = uv_buf_init(new char[size], size);
    }

    template<typename F, typename... Args>
    void invoke(F &&f, Args &&...args) {
        auto err = std::forward<F>(f)(std::forward<Args>(args)...);
        if(err) { Emitter<T>::publish(ErrorEvent{err}); }
    }

    template<typename F, typename... Args>
    bool initialize(F &&f, Args &&...args) {
        if(!this->self()) {
            if(auto err = std::forward<F>(f)(this->parent(), this->get(), std::forward<Args>(args)...); err) {
                this->publish(ErrorEvent{err});
            } else {
                this->leak();
            }
        }
        return this->self();
    }
};


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