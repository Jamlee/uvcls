#ifndef UVCLS_TCP_INCLUDE_H
#define UVCLS_TCP_INCLUDE_H

#include "config.h"
#include "stream.hpp"
#include "util.hpp"

namespace uvcls {

enum class UVTCPFlags : std::underlying_type_t<uv_tcp_flags> {
    IPV6ONLY = UV_TCP_IPV6ONLY
};

static constexpr std::size_t DEFAULT_SIZE = 1024;

struct IPv4 {};

struct IPv6 {};

struct Addr {
    std::string ip;    /*!< Either an IPv4 or an IPv6. */
    unsigned int port; /*!< A valid service identifier. */
};

// 对 libuv 用到的类型做封装 uv_handle_type, uv_file, uv_os_fd_t 等
template <typename T>
struct UVTypeWrapper {
    using Type = T;

    constexpr UVTypeWrapper()
        : value{} {}

    constexpr UVTypeWrapper(Type val)
        : value{val} {}

    constexpr operator Type() const noexcept {
        return value;
    }

    bool operator==(UVTypeWrapper other) const noexcept {
        return value == other.value;
    }

   private:
    const Type value;
};

template <typename>
struct IpTraits;

template <>
struct IpTraits<IPv4> {
    using Type = sockaddr_in;
    using AddrFuncType = int (*)(const char *, int, Type *);
    using NameFuncType = int (*)(const Type *, char *, std::size_t);

    inline static const AddrFuncType addrFunc = &uv_ip4_addr;
    inline static const NameFuncType nameFunc = &uv_ip4_name;

    static constexpr auto sinPort(const Type *addr) {
        return addr->sin_port;
    }
};

template <>
struct IpTraits<IPv6> {
    using Type = sockaddr_in6;
    using AddrFuncType = int (*)(const char *, int, Type *);
    using NameFuncType = int (*)(const Type *, char *, std::size_t);

    inline static const AddrFuncType addrFunc = &uv_ip6_addr;
    inline static const NameFuncType nameFunc = &uv_ip6_name;

    static constexpr auto sinPort(const Type *addr) {
        return addr->sin6_port;
    }
};

template <typename I>
Addr address(const typename IpTraits<I>::Type *aptr) noexcept {
    Addr addr{};
    char name[DEFAULT_SIZE];

    int err = IpTraits<I>::nameFunc(aptr, name, DEFAULT_SIZE);

    if (0 == err) {
        addr.port = ntohs(IpTraits<I>::sinPort(aptr));
        addr.ip = std::string{name};
    }

    return addr;
}

template <typename I, typename F, typename H>
Addr address(F &&f, const H *handle) noexcept {
    sockaddr_storage ssto;
    int len = sizeof(ssto);
    Addr addr{};

    int err = std::forward<F>(f)(handle, reinterpret_cast<sockaddr *>(&ssto), &len);

    if (0 == err) {
        typename IpTraits<I>::Type *aptr = reinterpret_cast<typename IpTraits<I>::Type *>(&ssto);
        addr = address<I>(aptr);
    }

    return addr;
}

// 类型包装，可以获取到其内部的值
using OSSocketHandle = UVTypeWrapper<uv_os_sock_t>;

class TCPHandle final : public StreamHandle<TCPHandle, uv_tcp_t> {
   public:
    using Time = std::chrono::duration<unsigned int>;
    using Bind = UVTCPFlags;

    explicit TCPHandle(std::shared_ptr<Loop> ref, unsigned int f = {});

    bool init();

    void open(OSSocketHandle socket);

    bool noDelay(bool value = false);

    bool keepAlive(bool enable = false, Time time = Time{0});

    bool simultaneousAccepts(bool enable = true);

    void bind(const sockaddr &addr, Flags<Bind> opts = Flags<Bind>{});

    template <typename I = IPv4>
    void bind(const std::string &ip, unsigned int port, Flags<Bind> opts = Flags<Bind>{});

    template <typename I = IPv4>
    void bind(Addr addr, Flags<Bind> opts = Flags<Bind>{});

    template <typename I = IPv4>
    Addr sock() const noexcept;

    template <typename I = IPv4>
    Addr peer() const noexcept;

    void connect(const sockaddr &addr);

    template <typename I = IPv4>
    void connect(const std::string &ip, unsigned int port);

    template <typename I = IPv4>
    void connect(Addr addr);

    void closeReset();

   private:
    enum {
        DEFAULT,
        FLAGS
    } tag;  // 判断是否使用 flag。FLAGS 注意不要和 flag 搞混淆了

    // flags 配置用途，不是简简单单的 1 个 int
    unsigned int flags;
};

// TCPHandle 构造函数。首先调用 StreamHandle{std::move(ref)} 然后 tag 和 flags
UVCLS_INLINE TCPHandle::TCPHandle(std::shared_ptr<Loop> ref, unsigned int f)
    : StreamHandle{std::move(ref)}, tag{f ? FLAGS : DEFAULT}, flags{f} {}

UVCLS_INLINE bool TCPHandle::init() {
    return (tag == FLAGS) ? initialize(&uv_tcp_init_ex, flags) : initialize(&uv_tcp_init);
}

UVCLS_INLINE void TCPHandle::open(OSSocketHandle socket) {
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

UVCLS_INLINE void TCPHandle::bind(const sockaddr &addr, Flags<Bind> opts) {
    invoke(&uv_tcp_bind, get(), &addr, opts);
}

template <typename I>
UVCLS_INLINE void TCPHandle::bind(const std::string &ip, unsigned int port, Flags<Bind> opts) {
    typename IpTraits<I>::Type addr;
    IpTraits<I>::addrFunc(ip.data(), port, &addr);
    bind(reinterpret_cast<const sockaddr &>(addr), std::move(opts));
}

template <typename I>
UVCLS_INLINE void TCPHandle::bind(Addr addr, Flags<Bind> opts) {
    bind<I>(std::move(addr.ip), addr.port, std::move(opts));
}

template <typename I>
UVCLS_INLINE Addr TCPHandle::sock() const noexcept {
    return address<I>(&uv_tcp_getsockname, get());
}

template <typename I>
UVCLS_INLINE Addr TCPHandle::peer() const noexcept {
    return address<I>(&uv_tcp_getpeername, get());
}

template <typename I>
UVCLS_INLINE void TCPHandle::connect(const std::string &ip, unsigned int port) {
    typename IpTraits<I>::Type addr;
    IpTraits<I>::addrFunc(ip.data(), port, &addr);
    connect(reinterpret_cast<const sockaddr &>(addr));
}

template <typename I>
UVCLS_INLINE void TCPHandle::connect(Addr addr) {
    connect<I>(std::move(addr.ip), addr.port);
}

UVCLS_INLINE void TCPHandle::connect(const sockaddr &addr) {
    auto listener = [ptr = shared_from_this()](const auto &event, const auto &) {
        ptr->publish(event);
    };
    auto req = std::make_shared<uvcls::ConnectReq>(this->loop().shared_from_this());
    req->once<ErrorEvent>(listener);
    req->once<ConnectEvent>(listener);
    req->connect(&uv_tcp_connect, get(), &addr);
}

UVCLS_INLINE void TCPHandle::closeReset() {
    invoke(&uv_tcp_close_reset, get(), &this->closeCallback);
}

}  // namespace uvcls

#endif