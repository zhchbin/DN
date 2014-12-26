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
        'src/main.cc',
        'src/person.proto',
      ],
      'include_dirs': [
        'third_party/libevent/include',
      ],
      'dependencies': [
        'lib_foo',
        'third_party/protobuf/protobuf.gyp:protobuf_lite',
      ],
      'variables': {
        'proto_in_dir': 'src',
        'proto_out_dir': 'message'
      },
      'includes': [
        'build/protoc.gypi',
      ],
      'conditions': [
        ['OS=="linux"', {
          'link_settings': {
            "libraries": [
              '-L../../third_party/libevent/libs/',
              "-levent",
            ],
          },
        }],
        ['OS=="win"', {
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                'libevent.lib',
                'libevent_core.lib',
                'libevent_extras.lib',
              ],
              'AdditionalLibraryDirectories': [
                'third_party\\libevent\\libs',
              ],
            },
          },
        }],
      ],
    },
  ],
}
