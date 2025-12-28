#pragma once
#include "Win21H2.h"
#include "Common.h"
#include "mem.h"

extern "C" {

    NTSTATUS MmCopyVirtualMemory(
        PEPROCESS FromProcess,
        PVOID FromAddress,
        PEPROCESS ToProcess,
        PVOID ToAddress,
        SIZE_T BufferSize,
        KPROCESSOR_MODE PreviousMode,
        PSIZE_T NumberOfBytesCopied
    );

    NTSTATUS IoCreateDriver(
        PUNICODE_STRING DriverName,
        PDRIVER_INITIALIZE InitializationRoutine
    );

    NTSTATUS DriverEntry(
        _In_ PDRIVER_OBJECT DriverObject,
        _In_ PUNICODE_STRING RegistryPath
    );
}


PLIST_ENTRY PsLoadedModuleList;
PEPROCESS target_process;

typedef INT64(__fastcall* fQword)(ULONG64);
fQword original_qword;

PINFORMATION info = nullptr;


PVOID get_sys_module(UNICODE_STRING& name) {
	if (PsLoadedModuleList != NULL) {

		for (PLIST_ENTRY pListEntry = PsLoadedModuleList->Flink; pListEntry != PsLoadedModuleList; pListEntry = pListEntry->Flink) {

			auto pEntry = CONTAINING_RECORD(pListEntry, _KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
			if (wcsstr(pEntry->FullDllName.Buffer, name.Buffer) != NULL) {
				LOG("[+]Driver Entry Found: 0x%llx\n", (ULONG64)pEntry);
				return pEntry->DllBase;
			}
		}
	}
	return NULL;
}

INT64 __fastcall NtUserQueryDisplayConfig_hk(ULONG64 a1) {
	LOG("[+]Function h00ked, a1: 0x%llx\n", a1);

	const ULONG64 min_addr = 0x10000;
	const ULONG64 max_addr = 0x7FFFFFFEFFFF;

	if (a1 < min_addr || a1 > max_addr) {
		LOG("[!]Parameter is an invalid address \n");
		return original_qword(a1);
	}

	if (ExGetPreviousMode() != UserMode)
		return original_qword(a1);


	MM_COPY_ADDRESS src;
	SIZE_T bt;
	NTSTATUS status = NULL;

	src.VirtualAddress = (PVOID)a1;


	if (info) {
		LOG("[?]Trying to copy buffer\n");

		if (!NT_SUCCESS(MmCopyMemory(info, src, sizeof(INFORMATION), MM_COPY_MEMORY_VIRTUAL, &bt))) {
			LOG("[!]Failed\n");
			return 3221225500;
		}

		LOG("[+]Done, copied %d bytes\n", bt);

		if (info->key != SPECIAL_CALL)
			return 3221225500;

		LOG("[+]Special call \n");

		SIZE_T bytes_read = 0;

		switch (info->operation) {
		case 0:
			ReadProcessMemory(info->process_id, info->target, info->buffer, info->size, &bytes_read);

			info->read = bytes_read;
			LOG("[>]Read operation at address: 0x%llx\n", info->target);
			break;

		default:
			break;
		}
	}

	return 3221225500;
}



void DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    DbgPrint("[*] driver unloaded\n");
}



EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
	UNREFERENCED_PARAMETER(DriverObject);

	UNICODE_STRING rtName;
	RtlInitUnicodeString(&rtName, L"PsLoadedModuleList");
	PsLoadedModuleList = (PLIST_ENTRY)MmGetSystemRoutineAddress(&rtName);


	info = (PINFORMATION)ExAllocatePool(NonPagedPool, sizeof(INFORMATION));

	UNICODE_STRING module_name;
	RtlInitUnicodeString(&module_name, L"win32k.sys");

	auto win32k_base = get_sys_module(module_name);

	if (!win32k_base) {
		LOG("win32k not found\n");
		return -1;
	}

	auto data_ptr = ((ULONG64)win32k_base + 0x66910);


	original_qword = (fQword)InterlockedExchangePointer((PVOID*)data_ptr, (PVOID)NtUserQueryDisplayConfig_hk);



	LOG("[+]Driver Loaded, Original function : 0x % llx\n", original_qword);
	return 0;
}