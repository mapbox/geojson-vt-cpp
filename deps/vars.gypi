{
  'variables' : {
    'cflags_cc%': [],
    'ldflags%': [],
    'libraries%': [],
  },
  'conditions': [
    ['OS == "mac"', {
      'libraries': [ '<@(libraries)' ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
        'OTHER_LDFLAGS': [ '<@(ldflags)' ],
      }
    }, {
      'cflags_cc': [ '<@(cflags_cc)' ],
      'libraries': [ '<@(libraries)', '<@(ldflags)' ],
    }]
  ],
}