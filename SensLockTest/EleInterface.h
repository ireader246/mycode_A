#pragma once
#include "CcmdEle.h"

typedef struct _ELE_LIBVERSIONINFO {
	long ulVersionInfoSize;
	long ulMajorVersion;
	long ulMinorVersion;
	long ulClientID;
	char acDesp[128];
} ELE_LIBVERSIONINFO;

class CEleInterface
{
public:
	CEleInterface();
	~CEleInterface();
	int CheckParameter(int ulShareMode, DWORD *pDeviceContext);
	BOOL EleOpenFirstDevice(unsigned char *pucDevNumber, unsigned char *pucDesp, unsigned char *pucSerialNumber, long ulShareMode, ELE_DEVICE_CONTEXT *pDeviceContext);
	int EcmdOpenDevice(unsigned char *pucDevNumber, unsigned char *pucDesp, unsigned char *pucSerialNumber, long ulShareMode, ELE_DEVICE_CONTEXT *pDeviceContext, int &a6);
	int IsElite3ContextValid(ELE_DEVICE_CONTEXT *eletcx);
	BOOL EleClose(ELE_DEVICE_CONTEXT *eletcx);
	int EleGetFirstModuleName(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcModuleNameBuffer, int ulModuleNameBufferLen, int *pulModuleNameLen, int *pulIndex);
	BOOL EleControl(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, unsigned char *pucInput, int ulInputLen, char *pucOutput, int ulOutputLen, int &pulActualLen);
	BOOL EleExecute(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcModuleName, char *pucInput, int ulInputLen, char *pucOutput, int ulOutputLen, int *pulActualOutputLen);
	BOOL EleVerifyPin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPin);
	BOOL EleChangeModuleName(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcOldModuleName, char *pcNewModuleName);
	BOOL EleChangePin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucOldPin, unsigned char *pucNewPin);
	BOOL EleGetVersion(ELE_LIBVERSIONINFO *a1);
	BOOL EleUpdate(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPkgContent, int ulPkgLen);
	BOOL EleOpenNextDevice(ELE_DEVICE_CONTEXT *pDeviceContext);
public:
	CcmdEle objEle;
};

