#ifndef UVCLS_UTIL_INCLUDE_H
#define UVCLS_UTIL_INCLUDE_H

namespace uvcls {

// 标志类。
// 1. E 表示1个枚举类

// #include <iostream>

// template<typename T>
// class Person {
// public:
// template<T V>
// void hello() {
//     std::cout << V << std::endl;
// }
// };

// int main() {
//     Person<int> person{};
//     person.hello<1>();
//     return 0;
// }

template <typename E>
class Flags final {
    static_assert(std::is_enum_v<E>);

    using InnerType = std::underlying_type_t<E>;

    // 将 flag 转成 InnerType
    constexpr InnerType toInnerType(E flag) const noexcept {
        return static_cast<InnerType>(flag);
    }

   public:
    using Type = InnerType;

    // 工厂方法创建 Flag。这里
    // 1. C++ 17 fold expression
    // template<typename... Args>
    // auto sum(Args... ns)
    // {
    //     return (0 + ... + ns);
    // }
    //
    // 2. E 是类型（枚举类型），V 是值。 E 来自于顶部类上的模板 E。
    // 3. 非类型模板参数。
    template <E... V>
    static constexpr Flags<E> from() {
        return (Flags<E>{} | ... | V);
    }

    // 构造函数。注意这里可以调用函数 toInnerType
    /**
     * @brief Constructs a Flags object from a value of the enum `E`.
     * @param flag A value of the enum `E`.
     */
    constexpr Flags(E flag) noexcept
        : flags{toInnerType(flag)} {}

    // 构造函数。直接使用 Type 类型的参数
    constexpr Flags(Type f)
        : flags{f} {}

    constexpr Flags()
        : flags{} {}

    constexpr Flags(const Flags &f) noexcept
        : flags{f.flags} {}

    constexpr Flags(Flags &&f) noexcept
        : flags{std::move(f.flags)} {}

    constexpr Flags &operator=(const Flags &f) noexcept {
        flags = f.flags;
        return *this;
    }

    constexpr Flags &operator=(Flags &&f) noexcept {
        flags = std::move(f.flags);
        return *this;
    }

    constexpr Flags operator|(const Flags &f) const noexcept {
        return Flags{flags | f.flags};
    }

    constexpr Flags operator|(E flag) const noexcept {
        return Flags{flags | toInnerType(flag)};
    }

    constexpr Flags operator&(const Flags &f) const noexcept {
        return Flags{flags & f.flags};
    }

    constexpr Flags operator&(E flag) const noexcept {
        return Flags{flags & toInnerType(flag)};
    }

    explicit constexpr operator bool() const noexcept {
        return !(flags == InnerType{});
    }

    constexpr operator Type() const noexcept {
        return flags;
    }

   private:
    InnerType flags;
};

}  // namespace uvcls
#endif