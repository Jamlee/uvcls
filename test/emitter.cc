#include <type_traits>
#include "gtest/gtest.h"
#include "emitter.hpp"
#include <iostream>

struct FakeEvent {};

// 继承 1 个事件发射器。发送 FakeEvent 事件
struct TestEmitter: uvcls::Emitter<TestEmitter> {
    void emit() {
        publish(FakeEvent{});
    }
};

// ErrorEvent 事件用于封装 uv 的 error 事件
TEST(ErrorEvent, Functionalities) {
    auto ecode = static_cast<std::underlying_type_t<uv_errno_t>>(UV_EADDRINUSE);

    uvcls::ErrorEvent event{ecode};

    ASSERT_EQ(ecode, uvcls::ErrorEvent::translate(ecode));
    ASSERT_NE(event.what(), nullptr);
    ASSERT_NE(event.name(), nullptr);
    ASSERT_EQ(event.code(), ecode);

    ASSERT_FALSE(static_cast<bool>(uvcls::ErrorEvent{0}));
    ASSERT_TRUE(static_cast<bool>(uvcls::ErrorEvent{ecode}));

    // 错误信息的字符表示
    // std::cout << event.name() << std::endl;
    ASSERT_STREQ(event.name(), "EADDRINUSE");
}

TEST(Emitter, EmptyAndClear) {
    TestEmitter emitter{};

    ASSERT_TRUE(emitter.empty());

    // 监听 ErrorEvent。handlers 根据类型 id 存储了多个 Handler 对象。
    // 1 个事件类型 1 个 Handler 对象。例如  ErrorEvent 对应 1 个 Handler<ErrorEvent> 对象
    emitter.on<uvcls::ErrorEvent>([](const auto &, auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<uvcls::ErrorEvent>());
    ASSERT_TRUE(emitter.empty<FakeEvent>());

    emitter.clear<FakeEvent>();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<uvcls::ErrorEvent>());
    ASSERT_TRUE(emitter.empty<FakeEvent>());

    emitter.clear<uvcls::ErrorEvent>();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<uvcls::ErrorEvent>());
    ASSERT_TRUE(emitter.empty<FakeEvent>());

    emitter.on<uvcls::ErrorEvent>([](const auto &, auto &) {});
    emitter.on<FakeEvent>([](const auto &, auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<uvcls::ErrorEvent>());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    emitter.clear();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<uvcls::ErrorEvent>());
    ASSERT_TRUE(emitter.empty<FakeEvent>());
}

TEST(Emitter, On) {
    TestEmitter emitter{};

    // 传入空的函数（lambda表达式）
    emitter.on<FakeEvent>([](const auto &, auto &) {
        // std::cout << "hello fake event" << std::endl;
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    // TestEmitter 是继承的 emitter 这个 emitter 暂时只能发送 1 种事件  FakeEvent
    emitter.emit();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());
}

TEST(Emitter, Once) {
    TestEmitter emitter{};

    emitter.once<FakeEvent>([](const auto &, auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    emitter.emit();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FakeEvent>());
}

TEST(Emitter, OnceAndErase) {
    TestEmitter emitter{};

    auto conn = emitter.once<FakeEvent>([](const auto &, auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    emitter.erase(conn);

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FakeEvent>());
}

TEST(Emitter, OnAndErase) {
    TestEmitter emitter{};

    auto conn = emitter.on<FakeEvent>([](const auto &, auto &) {});

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    emitter.erase(conn);

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FakeEvent>());
}

TEST(Emitter, CallbackClear) {
    TestEmitter emitter{};

    emitter.on<FakeEvent>([](const auto &, auto &ref) {
        ref.template on<FakeEvent>([](const auto &, auto &) {});
        ref.clear();
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    emitter.emit();

    ASSERT_TRUE(emitter.empty());
    ASSERT_TRUE(emitter.empty<FakeEvent>());

    emitter.on<FakeEvent>([](const auto &, auto &ref) {
        ref.clear();
        ref.template on<FakeEvent>([](const auto &, auto &) {});
    });

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());

    emitter.emit();

    ASSERT_FALSE(emitter.empty());
    ASSERT_FALSE(emitter.empty<FakeEvent>());
}

class DataEvent {
public:
    void SayData() const {
        std::cout << "hello data event" << std::endl;
    }
};

class CloseEvent {
public:
    void SayClose() const {
        std::cout << "hello close event" << std::endl;
    }
};

struct TestMutilEventEmitter: uvcls::Emitter<TestMutilEventEmitter> {
    void emitData() {
        publish(DataEvent{});
    }

    void emitClose() {
        publish(CloseEvent{});
    }
};

// 测试一个 emitter 触发
TEST(Emitter, OnMutipleTypeEvent) {
    TestMutilEventEmitter emitter{};
    auto conn1 = emitter.on<DataEvent>([](const auto & event, auto & emitter) {
        event.SayData();
    });
    auto conn2 = emitter.on<CloseEvent>([](const auto & event, auto & emitter) {
        event.SayClose();
    });
    emitter.emitClose();
    emitter.emitData();
}

class HelloWorld {};

TEST(type_info, type) {
  auto id = uvcls::type<HelloWorld>();
  // std::cout << id << std::endl;
  // std::cout << __PRETTY_FUNCTION__ << std::endl;
  EXPECT_EQ(id, 3269991986);
}