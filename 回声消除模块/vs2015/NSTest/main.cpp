#include<stdio.h>
#include"webrtc\modules\audio_processing\ns\noise_suppression.h"
#include"webrtc\modules\audio_processing\audio_buffer.h"

int main(int argc, char *argv[])
{
	int iSampleRate = 16000;
	int iChannels = 2;
	int iBit = 16;

	FILE *pSrc = nullptr,*pOut = nullptr;

	errno_t er = fopen_s(&pSrc, "../../1_1600_16_2.pcm", "rb+");

	er = fopen_s(&pOut,"../../ns.pcm","wb+");

	NsHandle *phandle = WebRtcNs_Create();

	int ir = WebRtcNs_Init(phandle, iSampleRate);
	
	ir = WebRtcNs_set_policy(phandle, 0);

	short *pbuf = new short[iSampleRate  / 100];
	float *pbuff = new float[iSampleRate  / 100];
	float *pbout = new float[iSampleRate  / 100];

	int iR = fread(pbuf, sizeof(short), iSampleRate  / 100, pSrc);
	while (iR)
	{
		webrtc::S16ToFloat((int16_t*)pbuf,iSampleRate /100,pbuff);

		WebRtcNs_Analyze(phandle, pbuff);

		WebRtcNs_Process(phandle, &pbuff, 1, &pbout);
		
		fwrite(pbout, sizeof(float), iSampleRate  / 100, pOut);

		iR = fread(pbuf, sizeof(short), iSampleRate  / 100, pSrc);
	}

	WebRtcNs_Free(phandle);

	fclose(pSrc);
	fclose(pOut);

	return 0;
}