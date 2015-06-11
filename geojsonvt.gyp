{
  'includes': [
    'deps/common.gypi',
  ],
  'targets': [
    { 'target_name': 'geojsonvt',
      'product_name': 'geojsonvt',
      'type': 'static_library',

      'include_dirs': [
        'include',
      ],

      'sources': [
        '<!@(find include src -name "*.cpp" -o -name "*.c" -o -name "*.hpp" -o -name "*.h")',
      ],

      'variables': {
        'cflags_cc': [
          '<@(variant_cflags)',
          '<@(rapidjson_cflags)',
        ],
        'ldflags': [],
        'libraries': [],
      },

      'includes': [
        'deps/vars.gypi',
      ],
    },
  ],
}
