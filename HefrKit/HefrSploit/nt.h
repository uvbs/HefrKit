#pragma once
#include <windows.h>
#include <winternl.h>

typedef enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed = NonPagedPool + 2,
    DontUseThisType,
    NonPagedPoolCacheAligned = NonPagedPool + 4,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS = NonPagedPool + 6,
    MaxPoolType,
    NonPagedPoolBase = 0,
    NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
    NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
    NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,
    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,
    NonPagedPoolNx = 512,
    NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32
} POOL_TYPE;

typedef
NTSTATUS
DRIVER_INITIALIZE(
    _In_ struct _DRIVER_OBJECT *DriverObject,
    _In_ PUNICODE_STRING RegistryPath
);

typedef DRIVER_INITIALIZE *PDRIVER_INITIALIZE;


typedef ULONG(*DbgPrint_t)(
    PCHAR  Format, ...);

typedef
PVOID
(NTAPI * MmGetSystemRoutineAddress_t)(
    PUNICODE_STRING SystemRoutineName);

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
    ULONG tag);

typedef 
NTSTATUS
(NTAPI * IoCreateDriver_t)(
    PUNICODE_STRING DriverName, 
    PDRIVER_INITIALIZE InitializationFunction);

typedef 
NTSTATUS
(NTAPI * PsCreateSystemThread_t)(
    PHANDLE ThreadHandle, 
    ULONG DesiredAccess, 
    POBJECT_ATTRIBUTES ObjectAttributes, 
    HANDLE ProcessHandle,
    LPVOID ClientId,
    LPVOID StartRoutine,
    PVOID StartContext);

typedef 
PIMAGE_NT_HEADERS
(NTAPI * RtlImageNtHeader_t)(
    PVOID ModuleAddress);

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
NTSTATUS
(NTAPI * ZwClose_t)(
    HANDLE handle);

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL         0xC0000001L
#endif

#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER    0xc000000dL
#endif

#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY            0xC0000017L
#endif

#define DECLARE_UNICODE_STRING(_var, _string) \
	WCHAR _var ## _buffer[] = _string; \
	__pragma(warning(push)) \
	__pragma(warning(disable:4221)) __pragma(warning(disable:4204)) \
	UNICODE_STRING _var = { sizeof(_string)-sizeof(WCHAR), sizeof(_string), (PWCH)_var ## _buffer } \
	__pragma(warning(pop))