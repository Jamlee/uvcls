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

template<typename E>
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
    /**
     * @brief Utility factory method to pack a set of values all at once.
     * @return A valid instance of Flags instantiated from values `V`.
     */
    template<E... V>
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
    /**
     * @brief Constructs a Flags object from an instance of the underlying type
     * of the enum `E`.
     * @param f An instance of the underlying type of the enum `E`.
     */
    constexpr Flags(Type f)
        : flags{f} {}

    /**
     * @brief Constructs an uninitialized Flags object.
     */
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

    /**
     * @brief Or operator.
     * @param f A valid instance of Flags.
     * @return This instance _or-ed_ with `f`.
     */
    constexpr Flags operator|(const Flags &f) const noexcept {
        return Flags{flags | f.flags};
    }

    /**
     * @brief Or operator.
     * @param flag A value of the enum `E`.
     * @return This instance _or-ed_ with `flag`.
     */
    constexpr Flags operator|(E flag) const noexcept {
        return Flags{flags | toInnerType(flag)};
    }

    /**
     * @brief And operator.
     * @param f A valid instance of Flags.
     * @return This instance _and-ed_ with `f`.
     */
    constexpr Flags operator&(const Flags &f) const noexcept {
        return Flags{flags & f.flags};
    }

    /**
     * @brief And operator.
     * @param flag A value of the enum `E`.
     * @return This instance _and-ed_ with `flag`.
     */
    constexpr Flags operator&(E flag) const noexcept {
        return Flags{flags & toInnerType(flag)};
    }

    /**
     * @brief Checks if this instance is initialized.
     * @return False if it's uninitialized, true otherwise.
     */
    explicit constexpr operator bool() const noexcept {
        return !(flags == InnerType{});
    }

    /**
     * @brief Casts the instance to the underlying type of `E`.
     * @return An integral representation of the contained flags.
     */
    constexpr operator Type() const noexcept {
        return flags;
    }

private:
    InnerType flags;
};

}
#endif