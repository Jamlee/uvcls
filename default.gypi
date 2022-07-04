{
    'variables': {
        'uv_library': 'static_library'
    },
    'target_defaults': {
        'default_configuration': 'Debug',
        'configurations': {
            'Debug': {
                'cflags': ['-g', '-O0'],
            },
            'Release': {
                'cflags': ['-O3'],
            },
        },
    },
}
