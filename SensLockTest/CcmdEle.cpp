#include "stdafx.h"
#include "CcmdEle.h"
#include <Setupapi.h>
#include <hidsdi.h>
#include <string.h>
#include <rsa.h>
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"Hid.lib")
#pragma comment(lib,"rsa.lib")


CcmdEle::CcmdEle()
{
	memset(EncryptBuffer, 0, sizeof(EncryptBuffer));
}


CcmdEle::~CcmdEle()
{
}


void CcmdEle::setlow(int &num, int low)
{
	short int *piTest = (short int *)&num;
	*piTest = low;   //低16位值  
}

void CcmdEle::sethigh(int &num, int high)
{
	short int *piTest = (short int *)&num;
	piTest++;
	*piTest = high;   //高16位值
}

BOOL CcmdEle::usbControl_0(HANDLE hDevice, HANDLE hMutex, unsigned char *random, int a4, unsigned char *outbuf, int &uSize)
{
	int a = 0;
	setlow(a, 0);
	sethigh(a, a4);
	if (!hMutex)
		return 0;
	int bRet = WaitForSingleObject(hMutex, -1);
	if (bRet)
	{
		if (bRet != 128)
			return 0;
	}
	if (!SU_ControlWrite_HID(hDevice, 64, a, random))
	{
		ReleaseMutex(hMutex);
		return 0;
	}
	bRet = SU_ControlRead_HID(hDevice, outbuf, uSize);
	if (!bRet)
	{
		ReleaseMutex(hMutex);
		return 0;
	}
	ReleaseMutex(hMutex);
	return 1;
}

BOOL CcmdEle::usbControl_1(unsigned char *outbuf, HANDLE hDevice, int a3, unsigned char *random, int &uSize)
{
	int result = 0;
	DWORD BytesReturned = 0;
	char OutBuffer[256] = { 0 };
	BytesReturned = 0;
	if (DeviceIoControl(hDevice, 0x220028, random, a3, OutBuffer, 255, &BytesReturned, 0))
	{
		uSize = BytesReturned;
		memcpy(outbuf, &OutBuffer, BytesReturned);
		result = 1;
	}
	else
	{
		result = 0;
	}
	return result;
}

int CcmdEle::SU_ControlWrite_HID(HANDLE hDevice, int a2, int a3, unsigned char *random)
{
	int uNuma = 0;
	int uNumb = 0;
	char buf[128] = { 0 };
	int v1 = a2 >> 0x8;
	int a = LOBYTE(v1);
	if (a == 1)
	{
		buf[2] = (unsigned int)a2 >> 16;
		buf[0] = -128;
		buf[1] = ((unsigned int)a3 >> 16) + 1;
		memcpy(&buf[3], random, HIWORD(a3));
	}
	else
	{
		uNumb = a3 >> 16;
		if (HIWORD(a3) + 2 > 64)
		{
			uNuma = 16 * ((HIWORD(a3) + 17) / 16);
			buf[0] = 12 - (HIWORD(a3) - 63) / 16;
			if (uNuma == 256)
				uNuma = 258;
			int b = a3 >> 16;
			setlow(uNumb, b);
			if (12 == ((HIWORD(a3) - 63) / 16))
			{
				buf[0] = 1;
				uNuma = 258;
			}
		}
		else
		{
			buf[0] = 20 - (HIWORD(a3) + 1) / 8;
			uNuma = 8 * ((HIWORD(a3) + 9) / 8);
		}
		buf[1] = uNumb;
		memcpy(&buf[2], random, HIWORD(a3));
	}

	return HidD_SetFeature(hDevice, buf, sizeof(buf));
}

