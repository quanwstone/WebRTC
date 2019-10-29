#pragma once
#include<vector>
#include<Windows.h>
#include<mmsystem.h>
#pragma comment(lib,"winmm.lib")

struct STR_CONTEXT {
	LPWAVEHDR hd;
	bool iUser;
};

class CWAVOUT {

public:
	CWAVOUT();
	~CWAVOUT();

public:
	int m_iChannels;
	int m_iSampleRate;
	int m_iBit;

	HWAVEOUT m_phout;
	
	std::vector<STR_CONTEXT *>m_vectQueue;
	
	CRITICAL_SECTION	m_csLock;

public:
	bool initplay(int iSampleRate,int iChannels,int iBit);
	
	bool palyout(unsigned char *pData, int iLen);

	void destory();
private:
	
	LPWAVEHDR GetFreeHdr();

	MMRESULT AllocQueue(DWORD dLen);

	static void (CALLBACK callback)(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
};