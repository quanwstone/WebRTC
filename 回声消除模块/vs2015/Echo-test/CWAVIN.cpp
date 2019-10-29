#include"CWAVIN.h"

CWAVIN::CWAVIN()
{
	m_pIn = nullptr;
	m_hEvent = nullptr;
}
CWAVIN::~CWAVIN()
{
	if(m_pIn)
		waveInClose(m_pIn);

	if (m_hEvent)
		CloseHandle(m_hEvent);
}

bool CWAVIN::initcapture(int iSampleRate, int iChannels, int iBit)
{
	UINT idev = waveInGetNumDevs();
	if (idev == 0)
	{
		return false;
	}
	
	WAVEFORMATEX format{0};

	format.cbSize = 0;
	format.nChannels = iChannels;
	format.nSamplesPerSec = iSampleRate;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nBlockAlign = (iChannels *iBit) / 8;
	format.nAvgBytesPerSec = (iSampleRate * iChannels *iBit)/ (8);//sÊý¾Ý
	format.wBitsPerSample = iBit;

	MMRESULT re = waveInOpen(&m_pIn, WAVE_MAPPER, &format, (DWORD)callback, (DWORD)this, CALLBACK_FUNCTION);
	if (re != MMSYSERR_NOERROR)
	{
		return false;
	}

	for (int i = 0; i < 2; i++)
	{
		LPWAVEHDR phdr = new WAVEHDR();

		phdr->dwBufferLength = format.nAvgBytesPerSec;

		phdr->lpData = new char[phdr->dwBufferLength];
		
		memset(phdr->lpData, 0, phdr->dwBufferLength);
		
		phdr->dwBytesRecorded = 0;
		phdr->dwUser = 0;
		phdr->dwLoops = 0;
		phdr->lpNext = NULL;
		phdr->reserved = 0;

		re = waveInPrepareHeader(m_pIn, phdr, sizeof(WAVEHDR));
		if (re != MMSYSERR_NOERROR)
		{
			return false;
		}

		re = waveInAddBuffer(m_pIn, phdr, sizeof(WAVEHDR));
		if (re != MMSYSERR_NOERROR)
		{
			return false;
		}
	}

	m_hEvent = CreateEvent(nullptr, true, false, nullptr);

	re = waveInStart(m_pIn);
	if (re != MMSYSERR_NOERROR)
	{
		return false;
	}

	return true;
}

void(CWAVIN::callback)(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CWAVIN *pdlg = (CWAVIN *)dwUser;

	if (uMsg == WIM_DATA)
	{
		pdlg->m_vecHDR.push_back((LPWAVEHDR)dw1);
		SetEvent(pdlg->m_hEvent);
	}
}
bool CWAVIN::getcapturedata(char *pdata, int &iLen)
{

	DWORD dre = WaitForSingleObject(m_hEvent, 1000);//10ms
	if (dre != WAIT_OBJECT_0)
	{
		return false;
	}

	LPWAVEHDR phd = *(m_vecHDR.begin());
	m_vecHDR.erase(m_vecHDR.begin());

	waveInUnprepareHeader(m_pIn, phd, sizeof(WAVEHDR));

	memcpy(pdata, phd->lpData, phd->dwBytesRecorded);
	iLen = phd->dwBytesRecorded;

	MMRESULT re = waveInPrepareHeader(m_pIn, phd, sizeof(WAVEHDR));
	if (re != MMSYSERR_NOERROR)
	{
		return false;
	}
	re = waveInAddBuffer(m_pIn, phd, sizeof(WAVEHDR));
	if (re != MMSYSERR_NOERROR)
	{
		return false;
	}

	ResetEvent(m_hEvent);

	return true;
}