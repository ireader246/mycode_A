#pragma once
#include <windows.h>
#include <vector>
#include <string>

// device information
#define ELE_GET_DEVICE_SERIAL                 0x00000001// device serial number, 8bytes
#define ELE_GET_VENDOR_DESC                   0x00000002// manufacturer descritpion, 8bytes
#define ELE_GET_CURRENT_TIME                  0x00000004// current time from dongle, only for RTC dongle
#define ELE_GET_DEVICE_VERSION                0x00000007// device verion
#define ELE_GET_DEVICE_TYPE                   0x00000008// device type
#define ELE_GET_MODIFY_TIME                   0x0000000A// production date of the dongle
#define ELE_GET_COMM_MODE                     0x0000000B// communication mode, USB or HID
#define ELE_GET_DEVELOPER_NUMBER              0x00000012// developer number
#define ELE_GET_MODULE_COUNT                  0x00000013// module amount
#define ELE_GET_MODULE_SIZE                   0x00000014// module size

// control code
#define ELE_RESET_DEVICE                      0x00000016// reset device
#define ELE_SET_LED_UP                        0x00000017// LED up
#define ELE_SET_LED_DOWN                      0x00000018// LED down
#define ELE_SET_LED_FLASH                     0x00000019// LED blinking
#define ELE_SET_COMM_MODE                     0x0000001A// set communication mode
#define ELE_SET_VENDOR_DESC                   0x0000001B// set manufacturer description

typedef  struct _ELE_DEVICE_CONTEXT
{
	long ulSize;                         //0
	long ulFinger;						//4
	long ulMask;						//8
	unsigned char ucDevNumber[8];		//12
	unsigned char ucDesp[8];			//20
	unsigned char ucSerialNumber[8];	//28
	long ulShareMode;					//36
	int ulIndex;						//40
	long ulDriverMode;					//44
	HANDLE hDevice;				//48
	HANDLE hMutex;					//52
} ELE_DEVICE_CONTEXT, *PELE_DEVICE_CONTEXT;

class CcmdEle
{
public:
	CcmdEle();
	~CcmdEle();
	void setlow(int &num, int low);  //设置一个数的低16位
	void sethigh(int &num, int high);  //设置一个数的高16位
	int SU_ControlWrite_HID(HANDLE hDevice, int a2, int a3, unsigned char *random);
	int SU_ControlRead_HID(HANDLE hDevice, unsigned char *outbuf, int &uSize);
	int SRU_Transmit(ELE_DEVICE_CONTEXT *electx, unsigned char *random, int a3, unsigned char *outbuf, int &uNum);
	int Transmit(ELE_DEVICE_CONTEXT *exectx, unsigned char *random, int a3, unsigned char *outbuf, int &uNum);
	int SRU_OpenDevice(unsigned char *pucDevNumber, unsigned char *pucDesp,
		unsigned char *pucSerialNumber, long ulShareMode, ELE_DEVICE_CONTEXT *pDeviceContext, int &ulIndex); //打开设备
	BOOL USB_GetDeviceList(GUID* hidGuid, char *hidPath, int &uNum);
	int EcmdVerifyDevice(ELE_DEVICE_CONTEXT *electx); //验证打开设备
	BOOL usbControl_1(unsigned char *outbuf, HANDLE hDevice, int a3, unsigned char *random, int &uSize);
	BOOL usbControl_0(HANDLE hDevice, HANDLE hMutex, unsigned char *random, int a4, unsigned char *outbuf, int &uSize);
	BOOL SRU_CloseDevice(ELE_DEVICE_CONTEXT *electx);  //关闭设备
	int EcmdGetModuleName(ELE_DEVICE_CONTEXT *electx, int a2, char *inbuf, int &a4); //获取模块名称
	int EcmdControlDevice(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, unsigned char *pucInput);
	int EcmdSetDeviceInfo(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, unsigned char *pucInput);
	int EcmdGetDeviceInfo(ELE_DEVICE_CONTEXT *pDeviceContext, int ulCtrlCode, char *pucOutput);
	int SRU_Control(ELE_DEVICE_CONTEXT *electx, int a2, unsigned char *inputbuf, int a4, unsigned char *outbuf, int &uNum);
	int EcmdSetVendorDesc(ELE_DEVICE_CONTEXT *exectx, unsigned char *pucInput);
	int EcmdGetCommMode(ELE_DEVICE_CONTEXT *exectx, char *outbuf);
	int EcmdGetDeveloperNumber(ELE_DEVICE_CONTEXT *exectx, unsigned char *outbuf);
	int usbControl_a(unsigned char *inputbuf, int &uIndex, HANDLE hDevice, HANDLE hMutex, int a5, int a6, unsigned char *outbuf);
	int usbControl_b(int Size, unsigned char *outbuf, HANDLE hDevice, int a4, unsigned char *inputbuf, int &uIndex);
	int EcmdExecuteModule(ELE_DEVICE_CONTEXT *pDeviceContext, char *pucInput, int ulInputLen, char *outbuf, int &uNum);
	int EcmdSelectModule(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcModuleName);
	int EcmdVerifyPin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPin);
	int EcmdChangeModuleName(ELE_DEVICE_CONTEXT *pDeviceContext, char *pcNewModuleName);
	int EcmdChangePin(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucOldPin, unsigned char *pucNewPin);
	int EcmdUpdateModule(ELE_DEVICE_CONTEXT *pDeviceContext, unsigned char *pucPkgContent, int ulPkgLen);

private:
	std::vector<std::wstring> usbPath;
	unsigned char EncryptBuffer[128];
};