int CcmdEle::SU_ControlRead_HID(HANDLE hDevice, unsigned char *outbuf, int &uSize)
{
	int bRet = 0;
	unsigned char encrptbuf[258] = { 0 };
	encrptbuf[0] = 0x1;
	encrptbuf[1] = 0x0;
	while (1)
	{
		bRet = HidD_GetFeature(hDevice, encrptbuf, 258);
		if (bRet)
			break;
		if (GetLastError() != 121)
			return bRet;
	}
	if (encrptbuf[1] > uSize)
	{
		memcpy(outbuf, encrptbuf + 2, uSize);
	}
	else
	{
		uSize = encrptbuf[1];
		memcpy(outbuf, encrptbuf + 2, uSize);
	}
	return bRet;
}


int CcmdEle::SRU_Transmit(ELE_DEVICE_CONTEXT *electx, unsigned char *random, int a3, unsigned char *buf, int &uNum)
{
	int result = 0;
	unsigned char outbuf[256] = { 0 };
	int uSize = 255;
	if (!electx)
		return 1;
	if (random)
	{
		if (electx->ulDriverMode)
		{
			if (!usbControl_0(electx->hDevice, electx->hMutex, random, a3, outbuf, uSize))
				return 5;
		}
		else
		{
			if (!usbControl_1(outbuf, electx->hDevice, a3, random, uSize))
				return 5;
		}
		uNum = uSize;
		if (255 >= uSize)
		{
			memcpy(buf, outbuf, uSize);
			result = 0;
		}
		else
		{
			result = 2;
		}
	}
	else
	{
		result = 1;
	}
	return result;
}

int CcmdEle::Transmit(ELE_DEVICE_CONTEXT *exectx, unsigned char *random, int a3, unsigned char *outbuf, int &uNum)
{
	int result = 0;
	if (uNum)
	{
		result = SRU_Transmit(exectx, random, a3, outbuf, uNum);
		if (!result)
		{
			if (uNum >= 2){
				int uNuma = outbuf[uNum - 2];
				int uNumb = outbuf[uNum - 1];
				uNum = uNum - 2;
				result = uNuma | (unsigned __int16)(uNumb << 8);
			}
			else
				result = 5;
		}
	}
	else
	{
		result = 1;
	}
	return result;
}

int CcmdEle::EcmdVerifyDevice(ELE_DEVICE_CONTEXT *electx)
{
	int result = 0;
	R_RSA_PUBLIC_KEY PubKey;
	FILE *f = NULL;
	if (errno_t err = fopen_s(&f, "public.key", "rb"))
	{
		GetLastError();
		printf("open file error 0x%x\n", err);
		return 1;
	}
	else
	{
		fread(&PubKey, sizeof(PubKey), 1, f);
		fclose(f);
	}
	unsigned char DecryptBuffer[128] = { 0 };
	unsigned int InputLen = 64;
	unsigned int OutputLen = sizeof(DecryptBuffer);

	unsigned char random[24] = { 0x80, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, \
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
	for (int i = 0; i < 16; i++)
	{
		srand((UINT)GetCurrentTime());
		int x = rand() % 255;
		random[i + 8] = x;
	}

	int uNum = 255;
	result = Transmit(electx, random, 0x18, EncryptBuffer, uNum);

	if (!result)
	{
		if (uNum == 64)
		{
			if (!RSAPublicDecrypt(DecryptBuffer, &OutputLen, EncryptBuffer, InputLen, &PubKey))
			{
				for (int i = 0; DecryptBuffer[i] == random[i + 8]; i++)
				{
					if (i == 15)
					{
						return 0;
					}
				}
			}
			result = 104;
		}
		else
		{
			result = 5;
		}
	}
	return result;
}

BOOL CcmdEle::USB_GetDeviceList(GUID* hidGuid, char *hidPath, int &uNum)
{
	HDEVINFO hDevInfo = NULL;
	int deviceNo = 0; //设备列表中的序号
	SP_DEVICE_INTERFACE_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	hDevInfo = SetupDiGetClassDevs(hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE); //得到一类设备信息，只返回当前存在的设备
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		GetLastError();
		return FALSE;
	}
	ULONG  requiredLength = 0;
	SP_DEVINFO_DATA devData;
	devData.cbSize = sizeof(SP_DEVINFO_DATA);
	char buffer[1024] = { 0 };
	DWORD buffersize = 1024;
	DWORD DataT = 0;
	while (SetupDiEnumDeviceInterfaces(hDevInfo, 0, hidGuid, deviceNo, &devInfoData))
	{
		if (hidPath)
		{
			devData.cbSize = 28;
			if (!SetupDiEnumDeviceInfo(hDevInfo, deviceNo, &devData))
			{
				return FALSE;
			}
			if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData, SPDRP_HARDWAREID, &DataT, (PBYTE)buffer, buffersize, &buffersize))
			{
				GetLastError();
				return FALSE;
			}
			if (memcmp(hidPath, buffer, sizeof(hidPath)))
			{
				if (++deviceNo >= 127)
				{
					break;
				}
				continue;
			}
		}
		//取得该设备接口的细节(设备路径)
		SetupDiGetInterfaceDeviceDetail(hDevInfo,// 设备信息集句柄
			&devInfoData,  // 设备接口信息
			NULL, // 设备接口细节(设备路径)
			0, // 输出缓冲区大小
			&requiredLength,// 计算输出缓冲区大小
			NULL);  // 不需额外的设备描述
		PSP_INTERFACE_DEVICE_DETAIL_DATA devDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredLength);//分配大小为requiredLength的内存块
		devDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

		if (!SetupDiGetInterfaceDeviceDetail(hDevInfo, &devInfoData, devDetail, requiredLength, &requiredLength, NULL))
		{
			free(devDetail);
			SetupDiDestroyDeviceInfoList(hDevInfo);
			return FALSE;
		}
		if (devDetail != NULL)
		{
			std::wstring str(devDetail->DevicePath);
			usbPath.push_back(str);
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
		uNum = usbPath.size();
		if (++deviceNo >= 127)
		{
			break;
		}
	}
	usbPath.resize(uNum);
	return TRUE;
}


