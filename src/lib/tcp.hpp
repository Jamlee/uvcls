#ifndef UVCLS_TCP_INCLUDE_H
#define UVCLS_TCP_INCLUDE_H

#include <chrono>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <uv.h>

#include "request.hpp"
#include "stream.hpp"
#include "handle.hpp"


namespace uvcls {

struct IPv6 {};
struct IPv4 {};

struct Addr {
    std::string ip;    /*!< Either an IPv4 or an IPv6. */
    unsigned int port; /*!< A valid service identifier. */
};

template<typename E>
class Flags final {
    static_assert(std::is_enum_v<E>);

    using InnerType = std::underlying_type_t<E>;

    constexpr InnerType toInnerType(E flag) const noexcept {
        return static_cast<InnerType>(flag);
    }

public:
    using Type = InnerType;

    /**
     * @brief Utility factory method to pack a set of values all at once.
     * @return A valid instance of Flags instantiated from values `V`.
     */
    template<E... V>
    static constexpr Flags<E> from() {
        return (Flags<E>{} | ... | V);
    }

    /**
     * @brief Constructs a Flags object from a value of the enum `E`.
     * @param flag A value of the enum `E`.
     */
    constexpr Flags(E flag) noexcept
        : flags{toInnerType(flag)} {}

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

enum class UVTCPFlags : std::underlying_type_t<uv_tcp_flags> {
    IPV6ONLY = UV_TCP_IPV6ONLY
};

class TCPHandle final: public StreamHandle<TCPHandle, uv_tcp_t> {

public:
    using Time = std::chrono::duration<unsigned int>;
    using OSSocketHandle = uvcls::UVTypeWrapper<uv_os_sock_t>; 

    explicit TCPHandle(std::shared_ptr<Loop> ref, unsigned int f = {});

    bool init();

    void open(OSSocketHandle socket);

    bool noDelay(bool value = false);

    bool keepAlive(bool enable = false, Time time = Time{0});

    bool simultaneousAccepts(bool enable = true);

    void bind(const sockaddr &addr, Flags<UVTCPFlags> opts = Flags<UVTCPFlags>{});

    template<typename I = IPv4>
    void bind(const std::string &ip, unsigned int port, Flags<UVTCPFlags> opts = Flags<UVTCPFlags>{});

    template<typename I = IPv4>
    void bind(Addr addr, Flags<UVTCPFlags> opts = Flags<UVTCPFlags>{});

    template<typename I = IPv4>
    Addr sock() const noexcept;

    template<typename I = IPv4>
    Addr peer() const noexcept;

    void connect(const sockaddr &addr);

    template<typename I = IPv4>
    void connect(const std::string &ip, unsigned int port);

    template<typename I = IPv4>
    void connect(Addr addr);

    void closeReset();

private:
    enum {
        DEFAULT,
        FLAGS
    } tag;

    // flags 配置用途，不是简简单单的 1 个 int
    unsigned int flags;
};

// 显式专用化(模板特化)声明在前。explicit instantiations
template void TCPHandle::bind<IPv4>(const std::string &, unsigned int, Flags<UVTCPFlags>);
template void TCPHandle::bind<IPv6>(const std::string &, unsigned int, Flags<UVTCPFlags>);

template void TCPHandle::bind<IPv4>(Addr, Flags<UVTCPFlags>);
template void TCPHandle::bind<IPv6>(Addr, Flags<UVTCPFlags>);

template Addr TCPHandle::sock<IPv4>() const noexcept;
template Addr TCPHandle::sock<IPv6>() const noexcept;

template Addr TCPHandle::peer<IPv4>() const noexcept;
template Addr TCPHandle::peer<IPv6>() const noexcept;

template void TCPHandle::connect<IPv4>(const std::string &, unsigned int);
template void TCPHandle::connect<IPv6>(const std::string &, unsigned int);

template void TCPHandle::connect<IPv4>(Addr addr);
template void TCPHandle::connect<IPv6>(Addr addr);


// TCPHandle 构造函数。首先调用 StreamHandle{std::move(ref)} 然后 tag 和 flags
UVCLS_INLINE TCPHandle::TCPHandle(std::shared_ptr<Loop> ref, unsigned int f)
    : StreamHandle{std::move(ref)}, tag{f ? FLAGS : DEFAULT}, flags{f} {}

UVCLS_INLINE bool TCPHandle::init() {
    return (tag == FLAGS) ? initialize(&uv_tcp_init_ex, flags) : initialize(&uv_tcp_init);
}

UVCLS_INLINE void TCPHandle::open(TCPHandle::OSSocketHandle socket) {
    invoke(&uv_tcp_open, get(), socket);
}

UVCLS_INLINE bool TCPHandle::noDelay(bool value) {
    return (0 == uv_tcp_nodelay(get(), value));
}

UVCLS_INLINE bool TCPHandle::keepAlive(bool enable, TCPHandle::Time time) {
    return (0 == uv_tcp_keepalive(get(), enable, time.count()));
}

UVCLS_INLINE bool TCPHandle::simultaneousAccepts(bool enable) {
    return (0 == uv_tcp_simultaneous_accepts(get(), enable));
}

UVCLS_INLINE void TCPHandle::bind(const sockaddr &addr, Flags<UVTCPFlags> opts) {
    invoke(&uv_tcp_bind, get(), &addr, opts);
}

template<typename I>
UVCLS_INLINE void TCPHandle::bind(const std::string &ip, unsigned int port, Flags<UVTCPFlags> opts) {
    typename details::IpTraits<I>::Type addr;
    details::IpTraits<I>::addrFunc(ip.data(), port, &addr);
    bind(reinterpret_cast<const sockaddr &>(addr), std::move(opts));
}

template<typename I>
UVCLS_INLINE void TCPHandle::bind(Addr addr, Flags<UVTCPFlags> opts) {
    bind<I>(std::move(addr.ip), addr.port, std::move(opts));
}

template<typename I>
UVCLS_INLINE Addr TCPHandle::sock() const noexcept {
    return details::address<I>(&uv_tcp_getsockname, get());
}

template<typename I>
UVCLS_INLINE Addr TCPHandle::peer() const noexcept {
    return details::address<I>(&uv_tcp_getpeername, get());
}

template<typename I>
UVCLS_INLINE void TCPHandle::connect(const std::string &ip, unsigned int port) {
    typename details::IpTraits<I>::Type addr;
    details::IpTraits<I>::addrFunc(ip.data(), port, &addr);
    connect(reinterpret_cast<const sockaddr &>(addr));
}

template<typename I>
UVCLS_INLINE void TCPHandle::connect(Addr addr) {
    connect<I>(std::move(addr.ip), addr.port);
}

UVCLS_INLINE void TCPHandle::connect(const sockaddr &addr) {
    auto listener = [ptr = shared_from_this()](const auto &event, const auto &) {
        ptr->publish(event);
    };

    auto req = loop().resource<details::ConnectReq>();
    req->once<ErrorEvent>(listener);
    req->once<ConnectEvent>(listener);
    req->connect(&uv_tcp_connect, get(), &addr);
}

UVCLS_INLINE void TCPHandle::closeReset() {
    invoke(&uv_tcp_close_reset, get(), &this->closeCallback);
}

}

#endif
