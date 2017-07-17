#include "stdafx.h"
#include "EleInterface.h"
#include "openssl/sha.h"
#pragma comment(lib,"libeay32.lib")
#pragma comment(lib,"ssleay32.lib")

CEleInterface::CEleInterface()
{
}


CEleInterface::~CEleInterface()
{
}


int CEleInterface::CheckParameter(int ulShareMode, DWORD *pDeviceContext)
{
	int result = 0;

	if (ulShareMode && ulShareMode != 1)
	{
		result = 6;
	}
	else if (pDeviceContext)
	{
		result = (*pDeviceContext != 56) ? 0x66 : 0;
	}
	else
	{
		result = 1;
	}
	return result;
}

BOOL CEleInterface::EleOpenFirstDevice(unsigned char *pucDevNumber, unsigned char *pucDesp, unsigned char *pucSerialNumber, long ulShareMode, ELE_DEVICE_CONTEXT *pDeviceContext)
{
	int bError = 0;
	int result = 0;
	int ulIndex = 0;
	bError = CheckParameter(ulShareMode, (DWORD*)pDeviceContext);
	if (bError)
	{
		GetCurrentThreadId();
		SetLastError(bError);
		result = 0;
	}
	else
	{
		pDeviceContext->ulFinger = 0x454C5433;
		bError = EcmdOpenDevice(pucDevNumber, pucDesp, pucSerialNumber, ulShareMode, pDeviceContext, ulIndex);
		if (bError)
		{
			GetCurrentThreadId();
			SetLastError(bError);
			result = 0;
		}
		else
		{
			result = 1;
		}
	}
	return result;
}

int CEleInterface::EcmdOpenDevice(unsigned char *pucDevNumber, unsigned char *pucDesp, unsigned char *pucSerialNumber, long ulShareMode, ELE_DEVICE_CONTEXT *pDeviceContext, int &uNum)
{
	int result = 0;
	int bRet = 0;
	result = objEle.SRU_OpenDevice(pucDevNumber, pucDesp, pucSerialNumber, ulShareMode, pDeviceContext, uNum);
	if (!result)
	{
		pDeviceContext->ulIndex = uNum;
		bRet = objEle.EcmdVerifyDevice(pDeviceContext);
		if (bRet)
		{
			EleClose(pDeviceContext);
			result = bRet;
		}
		else
		{
			result = 0;
		}
	}
	return result;
}

int CEleInterface::IsElite3ContextValid(ELE_DEVICE_CONTEXT *eletcx)
{
	int result = 0;
	if (!eletcx)
		return 0;

	if (eletcx->ulFinger == 0x454C5433)
	{
		if (!eletcx->ulShareMode || eletcx->ulShareMode == 1)
		{
			if (!eletcx->ulDriverMode || eletcx->ulDriverMode == 170)
			{
				if (eletcx->hDevice && eletcx->hDevice != INVALID_HANDLE_VALUE)
				{
					if (eletcx->ulDriverMode != 170 || eletcx->hMutex)
						result = 1;
				}
			}
		}
	}
	return result;
}

BOOL CEleInterface::EleClose(ELE_DEVICE_CONTEXT *eletcx)
{
	int result = 0;
	if (IsElite3ContextValid(eletcx))
	{
		if (objEle.SRU_CloseDevice(eletcx))
		{
			GetCurrentThreadId();
			SetLastError(0x67u);
			result = 0;
		}
		else
		{
			eletcx->hMutex = 0;
			eletcx->hDevice = INVALID_HANDLE_VALUE;
			result = 1;
		}
	}
	else
	{
		GetCurrentThreadId();
		SetLastError(1);
		result = 0;
	}
	return result;
}


BOOL CEleInterface::EleOpenNextDevice(ELE_DEVICE_CONTEXT *pDeviceContext)
{
	int result = 0;
	int bError = 0;
	int uMask = 0;

	unsigned char *pucDevNumber;
	unsigned char *pucDesp;
	unsigned char *pucSerialNumber;
	if (pDeviceContext)
	{
		bError = CheckParameter(pDeviceContext->ulShareMode, (DWORD *)pDeviceContext);
		if (bError)
		{
			uMask = pDeviceContext->ulMask;
			pucDevNumber = !(uMask & 1) ? NULL : (pDeviceContext->ucDevNumber);
			pucDesp = !(uMask & 2) ? NULL : (pDeviceContext->ucDesp);
			pucSerialNumber = !(uMask & 4) ? NULL : (pDeviceContext->ucSerialNumber);
			pDeviceContext->ulFinger = 1162630195;
			result = EcmdOpenDevice(pucDevNumber, pucDesp, pucSerialNumber, pDeviceContext->ulShareMode, pDeviceContext, pDeviceContext->ulIndex);
		}
		if (result)
		{
			GetCurrentThreadId();
			SetLastError(1);
			result = 0;
		}
		else
		{
			result = 1;
		}
	}
	return result;
}

