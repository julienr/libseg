{
  'variables': {
    'pkg-config' : 'pkg-config',
  },

  'target_defaults': {
    'cflags':[
      '-Wall', '-Wextra', '-Wno-unused-parameter', '-Werror', '-std=c++11',
      #'-O2',
      '-ggdb'
    ],
    'libraries':[
      '-L<(ROOTDIR)/third_party/_install/lib',
      '-lglog',
    ],
    'include_dirs':[
      '<(ROOTDIR)/third_party/_install/include/'
    ]
  },

  'targets': [
    {
      # See gmock README
      'target_name' : 'gtest_mock',
      'type': 'static_library',
      'cflags': [
        '-Wno-missing-field-initializers',
        '-isystem <(ROOTDIR)/third_party/gmock-1.7.0/include',
        '-isystem <(ROOTDIR)/third_party/gmock-1.7.0/gtest/include',
      ],
      'sources':[
        'third_party/gmock-1.7.0/src/gmock-all.cc',
        'third_party/gmock-1.7.0/src/gmock_main.cc',
        'third_party/gmock-1.7.0/gtest/src/gtest-all.cc',
      ],
      'include_dirs': [
        'third_party/gmock-1.7.0/',
        'third_party/gmock-1.7.0/gtest/',
      ],
      'libraries': [
        '-pthread',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'third_party/gmock-1.7.0/include',
          'third_party/gmock-1.7.0/gtest/include',
        ],
        'libraries': [
          '-lpthread',
        ]
      },
    },
    {
      'target_name' : 'libmatting',
      'type': 'static_library',
      'sources':[
        'api.cc',
        'kde.cc',
        'geodesic.cc',
        'matting.cc',
      ],
      'include_dirs':[
        'third_party/figtree-0.9.3/include/',
      ],
      'direct_dependent_settings': {
        'libraries': [
          '-L<(ROOTDIR)/third_party/figtree-0.9.3/lib/',
          '-lfigtree',
          '-lann_figtree_version'
        ]
      },
    },
    {
      'target_name' : 'main',
      'type' : 'executable',
      'sources':[
        'cvutils.cc',
        'main.cc',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
        '-lglog',
      ],
      'cflags':[
        '<!@(<(pkg-config) --cflags opencv)',
      ],
      'dependencies' : [
        'libmatting',
      ]
    },
    {
      'target_name' : 'interactive',
      'type' : 'executable',
      'sources':[
        'cvutils.cc',
        'interactive.cc',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
        '-lglog',
      ],
      'cflags':[
        '<!@(<(pkg-config) --cflags opencv)',
      ],
      'dependencies' : [
        'libmatting',
      ]
    },

    {
      'target_name' : 'tests',
      'type' : 'executable',
      'sources':[
        'kde_test.cc',
        'geodesic_test.cc',
      ],
      'dependencies' : [
        'gtest_mock',
        'libmatting',
      ]
    }
  ]
}
