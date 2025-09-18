# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'include_tests%': 1,
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'cast_transport',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)/',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/net/net.gyp:net',
      ],
      'sources': [
        'cast_transport_defines.h', 
        'cast_transport_sender.h',
        'pacing/paced_sender.cc',
        'pacing/paced_sender.h',
        'rtcp/rtcp_builder.cc',
        'rtcp/rtcp_builder.h',
        'rtp_sender/packet_storage/packet_storage.cc',
        'rtp_sender/packet_storage/packet_storage.h',
        'rtp_sender/rtp_packetizer/rtp_packetizer.cc',
        'rtp_sender/rtp_packetizer/rtp_packetizer.h',
        'rtp_sender/rtp_sender.cc',
        'rtp_sender/rtp_sender.h',
        'transport/transport.cc',
        'transport/transport.h',
      ], # source
    },
  ],  # targets,
}
