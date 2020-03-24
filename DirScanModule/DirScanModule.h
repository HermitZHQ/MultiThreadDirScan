#pragma once
#include <mutex>
#include <vector>
#include <string>

#ifdef DllExport
#define DIR_SCAN_MODULE __declspec(dllexport)
#else
#define DIR_SCAN_MODULE __declspec(dllimport)
#endif

typedef void(*NotifyCallbackFunc)(const char *dir, unsigned long long size);
typedef void(*NotifyFinishCallbackFunc)(unsigned long long fileNum, unsigned long long fileSize);

class DirScan
{
public:
	DirScan();

	static DirScan* SingletonPtr();

	void RegisterNotifyCallbackFunc(NotifyCallbackFunc func) {
		if (nullptr == func)
			return;

		m_notifyCallbackFunc = func;
	}

	void RegisterNotifyFinishCallbackFunc(NotifyFinishCallbackFunc func) {
		if (nullptr == func)
			return;

		m_notifyFinishCallbackFunc = func;
	}

	void StartDirScan(const char *dir);
	static void DirScanThread(void *pData, unsigned long long pId);
	int DirScanProcess(const char *dir);
	void StopDirScan();
	void Reset();

	unsigned long long GetFileNum() const {
		return m_fileNum;
	}

	unsigned long long GetFileSize() const {
		return m_fileSize;
	}

protected:
	~DirScan();

private:
	static DirScan								*m_singleton;
	unsigned long long							m_fileNum;
	unsigned long long							m_fileSize;

	std::vector<std::string>					m_scanPathVec;

	unsigned int								m_processorsNum;
	unsigned long long							m_curThreadNum;

	NotifyCallbackFunc							m_notifyCallbackFunc;
	NotifyFinishCallbackFunc					m_notifyFinishCallbackFunc;

	unsigned long long							m_terminateFlag;
	std::mutex									m_notifyLock;
	CRITICAL_SECTION							m_csPathVecLock;
};

void DIR_SCAN_MODULE RegisterNotifyCB(NotifyCallbackFunc func);
void DIR_SCAN_MODULE RegisterNotifyFinishCB(NotifyFinishCallbackFunc func);
void DIR_SCAN_MODULE StartDirScan(const char *dir);
void DIR_SCAN_MODULE StopDirScan();
unsigned long long GetFileNum();
unsigned long long GetFileSize();