{
    "target_defaults": {
        "include_dirs": ["deps/uv/include", "src/lib"],
        "sources": [
            "src/lib/emitter.hpp",
            "src/lib/tyoe_info.hpp",
            "src/lib/loop.cc",
            "src/lib/loop.h",
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
                "test/type_info.cc",
                "test/emitter.cc",
            ],
        },
    ],
}
