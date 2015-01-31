{
  'includes': [
    'build/win_precompile.gypi',
  ],
  'variables': {
    'proto_in_dir': 'src/rpc',
    'proto_out_dir': 'proto'
  },
  'targets': [
    {
      'target_name': 'libdn',
      'type': 'static_library',
      'sources': [
        'src/common/main_runner.cc',
        'src/master/master_main_runner.cc',
        'src/master/master_rpc.cc',
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
        'src/ninja/ninja_builder.cc',
        'src/rpc/rpc_connection.cc',
        'src/rpc/rpc_message.proto',
        'src/rpc/rpc_options.cc',
        'src/rpc/rpc_socket_client.cc',
        'src/rpc/rpc_socket_server.cc',
        'src/rpc/service_manager.cc',
        'src/rpc/slave_services.proto',
        'src/slave/command_executor.cc',
        'src/slave/slave_main_runner.cc',
        'src/slave/slave_rpc.cc',
        'src/thread/ninja_thread_impl.cc',
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
      'target_name': 'dn',
      'type': 'executable',
      'sources': [
        'src/main.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'libdn',
        'third_party/ninja.gyp:libninja',
      ],
      'include_dirs': [
        'src',
      ],
    },

    {
      'target_name': 'dn_unittest',
      'type': 'executable',
      'sources': [
        'src/rpc/echo_unittest.proto',
        'src/rpc/rpc_socket_unittest.cc',
        'src/run_all_unittest.cc',
        'src/thread/ninja_thread_unittest.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'libdn',
        'testing/gtest.gyp:gtest',
        'testing/gmock.gyp:gmock',
      ],
      'include_dirs': [
        'src',
      ],
      'includes': [
        'build/protoc.gypi',
      ],
    },
  ],
}