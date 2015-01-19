{
  'includes': [
    'build/win_precompile.gypi',
  ],
  'targets': [
    {
      'variables': {
        'proto_in_dir': 'src/rpc',
        'proto_out_dir': 'proto'
      },
      'target_name': 'pn',
      'type': 'static_library',
      'sources': [
        'src/net/address_list.cc',
        'src/net/io_buffer.cc',
        'src/net/ip_endpoint.cc',
        'src/net/net_errors.cc',
        'src/net/net_errors_posix.cc',
        'src/net/net_errors_win.cc',
        'src/net/net_util.cc',
        'src/net/server_socket.cc',
        'src/net/socket_descriptor.cc',
        'src/net/socket_libevent.cc',
        'src/net/tcp_client_socket.cc',
        'src/net/tcp_server_socket.cc',
        'src/net/tcp_socket_libevent.cc',
        'src/net/tcp_socket_win.cc',
        'src/net/winsock_init.cc',
        'src/net/winsock_util.cc',
        'src/ninja_thread_impl.cc',
        'src/rpc/rpc_channel.cc',
        'src/rpc/rpc_client_main.cc',
        'src/rpc/rpc_connection.cc',
        'src/rpc/rpc_message.proto',
        'src/rpc/rpc_options.cc',
        'src/rpc/rpc_server.cc',
        'src/rpc/rpc_server_main.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'third_party/protobuf/protobuf.gyp:protobuf_full',
      ],
      'include_dirs': [
        'src',
      ],
      'includes': [
        'build/protoc.gypi',
      ],
      'conditions': [
        [ 'OS == "win"', {
            'sources!': [
              'src/net/socket_libevent.cc',
              'src/net/socket_libevent.h',
              'src/net/tcp_socket_libevent.cc',
              'src/net/tcp_socket_libevent.h',
            ],
            'msvs_disabled_warnings': [4267, 4125],
          }, { # else: OS != "win"
            'sources!': [
              'src/net/winsock_init.cc',
              'src/net/winsock_init.h',
              'src/net/winsock_util.cc',
              'src/net/winsock_util.h',
            ],
          },
        ],
      ],
    },
    {
      'target_name': 'p_ninja',
      'type': 'executable',
      'sources': [
        'src/main.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'pn',
      ],
      'include_dirs': [
        'src',
      ],
    },

    {
      'target_name': 'p_ninja_unittest',
      'type': 'executable',
      'sources': [
        'src/ninja_thread_unittest.cc',
        'src/run_all_unittest.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'pn',
        'testing/gtest.gyp:gtest',
      ],
      'include_dirs': [
        'src',
      ],
    },
  ],
}
