#include<stdio.h>
#include"webrtc\modules\audio_conference_mixer\include\audio_conference_mixer.h"
#include"webrtc\modules\audio_conference_mixer\include\audio_conference_mixer_defines.h"

#define DEF_SAMPLE_RATE 16000
class AudioCallback :public webrtc::AudioMixerOutputReceiver
{
public:
	virtual void NewMixedAudio(const int32_t id,
		const webrtc::AudioFrame& generalAudioFrame,
		const webrtc::AudioFrame** uniqueAudioFrames,
		const uint32_t size)
	{
		printf("callback Len=%d\n",size);

		m_audioFrame.CopyFrom(generalAudioFrame);
	}
public:
	webrtc::AudioFrame m_audioFrame;
};

class Participant : public webrtc::MixerParticipant
{
public:
	Participant(int i) :m_id(i)
	{

	}

	virtual int32_t GetAudioFrame(int32_t id,webrtc::AudioFrame* audioFrame) 
	{
		printf("GetAudioFrame id=%d\n",id);
		
		audioFrame->CopyFrom(m_audioFrame);

		return 0;
	}

	virtual int32_t NeededFrequency(int32_t id) const
	{
		return DEF_SAMPLE_RATE;
	}
	
	void filldata(short *pdata,int iSample)
	{
		m_audioFrame.id_ = m_id;
		m_audioFrame.num_channels_ = 2;
		m_audioFrame.samples_per_channel_ = DEF_SAMPLE_RATE / 100;
		m_audioFrame.sample_rate_hz_ = DEF_SAMPLE_RATE;
		m_audioFrame.speech_type_ = webrtc::AudioFrame::kNormalSpeech;
		m_audioFrame.vad_activity_ = webrtc::AudioFrame::kVadActive;

		memcpy(m_audioFrame.data_, pdata, iSample * sizeof(short) * 2);
	}

public:
	webrtc::AudioFrame m_audioFrame;
	int m_id;
};

int main(int argc, char *argv[])
{
	int id = 0;
	int iSampleRate = 16000;
	int iChannels = 2;
	int iBit = 8;

	FILE *pf1 = nullptr, *pf2 = nullptr, *pf3 = nullptr, *pout = nullptr;

	errno_t er = fopen_s(&pf1, "../../1_1600_16_2.pcm", "rb+");

	er = fopen_s(&pf2, "../../2_1600_16_2.pcm", "rb+");
	
	er = fopen_s(&pf3, "../../3_1600_16_2.pcm", "rb+");

	er = fopen_s(&pout, "../../conferenceMix.pcm", "wb+");

	short *pB1 = new short[iSampleRate *2 / 100];//10ms sample
	short *pB2 = new short[iSampleRate * 2 / 100];
	short *pB3 = new short[iSampleRate * 2 / 100];

	webrtc::AudioConferenceMixer *pMixer = webrtc::AudioConferenceMixer::Create(id);

	Participant *pPar1 = new Participant(1);
	Participant *pPar2 = new Participant(2);
	Participant *pPar3 = new Participant(3);

	pMixer->SetMixabilityStatus(pPar1, true);
	pMixer->SetMixabilityStatus(pPar2, true);
	pMixer->SetMixabilityStatus(pPar3, true);

	AudioCallback *pCb = new AudioCallback;
	
	pMixer->RegisterMixedStreamCallback(pCb);

	int iR1 = fread(pB1, sizeof(short), iSampleRate * 2 / 100, pf1);
	int iR2 = fread(pB2, sizeof(short), iSampleRate * 2 / 100, pf2);
	int iR3 = fread(pB3, sizeof(short), iSampleRate * 2 / 100, pf3);

	while (iR1 && iR2 && iR3)
	{
		pPar1->filldata(pB1, iSampleRate / 100);
		pPar2->filldata(pB2, iSampleRate / 100);
		pPar3->filldata(pB3, iSampleRate / 100);

		pMixer->Process();

		fwrite(pCb->m_audioFrame.data(), sizeof(short), iSampleRate * 2/ 100, pout);

		iR1 = fread(pB1, sizeof(short), iSampleRate * 2 / 100, pf1);
		iR2 = fread(pB2, sizeof(short), iSampleRate * 2 / 100, pf2);
		iR3 = fread(pB3, sizeof(short), iSampleRate * 2 / 100, pf3);
	}

	fclose(pf1);
	fclose(pf2);
	fclose(pf3);
	fclose(pout);
	
	return 0;
}