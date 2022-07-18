{
    "target_defaults": {
        "include_dirs": ["deps/uv/include", "src/lib"],
        "sources": [
            "src/lib/config.h",
            "src/lib/emitter.hpp",
            "src/lib/loop.hpp",
            "src/lib/handle.hpp",
            "src/lib/idle.hpp",
            "src/lib/stream.hpp",
            "src/lib/tcp.hpp",
            "src/lib/util.hpp",
        ],
        "dependencies": ["deps/uv/uv.gyp:libuv"],
    },
    "targets": [
        {
            "target_name": "netserver",
            "type": "executable",
            "cflags": ['-std=c++17', '-g', '-O0'],
            "sources": [
                "src/main.cc",
            ],
        },
        {
            "target_name": "cctest",
            "type": "executable",
            "cflags": ['-std=c++17', '-g', '-O0'],
            "include_dirs": [
                "test/googletest/include",
                "test/googletest",
            ],
            "sources": [
                "test/googletest/src/gtest_main.cc",
                "test/googletest/src/gtest-all.cc",
                "test/emitter.cc",
                "test/loop.cc",
                "test/handle.cc",
            ],
        },
    ],
}
