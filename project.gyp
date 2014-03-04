{
  'variables': {
    'pkg-config' : 'pkg-config',
  },

  'target_defaults': {
    'cflags':[
      '-Wall', '-Wextra', '-Wno-unused', '-Werror', '-std=c++11',
      #'-O2',
      '-ggdb'
    ],
    'libraries':[
      # TODO: Remove dependency on volumit's glog
      '-L/home/julien/tm/v2/libs/_install/lib',
      '-lglog',
    ],
    'include_dirs':[
      # TODO: Remove dependency on volumit's glog
      '/home/julien/tm/v2/libs/_install/include/'
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
        'kde.cc',
        'geodesic.cc',
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
