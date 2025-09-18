# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

from telemetry import test
from telemetry.page import page_measurement

from measurements import media


class MSEMeasurement(page_measurement.PageMeasurement):
  def MeasurePage(self, page, tab, results):
    media_metric = tab.EvaluateJavaScript('window.__testMetrics')
    trace = media_metric['id']
    metrics = media_metric['metrics']
    for m in metrics:
      if isinstance(metrics[m], list):
        values = [float(v) for v in metrics[m]]
      else:
        values = float(metrics[m])
      results.Add(trace, 'ms', values, chart_name=m)


class Media(test.Test):
  """Obtains media metrics for key user scenarios."""
  test = media.Media
  page_set = 'page_sets/tough_video_cases.json'

class MediaNetworkSimulation(test.Test):
  """Obtains media metrics under different network simulations."""
  test = media.Media
  enabled = not sys.platform.startswith('linux')
  page_set = 'page_sets/media_cns_cases.json'

class MediaAndroid(test.Test):
  """Obtains media metrics for key user scenarios on Android."""
  test = media.Media
  tag = 'android'
  enabled = not sys.platform.startswith('linux')
  page_set = 'page_sets/tough_video_cases.json'
  # Exclude crowd* media files (50fps 2160p).
  options = {
      'page_filter_exclude': '.*crowd.*'
  }

  def CustomizeBrowserOptions(self, options):
    # Needed to run media actions in JS in Android.
    options.AppendExtraBrowserArgs(
        '--disable-gesture-requirement-for-media-playback')

class MediaSourceExtensions(test.Test):
  """Obtains media metrics for key media source extensions functions."""
  test = media.Media
  enabled = not sys.platform.startswith('linux')
  test = MSEMeasurement
  page_set = 'page_sets/mse_cases.json'

  def CustomizeBrowserOptions(self, options):
    # Needed to allow XHR requests to return stream objects.
    options.AppendExtraBrowserArgs(
        '--enable-experimental-web-platform-features')
