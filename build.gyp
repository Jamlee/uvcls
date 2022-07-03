{
  'targets': [{
    'target_name': 'netserver',
    'type': 'executable',
    'include_dirs': [
      'deps/uv/include',
      'src'
    ],
    'dependencies': [ 'deps/uv/uv.gyp:libuv' ],
    'sources': [
      'src/main.cc',
      'src/loop.cc',
      'src/loop.h',
    ],
  }]
}
