// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MIDI_MIDI_MANAGER_ALSA_H_
#define MEDIA_MIDI_MIDI_MANAGER_ALSA_H_

#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/threading/thread.h"
#include "media/midi/midi_manager.h"

namespace media {

class MIDIManagerAlsa : public MIDIManager {
 public:
  MIDIManagerAlsa();
  virtual ~MIDIManagerAlsa();

  // MIDIManager implementation.
  virtual bool Initialize() OVERRIDE;
  virtual void DispatchSendMIDIData(MIDIManagerClient* client,
                                    uint32 port_index,
                                    const std::vector<uint8>& data,
                                    double timestamp) OVERRIDE;

 private:
  class MIDIDeviceInfo;
  std::vector<scoped_refptr<MIDIDeviceInfo> > in_devices_;
  std::vector<scoped_refptr<MIDIDeviceInfo> > out_devices_;
  base::Thread send_thread_;
  DISALLOW_COPY_AND_ASSIGN(MIDIManagerAlsa);
};

}  // namespace media

#endif  // MEDIA_MIDI_MIDI_MANAGER_ALSA_H_
