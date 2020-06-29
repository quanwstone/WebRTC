#include"windowscaputre.h"
#include"webrtc\modules\desktop_capture\desktop_capture_options.h"
#include"third_party\libyuv\x86-windows-cl14\include\libyuv.h"
#include"webrtc\base\messagequeue.h"
#include"webrtc\base\thread.h"

windowscaputre::windowscaputre()
{
	std::vector<cricket::VideoFormat>formats;
	formats.push_back(cricket::VideoFormat(800, 600, cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_I420));

	SetSupportedFormats(formats);
}
windowscaputre::~windowscaputre()
{

}

cricket::CaptureState windowscaputre::Start(const cricket::VideoFormat & capture_format)
{
	cricket::VideoFormat supported;
	
	if (GetBestCaptureFormat(capture_format, &supported))
	{
		SetCaptureFormat(&supported);
	}
	SetCaptureState(cricket::CS_RUNNING);

	auto options = webrtc::DesktopCaptureOptions::CreateDefault();
	options.set_allow_directx_capturer(true);
	
	capture = webrtc::DesktopCapturer::CreateScreenCapturer(options);
	capture->Start(this);
	
	CaptureFrame();

	return cricket::CS_RUNNING;
}

void windowscaputre::Stop()
{
	SetCaptureState(cricket::CS_STOPPED);
	SetCaptureFormat(NULL);
}

bool windowscaputre::IsRunning()
{
	return capture_state()==cricket::CS_RUNNING;
}

bool windowscaputre::IsScreencast() const
{
	return true;
}

bool windowscaputre::GetPreferredFourccs(std::vector<uint32_t>* fourccs)
{
	fourccs->push_back(cricket::FOURCC_I420);
	fourccs->push_back(cricket::FOURCC_MJPG);

	return false;
}

void windowscaputre::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame)
{
	if (result != webrtc::DesktopCapturer::Result::SUCCESS)
		return;

	int width = frame->size().width();
	int height = frame->size().height();

	if (!i420_buffer.get())
	{
		i420_buffer = webrtc::I420Buffer::Create(width, height);
	}
	libyuv::ConvertToI420(frame->data(),0,i420_buffer->MutableDataY(),
		i420_buffer->StrideY(),i420_buffer->MutableDataU(),
		i420_buffer->StrideU(),i420_buffer->MutableDataV(),
		i420_buffer->StrideV(),0,0,width,height,width,height,
		libyuv::kRotate0,libyuv::FOURCC_ABGR);
	OnFrame(webrtc::VideoFrame(i420_buffer, 0, 0, webrtc::kVideoRotation_0), width, height);
}

void windowscaputre::OnMessage(rtc::Message * msg)
{
	if (msg->message_id == 0)
	{
		CaptureFrame();
	}
}

void windowscaputre::CaptureFrame()
{
	capture->CaptureFrame();

	rtc::Location loc(__FUNCTION__, __FILE__);
	rtc::Thread::Current()->PostDelayed(loc, 33, this, 0);
}