int CcmdEle::SRU_OpenDevice(unsigned char *pucDevNumber, unsigned char *pucDesp,
	unsigned char *pucSerialNumber,
	long ulShareMode, ELE_DEVICE_CONTEXT *pDeviceContext, int &ulIndex)
{
	SECURITY_ATTRIBUTES MutexAttributes;
	SECURITY_DESCRIPTOR pSecurityDescriptor;
	MutexAttributes.nLength = 12;
	MutexAttributes.bInheritHandle = 0;
	pDeviceContext->hMutex = 0;
	HANDLE hFile = 0;
	if (pDeviceContext == NULL)
	{
		return 1;
	}
	if (ulShareMode != 0 && ulShareMode != 1)
	{
		return 6;
	}
	if (!InitializeSecurityDescriptor(&pSecurityDescriptor, 1)
		|| !SetSecurityDescriptorDacl(&pSecurityDescriptor, 1, 0, 0))
	{
		return 3;
	}
	MutexAttributes.lpSecurityDescriptor = &pSecurityDescriptor;
	HANDLE hMutex1 = CreateMutex(&MutexAttributes, FALSE, L"Global\\{8E1FBD9A-9BE4-4134-A7B0-05D530F835E6}");
	if (!hMutex1)
	{
		return 3;
	}
	WaitForSingleObject(hMutex1, INFINITE);

	GUID hidGuid;
	HidD_GetHidGuid(&hidGuid);//得到HID路径
	int DevCount2 = 0;
	if (!USB_GetDeviceList(&hidGuid, "HID\\Vid_1bc0&Pid_8013", DevCount2))
	{
		ReleaseMutex(hMutex1);
		CloseHandle(hMutex1);
		return 8;
	}
	if (DevCount2 <= 0)
	{
		ReleaseMutex(hMutex1);
		CloseHandle(hMutex1);
		return 9;
	}
	for (int i = 0; i < DevCount2; i++)
	{
		hFile = CreateFile(usbPath[i].c_str(), GENERIC_READ, FILE_READ_ACCESS, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			break;
		}
	}
	HANDLE hMutex2 = CreateMutex(&MutexAttributes, 0, L"Global\\hid#vid_1bc0&pid_8013#6&21f28757&0&0000#{4d1e55b2-f16f-1");
	if (!hMutex2)
	{
		return 3;
	}
	ReleaseMutex(hMutex1);
	CloseHandle(hMutex1);
	pDeviceContext->hDevice = hFile;
	pDeviceContext->hMutex = hMutex2;
	pDeviceContext->ulDriverMode = 0xAA;
	return 0;
}

