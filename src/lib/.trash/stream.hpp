#ifndef UVCLS_STREAM_INCLUDE_H
#define UVCLS_STREAM_INCLUDE_H
#include <uv.h>
#include "handle.hpp"
#include "request.hpp"

/*
Stream 统一封装的流操作接口。不可以理解成是 TCP 继承了 Stream。而是 Stream 通过统一的操作，根据传入
的参数进行分发。tcp 和 name piped，udp 都可以用流接口操作。

1. Listen 的回调函数是 on_new_connection。在 on_new_connection 中创建新结构体 uv_tcp_t。用 uv_accept
初始化这个 uv_tcp_t 继续在 loop 中运行。
2. 
*/

namespace uvw {

struct ConnectEvent {};
struct EndEvent {};
struct ListenEvent {};
struct ShutdownEvent {};
struct WriteEvent {};
struct DataEvent {
    explicit DataEvent(std::unique_ptr<char[]> buf, std::size_t len) noexcept;

    std::unique_ptr<char[]> data; /*!< A bunch of data read on the stream. */
    std::size_t length;           /*!< The amount of data read on the stream. */
};


struct ConnectReq final: public Request<ConnectReq, uv_connect_t> {
    using Request::Request;

    template<typename F, typename... Args>
    void connect(F &&f, Args &&...args) {
        invoke(std::forward<F>(f), get(), std::forward<Args>(args)..., &defaultCallback<ConnectEvent>);
    }
};

struct ShutdownReq final: public Request<ShutdownReq, uv_shutdown_t> {
    using Request::Request;

    void shutdown(uv_stream_t *handle);
};

template<typename Deleter>
class WriteReq final: public Request<WriteReq<Deleter>, uv_write_t> {
    using ConstructorAccess = typename Request<WriteReq<Deleter>, uv_write_t>::ConstructorAccess;

public:
    WriteReq(ConstructorAccess ca, std::shared_ptr<Loop> loop, std::unique_ptr<char[], Deleter> dt, unsigned int len)
        : Request<WriteReq<Deleter>, uv_write_t>{ca, std::move(loop)},
          data{std::move(dt)},
          buf{uv_buf_init(data.get(), len)} {}

    void write(uv_stream_t *handle) {
        this->invoke(&uv_write, this->get(), handle, &buf, 1, &this->template defaultCallback<WriteEvent>);
    }

    void write(uv_stream_t *handle, uv_stream_t *send) {
        this->invoke(&uv_write2, this->get(), handle, &buf, 1, send, &this->template defaultCallback<WriteEvent>);
    }

private:
    std::unique_ptr<char[], Deleter> data;
    uv_buf_t buf;
};

template<typename T, typename U>
class StreamHandle: public Handle<T, U> {

    static constexpr unsigned int DEFAULT_BACKLOG = 1024;

    // 数据读取回调。供 uv_read_start 使用
    static void readCallback(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf) {
        T &ref = *(static_cast<T *>(handle->data));
        // data will be destroyed no matter of what the value of nread is
        std::unique_ptr<char[]> data{buf->base};

        // nread == 0 is ignored (see http://docs.libuv.org/en/v1.x/stream.html)
        // equivalent to EAGAIN/EWOULDBLOCK, it shouldn't be treated as an error
        // for we don't have data to emit though, it's fine to suppress it

        if(nread == UV_EOF) {
            // end of stream
            ref.publish(EndEvent{});
        } else if(nread > 0) {
            // data available
            ref.publish(DataEvent{std::move(data), static_cast<std::size_t>(nread)});
        } else if(nread < 0) {
            // transmission error
            ref.publish(ErrorEvent(nread));
        }
    }

    // fd 监听成功回调。供 uv_listen 使用
    static void listenCallback(uv_stream_t *handle, int status) {
        if(T &ref = *(static_cast<T *>(handle->data)); status) {
            ref.publish(ErrorEvent{status});
        } else {
            ref.publish(ListenEvent{});
        }
    }

public:
    using Handle<T, U>::Handle;

    void shutdown() {
        auto listener = [ptr = this->shared_from_this()](const auto &event, const auto &) {
            ptr->publish(event);
        };
        auto shutdown = this->loop().template resource<details::ShutdownReq>();
        shutdown->template once<ErrorEvent>(listener);
        shutdown->template once<ShutdownEvent>(listener);
        shutdown->shutdown(this->template get<uv_stream_t>());
    }

    void listen(int backlog = DEFAULT_BACKLOG) {
        this->invoke(&uv_listen, this->template get<uv_stream_t>(), backlog, &listenCallback);
    }

    template<typename S>
    void accept(S &ref) {
        this->invoke(&uv_accept, this->template get<uv_stream_t>(), this->template get<uv_stream_t>(ref));
    }

