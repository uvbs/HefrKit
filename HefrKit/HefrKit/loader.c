#include "loader.h"
#include "Hefrkit.h"
#include "shared.h"

// Ensure DriverBootstrap isn't optimized out
#pragma comment(linker, "/include:DriverBootstrap") 
__declspec(dllexport)
NTSTATUS
DriverBootstrap(
    PVOID StartContext);

static
NTSTATUS 
DoRelocation(
    PLDRFUNCS ft, 
    PVOID hDriver);

static
NTSTATUS
FindImports(
    PLDRFUNCS ft, 
    PVOID hDriver);

static
PVOID
GetModuleByName(
    PLDRFUNCS ft,
    LPCSTR driverName);

static
PVOID
GetRoutineByName(
    PLDRFUNCS ft, 
    PVOID hDriver, 
    LPCSTR FunctionName);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DoRelocation)
#pragma alloc_text(INIT, FindImports)
#pragma alloc_text(INIT, GetModuleByName)
#pragma alloc_text(INIT, GetRoutineByName)
#pragma alloc_text(INIT, DriverBootstrap)
#endif


//Not an accurate stricmp! Works fine for our needs
inline BOOLEAN xstricmp(LPCSTR s1, LPCSTR s2) {
    for (ULONG i = 0; 0 == ((s1[i] ^ s2[i]) & 0xDF); ++i)
        if (0 == s1[i]) return TRUE;
    return FALSE;
}

NTSTATUS
DriverBootstrap(
    PVOID lp)
{
    LDRFUNCS ft;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PVOID hDriver = NULL;
    PSHARED_CONTEXT ctx = (PSHARED_CONTEXT)lp;
    PIMAGE_NT_HEADERS nt;

    // Using generated driver name instead
    //DECLARE_CONST_UNICODE_STRING(drvName, L"\\Driver\\Hefr");

    if( NULL == (ft.ExAllocatePoolWithTag = (ExAllocatePoolWithTag_t)ctx->ExAllocatePoolWithTag))
    {
        goto end;
    }

    if (NULL == (ft.ExFreePoolWithTag = (ExFreePoolWithTag_t)ctx->ExFreePoolWithTag))
    {
        goto end;
    }

    if (NULL == (ft.IoCreateDriver = (IoCreateDriver_t)ctx->IoCreateDriver))
    {
        goto end;
    }

    if (NULL == (ft.RtlImageNtHeader = (RtlImageNtHeader_t)ctx->RtlImageNtHeader))
    {
        goto end;
    }

    if (NULL == (ft.RtlQueryModuleInformation = (RtlQueryModuleInformation_t)ctx->RtlQueryModuleInformation))
    {
        goto end;
    }

    if (NULL == (ft.RtlImageDirectoryEntryToData = (RtlImageDirectoryEntryToData_t)ctx->RtlImageDirectoryEntryToData))
    {
        goto end;
    }

    nt = ft.RtlImageNtHeader(ctx->RawDriver);

    hDriver = ft.ExAllocatePoolWithTag(NonPagedPoolExecute, nt->OptionalHeader.SizeOfImage, HEFR_TAG);
    if (NULL == hDriver)
    {
        status = STATUS_NO_MEMORY;
        goto end;
    }
    
    PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(nt);
    __movsq((PULONG64)hDriver, (PULONG64)ctx->RawDriver, nt->OptionalHeader.SizeOfHeaders / sizeof(__int64));
    for (ULONG i = 0; i < nt->FileHeader.NumberOfSections; ++i)
    {
        __movsq((PULONG64)RVA_TO_VA(hDriver, pSection[i].VirtualAddress), (PULONG64)RVA_TO_VA(ctx->RawDriver, pSection[i].PointerToRawData), pSection[i].SizeOfRawData / sizeof(__int64));
    }

    if NT_SUCCESS(DoRelocation(&ft, hDriver)) 
    {
        if NT_SUCCESS(FindImports(&ft, hDriver))
        {
            PDRIVER_INITIALIZE DriverEntry = (PDRIVER_INITIALIZE)RVA_TO_VA(hDriver, nt->OptionalHeader.AddressOfEntryPoint);
            status = ft.IoCreateDriver(NULL, DriverEntry);
        }
    }
    if (!NT_SUCCESS(status))
    {   
        ft.ExFreePoolWithTag(hDriver, HEFR_TAG);
    }

end:
    return status;
}

