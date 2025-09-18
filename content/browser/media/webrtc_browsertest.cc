// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/debug/trace_event_impl.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/browser/media/webrtc_internals.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test.h"
#include "content/test/content_browser_test_utils.h"
#include "media/audio/audio_manager.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/perf/perf_test.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace {

static const char kGetUserMediaAndStop[] = "getUserMediaAndStop";
static const char kGetUserMediaAndWaitAndStop[] = "getUserMediaAndWaitAndStop";
static const char kGetUserMediaAndAnalyseAndStop[] =
    "getUserMediaAndAnalyseAndStop";

// Results returned by JS.
static const char kOK[] = "OK";
static const char kGetUserMediaFailed[] =
    "GetUserMedia call failed with code undefined";

std::string GenerateGetUserMediaCall(const char* function_name,
                                     int min_width,
                                     int max_width,
                                     int min_height,
                                     int max_height,
                                     int min_frame_rate,
                                     int max_frame_rate) {
  return base::StringPrintf(
      "%s({video: {mandatory: {minWidth: %d, maxWidth: %d, "
      "minHeight: %d, maxHeight: %d, minFrameRate: %d, maxFrameRate: %d}, "
      "optional: []}});",
      function_name,
      min_width,
      max_width,
      min_height,
      max_height,
      min_frame_rate,
      max_frame_rate);
}

std::string GenerateGetUserMediaWithMandatorySourceID(
    const std::string& function_name,
    const std::string& audio_source_id,
    const std::string& video_source_id) {
  const std::string audio_constraint =
      "audio: {mandatory: { sourceId:\"" + audio_source_id + "\"}}, ";

  const std::string video_constraint =
      "video: {mandatory: { sourceId:\"" + video_source_id + "\"}}";
  return function_name + "({" + audio_constraint + video_constraint + "});";
}

std::string GenerateGetUserMediaWithOptionalSourceID(
    const std::string& function_name,
    const std::string& audio_source_id,
    const std::string& video_source_id) {
  const std::string audio_constraint =
      "audio: {optional: [{sourceId:\"" + audio_source_id + "\"}]}, ";

  const std::string video_constraint =
      "video: {optional: [{ sourceId:\"" + video_source_id + "\"}]}";
  return function_name + "({" + audio_constraint + video_constraint + "});";
}

}