    void read() {
        this->invoke(&uv_read_start, this->template get<uv_stream_t>(), &this->allocCallback, &readCallback);
    }

    template<typename Deleter>
    void write(std::unique_ptr<char[], Deleter> data, unsigned int len) {
        auto req = this->loop().template resource<details::WriteReq<Deleter>>(std::move(data), len);
        auto listener = [ptr = this->shared_from_this()](const auto &event, const auto &) {
            ptr->publish(event);
        };

        req->template once<ErrorEvent>(listener);
        req->template once<WriteEvent>(listener);
        req->write(this->template get<uv_stream_t>());
    }

    void write(char *data, unsigned int len) {
        auto req = this->loop().template resource<details::WriteReq<void (*)(char *)>>(std::unique_ptr<char[], void (*)(char *)>{data, [](char *) {}}, len);
        auto listener = [ptr = this->shared_from_this()](const auto &event, const auto &) {
            ptr->publish(event);
        };

        req->template once<ErrorEvent>(listener);
        req->template once<WriteEvent>(listener);
        req->write(this->template get<uv_stream_t>());
    }

    template<typename S, typename Deleter>
    void write(S &send, std::unique_ptr<char[], Deleter> data, unsigned int len) {
        auto req = this->loop().template resource<details::WriteReq<Deleter>>(std::move(data), len);
        auto listener = [ptr = this->shared_from_this()](const auto &event, const auto &) {
            ptr->publish(event);
        };

        req->template once<ErrorEvent>(listener);
        req->template once<WriteEvent>(listener);
        req->write(this->template get<uv_stream_t>(), this->template get<uv_stream_t>(send));
    }

    template<typename S>
    void write(S &send, char *data, unsigned int len) {
        auto req = this->loop().template resource<details::WriteReq<void (*)(char *)>>(std::unique_ptr<char[], void (*)(char *)>{data, [](char *) {}}, len);
        auto listener = [ptr = this->shared_from_this()](const auto &event, const auto &) {
            ptr->publish(event);
        };

        req->template once<ErrorEvent>(listener);
        req->template once<WriteEvent>(listener);
        req->write(this->template get<uv_stream_t>(), this->template get<uv_stream_t>(send));
    }

    int tryWrite(std::unique_ptr<char[]> data, unsigned int len) {
        uv_buf_t bufs[] = {uv_buf_init(data.get(), len)};
        auto bw = uv_try_write(this->template get<uv_stream_t>(), bufs, 1);

        if(bw < 0) {
            this->publish(ErrorEvent{bw});
            bw = 0;
        }

        return bw;
    }

    template<typename V, typename W>
    int tryWrite(std::unique_ptr<char[]> data, unsigned int len, StreamHandle<V, W> &send) {
        uv_buf_t bufs[] = {uv_buf_init(data.get(), len)};
        auto bw = uv_try_write2(this->template get<uv_stream_t>(), bufs, 1, send.raw());

        if(bw < 0) {
            this->publish(ErrorEvent{bw});
            bw = 0;
        }

        return bw;
    }

    int tryWrite(char *data, unsigned int len) {
        uv_buf_t bufs[] = {uv_buf_init(data, len)};
        auto bw = uv_try_write(this->template get<uv_stream_t>(), bufs, 1);

        if(bw < 0) {
            this->publish(ErrorEvent{bw});
            bw = 0;
        }

        return bw;
    }

    template<typename V, typename W>
    int tryWrite(char *data, unsigned int len, StreamHandle<V, W> &send) {
        uv_buf_t bufs[] = {uv_buf_init(data, len)};
        auto bw = uv_try_write2(this->template get<uv_stream_t>(), bufs, 1, send.raw());

        if(bw < 0) {
            this->publish(ErrorEvent{bw});
            bw = 0;
        }

        return bw;
    }

    bool readable() const noexcept {
        return (uv_is_readable(this->template get<uv_stream_t>()) == 1);
    }

    bool writable() const noexcept {
        return (uv_is_writable(this->template get<uv_stream_t>()) == 1);
    }

    bool blocking(bool enable = false) {
        return (0 == uv_stream_set_blocking(this->template get<uv_stream_t>(), enable));
    }

    size_t writeQueueSize() const noexcept {
        return uv_stream_get_write_queue_size(this->template get<uv_stream_t>());
    }
}

UVCLS_INLINE DataEvent::DataEvent(std::unique_ptr<char[]> buf, std::size_t len) noexcept
    : data{std::move(buf)}, length{len} {}

UVCLS_INLINE void details::ShutdownReq::shutdown(uv_stream_t *handle) {
    invoke(&uv_shutdown, get(), handle, &defaultCallback<ShutdownEvent>);
}

}

#endif