/*
 * libjingle
 * Copyright 2004 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_MEDIA_DEVICES_WINRTDEVICEMANAGER_H_
#define TALK_MEDIA_DEVICES_WINRTDEVICEMANAGER_H_

#ifndef WINRT
#error Invalid build configuration
#endif  // WINRT

#include <string>
#include <vector>

#include "webrtc/media/devices/devicemanager.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/stringencode.h"
#include "webrtc/base/logging.h"

namespace cricket {

class WinRTDeviceManager : public DeviceManager {
 public:
  WinRTDeviceManager();
  virtual ~WinRTDeviceManager();

  // Initialization
  virtual bool Init();
  virtual void Terminate();

  virtual bool GetAudioInputDevices(std::vector<Device>* devices);
  virtual bool GetAudioOutputDevices(std::vector<Device>* devices);

  virtual bool GetVideoCaptureDevices(std::vector<Device>* devs);

 private:
  virtual bool GetDefaultVideoCaptureDevice(Device* device);
  void OnDeviceChange();

  static const char* kUsbDevicePathPrefix;

  ref class WinRTWatcher sealed {
   public:
    WinRTWatcher();
    void Start();
    void Stop();

   private:
    friend WinRTDeviceManager;
    WinRTDeviceManager* deviceManager_;
    Windows::Devices::Enumeration::DeviceWatcher^ videoCaptureWatcher_;
    Windows::Devices::Enumeration::DeviceWatcher^ videoAudioInWatcher_;
    Windows::Devices::Enumeration::DeviceWatcher^ videoAudioOutWatcher_;

    void OnVideoCaptureAdded(Windows::Devices::Enumeration::DeviceWatcher
      ^sender, Windows::Devices::Enumeration::DeviceInformation ^args);
    void OnVideoCaptureRemoved(Windows::Devices::Enumeration::DeviceWatcher
      ^sender, Windows::Devices::Enumeration::DeviceInformationUpdate ^args);

    void OnAudioInAdded(Windows::Devices::Enumeration::DeviceWatcher
      ^sender, Windows::Devices::Enumeration::DeviceInformation ^args);
    void OnAudioInRemoved(Windows::Devices::Enumeration::DeviceWatcher ^sender,
      Windows::Devices::Enumeration::DeviceInformationUpdate ^args);

    void OnAudioOutAdded(Windows::Devices::Enumeration::DeviceWatcher ^sender,
      Windows::Devices::Enumeration::DeviceInformation ^args);
    void OnAudioOutRemoved(Windows::Devices::Enumeration::DeviceWatcher
      ^sender, Windows::Devices::Enumeration::DeviceInformationUpdate ^args);

    void OnDeviceChange();
  };

  WinRTWatcher^ watcher_;
};

}  // namespace cricket

#endif  // TALK_MEDIA_DEVICES_WINRTDEVICEMANAGER_H_
