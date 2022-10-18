project('limine-terminal-port', ['c', 'cpp'],
    version : '1.0',
    license : 'BSD-2-Clause',
    default_options : [
        'c_std=gnu17',
        'cpp_std=c++17'
    ]
)

terminal_inc = [
    include_directories('source'),
    include_directories('source/cpp')
]
terminal_dep = declare_dependency(
    include_directories : terminal_inc,
    sources : [
        'source/term.c',
        'source/gterm.c',
        'source/tterm.c',
        'source/image.c'
    ]
)
meson.override_dependency('limine-terminal-port', terminal_dep)