namespace content {

class WebrtcBrowserTest: public ContentBrowserTest {
 public:
  WebrtcBrowserTest() {}
  virtual ~WebrtcBrowserTest() {}

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    // We need fake devices in this test since we want to run on naked VMs. We
    // assume these switches are set by default in content_browsertests.
    ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kUseFakeDeviceForMediaStream));
    ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kUseFakeUIForMediaStream));

    // The video playback will not work without a GPU, so force its use here.
    // This may not be available on all VMs though.
    command_line->AppendSwitch(switches::kUseGpuInTests);
  }

  void DumpChromeTraceCallback(
      const scoped_refptr<base::RefCountedString>& events,
      bool has_more_events) {
    // Convert the dump output into a correct JSON List.
    std::string contents = "[" + events->data() + "]";

    int error_code;
    std::string error_message;
    scoped_ptr<base::Value> value(
        base::JSONReader::ReadAndReturnError(contents,
                                             base::JSON_ALLOW_TRAILING_COMMAS,
                                             &error_code,
                                             &error_message));

    ASSERT_TRUE(value.get() != NULL) << error_message;
    ASSERT_EQ(value->GetType(), base::Value::TYPE_LIST);

    base::ListValue* values;
    ASSERT_TRUE(value->GetAsList(&values));

    int duration_ns = 0;
    std::string samples_duration;
    double timestamp_ns = 0.0;
    double previous_timestamp_ns = 0.0;
    std::string samples_interarrival_ns;
    for (base::ListValue::iterator it = values->begin();
         it != values->end(); ++it) {
      const base::DictionaryValue* dict;
      EXPECT_TRUE((*it)->GetAsDictionary(&dict));

      if (dict->GetInteger("dur", &duration_ns))
        samples_duration.append(base::StringPrintf("%d,", duration_ns));
      if (dict->GetDouble("ts", &timestamp_ns)) {
        if (previous_timestamp_ns) {
          samples_interarrival_ns.append(
              base::StringPrintf("%f,", timestamp_ns - previous_timestamp_ns));
        }
        previous_timestamp_ns = timestamp_ns;
      }
    }
    ASSERT_GT(samples_duration.size(), 0u)
        << "Could not collect any samples during test, this is bad";
    perf_test::PrintResultList("video_capture",
                               "",
                               "sample_duration",
                               samples_duration,
                               "ns",
                               true);
    perf_test::PrintResultList("video_capture",
                               "",
                               "interarrival_time",
                               samples_interarrival_ns,
                               "ns",
                               true);
  }

  void GetSources(std::vector<std::string>* audio_ids,
                  std::vector<std::string>* video_ids) {
    GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));
    NavigateToURL(shell(), url);

    std::string sources_as_json = ExecuteJavascriptAndReturnResult(
        "getSources()");
    EXPECT_FALSE(sources_as_json.empty());

    int error_code;
    std::string error_message;
    scoped_ptr<base::Value> value(
        base::JSONReader::ReadAndReturnError(sources_as_json,
                                             base::JSON_ALLOW_TRAILING_COMMAS,
                                             &error_code,
                                             &error_message));

    ASSERT_TRUE(value.get() != NULL) << error_message;
    EXPECT_EQ(value->GetType(), base::Value::TYPE_LIST);

    base::ListValue* values;
    ASSERT_TRUE(value->GetAsList(&values));

    for (base::ListValue::iterator it = values->begin();
         it != values->end(); ++it) {
      const base::DictionaryValue* dict;
      std::string kind;
      std::string id;
      ASSERT_TRUE((*it)->GetAsDictionary(&dict));
      ASSERT_TRUE(dict->GetString("kind", &kind));
      ASSERT_TRUE(dict->GetString("id", &id));
      ASSERT_FALSE(id.empty());
      EXPECT_TRUE(kind == "audio" || kind == "video");
      if (kind == "audio") {
        audio_ids->push_back(id);
      } else if (kind == "video") {
        video_ids->push_back(id);
      }
    }
    ASSERT_FALSE(audio_ids->empty());
    ASSERT_FALSE(video_ids->empty());
  }

 protected:
  bool ExecuteJavascript(const std::string& javascript) {
    return ExecuteScript(shell()->web_contents(), javascript);
  }

  // Executes |javascript|. The script is required to use
  // window.domAutomationController.send to send a string value back to here.
  std::string ExecuteJavascriptAndReturnResult(const std::string& javascript) {
    std::string result;
    EXPECT_TRUE(ExecuteScriptAndExtractString(shell()->web_contents(),
                                              javascript,
                                              &result));
    return result;
  }

  void ExpectTitle(const std::string& expected_title) const {
    base::string16 expected_title16(base::ASCIIToUTF16(expected_title));
    TitleWatcher title_watcher(shell()->web_contents(), expected_title16);
    EXPECT_EQ(expected_title16, title_watcher.WaitAndGetTitle());
  }

  // Convenience function since most peerconnection-call.html tests just load
  // the page, kick off some javascript and wait for the title to change to OK.
  void MakeTypicalPeerConnectionCall(const std::string& javascript) {
    ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

    GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
    NavigateToURL(shell(), url);

#if defined (OS_ANDROID)
    // Always force iSAC 16K on Android for now (Opus is broken).
    ASSERT_TRUE(ExecuteJavascript("forceIsac16KInSdp();"));
#endif

    ASSERT_TRUE(ExecuteJavascript(javascript));
    ExpectTitle("OK");
  }
};