int CEleInterface::EleGetFirstModuleName(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcModuleNameBuffer, 
	int ulModuleNameBufferLen, int *pulModuleNameLen, int *pulIndex)
{
	int bError = 0;
	char inbuf[256] = { 0 };
	int uSize = 255;
	if (!IsElite3ContextValid(pDeviceContext) || !pulIndex)
	{
		GetCurrentThreadId();
		SetLastError(1);
		return 0;
	}
	bError = objEle.EcmdGetModuleName(pDeviceContext, 0, inbuf, uSize);
	if (bError)
	{
		GetCurrentThreadId();
		SetLastError(bError);
		return 0;
	}
	if (ulModuleNameBufferLen < uSize + 1 || !pcModuleNameBuffer)
	{
		GetCurrentThreadId();
		if (pulModuleNameLen)
			*(DWORD *)pulModuleNameLen = uSize + 1;
		SetLastError(2u);
		return 0;
	}
	if (pulModuleNameLen)
		*pulModuleNameLen = uSize + 1;
	memset(pcModuleNameBuffer, 0, (uSize + 1));
	memcpy(pcModuleNameBuffer, inbuf, uSize);
	*pulIndex = 1;
	return 1;
}

BOOL CEleInterface::EleControl(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, unsigned char *pucInput, 
	int ulInputLen, char *pucOutput, int ulOutputLen, int &pulActualLen)
{
	signed int result;
	DWORD bRet = 1;
	DWORD bError = 0;
	char buffer[20] = { 0 };

	if (IsElite3ContextValid(pDeviceContext))
	{
		switch (ulCtrlCode)
		{
		case ELE_RESET_DEVICE:
		{
			bRet = objEle.EcmdControlDevice(pDeviceContext, ulCtrlCode, pucInput);
		}break;
		case ELE_SET_LED_UP:
		{
			bRet = objEle.EcmdControlDevice(pDeviceContext, ulCtrlCode, pucInput);
		}break;
		case ELE_SET_LED_DOWN:
		{
			bRet = objEle.EcmdControlDevice(pDeviceContext, ulCtrlCode, pucInput);
		}break;
		case ELE_SET_LED_FLASH:
		{
			if (!pucInput || ulInputLen != 1)
			  break;
			bRet = objEle.EcmdControlDevice(pDeviceContext, ulCtrlCode, pucInput);
		}break;
		case ELE_SET_VENDOR_DESC:
		{
			if (!pucInput || ulInputLen != 8)
				break;
			bRet = objEle.EcmdSetDeviceInfo(pDeviceContext, ulCtrlCode, pucInput);
		}break;
		case ELE_SET_COMM_MODE:
		{
			if (!pucInput || ulInputLen != 4 || *pucInput && *pucInput != 170)
			  break;
			bRet = objEle.EcmdSetDeviceInfo(pDeviceContext, ulCtrlCode, pucInput);
		}break;
		case ELE_GET_DEVICE_SERIAL:
		case ELE_GET_VENDOR_DESC:
		{
			if (!pucOutput)
				break;
			if (ulOutputLen < 8){
				bRet = 2;
				break;
			}
			pulActualLen = 8;
			bRet = objEle.EcmdGetDeviceInfo(pDeviceContext, ulCtrlCode, pucOutput);
		}break;
		case ELE_GET_DEVICE_VERSION:
		case ELE_GET_DEVICE_TYPE:
		case ELE_GET_MODIFY_TIME:
		case ELE_GET_MODULE_COUNT:
		case ELE_GET_MODULE_SIZE:
		{
			if (!pucOutput)
				break;
			if (ulOutputLen < 4)
				goto ERROR_0;
			pulActualLen = 4;
			bRet = objEle.EcmdGetDeviceInfo(pDeviceContext, ulCtrlCode, pucOutput);
		}break;
		case ELE_GET_COMM_MODE:
		{
			if (!pucOutput)
			  break;
			if (ulOutputLen < 4)
			  goto ERROR_0;
			pulActualLen = 4;
			bRet = objEle.EcmdGetCommMode(pDeviceContext, pucOutput);
		}break;
		case ELE_GET_CURRENT_TIME:
		{
			if (!pucOutput)
				 break;
			if (ulOutputLen < 4)
			{
			ERROR_0:
				 GetCurrentThreadId();
				 pulActualLen = 4;
				 SetLastError(2u);
				 return 0;
			}
			pulActualLen = 4;
			bError = objEle.EcmdGetDeviceInfo(pDeviceContext, 8, buffer);
			if (bError)
			{
				 GetCurrentThreadId();
				 SetLastError(bError);
				 return 0;
			}
			if (!((int)buffer & 2))
			{
				 GetCurrentThreadId();
				 SetLastError(0x24u);
				 return 0;
			}
			bRet = objEle.EcmdGetDeviceInfo(pDeviceContext, ulCtrlCode, pucOutput);
		}break;
		case ELE_GET_DEVELOPER_NUMBER:
		{
			if (!pucOutput)
				 break;
			if (ulOutputLen < 8)
			{
				 pulActualLen = 8;
				 bRet = 2;
			}
			else
			{
				 pulActualLen = 8;
				 bRet = objEle.EcmdGetDeveloperNumber(pDeviceContext, (unsigned char*)pucOutput);
			}
		}break;
		default:
			break;
		}
	}
	if (bRet)
	{
		GetCurrentThreadId();
		SetLastError(bRet);
		result = 0;
	}
	else
	{
		result = 1;
	}
	return result;
}

