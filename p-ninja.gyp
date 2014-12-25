{
  'includes': [
    'build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'lib_foo',
      'type': 'static_library',
      'sources': [
        'src/foo.cc',
        'src/foo.h',
      ],
    },
    {
      'target_name': 'lib_foo_unittest',
      'type': 'executable',
      'sources': [
        'src/foo_unittest.cc',
      ],
      'dependencies': [
        'lib_foo',
        'testing/gtest.gyp:gtest_main',
        'testing/gmock.gyp:gmock_main',
      ],
    },

    {
      'target_name': 'p_ninja',
      'type': 'executable',
      'sources': [
        'src/main.cc'
      ],
      'dependencies': [
        'lib_foo',
      ]
    },
  ],
}