// These tests will all make a getUserMedia call with different constraints and
// see that the success callback is called. If the error callback is called or
// none of the callbacks are called the tests will simply time out and fail.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, GetVideoStreamAndStop) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));
  NavigateToURL(shell(), url);

  ASSERT_TRUE(ExecuteJavascript(
      base::StringPrintf("%s({video: true});", kGetUserMediaAndStop)));

  ExpectTitle("OK");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, GetAudioAndVideoStreamAndStop) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));
  NavigateToURL(shell(), url);

  ASSERT_TRUE(ExecuteJavascript(base::StringPrintf(
      "%s({video: true, audio: true});", kGetUserMediaAndStop)));

  ExpectTitle("OK");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, GetAudioAndVideoStreamAndClone) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));
  NavigateToURL(shell(), url);

  ASSERT_TRUE(ExecuteJavascript("getUserMediaAndClone();"));

  ExpectTitle("OK");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, GetUserMediaWithMandatorySourceID) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  std::vector<std::string> audio_ids;
  std::vector<std::string> video_ids;
  GetSources(&audio_ids, &video_ids);

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));

  // Test all combinations of mandatory sourceID;
  for (std::vector<std::string>::const_iterator video_it = video_ids.begin();
       video_it != video_ids.end(); ++video_it) {
    for (std::vector<std::string>::const_iterator audio_it = audio_ids.begin();
         audio_it != audio_ids.end(); ++audio_it) {
      NavigateToURL(shell(), url);
      EXPECT_EQ(kOK, ExecuteJavascriptAndReturnResult(
          GenerateGetUserMediaWithMandatorySourceID(
              kGetUserMediaAndStop,
              *audio_it,
              *video_it)));
    }
  }
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       GetUserMediaWithInvalidMandatorySourceID) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  std::vector<std::string> audio_ids;
  std::vector<std::string> video_ids;
  GetSources(&audio_ids, &video_ids);

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));

  // Test with invalid mandatory audio sourceID.
  NavigateToURL(shell(), url);
  EXPECT_EQ(kGetUserMediaFailed, ExecuteJavascriptAndReturnResult(
      GenerateGetUserMediaWithMandatorySourceID(
          kGetUserMediaAndStop,
          "something invalid",
          video_ids[0])));

  // Test with invalid mandatory video sourceID.
  EXPECT_EQ(kGetUserMediaFailed, ExecuteJavascriptAndReturnResult(
      GenerateGetUserMediaWithMandatorySourceID(
          kGetUserMediaAndStop,
          audio_ids[0],
          "something invalid")));

  // Test with empty mandatory audio sourceID.
  EXPECT_EQ(kGetUserMediaFailed, ExecuteJavascriptAndReturnResult(
      GenerateGetUserMediaWithMandatorySourceID(
          kGetUserMediaAndStop,
          "",
          video_ids[0])));
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, GetUserMediaWithOptionalSourceID) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  std::vector<std::string> audio_ids;
  std::vector<std::string> video_ids;
  GetSources(&audio_ids, &video_ids);

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));
  NavigateToURL(shell(), url);

  // Test all combinations of mandatory sourceID;
  for (std::vector<std::string>::const_iterator video_it = video_ids.begin();
       video_it != video_ids.end(); ++video_it) {
    for (std::vector<std::string>::const_iterator audio_it = audio_ids.begin();
         audio_it != audio_ids.end(); ++audio_it) {
      EXPECT_EQ(kOK, ExecuteJavascriptAndReturnResult(
          GenerateGetUserMediaWithOptionalSourceID(
              kGetUserMediaAndStop,
              *audio_it,
              *video_it)));
    }
  }
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       GetUserMediaWithInvalidOptionalSourceID) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  std::vector<std::string> audio_ids;
  std::vector<std::string> video_ids;
  GetSources(&audio_ids, &video_ids);

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));

  // Test with invalid optional audio sourceID.
  NavigateToURL(shell(), url);
  EXPECT_EQ(kOK, ExecuteJavascriptAndReturnResult(
      GenerateGetUserMediaWithOptionalSourceID(
          kGetUserMediaAndStop,
          "something invalid",
          video_ids[0])));

  // Test with invalid optional video sourceID.
  EXPECT_EQ(kOK, ExecuteJavascriptAndReturnResult(
      GenerateGetUserMediaWithOptionalSourceID(
          kGetUserMediaAndStop,
          audio_ids[0],
          "something invalid")));

  // Test with empty optional audio sourceID.
  EXPECT_EQ(kOK, ExecuteJavascriptAndReturnResult(
      GenerateGetUserMediaWithOptionalSourceID(
          kGetUserMediaAndStop,
          "",
          video_ids[0])));
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CanSetupVideoCall DISABLED_CanSetupVideoCall
#else
#define MAYBE_CanSetupVideoCall CanSetupVideoCall
#endif

