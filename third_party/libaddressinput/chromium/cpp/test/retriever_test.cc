// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "retriever.h"

#include <libaddressinput/callback.h>
#include <libaddressinput/downloader.h>
#include <libaddressinput/util/scoped_ptr.h>

#include <string>

#include <gtest/gtest.h>

#include "fake_downloader.h"
#include "fake_storage.h"
#include "region_data_constants.h"

namespace i18n {
namespace addressinput {

namespace {

const char kKey[] = "data/CA/AB--fr";

// Empty data that the downloader can return.
const char kEmptyData[] = "{}";

// Tests for Retriever object.
class RetrieverTest : public testing::Test {
 protected:
  RetrieverTest()
      : retriever_(FakeDownloader::kFakeDataUrl,
                   scoped_ptr<const Downloader>(new FakeDownloader),
                   scoped_ptr<Storage>(new FakeStorage)),
        success_(false),
        key_(),
        data_() {}

  virtual ~RetrieverTest() {}

  scoped_ptr<Retriever::Callback> BuildCallback() {
    return ::i18n::addressinput::BuildCallback(
        this, &RetrieverTest::OnDataReady);
  }

  Retriever retriever_;
  bool success_;
  std::string key_;
  std::string data_;

 private:
  void OnDataReady(bool success,
                   const std::string& key,
                   const std::string& data) {
    success_ = success;
    key_ = key;
    data_ = data;
  }
};

TEST_F(RetrieverTest, RetrieveData) {
  retriever_.Retrieve(kKey, BuildCallback());

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_FALSE(data_.empty());
  EXPECT_NE(kEmptyData, data_);
}

TEST_F(RetrieverTest, ReadDataFromStorage) {
  retriever_.Retrieve(kKey, BuildCallback());
  retriever_.Retrieve(kKey, BuildCallback());

  EXPECT_TRUE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_FALSE(data_.empty());
  EXPECT_NE(kEmptyData, data_);
}

TEST_F(RetrieverTest, MissingKeyReturnsEmptyData) {
  static const char kMissingKey[] = "junk";

  retriever_.Retrieve(kMissingKey, BuildCallback());

  EXPECT_TRUE(success_);
  EXPECT_EQ(kMissingKey, key_);
  EXPECT_EQ(kEmptyData, data_);
}

// The downloader that always fails.
class FaultyDownloader : public Downloader {
 public:
  FaultyDownloader() {}
  virtual ~FaultyDownloader() {}

  // Downloader implementation.
  virtual void Download(const std::string& url,
                        scoped_ptr<Callback> downloaded) const {
    (*downloaded)(false, url, "garbage");
  }
};

TEST_F(RetrieverTest, FaultyDownloader) {
  Retriever bad_retriever(FakeDownloader::kFakeDataUrl,
                          scoped_ptr<const Downloader>(new FaultyDownloader),
                          scoped_ptr<Storage>(new FakeStorage));
  bad_retriever.Retrieve(kKey, BuildCallback());

  EXPECT_FALSE(success_);
  EXPECT_EQ(kKey, key_);
  EXPECT_TRUE(data_.empty());
}

}  // namespace

}  // namespace addressinput
}  // namespace i18n
