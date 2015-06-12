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
  ],
}