// These tests will make a complete PeerConnection-based call and verify that
// video is playing for the call.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CanSetupVideoCall) {
  MakeTypicalPeerConnectionCall("call({video: true});");
}

// This test will make a simple getUserMedia page, verify that video is playing
// in a simple local <video>, and for a couple of seconds, collect some
// performance traces.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, TracePerformanceDuringGetUserMedia) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));
  NavigateToURL(shell(), url);
  // Put getUserMedia to work and let it run for a couple of seconds.
  ASSERT_TRUE(ExecuteJavascript(base::StringPrintf(
      "%s({video: true}, 10);", kGetUserMediaAndWaitAndStop)));

  // Make sure the stream is up and running, then start collecting traces.
  ExpectTitle("Running...");
  base::debug::TraceLog* trace_log = base::debug::TraceLog::GetInstance();
  trace_log->SetEnabled(base::debug::CategoryFilter("video"),
                        base::debug::TraceLog::ENABLE_SAMPLING);
  // Check that we are indeed recording.
  ASSERT_EQ(trace_log->GetNumTracesRecorded(), 1);

  // Wait until the page title changes to "OK". Do not sleep() here since that
  // would stop both this code and the browser underneath.
  ExpectTitle("OK");

  // Note that we need to stop the trace recording before flushing the data.
  trace_log->SetDisabled();
  trace_log->Flush(base::Bind(&WebrtcBrowserTest::DumpChromeTraceCallback,
                              base::Unretained(this)));
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux, see http://crbug.com/240376
#define MAYBE_CanSetupAudioAndVideoCall DISABLED_CanSetupAudioAndVideoCall
#else
#define MAYBE_CanSetupAudioAndVideoCall CanSetupAudioAndVideoCall
#endif

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CanSetupAudioAndVideoCall) {
  MakeTypicalPeerConnectionCall("call({video: true, audio: true});");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MANUAL_CanSetupCallAndSendDtmf) {
  // Don't force iSAC on Android for this test: iSAC doesn't work with DTMF.
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
  NavigateToURL(shell(), url);

  ASSERT_TRUE(ExecuteJavascript("callAndSendDtmf(\'123,abc\');"));
  ExpectTitle("OK");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       DISABLED_CanMakeEmptyCallThenAddStreamsAndRenegotiate) {
  const char* kJavascript =
      "callEmptyThenAddOneStreamAndRenegotiate({video: true, audio: true});";
  MakeTypicalPeerConnectionCall(kJavascript);
}

// Below 2 test will make a complete PeerConnection-based call between pc1 and
// pc2, and then use the remote stream to setup a call between pc3 and pc4, and
// then verify that video is received on pc3 and pc4.
// Flaky on win xp. http://crbug.com/304775
#if defined(OS_WIN)
#define MAYBE_CanForwardRemoteStream DISABLED_CanForwardRemoteStream
#define MAYBE_CanForwardRemoteStream720p DISABLED_CanForwardRemoteStream720p
#else
#define MAYBE_CanForwardRemoteStream CanForwardRemoteStream
#define MAYBE_CanForwardRemoteStream720p CanForwardRemoteStream720p
#endif
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CanForwardRemoteStream) {
  MakeTypicalPeerConnectionCall(
      "callAndForwardRemoteStream({video: true, audio: true});");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CanForwardRemoteStream720p) {
  const std::string javascript = GenerateGetUserMediaCall(
      "callAndForwardRemoteStream", 1280, 1280, 720, 720, 30, 30);
  MakeTypicalPeerConnectionCall(javascript);
}

