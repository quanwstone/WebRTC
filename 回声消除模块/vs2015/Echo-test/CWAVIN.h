#pragma once
#include<Windows.h>
#include<mmsystem.h>
#include<vector>
#pragma comment(lib ,"winmm.lib")

class CWAVIN
{
public:
	CWAVIN();
	~CWAVIN();

private:
	HWAVEIN m_pIn;
	
	HANDLE m_hEvent;
	
	static void (CALLBACK callback)(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	std::vector<LPWAVEHDR>m_vecHDR;
public:
	bool initcapture(int iSampleRate, int iChannels, int iBit);

	bool getcapturedata(char *pdata,int &iLen);
};