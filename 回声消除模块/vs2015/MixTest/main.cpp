#include<stdio.h>
#include"webrtc/api/audio/audio_mixer.h"
#include"webrtc/modules/audio_mixer/audio_mixer_impl.h"
#include"webrtc/modules/audio_mixer/default_output_rate_calculator.h"

#define DEF_SAMPLE_RATE 16000

class AudioSrc : public webrtc::AudioMixer::Source
{
public:
	AudioSrc(int i):m_isrc(i)
	{

	}
	~AudioSrc()
	{

	}

	virtual AudioFrameInfo GetAudioFrameWithInfo(int sample_rate_hz,
		webrtc::AudioFrame* audio_frame) {
		
		printf("GetAudioFrameWithInfo %d\n",sample_rate_hz);

		audio_frame->CopyFrom(m_audioFrame);

		return AudioFrameInfo::kNormal;
	}

	virtual int Ssrc() const
	{
		return m_isrc;
	}

	virtual int PreferredSampleRate() const
	{
		return DEF_SAMPLE_RATE;
	}
	void fillaudioframe(short *pdata,int iSample)
	{
		m_audioFrame.samples_per_channel_ = DEF_SAMPLE_RATE / 100;
		m_audioFrame.sample_rate_hz_ = DEF_SAMPLE_RATE;
		m_audioFrame.id_ = 1;
		m_audioFrame.num_channels_ = 2;

		memcpy(m_audioFrame.data_, pdata, iSample * sizeof(short));

		m_audioFrame.vad_activity_ = webrtc::AudioFrame::kVadActive;
		m_audioFrame.speech_type_ = webrtc::AudioFrame::kNormalSpeech;
	}
	webrtc::AudioFrame m_audioFrame;
	int m_isrc;
};
int main(int argc, char *argv[])
{
	int iSampleRate = 16000;
	int iChannels = 2;
	int iBit = 16;

	FILE *pSrc1 = nullptr, *pSrc2 = nullptr, *pSrc3 = nullptr,*pOut = nullptr;

	errno_t er = fopen_s(&pSrc1, "../../1_1600_16_2.pcm", "rb+");

	er = fopen_s(&pSrc2, "../../2_1600_16_2.pcm", "rb+");

	er = fopen_s(&pSrc3, "../../3_1600_16_2.pcm", "rb+");

	er = fopen_s(&pOut, "../../mix.pcm", "wb+");


	short *pBuf1 = new short[iSampleRate *2/ 100];//10ms Sample
	short *pBuf2 = new short[iSampleRate *2/ 100];
	short *pBuf3 = new short[iSampleRate *2/ 100];

	auto mixptr = webrtc::AudioMixerImpl::Create();

	AudioSrc *src1 = new AudioSrc(1);
	AudioSrc *src2 = new AudioSrc(2);
	AudioSrc *src3 = new AudioSrc(3);

	webrtc::AudioFrame audioframe;

	int iR1 = fread(pBuf1, sizeof(short), iSampleRate *2/ 100, pSrc1);
	int iR2 = fread(pBuf2, sizeof(short), iSampleRate *2/ 100, pSrc2);
	int iR3 = fread(pBuf3, sizeof(short), iSampleRate *2/ 100, pSrc3);

	src1->fillaudioframe(pBuf1, iR1);
	src2->fillaudioframe(pBuf2, iR2);
	src3->fillaudioframe(pBuf3, iR3);

	mixptr->AddSource(src1);
	mixptr->AddSource(src2);
	mixptr->AddSource(src3);

	while (iR1 && iR2 && iR3)
	{
		mixptr->Mix(2, &audioframe);

		fwrite(audioframe.data(), sizeof(int16_t), iR1,pOut);

		iR1 = fread(pBuf1, sizeof(short), iSampleRate * 2/ 100,  pSrc1);
		iR2 = fread(pBuf2, sizeof(short), iSampleRate * 2/ 100,  pSrc2);
		iR3 = fread(pBuf3, sizeof(short), iSampleRate * 2 / 100, pSrc3);

		src1->fillaudioframe(pBuf1, iR1);
		src2->fillaudioframe(pBuf2, iR2);
		src3->fillaudioframe(pBuf3, iR3);
	}
	
	fclose(pSrc1);
	fclose(pSrc2);
	fclose(pSrc3);
	fclose(pOut);

	// Õ∑≈
	return 0;
}