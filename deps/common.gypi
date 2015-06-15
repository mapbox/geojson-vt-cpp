{
  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'cflags_cc': [ '-g', '-O0', '-std=c++1y', '-fno-omit-frame-pointer','-fwrapv', '-fstack-protector-all', '-fno-common' ],
        'defines': [ 'DEBUG' ],
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '0',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES',
          'DEAD_CODE_STRIPPING': 'NO',
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'NO',
          'OTHER_CPLUSPLUSFLAGS': [ '-std=c++1y', '-fno-omit-frame-pointer','-fwrapv', '-fstack-protector-all', '-fno-common'],
        }
      },
      'Release': {
        'cflags_cc': [ '-g', '-O3', '-std=c++1y' ],
        'defines': [ 'NDEBUG' ],
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '3',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES',
          'DEAD_CODE_STRIPPING': 'NO',
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'NO',
          'OTHER_CPLUSPLUSFLAGS': [ '-std=c++1y' ],
        }
      },
    },
  },
}
