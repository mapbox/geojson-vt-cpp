{
  'includes': [
    'deps/common.gypi',
    'deps/vars.gypi',
  ],
  'targets': [
    { 'target_name': 'geojsonvt',
      'product_name': 'geojsonvt',
      'type': 'static_library',

      'include_dirs': [
        'include',
      ],

      'sources': [
        'include/mapbox/geojsonvt/geojsonvt.hpp',
        'include/mapbox/geojsonvt/geojsonvt_tile.hpp',
        'include/mapbox/geojsonvt/geojsonvt_types.hpp',
        'src/geojsonvt.cpp',
        'src/geojsonvt_clip.cpp',
        'src/geojsonvt_clip.hpp',
        'src/geojsonvt_convert.cpp',
        'src/geojsonvt_convert.hpp',
        'src/geojsonvt_simplify.cpp',
        'src/geojsonvt_simplify.hpp',
        'src/geojsonvt_tile.cpp',
        'src/geojsonvt_util.hpp',
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
        }]
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
        'test/test_simplify.cpp',
      ],

      'variables': {
        'cflags_cc': [
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
}
