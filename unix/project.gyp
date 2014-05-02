# gyp --toplevel-dir option doesn't seem to work as we want when the .gyp
# file is also out of the source tree.
# So instead, use SRCDIR environment variable set by gen_ninja.sh to locate
# source files
{
  'variables': {
    'pkg-config' : 'pkg-config',

  },

  'target_defaults': {
    'cflags':[
      '-Wall', '-Wextra', '-Wno-unused-parameter', '-Werror', '-std=c++11',
      '-Wno-unused-variable',
      '-Wno-unused-but-set-variable',
      '-O2',
      #'-ggdb',
    ],
    'include_dirs': [
      '<(INCDIR)',
      '<(SRCDIR)/third_party/',
    ]
  },

  'targets': [
    {
      'target_name' : 'miniglog',
      'type': 'static_library',
      'cflags': [],
      'sources': [
        '<(SRCDIR)/third_party/miniglog/glog/logging.cc'
      ],
      'include_dirs': [
        '<(SRCDIR)/third_party/miniglog/'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SRCDIR)/third_party/miniglog/',
        ],
      }
    },
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
        '<(SRCDIR)/api.cc',
        '<(SRCDIR)/kde.cc',
        '<(SRCDIR)/geodesic.cc',
        '<(SRCDIR)/matting.cc',
      ],
      'include_dirs':[
        '<(FIGTREE)/include/figtree/',
      ],
      'direct_dependent_settings': {
        'libraries': [
          '-L<(FIGTREE)/unix/',
          '-lfigtree',
        ]
      },
      'export_dependent_settings': [
        'miniglog',
      ],
      'dependencies' : [
        'miniglog',
      ]
    },
    {
      'target_name' : 'main',
      'type' : 'executable',
      'sources':[
        'samples/cvutils.cc',
        'samples/main.cc',
      ],
      'include_dirs': [
        'samples',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
      ],
      'cflags':[
        '<!@(<(pkg-config) --cflags opencv)',
      ],
      'dependencies' : [
        'libmatting',
      ]
    },
    {
      'target_name' : 'shortest_main',
      'type' : 'executable',
      'sources':[
        'samples/cvutils.cc',
        'samples/shortest_main.cc',
      ],
      'include_dirs': [
        'samples',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
      ],
      'cflags':[
        '<!@(<(pkg-config) --cflags opencv)',
      ],
      'dependencies' : [
        'libmatting',
      ]
    },
    {
      'target_name' : 'shortest_interactive',
      'type' : 'executable',
      'sources':[
        'samples/cvutils.cc',
        'samples/shortest_interactive.cc',
      ],
      'include_dirs': [
        'samples',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
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
        'samples/cvutils.cc',
        'samples/interactive.cc',
      ],
      'include_dirs': [
        'samples',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
      ],
      'cflags':[
        '<!@(<(pkg-config) --cflags opencv)',
      ],
      'dependencies' : [
        'libmatting',
      ]
    },
    {
      'target_name' : 'simple_interactive',
      'type' : 'executable',
      'sources':[
        'samples/cvutils.cc',
        'samples/simple_interactive.cc',
      ],
      'include_dirs': [
        'samples',
      ],
      'libraries':[
        '<!@(<(pkg-config) --libs opencv)',
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
        '<(SRCDIR)/kde_test.cc',
        '<(SRCDIR)/geodesic_test.cc',
      ],
      'dependencies' : [
        'gtest_mock',
        'libmatting',
      ]
    }
  ]
}
