#include "pch.h"
#include <iostream>
#include <windows.h>
#include "../DirScanModule/DirScanModule.h"

#pragma comment(lib, "DirScanModule.lib")

using std::cout;
using std::endl;

bool g_finishFlag = false;

void NotifyCallback(const char *dir, unsigned long long size)
{
	cout << "now scanning:" << dir << " size:" << size << endl;
}

void FinishCallback(unsigned long long fileNum, unsigned long long fileSize)
{
	cout << "scan finished, file num:" << fileNum << " fileSize:" << fileSize << endl;
	g_finishFlag = true;
}

int main()
{
	RegisterNotifyCB(NotifyCallback);
	RegisterNotifyFinishCB(FinishCallback);
	StartDirScan("f:\\World of Warcraft");

	char cTmp[MAX_PATH] = { 0 };
	while (!g_finishFlag)
	{
		std::cin >> cTmp;
		//reset scan
		if (cTmp[0] == 'r')
		{
			StopDirScan();

			StartDirScan("f:\\yy");
			break;
		}

		Sleep(10);
	}

	system("pause");
	return 0;
}