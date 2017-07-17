// SensLockTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <time.h>
#include "EleInterface.h"
using std::cout;
using std::endl;

BOOL ReadFile(char *pucFilePath, unsigned long &pulFileLength, unsigned char *pucFileContent)
{
	FILE *stream = NULL;
	unsigned long ulRead = 0;
	if (errno_t err = fopen_s(&stream, pucFilePath, "rb"))
	{
		cout << "open file faild" << endl;
		return FALSE;
	}
	fseek(stream, 0, SEEK_END);
	pulFileLength = ftell(stream);
	fseek(stream, 0, SEEK_SET);
	ulRead = fread(pucFileContent, 0x1, pulFileLength, stream);
	fclose(stream);
	if (ulRead != pulFileLength)
	{
		return FALSE;
	}
	return TRUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	CEleInterface objA;
	ELE_DEVICE_CONTEXT EleCtx = { 0 };
	int Index = 0;
	EleCtx.ulSize = sizeof(ELE_DEVICE_CONTEXT);
	char bOutBuf[9] = { 0 };
	int dwRealLen = 0;
	char pcModuleNameBuffer[20] = { 0 };
	int pulModuleNameLen;
	int pulIndex;

	char test[10] = { 4, 3, 8, 2, 9, 7, 1, 5, 0, 6 };
	int len = sizeof(test);
	int size = 0;

	time_t temp = 0;
	tm newtime;
	char buffer[32] = { 0 };

	int res = 0;
	int err = 0;

	res = objA.EleOpenFirstDevice(NULL, NULL, NULL, 0, &EleCtx);
	if (!res)
	{
		cout << "open device error !" << endl;
		return 0;
	}
	res = objA.EleGetFirstModuleName(&EleCtx, pcModuleNameBuffer, 50, &pulModuleNameLen, &pulIndex);
	if (!res)
	{
		cout << "get module name errror !" << endl;
		return 0;
	}
	cout << pcModuleNameBuffer << endl;
	res = objA.EleControl(&EleCtx, ELE_SET_LED_UP, NULL, 0, bOutBuf, 8, dwRealLen);
	if (!res)
	{
		cout << "set device error !" << endl;
		return 0;
	}
	res = objA.EleExecute(&EleCtx, pcModuleNameBuffer, test, len, test, len, &size);
	if (!res)
	{
		cout << "run module error !" << endl;
		return 0;
	}
	for (int i = 0; i < len; i++)
	{
		printf("%d  ", test[i]);
	}
	printf("\n");

	ELE_LIBVERSIONINFO E3VerInfo = { 0 };
	E3VerInfo.ulVersionInfoSize = sizeof(ELE_LIBVERSIONINFO);
	objA.EleGetVersion(&E3VerInfo);

	memset(pcModuleNameBuffer, 0, sizeof(pulModuleNameLen));
	objA.EleControl(&EleCtx, ELE_GET_VENDOR_DESC, NULL, 0, bOutBuf, 8, dwRealLen);
	cout << bOutBuf << endl;

	objA.EleControl(&EleCtx, ELE_GET_MODIFY_TIME, NULL, 0, (char*)(&temp), 8, dwRealLen);
	gmtime_s(&newtime, &temp);
	asctime_s(buffer, 32, &newtime);
	printf("The lock is made at %s\n", buffer);

	memset(bOutBuf, 0, sizeof(bOutBuf));
	objA.EleControl(&EleCtx, ELE_GET_MODULE_SIZE, NULL, 0, bOutBuf, 4, dwRealLen);

	unsigned char Pin[16] = { 0 };
	memcpy(Pin, "1111111111111111", 16);
	int bResult = objA.EleVerifyPin(&EleCtx, Pin);
	if (!bResult)
	{
		err = GetLastError();
		printf("校验PIN码失败0x%x\n", err);
		objA.EleClose(&EleCtx);
		return 0;
	}

	int ulRealLen = 255;
	unsigned char DescribBuffer[20] = { 0 };
	unsigned long DescribBufferLen = 8;
	memcpy(DescribBuffer, "87654321", 8);
	bResult = objA.EleControl(&EleCtx, ELE_SET_VENDOR_DESC, DescribBuffer, DescribBufferLen, NULL, 0, ulRealLen);
	if (!bResult)
	{
		err = GetLastError();
		printf("设置厂商描述失败0x%x\n", err);
		return 0;
	}

	//更改模块名
	bResult = objA.EleChangeModuleName(&EleCtx, "BUBBLESORT", "BUBBLESORT");
	if (!bResult)
	{
		printf("更改模块名失败0x%x\n", err);
		return 0;
	}
	objA.EleGetFirstModuleName(&EleCtx, pcModuleNameBuffer, 50, &pulModuleNameLen, &pulIndex);
	std::cout << pcModuleNameBuffer << std::endl;

	//改变PIN码
	unsigned char newPin[16] = { 0 };
	memcpy(newPin, "0000000000000000", 16);
	bResult = objA.EleChangePin(&EleCtx, Pin, newPin);
	if (!bResult)
	{
		printf("改变PIN码失败0x%x\n", err);
		return 0;
	}
	return 0;
}

