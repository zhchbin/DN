{
  'variables': {
    'proto_in_dir': 'src',
    'proto_out_dir': 'proto'
  },
  'targets': [
    {
      'variables': {
        'proto_in_dir': 'src/rpc',
      },
      'target_name': 'pn',
      'type': 'static_library',
      'sources': [
        'src/net/address_list.cc',
        'src/net/io_buffer.cc',
        'src/net/ip_endpoint.cc',
        'src/net/net_error_posix.cc',
        'src/net/net_errors.cc',
        'src/net/net_util.cc',
        'src/net/server_socket.cc',
        'src/net/socket_descriptor.cc',
        'src/net/socket_libevent.cc',
        'src/net/tcp_client_socket.cc',
        'src/net/tcp_server_socket.cc',
        'src/net/tcp_socket_libevent.cc',
        'src/ninja_thread_impl.cc',
        'src/rpc/rpc_channel.cc',
        'src/rpc/rpc_connection.cc',
        'src/rpc/rpc_message.proto',
        'src/rpc/rpc_server.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'third_party/protobuf/protobuf.gyp:protobuf_lite',
      ],
      'include_dirs': [
        'src',
      ],
      'includes': [
        'build/protoc.gypi',
      ],
    },
    {
      'target_name': 'p_ninja',
      'type': 'executable',
      'sources': [
        'src/main.cc',
      ],
      'dependencies': [
        #'third_party/protobuf/protobuf.gyp:protobuf_lite',
        'base/base.gyp:base',
        'pn',
      ],
      'includes': [
        'build/protoc.gypi',
      ],
      'include_dirs': [
        'src',
      ],
    },
  ],
}