// This test will make a complete PeerConnection-based call but remove the
// MSID and bundle attribute from the initial offer to verify that
// video is playing for the call even if the initiating client don't support
// MSID. http://tools.ietf.org/html/draft-alvestrand-rtcweb-msid-02
#if defined(OS_WIN) && defined(USE_AURA)
// Disabled for win7_aura, see http://crbug.com/235089.
#define MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle\
        DISABLED_CanSetupAudioAndVideoCallWithoutMsidAndBundle
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux, see http://crbug.com/240373
#define MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle\
        DISABLED_CanSetupAudioAndVideoCallWithoutMsidAndBundle
#else
#define MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle\
        CanSetupAudioAndVideoCallWithoutMsidAndBundle
#endif
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       MAYBE_CanSetupAudioAndVideoCallWithoutMsidAndBundle) {
  MakeTypicalPeerConnectionCall("callWithoutMsidAndBundle();");
}

// This test will modify the SDP offer to an unsupported codec, which should
// cause SetLocalDescription to fail.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, NegotiateUnsupportedVideoCodec) {
  MakeTypicalPeerConnectionCall("negotiateUnsupportedVideoCodec();");
}

// This test will modify the SDP offer to use no encryption, which should
// cause SetLocalDescription to fail.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, NegotiateNonCryptoCall) {
  MakeTypicalPeerConnectionCall("negotiateNonCryptoCall();");
}

// This test will make a complete PeerConnection-based call using legacy SDP
// settings: GIce, external SDES, and no BUNDLE.
#if defined(OS_WIN) && defined(USE_AURA)
// Disabled for win7_aura, see http://crbug.com/235089.
#define MAYBE_CanSetupLegacyCall DISABLED_CanSetupLegacyCall
#elif defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux, see http://crbug.com/240373
#define MAYBE_CanSetupLegacyCall DISABLED_CanSetupLegacyCall
#else
#define MAYBE_CanSetupLegacyCall CanSetupLegacyCall
#endif

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CanSetupLegacyCall) {
  MakeTypicalPeerConnectionCall("callWithLegacySdp();");
}

// This test will make a PeerConnection-based call and test an unreliable text
// dataChannel.
// TODO(mallinath) - Remove this test after rtp based data channel is disabled.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, CallWithDataOnly) {
  MakeTypicalPeerConnectionCall("callWithDataOnly();");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, CallWithSctpDataOnly) {
  MakeTypicalPeerConnectionCall("callWithSctpDataOnly();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithDataAndMedia DISABLED_CallWithDataAndMedia
#else
#define MAYBE_CallWithDataAndMedia CallWithDataAndMedia
#endif

// This test will make a PeerConnection-based call and test an unreliable text
// dataChannel and audio and video tracks.
// TODO(mallinath) - Remove this test after rtp based data channel is disabled.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CallWithDataAndMedia) {
  MakeTypicalPeerConnectionCall("callWithDataAndMedia();");
}


#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithSctpDataAndMedia DISABLED_CallWithSctpDataAndMedia
#else
#define MAYBE_CallWithSctpDataAndMedia CallWithSctpDataAndMedia
#endif

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       MAYBE_CallWithSctpDataAndMedia) {
  MakeTypicalPeerConnectionCall("callWithSctpDataAndMedia();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithDataAndLaterAddMedia DISABLED_CallWithDataAndLaterAddMedia
#else
// Temporarily disable the test on all platforms. http://crbug.com/293252
#define MAYBE_CallWithDataAndLaterAddMedia DISABLED_CallWithDataAndLaterAddMedia
#endif

// This test will make a PeerConnection-based call and test an unreliable text
// dataChannel and later add an audio and video track.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CallWithDataAndLaterAddMedia) {
  MakeTypicalPeerConnectionCall("callWithDataAndLaterAddMedia();");
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithNewVideoMediaStream DISABLED_CallWithNewVideoMediaStream
#else
#define MAYBE_CallWithNewVideoMediaStream CallWithNewVideoMediaStream
#endif

// This test will make a PeerConnection-based call and send a new Video
// MediaStream that has been created based on a MediaStream created with
// getUserMedia.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CallWithNewVideoMediaStream) {
  MakeTypicalPeerConnectionCall("callWithNewVideoMediaStream();");
}

// This test will make a PeerConnection-based call and send a new Video
// MediaStream that has been created based on a MediaStream created with
// getUserMedia. When video is flowing, the VideoTrack is removed and an
// AudioTrack is added instead.
// TODO(phoglund): This test is manual since not all buildbots has an audio
// input.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MANUAL_CallAndModifyStream) {
  MakeTypicalPeerConnectionCall(
      "callWithNewVideoMediaStreamLaterSwitchToAudio();");
}

