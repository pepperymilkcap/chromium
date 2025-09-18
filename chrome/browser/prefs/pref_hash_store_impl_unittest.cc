// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/pref_hash_store_impl.h"

#include <string>

#include "base/prefs/pref_service.h"
#include "base/prefs/scoped_user_pref_update.h"
#include "base/prefs/testing_pref_service.h"
#include "base/values.h"
#include "chrome/browser/prefs/pref_hash_store.h"
#include "chrome/common/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PrefHashStoreImplTest, TestCase) {
  base::StringValue string_1("string1");
  base::StringValue string_2("string2");

  TestingPrefServiceSimple local_state;
  PrefHashStoreImpl::RegisterPrefs(local_state.registry());

  // 32 NULL bytes is the seed that was used to generate the legacy hash.
  PrefHashStoreImpl pref_hash_store(
      "store_id", std::string(32,0), "device_id", &local_state);

  ASSERT_EQ(PrefHashStore::UNKNOWN_VALUE,
            pref_hash_store.CheckValue("path1", &string_1));
  pref_hash_store.StoreHash("path1", &string_1);
  ASSERT_EQ(PrefHashStore::UNCHANGED,
            pref_hash_store.CheckValue("path1", &string_1));
  ASSERT_EQ(PrefHashStore::CLEARED, pref_hash_store.CheckValue("path1", NULL));
  pref_hash_store.StoreHash("path1", NULL);
  ASSERT_EQ(PrefHashStore::UNCHANGED,
            pref_hash_store.CheckValue("path1", NULL));
  ASSERT_EQ(PrefHashStore::CHANGED,
            pref_hash_store.CheckValue("path1", &string_2));

  base::DictionaryValue dict;
  dict.Set("a", new base::StringValue("foo"));
  dict.Set("d", new base::StringValue("bad"));
  dict.Set("b", new base::StringValue("bar"));
  dict.Set("c", new base::StringValue("baz"));

  // Manually shove in a legacy hash.
  DictionaryPrefUpdate update(&local_state, prefs::kProfilePreferenceHashes);
  base::DictionaryValue* child_dictionary = NULL;
  ASSERT_TRUE(update->GetDictionary("store_id", &child_dictionary));
  child_dictionary->SetString(
      "path1",
      "C503FB7C65EEFD5C07185F616A0AA67923C069909933F362022B1F187E73E9A2");

  ASSERT_EQ(PrefHashStore::MIGRATED,
            pref_hash_store.CheckValue("path1", &dict));
  pref_hash_store.StoreHash("path1", &dict);
  ASSERT_EQ(PrefHashStore::UNCHANGED,
            pref_hash_store.CheckValue("path1", &dict));
}
