#pragma once
//#include "webrtc/api/mediastreaminterface.h"
#include"webrtc\media\base\videocapturer.h"
#include"webrtc\modules\desktop_capture\desktop_capturer.h"
#include"webrtc\base\messagehandler.h"

class windowscaputre:public cricket::VideoCapturer,public webrtc::DesktopCapturer::Callback,public rtc::MessageHandler
{
public:
	windowscaputre();
	~windowscaputre();

	//videocapture
	virtual cricket::CaptureState Start(const cricket::VideoFormat& capture_format)override;

	virtual void Stop()override;

	virtual bool IsRunning()override;

	virtual bool IsScreencast() const override;

	virtual bool GetPreferredFourccs(std::vector<uint32_t>* fourccs)override;

	//desktopcapture
	virtual void OnCaptureResult(webrtc::DesktopCapturer::Result result,
		std::unique_ptr<webrtc::DesktopFrame> frame)override;

	//message
	virtual void OnMessage(rtc::Message* msg)override;

	void CaptureFrame();
private:
	std::unique_ptr<webrtc::DesktopCapturer>capture;
	rtc::scoped_refptr<webrtc::I420Buffer>i420_buffer;
};

