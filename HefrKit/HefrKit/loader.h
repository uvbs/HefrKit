#ifndef _LOADER_H_
#define _LOADER_H_

#include <ntifs.h>
#include <ntimage.h>

#pragma warning( disable : 4311 ) // typecast PIMAGE_BASE_RELOCATION to ULONG

#define RVA_TO_VA(base, offset) ((PVOID)((PUCHAR)(base) + (ULONG)(offset)))

typedef
ULONG(*DbgPrint_t)(
    PCHAR  Format, ...);

typedef
PVOID
(NTAPI * ExAllocatePoolWithTag_t)(
    POOL_TYPE PoolType,
    SIZE_T NumberOfBytes,
    ULONG Tag);

 typedef
 VOID
 (NTAPI * ExFreePoolWithTag_t)(
     PVOID P,
     ULONG Tag);

typedef
NTSTATUS
(*IoCreateDriver_t)(
    PUNICODE_STRING DriverName,
    DRIVER_INITIALIZE DriverEntry);

typedef
PVOID
(*MmGetSystemRoutineAddress_t)(
    PUNICODE_STRING SystemRoutineName);

typedef
PVOID
(NTAPI * RtlImageDirectoryEntryToData_t)(
    PVOID Base,
    BOOLEAN MappedAsImage,
    USHORT DirectoryEntry,
    PULONG Size);

typedef
NTSTATUS
(NTAPI * RtlQueryModuleInformation_t)(
    ULONG *InformationLength,
    ULONG SizePerModule,
    PVOID InformationBuffer);

typedef
PIMAGE_NT_HEADERS
(NTAPI * RtlImageNtHeader_t)(
    PVOID ModuleAddress);

typedef struct _RTL_MODULE_EXTENDED_INFO
{
    PVOID ImageBase;
    ULONG ImageSize;
    USHORT FileNameOffset;
    CHAR FullPathName[0x100];
} RTL_MODULE_EXTENDED_INFO, *PRTL_MODULE_EXTENDED_INFO;

typedef struct _LDRFUNCS
{
    ExAllocatePoolWithTag_t ExAllocatePoolWithTag;
    ExFreePoolWithTag_t ExFreePoolWithTag;
    IoCreateDriver_t IoCreateDriver;
    RtlImageNtHeader_t RtlImageNtHeader;
    RtlImageDirectoryEntryToData_t RtlImageDirectoryEntryToData;
    RtlQueryModuleInformation_t RtlQueryModuleInformation;
} LDRFUNCS, *PLDRFUNCS;


#endif