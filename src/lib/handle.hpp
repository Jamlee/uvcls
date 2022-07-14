#ifndef UVCLS_HANDLE_INCLUDE_H
#define UVCLS_HANDLE_INCLUDE_H

#include <memory>
#include "uv.h"
#include "loop.hpp"
#include "emitter.hpp"

namespace uvcls {

struct CloseEvent {};

template<typename T, typename U>
class UnderlyingType {

public:
    explicit UnderlyingType(std::shared_ptr<Loop> ref) noexcept
        : pLoop{std::move(ref)}, resource{} {}

    template<typename R = U>
    auto get() noexcept {
        return reinterpret_cast<R *>(&resource);
    }

    template<typename R = U>
    auto get() const noexcept {
        return reinterpret_cast<const R *>(&resource);
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
    explicit Resource(std::shared_ptr<Loop> ref)
        : UnderlyingType<T, U>{std::move(ref)} {
        this->get()->data = this;
    }

    bool self() const noexcept {
        return static_cast<bool>(sPtr);
    }
    
    void leak() noexcept {
        sPtr = this->shared_from_this();
    }

    void reset() noexcept {
        sPtr.reset();
    }

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
            // 关闭 handle 时调用回调地址, 类的成员函数需要 & 符号（不像C 函数名就是指针）
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

}
#endif