#ifndef _HEFRKIT_H_
#define _HEFRKIT_H_

#include <ntifs.h>

#define __MODULE__  "HefrKit.sys"

#ifdef _DEBUG
#define DPF(x)      DbgPrint x
#else
#define DPF(x)
#endif

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

#endif

