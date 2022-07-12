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

namespace uvcls {

enum class UVTCPFlags : std::underlying_type_t<uv_tcp_flags> {
    IPV6ONLY = UV_TCP_IPV6ONLY
};

class TCPHandle final: public StreamHandle<TCPHandle, uv_tcp_t> {
public:
    using Time = std::chrono::duration<unsigned int>;
    using Bind = UVTCPFlags;
    using IPv4 = uvcls::IPv4;
    using IPv6 = uvcls::IPv6;

    explicit TCPHandle(std::shared_ptr<Loop> ref, unsigned int f = {});

    bool init();

    void open(OSSocketHandle socket);

    bool noDelay(bool value = false);

    bool keepAlive(bool enable = false, Time time = Time{0});

    bool simultaneousAccepts(bool enable = true);

    void bind(const sockaddr &addr, Flags<Bind> opts = Flags<Bind>{});

    template<typename I = IPv4>
    void bind(const std::string &ip, unsigned int port, Flags<Bind> opts = Flags<Bind>{});

    template<typename I = IPv4>
    void bind(Addr addr, Flags<Bind> opts = Flags<Bind>{});

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

    unsigned int flags;
};

extern template void TCPHandle::bind<IPv4>(const std::string &, unsigned int, Flags<Bind>);
extern template void TCPHandle::bind<IPv6>(const std::string &, unsigned int, Flags<Bind>);

extern template void TCPHandle::bind<IPv4>(Addr, Flags<Bind>);
extern template void TCPHandle::bind<IPv6>(Addr, Flags<Bind>);

extern template Addr TCPHandle::sock<IPv4>() const noexcept;
extern template Addr TCPHandle::sock<IPv6>() const noexcept;

extern template Addr TCPHandle::peer<IPv4>() const noexcept;
extern template Addr TCPHandle::peer<IPv6>() const noexcept;

extern template void TCPHandle::connect<IPv4>(const std::string &, unsigned int);
extern template void TCPHandle::connect<IPv6>(const std::string &, unsigned int);

extern template void TCPHandle::connect<IPv4>(Addr addr);
extern template void TCPHandle::connect<IPv6>(Addr addr);

UVCLS_INLINE TCPHandle::TCPHandle(ConstructorAccess ca, std::shared_ptr<Loop> ref, unsigned int f)
    : StreamHandle{ca, std::move(ref)}, tag{f ? FLAGS : DEFAULT}, flags{f} {}

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

template<typename I>
UVCLS_INLINE void TCPHandle::bind(const std::string &ip, unsigned int port, Flags<Bind> opts) {
    typename details::IpTraits<I>::Type addr;
    details::IpTraits<I>::addrFunc(ip.data(), port, &addr);
    bind(reinterpret_cast<const sockaddr &>(addr), std::move(opts));
}

template<typename I>
UVCLS_INLINE void TCPHandle::bind(Addr addr, Flags<Bind> opts) {
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

// explicit instantiations
template void TCPHandle::bind<IPv4>(const std::string &, unsigned int, Flags<Bind>);
template void TCPHandle::bind<IPv6>(const std::string &, unsigned int, Flags<Bind>);

template void TCPHandle::bind<IPv4>(Addr, Flags<Bind>);
template void TCPHandle::bind<IPv6>(Addr, Flags<Bind>);

template Addr TCPHandle::sock<IPv4>() const noexcept;
template Addr TCPHandle::sock<IPv6>() const noexcept;

template Addr TCPHandle::peer<IPv4>() const noexcept;
template Addr TCPHandle::peer<IPv6>() const noexcept;

template void TCPHandle::connect<IPv4>(const std::string &, unsigned int);
template void TCPHandle::connect<IPv6>(const std::string &, unsigned int);

template void TCPHandle::connect<IPv4>(Addr addr);
template void TCPHandle::connect<IPv6>(Addr addr);

}

#endif
