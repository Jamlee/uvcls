#ifndef UVCLS_TYPE_INFO_INCLUDE_HPP
#define UVCLS_TYPE_INFO_INCLUDE_HPP

#include <cstddef>
#include <string_view>
#include <iostream>

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

} // namespace uvw

#endif // UVCLS_TYPE_INFO_INCLUDE_HPP
