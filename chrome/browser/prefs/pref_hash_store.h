// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_PREF_HASH_STORE_H_
#define CHROME_BROWSER_PREFS_PREF_HASH_STORE_H_

#include <string>

#include "base/memory/scoped_ptr.h"

namespace base {
class Value;
}  // namespace base

// Stores hashes of and verifies preference values. To use, first call
// |InitializeTrackedValue| with each preference that should be tracked. Then
// call |OnPrefValueChanged| to update the hash store when preference values
// change.
class PrefHashStore {
 public:
  virtual ~PrefHashStore() {}

  enum ValueState {
    // The preference value corresponds to its stored hash.
    UNCHANGED,
    // The preference has been cleared since the last hash.
    CLEARED,
    // The preference value corresponds to its stored hash, which was calculated
    // using a legacy hash algorithm.
    MIGRATED,
    // The preference value has been changed since the last hash.
    CHANGED,
    // No stored hash exists for the preference value.
    UNKNOWN_VALUE,
  };

  // Checks |initial_value| against the existing stored value hash.
  virtual ValueState CheckValue(
      const std::string& path, const base::Value* initial_value) const = 0;

  // Stores a hash of the current |value| of the preference at |path|.
  virtual void StoreHash(const std::string& path,
                         const base::Value* value) = 0;
};

#endif  // CHROME_BROWSER_PREFS_PREF_HASH_STORE_H_
