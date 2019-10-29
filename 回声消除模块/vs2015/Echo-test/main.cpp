#include<stdio.h>
#include<thread>
#include"webrtc\modules\audio_processing\aec\echo_cancellation.h"
#include"webrtc\modules\audio_processing\audio_buffer.h"
#include"CWAVOUT.h"
#include"CWAVIN.h"

typedef struct STU_AUDIO {
	
	char *pdata;
	int iLen;
	DWORD iTime;

}STU_AUDIO;
std::vector<STU_AUDIO *>m_vecCapt;
std::vector<STU_AUDIO *>m_vecPlay;

bool g_Run;

void PackRenderAudioBuffer(
	const webrtc::AudioBuffer* audio,
	size_t num_output_channels,
	size_t num_channels,
	std::vector<float>* packed_buffer)
{

	RTC_DCHECK_GE(160, audio->num_frames_per_band());
	RTC_DCHECK_EQ(num_channels, audio->num_channels());

	packed_buffer->clear();

	// ֻҪ low band
	packed_buffer->insert(packed_buffer->end(),
		audio->split_bands_const_f(0)[0],
		(audio->split_bands_const_f(0)[0] +
			audio->num_frames_per_band()));


}
void ProcessRenderAudio(void* aec, rtc::ArrayView<const float> packed_render_audio, int num_frames_per_band)
{
	webrtc::WebRtcAec_BufferFarend(aec, &packed_render_audio[0], num_frames_per_band);
}

int ProcessCaptureAudio(void* aec, webrtc::AudioBuffer* audio, int stream_delay_ms)
{
	RTC_DCHECK_GE(160, audio->num_frames_per_band());
	RTC_DCHECK_EQ(audio->num_channels(), 1);

	int err = webrtc::AudioProcessing::kNoError;

	//
	bool stream_has_echo_ = false;

	err = webrtc::WebRtcAec_Process(
		aec, audio->split_bands_const_f(0),
		audio->num_bands(), audio->split_bands_f(0),
		audio->num_frames_per_band(), stream_delay_ms, 0);

	if (err != webrtc::AudioProcessing::kNoError)
	{
		// TODO(ajm): Figure out how to return warnings properly.
		if (err != webrtc::AudioProcessing::kBadStreamParameterWarning)
		{
			return err;
		}
	}

	int status = 0;
	err = webrtc::WebRtcAec_get_echo_status(aec, &status);
	if (err != webrtc::AudioProcessing::kNoError)
	{
		return  err;
	}

	if (status == 1)
	{
		stream_has_echo_ = true;
	}
	return webrtc::AudioProcessing::kNoError;
}
void ProcessOneFrame(void* aec, int sample_rate_hz,
	int stream_delay_ms,
	webrtc::AudioBuffer* render_audio_buffer,
	webrtc::AudioBuffer* capture_audio_buffer)
{
	if (sample_rate_hz > webrtc::AudioProcessing::kSampleRate16kHz) {
		render_audio_buffer->SplitIntoFrequencyBands();
		capture_audio_buffer->SplitIntoFrequencyBands();
	}

	std::vector<float> render_audio;

	PackRenderAudioBuffer(render_audio_buffer, 1, render_audio_buffer->num_channels(), &render_audio);

	ProcessRenderAudio(aec, render_audio, render_audio_buffer->num_frames_per_band());

	ProcessCaptureAudio(aec, capture_audio_buffer, stream_delay_ms);

	if (sample_rate_hz > webrtc::AudioProcessing::kSampleRate16kHz) {

		capture_audio_buffer->MergeFrequencyBands();
	}
}
void thread_capture(FILE *p, 
	CWAVIN *pwavin, 
	int iSampleRate, 
	int iChannels, 
	int iBit
	)
{
	int iLen = (iSampleRate * iChannels *iBit) / (8);//s数据
	
	char *data = new char[iLen];

	while (g_Run)
	{
		bool bre = pwavin->getcapturedata(data, iLen);
		
		if (bre)
		{
			STU_AUDIO *pda = new STU_AUDIO;

			pda->pdata = new char[iLen];
			
			memcpy(pda->pdata,data,iLen);
			pda->iLen = iLen;
			pda->iTime = GetTickCount();

			m_vecCapt.push_back(pda);

			fwrite(data, 1, iLen, p);
		}
	}	
	delete[] data;
}
void thread_play(FILE *p, 
	CWAVOUT *pwavout,
	int iSampleRate,
	int iChannels,
	int iBit
	)
{
	FILE *pFile = p;

	int iLen = (iSampleRate * iBit * iChannels)/ (8);//读取数据长度为s

	char *data = new char[iLen];

	int ir = fread(data, 1, iLen, pFile);
	while (ir)
	{
		DWORD it = GetTickCount();

		pwavout->palyout((unsigned char *)data, ir);

		STU_AUDIO *pda = new STU_AUDIO;
		
		pda->pdata = new char[ir];
		
		memcpy(pda->pdata, data, ir);
		pda->iLen = ir;
		pda->iTime = GetTickCount();

		m_vecPlay.push_back(pda);

		printf("Play It = %d\n",pda->iTime - it);
		ir = fread(data, 1, iLen, pFile);
	}
	
	delete[] data;

	g_Run = false;
}
void thread_process(
	FILE *pOut,
	int iSampleRate,
	int iChannels,
	int iBit,
	webrtc::AudioBuffer *pfar,
	webrtc::AudioBuffer *pnear,
	webrtc::StreamConfig config,
	void *phead)
{
	int iSample = iSampleRate *10 /(1000);//10ms Sample
	int iSampleBit = iSampleRate * 10 * iBit / (8 * 1000);

	float *pcapf = new float[iSample];
	float *pplaf = new float[iSample];
	float *ppout = new float[iSample];

	char *pcap10 = new char[iSampleBit];
	char *pplay10 = new char[iSampleBit];

	while (g_Run)
	{
		if (m_vecCapt.empty() || m_vecPlay.empty())
		{
			Sleep(10);
			continue;
		}
		
		STU_AUDIO *pcapture = m_vecCapt.at(0);
		STU_AUDIO *pplay = m_vecPlay.at(0);

		char *pcap = pcapture->pdata;
		int icap = pcapture->iLen;
		DWORD itc = pcapture->iTime;

		char *ppla = pplay->pdata;
		int ipla = pplay->iLen;
		DWORD itp = pplay->iTime;

		//互斥
		m_vecCapt.erase(m_vecCapt.begin());
		m_vecPlay.erase(m_vecPlay.begin());

		while (ipla && icap)
		{
			memcpy(pcap10, pcap, iSampleBit);
			memcpy(pplay10, ppla, iSampleBit);

			webrtc::S16ToFloat((int16_t*)pcap10, iSample, pcapf);
			webrtc::S16ToFloat((int16_t*)pplay10, iSample, pplaf);

			pfar->CopyFrom(&pplaf, config);
			pnear->CopyFrom(&pcapf, config);

			ProcessOneFrame(phead, iSampleRate, 10, pfar, pnear);

			pnear->CopyTo(config, &ppout);

			fwrite(ppout, sizeof(float), iSample, pOut);

			ipla -= iSampleBit;
			icap -= iSampleBit;

			pcap += iSampleBit;
			ppla += iSampleBit;
		}
	}
	//释放
}