BOOL CEleInterface::EleExecute(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcModuleName, char *pucInput, int ulInputLen, char *pucOutput, int ulOutputLen, int *pulActualOutputLen)
{
	char outbuf[256] = { 0 };
	int uNum = 255;
	int bError = 0;

	if (!IsElite3ContextValid(pDeviceContext) || !pcModuleName || !strlen((const char *)pcModuleName) || strlen((const char *)pcModuleName) > 0x10)
	{
		GetCurrentThreadId();
		SetLastError(1);
		return 0;
	}
	if (ulInputLen > 0xF7 || ulInputLen && !pucInput || ulOutputLen && !pucOutput)
	{
		GetCurrentThreadId();
		SetLastError(1);
		return 0;
	}
	WaitForSingleObject(pDeviceContext->hMutex, -1);
	bError = objEle.EcmdSelectModule(pDeviceContext, pcModuleName);
	if (bError)
	{
		ReleaseMutex(pDeviceContext->hMutex);
		GetCurrentThreadId();
		SetLastError(bError);
		return 0;
	}
	bError = objEle.EcmdExecuteModule(pDeviceContext, pucInput, ulInputLen, outbuf, uNum);
	if (bError)
	{
		ReleaseMutex(pDeviceContext->hMutex);
		GetCurrentThreadId();
		SetLastError(bError);
		return 0;
	}
	ReleaseMutex(pDeviceContext->hMutex);
	if (uNum > ulOutputLen)
	{
		GetCurrentThreadId();
		memcpy(pucOutput, outbuf, ulOutputLen);
		if (pulActualOutputLen)
			*pulActualOutputLen = uNum;
		SetLastError(2);
		return 0;
	}
	if (pulActualOutputLen)
		*pulActualOutputLen = uNum;
	if (pucOutput)
		memcpy(pucOutput, outbuf, uNum);
	return 1;
}

BOOL CEleInterface::EleVerifyPin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPin)
{
	int result = 0;
	int bError = 0;

	if (IsElite3ContextValid(pDeviceContext) && pucPin)
	{
		bError = objEle.EcmdVerifyPin(pDeviceContext, pucPin);
		if (bError)
		{
			GetCurrentThreadId();
			SetLastError(bError);
			result = 0;
		}
		else
		{
			result = 1;
		}
	}
	else
	{
		GetCurrentThreadId();
		SetLastError(1);
		result = 0;
	}
	return result;
}

