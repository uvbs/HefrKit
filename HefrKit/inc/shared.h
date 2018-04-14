#ifndef _SHARED_H_
#define _SHARED_H_

#define HEFR_TAG 'rfeH'

#pragma pack(1)
typedef struct _SHARED_CONTEXT
{
    void * ExAllocatePoolWithTag;
    void * ExFreePoolWithTag;
    void * RtlImageNtHeader;
    void * RtlImageDirectoryEntryToData;
    void * RtlQueryModuleInformation;
    void * IoCreateDriver;
    void * RawDriver;
    unsigned long RawDriverSize;
    unsigned long BootstrapOffset;
} SHARED_CONTEXT, *PSHARED_CONTEXT;
#pragma pack()

#endif