{
  'includes': [
    'deps/common.gypi',
  ],
  'variables': {
    'gtest%': 0,
    'gtest_static_libs%': [],
    'glfw%': 0,
    'glfw_static_libs%': [],
    'mason_platform': 'osx',
  },
  'targets': [
    { 'target_name': 'geojsonvt',
      'product_name': 'geojsonvt',
      'type': 'static_library',
      'standalone_static_library': 1,

      'include_dirs': [
        'include',
      ],

      'sources': [
        'include/mapbox/geojsonvt.hpp',
        'include/mapbox/geojsonvt/clip.hpp',
        'include/mapbox/geojsonvt/convert.hpp',
        'include/mapbox/geojsonvt/simplify.hpp',
        'include/mapbox/geojsonvt/transform.hpp',
        'include/mapbox/geojsonvt/tile.hpp',
        'include/mapbox/geojsonvt/types.hpp',
        'include/mapbox/geojsonvt/wrap.hpp',
        'src/geojsonvt.cpp',
        'src/clip.cpp',
        'src/convert.cpp',
        'src/simplify.cpp',
        'src/transform.cpp',
        'src/tile.cpp',
        'src/wrap.cpp',
      ],

      'variables': {
        'cflags_cc': [
          '<@(variant_cflags)',
          '<@(rapidjson_cflags)',
        ],
        'ldflags': [],
        'libraries': [],
      },

      'conditions': [
        ['OS == "mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
          },
        }, {
          'cflags_cc': [ '<@(cflags_cc)' ],
        }],
      ],

      'link_settings': {
        'conditions': [
          ['OS == "mac"', {
            'libraries': [ '<@(libraries)' ],
            'xcode_settings': { 'OTHER_LDFLAGS': [ '<@(ldflags)' ] }
          }, {
            'libraries': [ '<@(libraries)', '<@(ldflags)' ],
          }]
        ],
      },

      'direct_dependent_settings': {
        'include_dirs': [
          'include',
        ],
      },
    },

    { 'target_name': 'install',
      'type': 'none',
      'hard_dependency': 1,
      'dependencies': [
        'geojsonvt',
      ],

      'copies': [
        { 'files': [ '<(PRODUCT_DIR)/libgeojsonvt.a' ], 'destination': '<(install_prefix)/lib' },
        { 'files': [ '<!@(find include/mapbox/geojsonvt -name "*.hpp")' ], 'destination': '<(install_prefix)/include/mapbox/geojsonvt' },
        { 'files': [ 'include/mapbox/geojsonvt.hpp' ], 'destination': '<(install_prefix)/include/mapbox' },
      ],
    },
  ],

  'conditions': [
    ['gtest', {
      'targets': [
        { 'target_name': 'test',
          'product_name': 'test',
          'type': 'executable',

          'dependencies': [
            'geojsonvt',
          ],

          'include_dirs': [
            'src',
          ],

          'sources': [
            'test/test.cpp',
            'test/util.hpp',
            'test/util.cpp',
            'test/test_clip.cpp',
            'test/test_full.cpp',
            'test/test_get_tile.cpp',
            'test/test_simplify.cpp',
          ],

          'variables': {
            'cflags_cc': [
              '<@(rapidjson_cflags)',
              '<@(variant_cflags)',
              '<@(gtest_cflags)',
            ],
            'ldflags': [
              '<@(gtest_ldflags)'
            ],
            'libraries': [
              '<@(gtest_static_libs)',
            ],
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
        },
      ],
    }],
    ['glfw', {
      'targets': [
        { 'target_name': 'debug',
          'product_name': 'debug',
          'type': 'executable',

          'dependencies': [
            'geojsonvt',
          ],

          'include_dirs': [
            'src',
          ],

          'sources': [
            'debug/debug.cpp',
          ],

          'variables': {
            'cflags_cc': [
              '<@(variant_cflags)',
              '<@(glfw_cflags)',
            ],
            'ldflags': [
              '<@(glfw_ldflags)'
            ],
            'libraries': [
              '<@(glfw_static_libs)',
            ],
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
        },
      ],
    }],
  ],
}
