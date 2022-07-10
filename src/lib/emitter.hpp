#ifndef UVCLS_EMITTER_INCLUDE_H
#define UVCLS_EMITTER_INCLUDE_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <uv.h>
#include "config.h"

namespace uvcls {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

// nodiscard 是c++17引入的一种标记符，其语法一般为[[nodiscard]]或[[nodiscard("string")]](c++20引入)，含义可以理解为“不应舍弃”。nodiscard一般用于标记函数的返回值或者某个类，当使用某个弃值表达式而不是cast to void 来调用相关函数时，编译器会发出相关warning。
// hash 算法：https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// Fowler-Noll-Vo hash function v. 1a - the good
[[nodiscard]] static constexpr std::uint32_t fnv1a(const char *curr) noexcept {
    constexpr std::uint32_t offset = 2166136261;
    constexpr std::uint32_t prime = 16777619;
    auto value = offset;

    while(*curr != 0) {
        value = (value ^ static_cast<std::uint32_t>(*(curr++))) * prime;
    }

    return value;
}

[[nodiscard]] static inline std::uint32_t counter() noexcept {
    static std::uint32_t cnt{};
    return cnt++;
}

template<typename Type>
[[nodiscard]] static std::uint32_t fake() noexcept {
    static std::uint32_t local = counter();
    return local;
}

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

// 对传入的模板类型，返回数字id。__PRETTY_FUNCTION__ 是 g++ 的内置的
/**
 * @brief Returns a numerical identifier for a given type.
 * @tparam Type The type for which to return the numerical identifier.
 * @return The numerical identifier of the give type.
 */
template<typename Type>
[[nodiscard]] static constexpr std::uint32_t type() noexcept {
#if defined __clang__ || defined __GNUC__
    // 会得到这种输出，带有 Type的类型
    // std::cout << __PRETTY_FUNCTION__ << std::endl;
    // constexpr uint32_t uvcls::type() [with Type = HelloWorld; uint32_t = unsigned int]
    return internal::fnv1a(__PRETTY_FUNCTION__);
#elif defined _MSC_VER
    return internal::fnv1a(__FUNCSIG__);
#else
    return internal::fake();
#endif
}


/**
 * @brief The ErrorEvent event.
 *
 * Custom wrapper around error constants of `libuv`.
 */
struct ErrorEvent {
    template<typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
    explicit ErrorEvent(U val) noexcept
        : ec{static_cast<int>(val)} {}

    /**
     * @brief Returns the `libuv` error code equivalent to the given platform dependent error code.
     *
     * It returns:
     * * POSIX error codes on Unix (the ones stored in errno).
     * * Win32 error codes on Windows (those returned by GetLastError() or WSAGetLastError()).
     *
     * If `sys` is already a `libuv` error code, it is simply returned.
     *
     * @param sys A platform dependent error code.
     * @return The `libuv` error code equivalent to the given platform dependent error code.
     */
    static int translate(int sys) noexcept;

    /**
     * @brief Returns the error message for the given error code.
     *
     * Leaks a few bytes of memory when you call it with an unknown error code.
     *
     * @return The error message for the given error code.
     */
    const char *what() const noexcept;

    /**
     * @brief Returns the error name for the given error code.
     *
     * Leaks a few bytes of memory when you call it with an unknown error code.
     *
     * @return The error name for the given error code.
     */
    const char *name() const noexcept;

    /**
     * @brief Gets the underlying error code, that is an error constant of `libuv`.
     * @return The underlying error code.
     */
    int code() const noexcept;

    /**
     * @brief Checks if the event contains a valid error code.
     * @return True in case of success, false otherwise.
     */
    explicit operator bool() const noexcept;

private:
    const int ec;
};

/**
 * @brief Event emitter base class.
 *
 * Almost everything in `uvw` is an event emitter.<br/>
 * This is the base class from which resources and loops inherit.
 */
template<typename T>
class Emitter {
    // 内部类
    struct BaseHandler {
        virtual ~BaseHandler() noexcept = default;
        virtual bool empty() const noexcept = 0;
        virtual void clear() noexcept = 0;
    };

    // 内部类，事件发射后，对应的处理类。1个Handler处理1类事件（例如 ErrorEvent）对应多个监听函数
    // 1. 模板 T 是 Emitter 的子类。
    // 2. 模板 E 是 事件。
    template<typename E>
    struct Handler final: BaseHandler {
        // 定义自定义类型。
        using Listener = std::function<void(E &, T &)>;
        using Element = std::pair<bool, Listener>;
        using ListenerList = std::list<Element>;
        using Connection = typename ListenerList::iterator;

        bool empty() const noexcept override {
            auto pred = [](auto &&element) { return element.first; };

            return std::all_of(onceL.cbegin(), onceL.cend(), pred) && std::all_of(onL.cbegin(), onL.cend(), pred);
        }

        void clear() noexcept override {
            if(publishing) {
                auto func = [](auto &&element) { element.first = true; };
                std::for_each(onceL.begin(), onceL.end(), func);
                std::for_each(onL.begin(), onL.end(), func);
            } else {
                onceL.clear();
                onL.clear();
            }
        }

        // 一次性的监听事件
        Connection once(Listener f) {
            return onceL.emplace(onceL.cend(), false, std::move(f));
        }

        // 持久性的监听事件
        Connection on(Listener f) {
            return onL.emplace(onL.cend(), false, std::move(f));
        }

        void erase(Connection conn) noexcept {
            conn->first = true;

            if(!publishing) {
                auto pred = [](auto &&element) { return element.first; };
                onceL.remove_if(pred);
                onL.remove_if(pred);
            }
        }