BOOL CcmdEle::SRU_CloseDevice(ELE_DEVICE_CONTEXT *eletcx)
{
	if (!eletcx)
		return 1;
	if (!eletcx->hMutex)
	{
		SetLastError(1u);
		return GetLastError();
	}
	if (!CloseHandle(eletcx->hMutex))
		return GetLastError();
	if (eletcx->hDevice == INVALID_HANDLE_VALUE || !eletcx->hDevice)
	{
		SetLastError(1u);
		return GetLastError();
	}
	if (CloseHandle(eletcx->hDevice))
		return 0;
	return GetLastError();
}

int CcmdEle::EcmdGetModuleName(ELE_DEVICE_CONTEXT *electx, int a2, char *outbuf, int &a4)
{
	int result = 0;

	unsigned char random[16] = { 0x90, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00 };
	int uNum = 255;
	unsigned char NameBuffer[128] = { 0 };
	result = Transmit(electx, random, 8, NameBuffer, uNum);
	if (!result)
	{
		a4 = uNum;
		memcpy(outbuf, NameBuffer, a4);
	}
	return result;
}

int CcmdEle::EcmdControlDevice(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, unsigned char *pucInput)
{
	int result = 0;
	int v4 = 0;
	int v6 = 0;

	unsigned char outbuf[128] = { 0 };
	int uNum = 255;
	unsigned char controlbuf[20] = { 0 };
	for (int i = 0; i < 8; i++)
	{
		controlbuf[i] = EncryptBuffer[i + 56];
	}
	switch (ulCtrlCode)
	{
	case 22:
	{
		v6 = 0;
		v4 = 1;
		result = SRU_Control(pDeviceContext, v4, controlbuf, v6, outbuf, uNum);
		break;
	}
	case 23:
	{
		controlbuf[0] = 0;
		v6 = 1;
		v4 = 4;
		result = SRU_Control(pDeviceContext, v4, controlbuf, v6, outbuf, uNum);
		break;
	}
	case 24:
	{
		controlbuf[0] = 1;
		v6 = 1;
		v4 = 4;
		result = SRU_Control(pDeviceContext, v4, controlbuf, v6, outbuf, uNum);
		break;
	}
	case 25:
	{
		controlbuf[0] = 2;
		v6 = 2;
		v4 = 4;
		result = SRU_Control(pDeviceContext, v4, controlbuf, v6, outbuf, uNum);
		break;
	}
	default:
		break;
	}
	return result;
}

int CcmdEle::EcmdSetDeviceInfo(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, unsigned char *pucInput)
{
	int result = 0;
	int uIndex = 255;
	unsigned char outbuf[128] = { 0 };

	if (ulCtrlCode == 26)
	{
		result = SRU_Control(pDeviceContext, 2, pucInput, 1, outbuf, uIndex);
	}
	else if (ulCtrlCode == 27)
	{
		result = EcmdSetVendorDesc(pDeviceContext, pucInput);
	}
	return result;
}

