#include"CWAVOUT.h"

CWAVOUT::CWAVOUT()
{
	m_phout = nullptr;
}

CWAVOUT::~CWAVOUT()
{

}
void CWAVOUT::destory()
{
	DeleteCriticalSection(&m_csLock);

	waveOutClose(m_phout);
}

bool CWAVOUT::initplay(int iSampleRate, int iChannels, int iBit)
{
	m_iSampleRate = iSampleRate;
	m_iChannels = iChannels;
	m_iBit = iBit;

	int id = waveOutGetNumDevs();
	if (id == 0)
	{
		return false;
	}

	WAVEFORMATEX format{0};

	format.cbSize = 0;
	format.nBlockAlign = (iChannels * iBit ) / 8;
	format.wBitsPerSample = iBit;
	format.nChannels = iChannels;
	format.nSamplesPerSec = iSampleRate;
	format.nAvgBytesPerSec = (iChannels * iSampleRate * iBit) / (8);//每s的平均速率.该值需要和填充的字节相同
	format.wFormatTag = WAVE_FORMAT_PCM;

	id = waveOutOpen(&m_phout, WAVE_MAPPER, &format, (DWORD)callback, DWORD(this), CALLBACK_FUNCTION);
	if (id != MMSYSERR_NOERROR)
	{
		return false;
	}

	InitializeCriticalSection(&m_csLock);

	AllocQueue(format.nAvgBytesPerSec);
	
	return true;
}
bool CWAVOUT::palyout(unsigned char *pData, int iLen)
{
	MMRESULT re = MMSYSERR_NOERROR;

	LPWAVEHDR lp = nullptr;
	do
	{
		EnterCriticalSection(&m_csLock);
		lp = GetFreeHdr();
		LeaveCriticalSection(&m_csLock);

		if (!lp)
		{
			Sleep(1000);
		}

	} while (!lp);

	lp->dwBufferLength = iLen;
	lp->dwFlags = 0;
	memcpy(lp->lpData, pData, iLen);


	re = waveOutPrepareHeader(m_phout, lp, sizeof(WAVEHDR));
	if (re != MMSYSERR_NOERROR)
	{
		printf("Play Failed\n");
	}

	re = waveOutWrite(m_phout, lp, sizeof(WAVEHDR));//WAVERR_UNPREPARED 
	if (re != MMSYSERR_NOERROR)
	{
		printf("PutWaveOut Failed\n");
	}

	return re;
}
void CWAVOUT::callback(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CWAVOUT *dlg = (CWAVOUT *)dwUser;

	if (uMsg == WOM_DONE)
	{
		printf("in callbackProc\n");

		//回调函数调用该方法是否会存在死锁的情况。 
		waveOutUnprepareHeader(dlg->m_phout, (LPWAVEHDR)dw1, sizeof(WAVEHDR));

		EnterCriticalSection(&dlg->m_csLock);
		for (std::vector<STR_CONTEXT *>::iterator iter = dlg->m_vectQueue.begin();
			iter != dlg->m_vectQueue.end(); iter++)
		{
			if ((*iter)->hd == (LPWAVEHDR)dw1)
			{
				(*iter)->iUser = false;
			}
		}
		LeaveCriticalSection(&dlg->m_csLock);
	}
}

LPWAVEHDR CWAVOUT::GetFreeHdr()
{
	for (std::vector<STR_CONTEXT *>::iterator iter = m_vectQueue.begin(); iter != m_vectQueue.end(); iter++)
	{
		if ((*iter)->iUser == false)
		{
			(*iter)->iUser = true;
			return (*iter)->hd;
		}
	}
	return nullptr;
}

MMRESULT CWAVOUT::AllocQueue(DWORD dLen)
{
	MMRESULT re = MMSYSERR_NOERROR;

	//申请两个的原因是waveOutWrite内部会缓存一个HDR.   
	for (int i = 0; i < 2; i++)
	{
		STR_CONTEXT *sCon = new STR_CONTEXT;

		sCon->hd = new WAVEHDR();
		sCon->hd->lpData = new char[dLen];
		sCon->hd->dwFlags = 0;
		sCon->iUser = false;

		memset(sCon->hd->lpData, 0, dLen);

		m_vectQueue.push_back(sCon);
	}
	return re;
}