// This test calls getUserMedia in sequence with different constraints.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, TestGetUserMediaConstraints) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));

  std::vector<std::string> list_of_get_user_media_calls;
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 320, 320, 180, 180, 30, 30));
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 320, 320, 240, 240, 30, 30));
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 640, 640, 360, 360, 30, 30));
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 640, 640, 480, 480, 30, 30));
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 960, 960, 720, 720, 30, 30));
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 1280, 1280, 720, 720, 30, 30));
  list_of_get_user_media_calls.push_back(GenerateGetUserMediaCall(
      kGetUserMediaAndStop, 1920, 1920, 1080, 1080, 30, 30));

  for (std::vector<std::string>::iterator const_iterator =
           list_of_get_user_media_calls.begin();
       const_iterator != list_of_get_user_media_calls.end();
       ++const_iterator) {
    DVLOG(1) << "Calling getUserMedia: " << *const_iterator;
    NavigateToURL(shell(), url);
    ASSERT_TRUE(ExecuteJavascript(*const_iterator));
    ExpectTitle("OK");
  }
}

// This test calls getUserMedia and checks for aspect ratio behavior.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, TestGetUserMediaAspectRatio) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  GURL url(embedded_test_server()->GetURL("/media/getusermedia.html"));

  std::string constraints_4_3 = GenerateGetUserMediaCall(
      kGetUserMediaAndAnalyseAndStop, 640, 640, 480, 480, 30, 30);
  std::string constraints_16_9 = GenerateGetUserMediaCall(
      kGetUserMediaAndAnalyseAndStop, 640, 640, 360, 360, 30, 30);

  // TODO(mcasas): add more aspect ratios, in particular 16:10 crbug.com/275594.

  NavigateToURL(shell(), url);
  ASSERT_TRUE(ExecuteJavascript(constraints_4_3));
  ExpectTitle("4:3 letterbox");

  NavigateToURL(shell(), url);
  ASSERT_TRUE(ExecuteJavascript(constraints_16_9));
  ExpectTitle("16:9 letterbox");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, AddTwoMediaStreamsToOnePC) {
  MakeTypicalPeerConnectionCall("addTwoMediaStreamsToOneConnection();");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       EstablishAudioVideoCallAndMeasureOutputLevel) {
  if (!media::AudioManager::Get()->HasAudioOutputDevices()) {
    // Bots with no output devices will force the audio code into a different
    // path where it doesn't manage to set either the low or high latency path.
    // This test will compute useless values in that case, so skip running on
    // such bots (see crbug.com/326338).
    LOG(INFO) << "Missing output devices: skipping test...";
    return;
  }

  ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseFakeDeviceForMediaStream))
          << "Must run with fake devices since the test will explicitly look "
          << "for the fake device signal.";

  MakeTypicalPeerConnectionCall("callAndEnsureAudioIsPlaying();");
}

IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       EstablishAudioVideoCallAndVerifyMutingWorks) {
  if (!media::AudioManager::Get()->HasAudioOutputDevices()) {
    // Bots with no output devices will force the audio code into a different
    // path where it doesn't manage to set either the low or high latency path.
    // This test will compute useless values in that case, so skip running on
    // such bots (see crbug.com/326338).
    LOG(INFO) << "Missing output devices: skipping test...";
    return;
  }

  ASSERT_TRUE(CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kUseFakeDeviceForMediaStream))
          << "Must run with fake devices since the test will explicitly look "
          << "for the fake device signal.";

  MakeTypicalPeerConnectionCall("callAndEnsureAudioMutingWorks();");
}

#if defined(OS_WIN) || (defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY))
// Timing out on ARM linux bot: http://crbug.com/238490
// Failing on Windows: http://crbug.com/331035
#define MAYBE_CallWithAecDump DISABLED_CallWithAecDump
#else
#define MAYBE_CallWithAecDump CallWithAecDump
#endif

// This tests will make a complete PeerConnection-based call, verify that
// video is playing for the call, and verify that a non-empty AEC dump file
// exists. The AEC dump is enabled through webrtc-internals, in contrast to
// using a command line flag (tested in webrtc_aecdump_browsertest.cc). The HTML
// and Javascript is bypassed since it would trigger a file picker dialog.
// Instead, the dialog callback FileSelected() is invoked directly. In fact,
// there's never a webrtc-internals page opened at all since that's not needed.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest, MAYBE_CallWithAecDump) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // We must navigate somewhere first so that the render process is created.
  NavigateToURL(shell(), GURL(""));

  base::FilePath dump_file;
  ASSERT_TRUE(CreateTemporaryFile(&dump_file));

  // This fakes the behavior of another open tab with webrtc-internals, and
  // enabling AEC dump in that tab.
  WebRTCInternals::GetInstance()->FileSelected(dump_file, -1, NULL);

  GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
  NavigateToURL(shell(), url);

  EXPECT_TRUE(ExecuteJavascript("call({video: true, audio: true});"));
  ExpectTitle("OK");

  EXPECT_TRUE(base::PathExists(dump_file));
  int64 file_size = 0;
  EXPECT_TRUE(base::GetFileSize(dump_file, &file_size));
  EXPECT_GT(file_size, 0);

  base::DeleteFile(dump_file, false);
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// Timing out on ARM linux bot: http://crbug.com/238490
#define MAYBE_CallWithAecDumpEnabledThenDisabled DISABLED_CallWithAecDumpEnabledThenDisabled
#else
#define MAYBE_CallWithAecDumpEnabledThenDisabled CallWithAecDumpEnabledThenDisabled
#endif

// As above, but enable and disable dump before starting a call. The file should
// be created, but should be empty.
IN_PROC_BROWSER_TEST_F(WebrtcBrowserTest,
                       MAYBE_CallWithAecDumpEnabledThenDisabled) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  // We must navigate somewhere first so that the render process is created.
  NavigateToURL(shell(), GURL(""));

  base::FilePath dump_file;
  ASSERT_TRUE(CreateTemporaryFile(&dump_file));

  // This fakes the behavior of another open tab with webrtc-internals, and
  // enabling AEC dump in that tab, then disabling it.
  WebRTCInternals::GetInstance()->FileSelected(dump_file, -1, NULL);
  WebRTCInternals::GetInstance()->DisableAecDump();

  GURL url(embedded_test_server()->GetURL("/media/peerconnection-call.html"));
  NavigateToURL(shell(), url);

  EXPECT_TRUE(ExecuteJavascript("call({video: true, audio: true});"));
  ExpectTitle("OK");

  EXPECT_TRUE(base::PathExists(dump_file));
  int64 file_size = 0;
  EXPECT_TRUE(base::GetFileSize(dump_file, &file_size));
  EXPECT_EQ(0, file_size);

  base::DeleteFile(dump_file, false);
}


}  // namespace content