int CcmdEle::EcmdGetDeviceInfo(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, char *pucOutput)
{
	int result = 0;
	unsigned char outbuf[128] = { 0 };
	unsigned char buf[20] = { 0x80, 0x06, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x35, 0xAE, 0x00 };
	int uIndex = 255;
	switch (ulCtrlCode)
	{
	case 1:
	{
		buf[2] = 0x01;
		buf[6] = 0x08;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		  memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 8:
	{
		buf[2] = 0x02;
		buf[6] = 0x01;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		  memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 7:
	{
		buf[2] = 0x03;
		buf[6] = 0x02;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		  memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 9:
	{
		buf[2] = 0x04;
		buf[6] = 0x04;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		  memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 10:
	{
		buf[2] = 0x05;
		buf[6] = 0x04;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		   memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 4:
	{
		buf[2] = 0x06;
		buf[6] = 0x04;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		  memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 2:
	{
		buf[2] = 0x07;
		buf[6] = 0x08;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		  memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 19:
	{
		buf[2] = 0x08;
		buf[6] = 0x01;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		   memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	case 20:
	{
		buf[2] = 0x09;
		buf[6] = 0x02;
		result = Transmit(pDeviceContext, buf, 8, outbuf, uIndex);
		if (!result)
		   memcpy(pucOutput, outbuf, uIndex);
		break;
	}
	default:
		break;
	}
	return result;
}

int CcmdEle::SRU_Control(ELE_DEVICE_CONTEXT *electx, int a2, unsigned char *inputbuf, int a4, unsigned char *outbuf, int &uNum)
{
	int result;
	unsigned char ubuf[128] = { 0 };
	int uIndex = 255;
	if (!electx || !inputbuf || a4 > 0xFF || !outbuf)
		return 1;
	if (electx->ulDriverMode)
	{
		if (!usbControl_a(inputbuf, uIndex, electx->hDevice, electx->hMutex, a2, a4, ubuf))
			return 5;
	}
	else if (!usbControl_b(a4, ubuf, electx->hDevice, a2, inputbuf, uIndex))
	{
		return 5;
	}
	if (uIndex <= uNum)
	{
		memcpy(outbuf, ubuf, uIndex);
		result = 0;
	}
	else
	{
		result = 2;
	}
	uNum = uIndex;
	return result;
}

int CcmdEle::EcmdSetVendorDesc(ELE_DEVICE_CONTEXT *exectx, unsigned char *pucInput)
{
	int uIndex = 255;
	unsigned char outbuf[128] = { 0 };
	unsigned char random[20] = { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00 };
	for (int i = 0; i < 8; i++)
	{
		random[i + 8] = pucInput[i];
	}
	return Transmit(exectx, random, 16, outbuf, uIndex);
}

int CcmdEle::EcmdGetCommMode(ELE_DEVICE_CONTEXT *exectx, char *outbuf)
{
	int result;
	int uNum = 255;
	unsigned char buf[128] = { 0 };
	unsigned char v5;

	result = SRU_Control(exectx, 3, &v5, 0, buf, uNum);
	if (!result)
		memcpy(outbuf, buf, sizeof(buf));
	return result;
}

int CcmdEle::EcmdGetDeveloperNumber(ELE_DEVICE_CONTEXT *exectx, unsigned char *outbuf)
{
	int result;
	int uNum = 255;
	int v9 = 0;
	unsigned char random[20] = { 0 };
	unsigned char buf[128] = { 0 };

	result = Transmit(exectx, random, 8, outbuf, uNum);
	if (!result)
	{
		outbuf[4] = v9;
	}
	return result;
}

int CcmdEle::usbControl_a(unsigned char *inputbuf, int &uIndex, HANDLE hDevice, HANDLE hMutex, int a5, int a6, unsigned char *outbuf)
{
	int uNuma = a6;
	if (!a6)
		uNuma = 8;

	int bError = 0;
	int para1 = 0;
	int para2 = 0;

	setlow(para1, 320);
	sethigh(para1, a5);

	setlow(para2, 0);
	sethigh(para2, uNuma);
	if (!hMutex)
		return 0;
	bError = WaitForSingleObject(hMutex, INFINITE);
	if (bError)
	{
		if (bError != 128)
			return 0;
	}
	if (!SU_ControlWrite_HID(hDevice, para1, para2, (unsigned char*)inputbuf))
	{
		ReleaseMutex(hMutex);
		return 0;
	}
	if (!SU_ControlRead_HID(hDevice, outbuf, uIndex))
	{
		ReleaseMutex(hMutex);
		return 0;
	}
	ReleaseMutex(hMutex);
	return 1;
}

int CcmdEle::usbControl_b(int Size, unsigned char *outbuf, HANDLE hDevice, int a4, unsigned char *inputbuf, int &uIndex)
{
	int result = 0;
	DWORD BytesReturned = 0;
	if (DeviceIoControl(hDevice, 0x220060u, inputbuf, Size + 1, outbuf, 0xFFu, &BytesReturned, 0))
	{
		uIndex = BytesReturned;
		result = 1;
	}
	else
	{
		result = 0;
	}
	return result;
}

int CcmdEle::EcmdExecuteModule(ELE_DEVICE_CONTEXT *pDeviceContext, char *pucInput, int ulInputLen, char *outbuf, int &uNum)
{
	int result;
	unsigned char buf[128] = { 0 };
	int uIndex = 255;
	unsigned char random[32] = { 0x90, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00 };
	random[6] = ulInputLen;
	for (int i = 0; i < ulInputLen; i++)
	{
		random[i + 8] = pucInput[i];
	}
	result = Transmit(pDeviceContext, random, ulInputLen + 8, buf, uIndex);
	if (!result)
	{
		uNum = uIndex;
		memcpy(outbuf, buf, uIndex);
	}
	return result;
}

int CcmdEle::EcmdSelectModule(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcModuleName)
{
	unsigned char outbuf[20] = { 0 };
	int uIndex = 255;
	unsigned char random[24] = { 0x90, 0x30, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00 };
	int uLenth = strlen(pcModuleName);
	random[6] = uLenth;
	for (int i = 0; i < uLenth; i++)
	{
		random[i + 8] = pcModuleName[i];
	}
	return Transmit(pDeviceContext, random, uLenth + 8, outbuf, uIndex);
}

int CcmdEle::EcmdVerifyPin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPin)
{
	unsigned char outbuf[64] = { 0 };
	int uIndex = 255;
	unsigned char random[32] = { 0xA0, 0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x44, 0x45, 0x56, 0x4C, 0x50, 0x31, 0x00, 0x00 };
	for (int i = 0; i < 16; i++)
	{
		random[i + 16] = pucPin[i];
	}
	return Transmit(pDeviceContext, random, 32, outbuf, uIndex);
}

int CcmdEle::EcmdChangeModuleName(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcNewModuleName)
{
	int uIndex = 255;
	int nameLen = strlen(pcNewModuleName);
	unsigned char outbuf[32] = { 0 };
	unsigned char random[20] = { 0x90, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	random[6] = nameLen;
	for (int i = 0; i < nameLen; i++)
	{
		random[i + 8] = pcNewModuleName[i];
	}
	return Transmit(pDeviceContext, random, nameLen + 8, outbuf, uIndex);
}

int CcmdEle::EcmdChangePin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucOldPin, unsigned char *pucNewPin)
{
	unsigned char outbuf[20] = { 0 };
	int uIndex = 255;
	unsigned char random[40] = { 0xA0, 0x62, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00 };
	for (int i = 0; i < 16; i++)
	{
		random[i + 8] = pucOldPin[i];
		random[i + 23] = pucNewPin[i];
	}
	return Transmit(pDeviceContext, random, 40, outbuf, uIndex);
}

int CcmdEle::EcmdUpdateModule(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPkgContent, int ulPkgLen)
{
	int uIndex = 244;
	unsigned char outbuf[128] = { 0 };
	unsigned char random[128] = { 0xB0, 0x92, 0x00, 0x00 };
	random[6] = ulPkgLen;
	for (int i = 0; i < ulPkgLen; i++)
	{
		random[i + 8] = pucPkgContent[i];
	}
	return Transmit(pDeviceContext, random, ulPkgLen + 8, outbuf, uIndex);
}