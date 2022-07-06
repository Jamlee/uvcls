#include <cstdlib>
#include <memory>
#include <gtest/gtest.h>
#include <util.hpp>

template<typename T>
struct tag { using type = T; };

TEST(Util, UnscopedFlags) {
    enum class UnscopedEnum {
        FOO = 1,
        BAR = 2,
        BAZ = 4,
        QUUX = 8
    };

    uvcls::Flags<UnscopedEnum> flags{};

    ASSERT_NO_THROW((flags = uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::FOO, UnscopedEnum::BAR>()));
    ASSERT_NO_THROW((flags = uvcls::Flags<UnscopedEnum>{UnscopedEnum::BAZ}));
    ASSERT_NO_THROW((flags = uvcls::Flags<UnscopedEnum>{static_cast<uvcls::Flags<UnscopedEnum>::Type>(UnscopedEnum::QUUX)}));

    ASSERT_NO_THROW((flags = uvcls::Flags<UnscopedEnum>{std::move(flags)}));
    ASSERT_NO_THROW((flags = uvcls::Flags<UnscopedEnum>{flags}));

    flags = uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::FOO, UnscopedEnum::QUUX>();

    ASSERT_TRUE(static_cast<bool>(flags));
    ASSERT_EQ(static_cast<uvcls::Flags<UnscopedEnum>::Type>(flags), 9);

    ASSERT_TRUE(flags & uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::FOO>());
    ASSERT_FALSE(flags & UnscopedEnum::BAR);
    ASSERT_FALSE(flags & uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::BAZ>());
    ASSERT_TRUE(flags & UnscopedEnum::QUUX);

    ASSERT_NO_THROW(flags = flags | UnscopedEnum::BAR);
    ASSERT_NO_THROW(flags = flags | uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::BAZ>());

    ASSERT_TRUE(flags & UnscopedEnum::FOO);
    ASSERT_TRUE(flags & uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::BAR>());
    ASSERT_TRUE(flags & UnscopedEnum::BAZ);
    ASSERT_TRUE(flags & uvcls::Flags<UnscopedEnum>::from<UnscopedEnum::QUUX>());
}

TEST(Util, ScopedFlags) {
    enum class ScopedEnum {
        FOO = 1,
        BAR = 2,
        BAZ = 4,
        QUUX = 8
    };

    uvcls::Flags<ScopedEnum> flags{};

    ASSERT_NO_THROW((flags = uvcls::Flags<ScopedEnum>::from<ScopedEnum::FOO, ScopedEnum::BAR>()));
    ASSERT_NO_THROW((flags = uvcls::Flags<ScopedEnum>{ScopedEnum::BAZ}));
    ASSERT_NO_THROW((flags = uvcls::Flags<ScopedEnum>{static_cast<uvcls::Flags<ScopedEnum>::Type>(ScopedEnum::QUUX)}));

    ASSERT_NO_THROW((flags = uvcls::Flags<ScopedEnum>{std::move(flags)}));
    ASSERT_NO_THROW((flags = uvcls::Flags<ScopedEnum>{flags}));

    flags = uvcls::Flags<ScopedEnum>::from<ScopedEnum::FOO, ScopedEnum::QUUX>();

    ASSERT_TRUE(static_cast<bool>(flags));
    ASSERT_EQ(static_cast<uvcls::Flags<ScopedEnum>::Type>(flags), 9);

    ASSERT_TRUE(flags & uvcls::Flags<ScopedEnum>::from<ScopedEnum::FOO>());
    ASSERT_FALSE(flags & ScopedEnum::BAR);
    ASSERT_FALSE(flags & uvcls::Flags<ScopedEnum>::from<ScopedEnum::BAZ>());
    ASSERT_TRUE(flags & ScopedEnum::QUUX);

    ASSERT_NO_THROW(flags = flags | ScopedEnum::BAR);
    ASSERT_NO_THROW(flags = flags | uvcls::Flags<ScopedEnum>::from<ScopedEnum::BAZ>());

    ASSERT_TRUE(flags & ScopedEnum::FOO);
    ASSERT_TRUE(flags & uvcls::Flags<ScopedEnum>::from<ScopedEnum::BAR>());
    ASSERT_TRUE(flags & ScopedEnum::BAZ);
    ASSERT_TRUE(flags & uvcls::Flags<ScopedEnum>::from<ScopedEnum::QUUX>());
}

