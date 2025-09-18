// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/snapshot/snapshot.h"

#include "base/callback.h"
#include "ui/gfx/rect.h"

namespace ui {

bool GrabViewSnapshot(gfx::NativeView view,
                      std::vector<unsigned char>* png_representation,
                      const gfx::Rect& snapshot_bounds) {
  // TODO(bajones): Implement iOS snapshot functionality
  return false;
}

bool GrabWindowSnapshot(gfx::NativeWindow window,
                        std::vector<unsigned char>* png_representation,
                        const gfx::Rect& snapshot_bounds) {
  // TODO(bajones): Implement iOS snapshot functionality
  return false;
}

SNAPSHOT_EXPORT void GrapWindowSnapshotAsync(
    gfx::NativeWindow window,
    const gfx::Rect& snapshot_bounds,
    const gfx::Size& target_size,
    scoped_refptr<base::TaskRunner> background_task_runner,
    GrapWindowSnapshotAsyncCallback callback) {
  NOTIMPLEMENTED();
}

}  // namespace ui
