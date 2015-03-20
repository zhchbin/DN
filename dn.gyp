{
  'includes': [
    'build/win_precompile.gypi',
  ],
  'variables': {
    'proto_in_dir': 'src/proto',
    'proto_out_dir': 'proto'
  },
  'targets': [
    {
      'target_name': 'libdn',
      'type': 'static_library',
      'sources': [
        'src/common/main_runner.cc',
        'src/common/main_runner.h',
        'src/common/options.cc',
        'src/common/options.h',
        'src/common/util.cc',
        'src/common/util.h',
        'src/master/curl_helper.cc',
        'src/master/curl_helper.h',
        'src/master/master_main_runner.cc',
        'src/master/master_main_runner.h',
        'src/master/master_rpc.cc',
        'src/master/master_rpc.h',
        'src/master/webui_thread.cc',
        'src/master/webui_thread.h',
        'src/net/address_family.h',
        'src/net/address_list.cc',
        'src/net/address_list.h',
        'src/net/completion_callback.h',
        'src/net/io_buffer.cc',
        'src/net/io_buffer.h',
        'src/net/ip_endpoint.cc',
        'src/net/ip_endpoint.h',
        'src/net/net_error_list.h',
        'src/net/net_errors.cc',
        'src/net/net_errors.h',
        'src/net/net_errors_posix.cc',
        'src/net/net_errors_win.cc',
        'src/net/net_util.cc',
        'src/net/net_util.h',
        'src/net/server_socket.cc',
        'src/net/server_socket.h',
        'src/net/socket.h',
        'src/net/socket_descriptor.cc',
        'src/net/socket_descriptor.h',
        'src/net/socket_libevent.cc',
        'src/net/socket_libevent.h',
        'src/net/stream_socket.h',
        'src/net/tcp_client_socket.cc',
        'src/net/tcp_client_socket.h',
        'src/net/tcp_server_socket.cc',
        'src/net/tcp_server_socket.h',
        'src/net/tcp_socket.h',
        'src/net/tcp_socket_libevent.cc',
        'src/net/tcp_socket_libevent.h',
        'src/net/tcp_socket_win.cc',
        'src/net/tcp_socket_win.h',
        'src/net/winsock_init.cc',
        'src/net/winsock_init.h',
        'src/net/winsock_util.cc',
        'src/net/winsock_util.h',
        'src/ninja/dn_builder.cc',
        'src/ninja/dn_builder.h',
        'src/ninja/ninja_main.cc',
        'src/ninja/ninja_main.h',
        'src/proto/rpc_message.proto',
        'src/proto/slave_services.proto',
        'src/rpc/rpc_connection.cc',
        'src/rpc/rpc_connection.h',
        'src/rpc/rpc_socket_client.cc',
        'src/rpc/rpc_socket_client.h',
        'src/rpc/rpc_socket_server.cc',
        'src/rpc/rpc_socket_server.h',
        'src/rpc/service_manager.cc',
        'src/rpc/service_manager.h',
        'src/slave/command_executor.cc',
        'src/slave/command_executor.h',
        'src/slave/slave_file_thread.cc',
        'src/slave/slave_file_thread.h',
        'src/slave/slave_main_runner.cc',
        'src/slave/slave_main_runner.h',
        'src/slave/slave_rpc.cc',
        'src/slave/slave_rpc.h',
        'src/thread/ninja_thread.h',
        'src/thread/ninja_thread_delegate.h',
        'src/thread/ninja_thread_impl.cc',
        'src/thread/ninja_thread_impl.h',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'third_party/protobuf/protobuf.gyp:protobuf_full',
        'third_party/mongoose.gyp:libmongoose',
        'third_party/ninja.gyp:libninja',
      ],
      'include_dirs': [
        'src',
      ],
      'includes': [
        'build/protoc.gypi',
      ],
      'conditions': [
        [ 'OS == "win"', {
            'defines': [
              'CURL_STATICLIB',
            ],
            'sources!': [
              'src/net/socket_libevent.cc',
              'src/net/socket_libevent.h',
              'src/net/tcp_socket_libevent.cc',
              'src/net/tcp_socket_libevent.h',
            ],
            'msvs_disabled_warnings': [4267, 4125],
            'link_settings': {
              'libraries': [
                '-l../../third_party/curl/lib/libcurl.lib',
              ],
            },
            'include_dirs': [
              'third_party/curl/include',
            ],
            'direct_dependent_settings': {
              'include_dirs': [
                'third_party/curl/include',
              ],
            },
          }, { # else: OS != "win"
            'sources!': [
              'src/net/winsock_init.cc',
              'src/net/winsock_init.h',
              'src/net/winsock_util.cc',
              'src/net/winsock_util.h',
            ],
            'link_settings': {
              'libraries': [
                '-lcurl',
              ],
            },
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
      ],
      'include_dirs': [
        'src',
      ],
    },
    {
      'target_name': 'dn_unittest',
      'type': 'executable',
      'sources': [
        'src/master/curl_helper_unittest.cc',
        'src/proto/echo_unittest.proto',
        'src/rpc/rpc_socket_unittest.cc',
        'src/run_all_unittest.cc',
        'src/slave/command_executor_unittest.cc',
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

    {
      'target_name': 'rpc_example_proto',
      'type': 'static_library',
      'sources': [
        'src/proto/calculator.proto',
      ],
      'includes': [
        'build/protoc.gypi',
      ],
    },
    {
      'target_name': 'rpc_example_server',
      'type': 'executable',
      'sources': [
        'src/rpc/example/server.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'libdn',
        'rpc_example_proto',
      ],
      'include_dirs': [
        'src',
      ],
    },
    {
      'target_name': 'rpc_example_client',
      'type': 'executable',
      'sources': [
        'src/rpc/example/client.cc',
      ],
      'dependencies': [
        'base/base.gyp:base',
        'libdn',
        'rpc_example_proto',
      ],
      'include_dirs': [
        'src',
      ],
    },
  ],
}
