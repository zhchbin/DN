{
  'targets': [
    {
      'target_name': 'p_ninja',
      'type': 'executable',
      'sources': [
        'src/main.cc',
      ],
      'dependencies': [
        'third_party/protobuf/protobuf.gyp:protobuf_lite',
        'base/base.gyp:base',
      ],
      'variables': {
        'proto_in_dir': 'src',
        'proto_out_dir': 'message'
      },
      'includes': [
        'build/protoc.gypi',
      ],
    },
  ],
}