        // 发布 1 个事件，执行对应的事件监听函数
        void publish(E event, T &ref) {
            // 将 onceL 置为空
            ListenerList currentL;
            onceL.swap(currentL);

            auto func = [&event, &ref](auto &&element) {
                return element.first ? void() : element.second(event, ref);
            };

            publishing = true;

            std::for_each(onL.rbegin(), onL.rend(), func);
            std::for_each(currentL.rbegin(), currentL.rend(), func);

            publishing = false;

            onL.remove_if([](auto &&element) { return element.first; });
        }

    private:
        bool publishing{false};
        ListenerList onceL{};
        ListenerList onL{};
    };

    // handler 函数，返回 1 个 handler。
    template<typename E>
    Handler<E> &handler() noexcept {
        auto id = type<E>();

        // 如果没有这个类型的 handlers, 创建新的 handler 对象
        if(!handlers.count(id)) {
            handlers[id] = std::make_unique<Handler<E>>();
        }

        return static_cast<Handler<E> &>(*handlers.at(id));
    }

protected:
    template<typename E>
    void publish(E event) {
        handler<E>().publish(std::move(event), *static_cast<T *>(this));
    }

public:
    template<typename E>
    // C++11使用using定义类型的别名（替代typedef）。这里需要加 typename 的原因是 Listener 是类内部的类型（所以它有可能是成员）
    // https://stackoverflow.com/questions/1600936/officially-what-is-typename-for
    using Listener = typename Handler<E>::Listener;

    // Connection 代表 ListenerList::iterator。std::list<Element>;
    // Connection conn 时， *conn 能够获取到元素
    /**
     * @brief Connection type for a given event type.
     *
     * Given an event type `E`, `Connection<E>` is the type of the connection
     * object returned by the event emitter whenever a listener for the given
     * type is registered.
     */
    template<typename E>
    struct Connection: private Handler<E>::Connection {
        template<typename>
        friend class Emitter;

        Connection() = default;
        Connection(const Connection &) = default;
        Connection(Connection &&) = default;

        Connection(typename Handler<E>::Connection conn)
            : Handler<E>::Connection{std::move(conn)} {}

        // 赋值构造函数
        Connection &operator=(const Connection &) = default;
        Connection &operator=(Connection &&) = default;
    };

    virtual ~Emitter() noexcept {
        static_assert(std::is_base_of_v<Emitter<T>, T>);
    }

    // on 给 list 添加数据
    /**
     * @brief Registers a long-lived listener with the event emitter.
     *
     * This method can be used to register a listener that is meant to be
     * invoked more than once for the given event type.<br/>
     * The Connection object returned by the method can be freely discarded. It
     * can be used later to disconnect the listener, if needed.
     *
     * Listener is usually defined as a callable object assignable to a
     * `std::function<void(const E &, T &)`, where `E` is the type of the event
     * and `T` is the type of the resource.
     *
     * @param f A valid listener to be registered.
     * @return Connection object to be used later to disconnect the listener.
     */
    template<typename E>
    Connection<E> on(Listener<E> f) {
        return handler<E>().on(std::move(f));
    }

    // 执行 1 次的 Listener
    /**
     * @brief Registers a short-lived listener with the event emitter.
     *
     * This method can be used to register a listener that is meant to be
     * invoked only once for the given event type.<br/>
     * The Connection object returned by the method can be freely discarded. It
     * can be used later to disconnect the listener, if needed.
     *
     * Listener is usually defined as a callable object assignable to a
     * `std::function<void(const E &, T &)`, where `E` is the type of the event
     * and `T` is the type of the resource.
     *
     * @param f A valid listener to be registered.
     * @return Connection object to be used later to disconnect the listener.
     */
    template<typename E>
    Connection<E> once(Listener<E> f) {
        return handler<E>().once(std::move(f));
    }

    /**
     * @brief Disconnects a listener from the event emitter.
     * @param conn A valid Connection object
     */
    template<typename E>
    void erase(Connection<E> conn) noexcept {
        handler<E>().erase(std::move(conn));
    }

    /**
     * @brief Disconnects all the listeners for the given event type.
     */
    template<typename E>
    void clear() noexcept {
        handler<E>().clear();
    }

    /**
     * @brief Disconnects all the listeners.
     */
    void clear() noexcept {
        std::for_each(handlers.begin(), handlers.end(), [](auto &&hdlr) { if(hdlr.second) { hdlr.second->clear(); } });
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @return True if there are no listeners registered for the specific event,
     * false otherwise.
     */
    template<typename E>
    bool empty() const noexcept {
        auto id = type<E>();

        return (!handlers.count(id) || static_cast<Handler<E> &>(*handlers.at(id)).empty());
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered with the event emitter,
     * false otherwise.
     */
    bool empty() const noexcept {
        return std::all_of(handlers.cbegin(), handlers.cend(), [](auto &&hdlr) { return !hdlr.second || hdlr.second->empty(); });
    }

private:
    std::unordered_map<std::uint32_t, std::unique_ptr<BaseHandler>> handlers{};
};


UVCLS_INLINE int ErrorEvent::translate(int sys) noexcept {
    return uv_translate_sys_error(sys);
}

UVCLS_INLINE const char *ErrorEvent::what() const noexcept {
    return uv_strerror(ec);
}

UVCLS_INLINE const char *ErrorEvent::name() const noexcept {
    return uv_err_name(ec);
}

UVCLS_INLINE int ErrorEvent::code() const noexcept {
    return ec;
}

UVCLS_INLINE ErrorEvent::operator bool() const noexcept {
    return ec < 0;
}

} // namespace uvcls

#endif // UVCLS_EMITTER_INCLUDE_H
