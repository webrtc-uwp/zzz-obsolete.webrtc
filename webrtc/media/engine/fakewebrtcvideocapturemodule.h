/*
 *  Copyright (c) 2004 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MEDIA_ENGINE_FAKEWEBRTCVIDEOCAPTUREMODULE_H_
#define WEBRTC_MEDIA_ENGINE_FAKEWEBRTCVIDEOCAPTUREMODULE_H_

#include <vector>

#include "webrtc/media/base/testutils.h"
#include "webrtc/media/engine/fakewebrtcdeviceinfo.h"
#include "webrtc/media/engine/webrtcvideocapturer.h"

class FakeWebRtcVcmFactory;

// Fake class for mocking out webrtc::VideoCaptureModule.
class FakeWebRtcVideoCaptureModule : public webrtc::VideoCaptureModule {
 public:
  FakeWebRtcVideoCaptureModule(FakeWebRtcVcmFactory* factory, int32_t id)
      : factory_(factory),
        id_(id),
        callback_(NULL),
        running_(false),
        delay_(0) {
  }
  int64_t TimeUntilNextProcess() override { return 0; }
  void Process() override {}
  void RegisterCaptureDataCallback(
      webrtc::VideoCaptureDataCallback& callback) override {
    callback_ = &callback;
  }
  void DeRegisterCaptureDataCallback() override { callback_ = NULL; }
  void RegisterCaptureCallback(
      webrtc::VideoCaptureFeedBack& callback) override {
    // Not implemented.
  }
  void DeRegisterCaptureCallback() override {
    // Not implemented.
  }
  void SetCaptureDelay(int32_t delay) override { delay_ = delay; }
  int32_t CaptureDelay() override { return delay_; }
  void EnableFrameRateCallback(const bool enable) override {
    // not implemented
  }
  void EnableNoPictureAlarm(const bool enable) override {
    // not implemented
  }
  int32_t StartCapture(const webrtc::VideoCaptureCapability& cap) override {
    if (running_) return -1;
    cap_ = cap;
    running_ = true;
    return 0;
  }
  int32_t StopCapture() override {
    running_ = false;
    return 0;
  }

  bool SuspendCapture() override {
    // not implemented
    return false;
  }

  bool ResumeCapture() override {
    // not implemented
    return false;
  }

  bool IsSuspended() override {
    // not implemented
    return false;
  }

  const char* CurrentDeviceName() const override {
    return NULL;  // not implemented
  }
  bool CaptureStarted() override { return running_; }
  int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) override {
    if (!running_) return -1;
    settings = cap_;
    return 0;
  }

  int32_t SetCaptureRotation(webrtc::VideoRotation rotation) override {
    return -1;  // not implemented
  }
  bool SetApplyRotation(bool enable) override {
    return false;  // not implemented
  }
  bool GetApplyRotation() override {
    return true;  // Rotation compensation is turned on.
  }
  VideoCaptureEncodeInterface* GetEncodeInterface(
      const webrtc::VideoCodec& codec) override {
    return NULL;  // not implemented
  }
  int32_t AddRef() const override { return 0; }
  int32_t Release() const override {
    delete this;
    return 0;
  }

  void SendFrame(int w, int h) {
    if (!running_) return;
    webrtc::VideoFrame sample;
    // Setting stride based on width.
    sample.CreateEmptyFrame(w, h, w, (w + 1) / 2, (w + 1) / 2);
    if (callback_) {
      callback_->OnIncomingCapturedFrame(id_, sample);
    }
  }

  const webrtc::VideoCaptureCapability& cap() const {
    return cap_;
  }

 private:
  // Ref-counted, use Release() instead.
  ~FakeWebRtcVideoCaptureModule();

  FakeWebRtcVcmFactory* factory_;
  int id_;
  webrtc::VideoCaptureDataCallback* callback_;
  bool running_;
  webrtc::VideoCaptureCapability cap_;
  int delay_;
};

#endif  // WEBRTC_MEDIA_ENGINE_FAKEWEBRTCVIDEOCAPTUREMODULE_H_
