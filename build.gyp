{
    "target_defaults": {
        "include_dirs": ["deps/uv/include", "src/lib"],
        "sources": [
            "src/lib/config.h",
            "src/lib/emitter.hpp",
            "src/lib/loop.hpp",
            "src/lib/handle.hpp",
            "src/lib/idle.hpp",
            "src/lib/request.hpp",
            "src/lib/stream.hpp",
        ],
        "dependencies": ["deps/uv/uv.gyp:libuv"],
    },
    "targets": [
        {
            "target_name": "netserver",
            "type": "executable",
            "cflags": ['-std=c++17'],
            "sources": [
                "src/main.cc",
            ],
        },
        {
            "target_name": "cctest",
            "type": "executable",
            "cflags": ['-std=c++17'],
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
