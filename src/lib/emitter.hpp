#ifndef UVCLS_EMITTER_INCLUDE_H
#define UVCLS_EMITTER_INCLUDE_H

#include <uv.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "config.h"

namespace uvcls {

namespace internal {

// nodiscard 是c++17引入的一种标记符，其语法一般为[[nodiscard]]或[[nodiscard("string")]](c++20引入)，含义可以理解为“不应舍弃”。nodiscard一般用于标记函数的返回值或者某个类，当使用某个弃值表达式而不是cast to void 来调用相关函数时，编译器会发出相关warning。
// hash 算法：https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// Fowler-Noll-Vo hash function v. 1a - the good
[[nodiscard]] static constexpr std::uint32_t fnv1a(const char *curr) noexcept {
    constexpr std::uint32_t offset = 2166136261;
    constexpr std::uint32_t prime = 16777619;
    auto value = offset;

    while (*curr != 0) {
        value = (value ^ static_cast<std::uint32_t>(*(curr++))) * prime;
    }

    return value;
}

[[nodiscard]] static inline std::uint32_t counter() noexcept {
    static std::uint32_t cnt{};
    return cnt++;
}

template <typename Type>
[[nodiscard]] static std::uint32_t fake() noexcept {
    static std::uint32_t local = counter();
    return local;
}

}  // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

// 对传入的模板类型，返回数字id。__PRETTY_FUNCTION__ 是 g++ 的内置的
template <typename Type>
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

struct ErrorEvent {
    template <typename U, typename = std::enable_if_t<std::is_integral_v<U>>>
    explicit ErrorEvent(U val) noexcept
        : ec{static_cast<int>(val)} {}

    static int translate(int sys) noexcept;

    const char *what() const noexcept;

    const char *name() const noexcept;

    int code() const noexcept;

    explicit operator bool() const noexcept;

   private:
    const int ec;
};

template <typename T>
class Emitter {
    // 用于作为父类 std::unordered_map 的存储对象，这里的虚析构函数用于析构子类对象。
    struct BaseListener {
        virtual ~BaseListener() noexcept = default;
        virtual bool empty() const noexcept = 0;
        virtual void clear() noexcept = 0;
    };

    // 内部类，事件发射后，对应的处理类。1个Listener处理1类事件（例如 ErrorEvent）对应多个监听函数
    // 1. 模板 T 是 Emitter 的子类。
    // 2. 模板 E 是 事件。
    template <typename E>
    struct Listener final : BaseListener {
        // 定义自定义类型。
        using Func = std::function<void(E &, T &)>;
        using Element = std::pair<bool, Func>;
        using FuncList = std::list<Element>;
        using Index = typename FuncList::iterator;

        bool empty() const noexcept override {
            auto pred = [](auto &&element) { return element.first; };

            return std::all_of(onceL.cbegin(), onceL.cend(), pred) && std::all_of(onL.cbegin(), onL.cend(), pred);
        }

        void clear() noexcept override {
            if (publishing) {
                auto func = [](auto &&element) { element.first = true; };
                std::for_each(onceL.begin(), onceL.end(), func);
                std::for_each(onL.begin(), onL.end(), func);
            } else {
                onceL.clear();
                onL.clear();
            }
        }

        // 一次性的监听事件
        Index once(Func f) {
            return onceL.emplace(onceL.cend(), false, std::move(f));
        }

        // 持久性的监听事件
        Index on(Func f) {
            return onL.emplace(onL.cend(), false, std::move(f));
        }

        void erase(Index conn) noexcept {
            conn->first = true;

            if (!publishing) {
                auto pred = [](auto &&element) { return element.first; };
                onceL.remove_if(pred);
                onL.remove_if(pred);
            }
        }

        // 发布 1 个事件，执行对应的事件监听函数
        void publish(E event, T &ref) {
            // 将 onceL 置为空
            FuncList currentL;
            onceL.swap(currentL);

            // 事件会将 event 和 event 的 emitter 传给处理函数。element.first 因为这里是 std::pair
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
        FuncList onceL{};
        FuncList onL{};
    };

    // listener 函数，返回1个listener。每个Event类型E对应1个Listener，1个Listener对应多个Func。
    template <typename E>
    Listener<E> &listener() noexcept {
        auto id = type<E>();

        // 如果没有这个类型的 listeners, 创建新的 listener 对象
        if (!listeners.count(id)) {
            listeners[id] = std::make_unique<Listener<E>>();
        }

        return static_cast<Listener<E> &>(*listeners.at(id));
    }

   protected:
    template <typename E>
    void publish(E event) {
        listener<E>().publish(std::move(event), *static_cast<T *>(this));
    }

   public:
    template <typename E>
    // C++11使用using定义类型的别名（替代typedef）。这里需要加 typename 的原因是 Func 是类内部的类型（所以它有可能是成员）
    // https://stackoverflow.com/questions/1600936/officially-what-is-typename-for
    using Func = typename Listener<E>::Func;

    // Index 代表 FuncList::iterator。std::list<Element>;
    // Index *conn 能够获取到元素
    template <typename E>
    struct Index : private Listener<E>::Index {
        template <typename>
        friend class Emitter;

        Index() = default;
        Index(const Index &) = default;
        Index(Index &&) = default;

        Index(typename Listener<E>::Index conn)
            : Listener<E>::Index{std::move(conn)} {}

        // 赋值构造函数
        Index &operator=(const Index &) = default;
        Index &operator=(Index &&) = default;
    };

    virtual ~Emitter() noexcept {
        static_assert(std::is_base_of_v<Emitter<T>, T>);
    }

    // on 给 Listener的 FuncList 添加数据
    template <typename E>
    Index<E> on(Func<E> f) {
        return listener<E>().on(std::move(f));
    }

    // on 给 Listener的 FuncList 添加数据（事件仅执行1次）。
    template <typename E>
    Index<E> once(Func<E> f) {
        return listener<E>().once(std::move(f));
    }

    template <typename E>
    void erase(Index<E> index) noexcept {
        listener<E>().erase(std::move(index));
    }

    template <typename E>
    void clear() noexcept {
        listener<E>().clear();
    }

    void clear() noexcept {
        std::for_each(listeners.begin(), listeners.end(), [](auto &&hdlr) { if(hdlr.second) { hdlr.second->clear(); } });
    }

    template <typename E>
    bool empty() const noexcept {
        auto id = type<E>();
        return (!listeners.count(id) || static_cast<Listener<E> &>(*listeners.at(id)).empty());
    }

    bool empty() const noexcept {
        return std::all_of(listeners.cbegin(), listeners.cend(), [](auto &&hdlr) { return !hdlr.second || hdlr.second->empty(); });
    }

   private:
    std::unordered_map<std::uint32_t, std::unique_ptr<BaseListener>> listeners{};
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

}  // namespace uvcls

#endif  // UVCLS_EMITTER_INCLUDE_H