NTSTATUS 
FindImports(
    PLDRFUNCS ft, 
    PVOID hDriver)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG size;
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)ft->RtlImageDirectoryEntryToData(hDriver, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
    if (pImportDesc)
    {
        for (; pImportDesc->Name; pImportDesc++)
        {
            LPSTR libName = (LPSTR)RVA_TO_VA(hDriver, pImportDesc->Name);
            PVOID hModule = GetModuleByName(ft, libName);
            if (hModule) 
            {
                PIMAGE_THUNK_DATA pNames = (PIMAGE_THUNK_DATA)RVA_TO_VA(hDriver, pImportDesc->OriginalFirstThunk);
                PIMAGE_THUNK_DATA pFuncP = (PIMAGE_THUNK_DATA)RVA_TO_VA(hDriver, pImportDesc->FirstThunk);

                for (; pNames->u1.ForwarderString; ++pNames, ++pFuncP)
                {
                    PIMAGE_IMPORT_BY_NAME pIName = (PIMAGE_IMPORT_BY_NAME)RVA_TO_VA(hDriver, pNames->u1.AddressOfData);
                    PVOID func = GetRoutineByName(ft, hModule, pIName->Name);
                    if (func)
                    {
                        pFuncP->u1.Function = (ULONGLONG)func;
                    }
                    else
                    {
                        return STATUS_PROCEDURE_NOT_FOUND;
                    }
                }
            }
            else
            {
                return STATUS_DRIVER_UNABLE_TO_LOAD;
            }
        }
        status = STATUS_SUCCESS;
    }
    return status;
}

NTSTATUS 
DoRelocation(
    PLDRFUNCS ft, 
    PVOID hDriver)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG size;
    PIMAGE_NT_HEADERS pNTHdr = ft->RtlImageNtHeader(hDriver);
    ULONGLONG delta = (ULONGLONG)hDriver - pNTHdr->OptionalHeader.ImageBase;
    PIMAGE_BASE_RELOCATION pRel = (PIMAGE_BASE_RELOCATION)ft->RtlImageDirectoryEntryToData(hDriver, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &size);
    if (pRel) 
    {
        size = pNTHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        for (ULONG i = 0; i < size; i += pRel->SizeOfBlock, pRel = (PIMAGE_BASE_RELOCATION)((ULONG)pRel + i))
        {
            for (PUSHORT chains = (PUSHORT)((ULONGLONG)pRel + sizeof(IMAGE_BASE_RELOCATION)); chains < (PUSHORT)((ULONGLONG)pRel + pRel->SizeOfBlock); ++chains)
            {
                switch (*chains >> 12)
                {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;
                case IMAGE_REL_BASED_HIGHLOW:
                    *(PULONG)RVA_TO_VA(hDriver, pRel->VirtualAddress + (*chains & 0x0fff)) += (ULONG)delta;
                    break;
                case IMAGE_REL_BASED_DIR64:
                    *(PULONGLONG)RVA_TO_VA(hDriver, pRel->VirtualAddress + (*chains & 0x0fff)) += delta;
                    break;
                default:
                    return STATUS_NOT_IMPLEMENTED;
                }
            }
        }
        status = STATUS_SUCCESS;
    }
    return status;
}

PVOID 
GetRoutineByName(
    PLDRFUNCS ft, 
    PVOID hDriver, 
    LPCSTR FunctionName)
{
    ULONG dirSize;
    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)ft->RtlImageDirectoryEntryToData(hDriver, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &dirSize);
    PULONG names = (PULONG)RVA_TO_VA(hDriver, pExportDir->AddressOfNames);
    PUSHORT ordinals = (PUSHORT)RVA_TO_VA(hDriver, pExportDir->AddressOfNameOrdinals);
    PULONG functions = (PULONG)RVA_TO_VA(hDriver, pExportDir->AddressOfFunctions);
    for (ULONG i = 0; i < pExportDir->NumberOfNames; ++i)
    {
        LPCSTR name = (LPCSTR)RVA_TO_VA(hDriver, names[i]);
        if (0 == strcmp(FunctionName, name))
        {
            return RVA_TO_VA(hDriver, functions[ordinals[i]]);
        }
    }
    return NULL;
}

PVOID 
GetModuleByName(
    PLDRFUNCS ft, 
    LPCSTR driverName)
{
    ULONG size = 0;
    PVOID ImageBase = NULL;
    NTSTATUS status = ft->RtlQueryModuleInformation(&size, sizeof(RTL_MODULE_EXTENDED_INFO), NULL);
    if NT_SUCCESS(status) {
        PRTL_MODULE_EXTENDED_INFO pDrivers = (PRTL_MODULE_EXTENDED_INFO)ft->ExAllocatePoolWithTag(PagedPool, size, HEFR_TAG);
        if (pDrivers) {
            status = ft->RtlQueryModuleInformation(&size, sizeof(RTL_MODULE_EXTENDED_INFO), pDrivers);
            if NT_SUCCESS(status) {
                for (ULONG i = 0; i < size / sizeof(RTL_MODULE_EXTENDED_INFO); ++i) {
                    if (xstricmp(driverName, &pDrivers[i].FullPathName[pDrivers[i].FileNameOffset])) {
                        ImageBase = pDrivers[i].ImageBase;
                        break;
                    }
                }
            }
            ft->ExFreePoolWithTag(pDrivers, HEFR_TAG);
        }
    }
    return ImageBase;
}
