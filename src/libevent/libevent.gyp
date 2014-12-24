# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'libevent',
      'product_name': 'event',
      'type': 'static_library',
      'toolsets': ['host', 'target'],
      'sources': [
        'buffer.c',
        'evbuffer.c',
        'evdns.c',
        'event.c',
        'event_tagging.c',
        'evrpc.c',
        'evutil.c',
        'http.c',
        'log.c',
        'poll.c',
        'select.c',
        'signal.c',
        'strlcpy.c',
      ],
      'defines': [
        'HAVE_CONFIG_H',
      ],
      'conditions': [
        # libevent has platform-specific implementation files.  Since its
        # native build uses autoconf, platform-specific config.h files are
        # provided and live in platform-specific directories.
        [ 'OS == "linux" or (OS == "android" and _toolset == "host")', {
          'sources': [ 'epoll.c' ],
          'include_dirs': [ 'linux' ],
          'link_settings': {
            'libraries': [
              # We need rt for clock_gettime().
              # TODO(port) Maybe on FreeBSD as well?
              '-lrt',
            ],
          },
          'cflags!': [
            '-Wall',
            '-Werror',
            '-Wextra',
          ],
        }],
      ],
    },
  ],
}
