#include "Hefrkit.h"

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath); // NULL because of custom loader
    DPF(("\n  ** %s DriverObject   %p\n", __MODULE__, DriverObject));

    DriverObject->DriverUnload = DriverUnload;

    return STATUS_UNSUCCESSFUL;
}

// Currently doesn't get called because of custom loader
// Driver will unload though.
VOID
DriverUnload(
    PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DPF(("\n  ** %s:%s  Unloaded\n", __MODULE__, __FUNCTION__));
}