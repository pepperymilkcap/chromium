# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'blacklist',
      'type': 'static_library',
      'sources': [
        'blacklist/blacklist.cc',
        'blacklist/blacklist.h',
        'blacklist/blacklist_interceptions.cc',
        'blacklist/blacklist_interceptions.h',
      ],
      'dependencies': [
        # Depend on base_static, but do NOT take a dependency on base.gyp:base
        # as that would risk pulling in base's link-time dependencies which
        # chrome_elf cannot do.
        '../base/base.gyp:base_static',
        '../sandbox/sandbox.gyp:sandbox',
      ],
    },
    {
      'target_name': 'blacklist_test_main_dll',
      'type': 'shared_library',
      'sources': [
        'blacklist/test/blacklist_test_main_dll.cc',
        'blacklist/test/blacklist_test_main_dll.def',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        'blacklist',
      ],
    },
    {
      'target_name': 'blacklist_test_dll_1',
      'type': 'loadable_module',
      'sources': [
        'blacklist/test/blacklist_test_dll_1.cc',
        'blacklist/test/blacklist_test_dll_1.def',
      ],
    },
    {
      'target_name': 'blacklist_test_dll_2',
      'type': 'loadable_module',
      'sources': [
        'blacklist/test/blacklist_test_dll_2.cc',
        'blacklist/test/blacklist_test_dll_2.def',
      ],
    },
    {
      'target_name': 'blacklist_test_dll_3',
      'type': 'loadable_module',
      'sources': [
        'blacklist/test/blacklist_test_dll_3.cc',
      ],
    },
  ],
}