TEST(Util, Utilities) {
    ASSERT_EQ(uvcls::PidType{}, uvcls::PidType{});

    ASSERT_NE(uvcls::Utilities::OS::pid(), uvcls::PidType{});
    ASSERT_NE(uvcls::Utilities::OS::parent(), uvcls::PidType{});
    ASSERT_FALSE(uvcls::Utilities::OS::homedir().empty());
    ASSERT_FALSE(uvcls::Utilities::OS::tmpdir().empty());
    ASSERT_NE(uvcls::Utilities::OS::hostname(), "");

    ASSERT_TRUE(uvcls::Utilities::OS::env("UVW_TEST_UTIL_UTILITIES", "TRUE"));
    ASSERT_TRUE(uvcls::Utilities::OS::env("UVW_TEST_UTIL_UTILITIES") == "TRUE");
    ASSERT_TRUE(uvcls::Utilities::OS::env("UVW_TEST_UTIL_UTILITIES", ""));
    ASSERT_FALSE(uvcls::Utilities::OS::env("UVW_TEST_UTIL_UTILITIES") == "TRUE");

    auto passwd = uvcls::Utilities::OS::passwd();

    ASSERT_TRUE(static_cast<bool>(passwd));
    ASSERT_FALSE(passwd.username().empty());
    ASSERT_FALSE(passwd.homedir().empty());
    ASSERT_NO_THROW(passwd.uid());
    ASSERT_NO_THROW(passwd.gid());

#ifndef _MSC_VER
    // libuv returns a null string for the shell on Windows
    ASSERT_FALSE(passwd.shell().empty());
#endif

    ASSERT_EQ(uvcls::Utilities::guessHandle(uvcls::FileHandle{-1}), uvcls::HandleType::UNKNOWN);
    ASSERT_NE(uvcls::Utilities::guessHandle(uvcls::StdIN), uvcls::HandleType::UNKNOWN);

    // auto guessHandle = [](auto tag, auto type) {
    //     auto loop = uvcls::Loop::getDefault();
    //     auto handle = loop->resource<typename decltype(tag)::type>();
    //     ASSERT_EQ(uvcls::Utilities::guessHandle(handle->category()), type);
    //     handle->close();
    //     loop->run();
    // };

    // guessHandle(tag<uvcls::AsyncHandle>{}, uvcls::HandleType::ASYNC);
    // guessHandle(tag<uvcls::CheckHandle>{}, uvcls::HandleType::CHECK);
    // guessHandle(tag<uvcls::FsEventHandle>{}, uvcls::HandleType::FS_EVENT);
    // guessHandle(tag<uvcls::FsPollHandle>{}, uvcls::HandleType::FS_POLL);
    // guessHandle(tag<uvcls::IdleHandle>{}, uvcls::HandleType::IDLE);
    // guessHandle(tag<uvcls::PipeHandle>{}, uvcls::HandleType::PIPE);
    // guessHandle(tag<uvcls::PrepareHandle>{}, uvcls::HandleType::PREPARE);
    // guessHandle(tag<uvcls::TCPHandle>{}, uvcls::HandleType::TCP);
    // guessHandle(tag<uvcls::TimerHandle>{}, uvcls::HandleType::TIMER);
    // guessHandle(tag<uvcls::UDPHandle>{}, uvcls::HandleType::UDP);
    // guessHandle(tag<uvcls::SignalHandle>{}, uvcls::HandleType::SIGNAL);

    auto cpuInfo = uvcls::Utilities::cpuInfo();

    ASSERT_NE(cpuInfo.size(), decltype(cpuInfo.size()){0});
    ASSERT_FALSE(cpuInfo[0].model.empty());
    ASSERT_NE(cpuInfo[0].speed, decltype(cpuInfo[0].speed){0});

    auto interfaceAddresses = uvcls::Utilities::interfaceAddresses();

    ASSERT_NE(interfaceAddresses.size(), decltype(interfaceAddresses.size()){0});
    ASSERT_FALSE(interfaceAddresses[0].name.empty());
    ASSERT_FALSE(interfaceAddresses[0].address.ip.empty());
    ASSERT_FALSE(interfaceAddresses[0].netmask.ip.empty());

    ASSERT_NO_THROW(uvcls::Utilities::indexToName(0));
    ASSERT_NO_THROW(uvcls::Utilities::indexToIid(0));

    ASSERT_TRUE(uvcls::Utilities::replaceAllocator(
        [](size_t size) { return malloc(size); },
        [](void *ptr, size_t size) { return realloc(ptr, size); },
        [](size_t num, size_t size) { return calloc(num, size); },
        [](void *ptr) { return free(ptr); }));

    ASSERT_NO_THROW(uvcls::Utilities::loadAverage());
    ASSERT_NE(uvcls::Utilities::totalMemory(), decltype(uvcls::Utilities::totalMemory()){0});
    ASSERT_NE(uvcls::Utilities::uptime(), decltype(uvcls::Utilities::uptime()){0});
    ASSERT_NO_THROW(uvcls::Utilities::rusage());
    ASSERT_NE(uvcls::Utilities::hrtime(), decltype(uvcls::Utilities::hrtime()){0});
    ASSERT_FALSE(uvcls::Utilities::path().empty());
    ASSERT_FALSE(uvcls::Utilities::cwd().empty());
    ASSERT_TRUE(uvcls::Utilities::chdir(uvcls::Utilities::cwd()));

    std::unique_ptr<char[], void (*)(void *)> fake{new char[1], [](void *ptr) { delete[] static_cast<char *>(ptr); }};
    char *argv = fake.get();
    argv[0] = '\0';

    ASSERT_NE(uvcls::Utilities::setupArgs(1, &argv), nullptr);
    ASSERT_NE(uvcls::Utilities::processTitle(), std::string{});
    ASSERT_TRUE(uvcls::Utilities::processTitle(uvcls::Utilities::processTitle()));

    // ASSERT_NE(uvcls::Utilities::availableParallelism(), 0u);
}
