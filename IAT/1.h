/*
#include <ntifs.h>
#include <ntimage.h>


void debug_print(PCSTR text) {
    UNREFERENCED_PARAMETER(text);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "%s\n", text));
}

namespace driver {
    namespace codes {
        constexpr ULONG attach =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        constexpr ULONG read =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

        constexpr ULONG write =
            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    }

    struct Request {
        HANDLE processId;

        PVOID target;
        PVOID buffer;

        SIZE_T size;
        SIZE_T return_size;


    };

    NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return irp->IoStatus.Status;
    }

    NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return irp->IoStatus.Status;
    }


    NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
        UNREFERENCED_PARAMETER(device_object);

        debug_print("[+] inside device control\n");

        NTSTATUS status = STATUS_UNSUCCESSFUL;

        PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

        auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer);

        if (stack_irp == nullptr || request == nullptr) {
            debug_print("[-] invalid parameters\n");
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return status;
        }

        static PEPROCESS target_process = nullptr;

        const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;

        switch (control_code) {

        case codes::attach:
            status = PsLookupProcessByProcessId(request->processId, &target_process);
            break;

        case codes::read:
            if (target_process != nullptr) {
                status =
                    MmCopyVirtualMemory(target_process,
                        request->target,
                        PsGetCurrentProcess(),
                        request->buffer, request->size,
                        KernelMode,
                        &request->return_size);
            }
            DbgPrintEx(
                DPFLTR_IHVDRIVER_ID,
                DPFLTR_INFO_LEVEL,
                "MmCopyVirtualMemory status: 0x%08X, bytes: %llu\n",
                status,
                request->return_size
            );
            break;

        case codes::write:
            if (target_process != nullptr) {
                status =
                    MmCopyVirtualMemory(PsGetCurrentProcess(),
                        request->buffer,
                        target_process,
                        request->target, request->size,
                        KernelMode,
                        &request->return_size);
            }
            break;
        default:
            break;


        } // switch control_code

        irp->IoStatus.Status = status;
        irp->IoStatus.Information = sizeof(Request);

        IoCompleteRequest(irp, IO_NO_INCREMENT);

        return irp->IoStatus.Status;

    } // device_control

}   // namespace driver


VOID UnloadDriver(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

}

NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);

    UNICODE_STRING dev_name, sym_link;
    PDEVICE_OBJECT dev_obj;

    RtlInitUnicodeString(&dev_name, L"\\Device\\IAT_Driver"); //die lit
    auto status = IoCreateDevice(driver_object, 0, &dev_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev_obj);
    if (status != STATUS_SUCCESS) return status;

    RtlInitUnicodeString(&sym_link, L"\\DosDevices\\IAT_Driver");
    status = IoCreateSymbolicLink(&sym_link, &dev_name);
    if (status != STATUS_SUCCESS) return status;

    SetFlag(dev_obj->Flags, DO_BUFFERED_IO); //set DO_BUFFERED_IO bit to 1


    //then set supported functions to appropriate handlers
    driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
    driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
    driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;
    driver_object->DriverUnload = NULL;

    ClearFlag(dev_obj->Flags, DO_DEVICE_INITIALIZING);
    return status;
}

NTSTATUS DriverEntry()
{
    debug_print("[+] IAT Driver Loaded\n");

    UNICODE_STRING driver_name = {};
    RtlInitUnicodeString(&driver_name, L"\\Driver\\IAT_Driver");
    return IoCreateDriver(&driver_name, &driver_main);
}


*/