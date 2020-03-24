#include "stdafx.h"
#include "DirScanModule.h"
#include "PreDef.h"
#include <io.h>
#include <thread>

DirScan *DirScan::m_singleton = nullptr;

DirScan::DirScan()
	:m_fileNum(0)
	, m_fileSize(0)
	, m_curThreadNum(1)
	, m_terminateFlag(0)
	, m_notifyCallbackFunc(nullptr), m_notifyFinishCallbackFunc(nullptr)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	m_processorsNum = si.dwNumberOfProcessors;
	m_scanPathVec.resize(m_processorsNum);

	InitializeCriticalSection(&m_csPathVecLock);
}

DirScan::~DirScan()
{
	DeleteCriticalSection(&m_csPathVecLock);
}

DirScan* DirScan::SingletonPtr()
{
	return (nullptr == m_singleton) ? (m_singleton = new DirScan) : (m_singleton);
}

void DirScan::StartDirScan(const char *dir)
{
	auto res = InterlockedCompareExchange(&m_curThreadNum, m_curThreadNum, m_curThreadNum);
	EnterCriticalSection(&m_csPathVecLock);
	m_scanPathVec[res - 1] = dir;
	LeaveCriticalSection(&m_csPathVecLock);

	if ((res + 1) == m_processorsNum)
	{
		cout << "already reach the max threads limit" << endl;
	}
	
	//cout << "start scan thread:" << (res + 1) << endl;
	std::thread thread(&DirScan::DirScanThread, this, res);
	thread.detach();
}

void DirScan::DirScanThread(void *pData, unsigned long long tId)
{
	DirScan *pThis = (DirScan*)pData;
	EnterCriticalSection(&pThis->m_csPathVecLock);
	string strTmp = pThis->m_scanPathVec[tId - 1];
	LeaveCriticalSection(&pThis->m_csPathVecLock);
	pThis->DirScanProcess(strTmp.c_str());

	auto res = InterlockedCompareExchange(&pThis->m_curThreadNum, pThis->m_curThreadNum - 1, pThis->m_curThreadNum);
	if (res == 1 && pThis->m_notifyFinishCallbackFunc)
	{
		pThis->m_notifyFinishCallbackFunc(pThis->m_fileNum, pThis->m_fileSize);
	}
	//cout << "thread " << res << " stopped" << endl;
}

int DirScan::DirScanProcess(const char *dir)
{
	string strPath = dir;
	if (nullptr == dir || strcmp(dir, "") == 0)
	{
		return -1;
	}

	intptr_t hFile = 0;
	struct _finddata_t fileInfo;

	if ((hFile = _findfirst((strPath + "\\*").c_str(), &fileInfo)) == -1)
		return -2;

	do {
		if (fileInfo.attrib & _A_ARCH
			|| fileInfo.attrib & _A_NORMAL
			|| fileInfo.attrib & _A_RDONLY
			|| fileInfo.attrib & _A_HIDDEN
			|| fileInfo.attrib & _A_SYSTEM)
		{
			//cout << "Arch:" << strPath + "\\" + fileInfo.name << endl;
			InterlockedCompareExchange(&m_fileNum, m_fileNum + 1, m_fileNum);
			InterlockedCompareExchange(&m_fileSize, m_fileSize + fileInfo.size, m_fileSize);

			//when find new arch file, notify callback
			if (m_notifyCallbackFunc)
			{
				std::lock_guard<std::mutex> lock(m_notifyLock);
				m_notifyCallbackFunc((strPath + "\\" + fileInfo.name).c_str(), fileInfo.size);
			}
		}

		if (strcmp(fileInfo.name, "..") && strcmp(fileInfo.name, ".") && fileInfo.attrib & _A_SUBDIR)
		{
			//cout << "SubDir:" << strPath + "\\" + fileInfo.name << endl;

			auto res = InterlockedCompareExchange(&m_curThreadNum, m_curThreadNum, m_curThreadNum);
			if (res < m_processorsNum)
			{
				InterlockedCompareExchange(&m_curThreadNum, m_curThreadNum + 1, m_curThreadNum);
				//start the new scan thread
				StartDirScan((strPath + "\\" + fileInfo.name).c_str());
			}
			else
			{
				DirScanProcess((strPath + "\\" + fileInfo.name).c_str());
			}
		}

	} while (InterlockedCompareExchange(&m_terminateFlag, m_terminateFlag, m_terminateFlag) == 0 && _findnext(hFile, &fileInfo) == 0);

	_findclose(hFile);
	return 0;
}

void DirScan::StopDirScan()
{
	InterlockedCompareExchange(&m_terminateFlag, 1, m_terminateFlag);

	while (InterlockedCompareExchange(&m_curThreadNum, m_curThreadNum, m_curThreadNum) != 0)
	{
		Sleep(10);
	}

	Reset();
}

void DirScan::Reset()
{
	m_curThreadNum = 1;
	m_fileNum = 0;
	m_fileSize = 0;
	m_terminateFlag = 0;
}

void DIR_SCAN_MODULE RegisterNotifyCB(NotifyCallbackFunc func)
{
	DirScan::SingletonPtr()->RegisterNotifyCallbackFunc(func);
}

void DIR_SCAN_MODULE RegisterNotifyFinishCB(NotifyFinishCallbackFunc func)
{
	DirScan::SingletonPtr()->RegisterNotifyFinishCallbackFunc(func);
}

void DIR_SCAN_MODULE StartDirScan(const char *dir)
{
	DirScan::SingletonPtr()->StartDirScan(dir);
}

void DIR_SCAN_MODULE StopDirScan()
{
	DirScan::SingletonPtr()->StopDirScan();
}

unsigned long long GetFileNum()
{
	return DirScan::SingletonPtr()->GetFileNum();
}

unsigned long long GetFileSize()
{
	return DirScan::SingletonPtr()->GetFileSize();
}