BOOL CEleInterface::EleChangeModuleName(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcOldModuleName, char *pcNewModuleName)
{
	int result = 0;
	int bError = 0;
	if (!IsElite3ContextValid(pDeviceContext)
		|| strlen(pcNewModuleName) && strlen(pcNewModuleName) > 0x10
		|| strlen(pcOldModuleName) && strlen(pcOldModuleName) > 0x10)
	{
		GetCurrentThreadId();
		SetLastError(1);
		return 0;
	}

	if (objEle.EcmdSelectModule(pDeviceContext, pcOldModuleName))
	{
		GetCurrentThreadId();
		SetLastError(0x27);
		result = 0;
	}
	else
	{
		bError = objEle.EcmdChangeModuleName(pDeviceContext, pcNewModuleName);
		if (!bError)
		{
			result = 1;
		}
		else
		{
			GetCurrentThreadId();
			SetLastError(bError);
			result = 0;
		}
	}
	return result;
}

BOOL CEleInterface::EleChangePin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucOldPin, unsigned char *pucNewPin)
{
	int result = 0;
	int bError = 0;

	if (!IsElite3ContextValid(pDeviceContext)
		||pucOldPin==NULL
		||pucNewPin==NULL)
	{
		GetCurrentThreadId();
		SetLastError(1);
		result = 0;
	}

	bError = objEle.EcmdVerifyPin(pDeviceContext, pucOldPin);
	if (bError)
	{
		GetCurrentThreadId();
		SetLastError(bError);
		result = 0;
	}
	else
	{
		bError = objEle.EcmdChangePin(pDeviceContext, pucOldPin, pucNewPin);
		if (bError)
		{
			GetCurrentThreadId();
			SetLastError(bError);
			result = 0;
		}
		else
		{
			result = 1;
		}
	}
	return result;
}

BOOL CEleInterface::EleGetVersion(ELE_LIBVERSIONINFO *eleversion)
{
	int result = 0;
	if (eleversion)
	{
		if (eleversion->ulVersionInfoSize == 144)
		{
			eleversion->ulMajorVersion = 2;
			eleversion->ulMinorVersion = 0;
			eleversion->ulClientID = 0;
			result = 1;
		}
		else
		{
			GetCurrentThreadId();
			SetLastError(0x66);
			result = 0;
		}
	}
	else
	{
		GetCurrentThreadId();
		SetLastError(1);
		result = 0;
	}
	return result;
}

BOOL CEleInterface::EleUpdate(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPkgContent, int ulPkgLen)
{
	unsigned int v3 = 0;
	int result = 0;
	char *v7 = { 0 };
	unsigned int v8 = 0;
	unsigned int v9 = 0;
	int v10 = 0;
	int v11 = 0;
	int bError = 0;
	__int16 v13 = 0;

	unsigned char *v5;
	SHA_CTX context;
	unsigned char digest[20] = { 0 };
	if (IsElite3ContextValid(pDeviceContext) && pucPkgContent)
	{
		if (ulPkgLen > 0x1C)
		{
			SHA1_Init(&context);
			SHA1_Update(&context, pucPkgContent, ulPkgLen - 20);
			SHA1_Final(digest, &context);
			v5 = (unsigned char *)pucPkgContent + ulPkgLen - 20;
			unsigned int v6 = 20;
			v7 = (char *)(digest - v5);
			while (*(DWORD*)&v5[(DWORD)v7] == *(DWORD*)v5)
			{
				v6 -= 4;
				v5 += 4;
				if (v6 < 4)
				{
					if (ulPkgLen != 20)
					{
						do
						{
							v8 = *(DWORD*)((char *)pucPkgContent + v3 + 4);
							if (*(WORD*)((char *)pucPkgContent + v3 + 2) != v13 || v8 > 0xF4)
							{
								GetCurrentThreadId();
								SetLastError(0x2C);
								return 0;
							}
							++v13;
							v3 += v8 + 8;
						} while (v3 < ulPkgLen - 20);
					}
					if (*(WORD*)pucPkgContent != v13)
						break;
					v9 = 0;
					if (ulPkgLen == 20)
					{
						result = 1;
					}
					else
					{
						while (1)
						{
							v10 = pucPkgContent[4];
							bError = objEle.EcmdUpdateModule(pDeviceContext, pucPkgContent + 8, v10);
							if (bError)
								break;
							v9 = v10 + 8;
							if (v9 >= ulPkgLen - 20)
								return 1;
						}
						GetCurrentThreadId();
						SetLastError(bError);
						result = 0;
					}
					return result;
				}
			}
		}
		GetCurrentThreadId();
		SetLastError(0x2C);
		result = 0;
	}
	else
	{
		GetCurrentThreadId();
		SetLastError(1);
		result = 0;
	}
	return result;
}