int main(int argc, char *argv[])
{
	int32_t iSampleRate = 16000;
	int32_t iChannels = 2;
	int32_t iBit = 16;
	g_Run = true;

	void *phead = webrtc::WebRtcAec_Create();
	if (phead == nullptr)
	{
		printf("");
	}

	int ir = webrtc::WebRtcAec_Init(phead, iSampleRate, iSampleRate);
	if (ir != 0)
	{
		printf("");
	}

	webrtc::StreamConfig config;
	config.set_sample_rate_hz(iSampleRate);
	config.set_num_channels(1);
	config.set_has_keyboard(false);

	webrtc::AudioBuffer *pfar = nullptr,*pnear = nullptr;

	pfar = new webrtc::AudioBuffer(config.num_frames(), config.num_channels(),
		config.num_frames(), 1, config.num_frames());

	pnear = new webrtc::AudioBuffer(config.num_frames(), config.num_channels(),
		config.num_frames(), 1, config.num_frames());

	FILE *pFar = nullptr, *pNear = nullptr,*pOut = nullptr;

	errno_t er = fopen_s(&pFar, "../../1_1600_16_2.pcm", "rb+");//用于播放
	if (er != 0)
	{
		printf("");
	}
	
	er = fopen_s(&pNear, "../../1_1600_16_2_near.pcm", "rb+");//包含回声
	if (er != 0)
	{
		printf("");
	}

	er = fopen_s(&pOut, "../../out.pcm", "wb+");
	if (er != 0)
	{
		printf("");
	}
#if 1
	CWAVOUT *waveOut = new CWAVOUT();

	waveOut->initplay(iSampleRate, iChannels, iBit);

	std::thread play(thread_play,pFar,waveOut,iSampleRate,iChannels,iBit);
	play.detach();

	CWAVIN *waveIn = new CWAVIN();

	waveIn->initcapture(iSampleRate, iChannels, iBit);

	std::thread capture(thread_capture,pNear,waveIn, iSampleRate, iChannels, iBit);
	capture.detach();

	std::thread process(thread_process,pOut, iSampleRate, iChannels, iBit, pfar, pnear, config, phead);
	process.detach();

#endif

#if 0
	////每次读取sample个数 10ms单通道
	int iLen = 160;//10读取的sample个数
	short *pfarB = new short[iLen];
	short *pnearB = new short[iLen];
	float *pfarf = new float[iLen];
	float *pnearf = new float[iLen];
	float *poutf = new float[iLen];

	int iRfar = fread(pfarB, sizeof(short), iLen, pFar);
	int iRnear = fread(pnearB, sizeof(short), iLen, pNear);

	while (iRfar > 0 && iRnear > 0)
	{
		webrtc::S16ToFloat((int16_t*)pfarB, iLen, pfarf);
		webrtc::S16ToFloat((int16_t*)pnearB, iLen, pnearf);

		pfar->CopyFrom(&pfarf, config);
		pnear->CopyFrom(&pnearf, config);

		ProcessOneFrame(phead, iSampleRate, 100, pfar, pnear);

		pnear->CopyTo(config, &poutf);

		fwrite(poutf, sizeof(float), iLen, pOut);

		iRfar = fread(pfarB, sizeof(short), iLen , pFar);
		iRnear = fread(pnearB, sizeof(short), iLen, pNear);
	}
#endif 

	for (;;);

	fclose(pFar);
	fclose(pNear);
	fclose(pOut);

	return 0;
}