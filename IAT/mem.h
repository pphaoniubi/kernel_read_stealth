#pragma once
#include "Win21H2.h"
//#include <stdint.h>

extern "C" {
	NTKERNELAPI
		PVOID
		PsGetProcessSectionBaseAddress(
			__in PEPROCESS Process
		);
}

PVOID GetProcessBaseAddress(int pid)
{
	PEPROCESS pProcess = NULL;
	if (pid == 0) return NULL;

	NTSTATUS NtRet = PsLookupProcessByProcessId((HANDLE)pid, &pProcess);
	if (NtRet != STATUS_SUCCESS) return NULL;

	PVOID Base = PsGetProcessSectionBaseAddress(pProcess);
	ObDereferenceObject(pProcess);
	return Base;
}

//https://ntdiff.github.io/
#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19569
#define WINDOWS_21H1 20180
#define WINDOWS_22H2 19045

ULONG GetUserDirectoryTableBaseOffset()
{
	RTL_OSVERSIONINFOW ver = { 0 };
	RtlGetVersion(&ver);

	switch (ver.dwBuildNumber)
	{
	case WINDOWS_1803:
		return 0x0278;
		break;
	case WINDOWS_1809:
		return 0x0278;
		break;
	case WINDOWS_1903:
		return 0x0280;
		break;
	case WINDOWS_1909:
		return 0x0280;
		break;
	case WINDOWS_2004:
		return 0x0388;
		break;
	case WINDOWS_20H2:
		return 0x0388;
		break;
	case WINDOWS_21H1:
		return 0x0388;
		break;
	case WINDOWS_22H2:
		return 0x0388;
		break;
	default:
		return 0x0388;
	}
}

//check normal dirbase if 0 then get from UserDirectoryTableBas
ULONG_PTR GetProcessCr3(PEPROCESS pProcess)
{
	PUCHAR process = (PUCHAR)pProcess;
	ULONG_PTR process_dirbase = *(PULONG_PTR)(process + 0x28); //dirbase x64, 32bit is 0x18
	if (process_dirbase == 0)
	{
		ULONG UserDirOffset = GetUserDirectoryTableBaseOffset();
		ULONG_PTR process_userdirbase = *(PULONG_PTR)(process + UserDirOffset);
		return process_userdirbase;
	}
	return process_dirbase;
}


ULONG_PTR GetKernelDirBase()
{
	PUCHAR process = (PUCHAR)PsGetCurrentProcess();
	ULONG_PTR cr3 = *(PULONG_PTR)(process + 0x28); //dirbase x64, 32bit is 0x18
	return cr3;
}


//MmMapIoSpaceEx limit is page 4096 byte
NTSTATUS WritePhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten)
{
	if (!TargetAddress)
		return STATUS_UNSUCCESSFUL;

	PHYSICAL_ADDRESS AddrToWrite = { 0 };
	AddrToWrite.QuadPart = (ULONG_PTR)TargetAddress;

	PVOID pmapped_mem = MmMapIoSpaceEx(AddrToWrite, Size, PAGE_READWRITE);

	if (!pmapped_mem)
		return STATUS_UNSUCCESSFUL;

	memcpy(pmapped_mem, lpBuffer, Size);

	*BytesWritten = Size;
	MmUnmapIoSpace(pmapped_mem, Size);
	return STATUS_SUCCESS;
}

NTSTATUS ReadPhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead)
{
	MM_COPY_ADDRESS AddrToRead = { 0 };
	AddrToRead.PhysicalAddress.QuadPart = (ULONG_PTR)TargetAddress;
	return MmCopyMemory(lpBuffer, AddrToRead, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead);
}

#define PAGE_OFFSET_SIZE 12
#define PMASK 0x000FFFFFFFFFF000ULL
ULONGLONG TranslateLinearAddress(ULONGLONG directoryTableBase, ULONGLONG virtualAddress) {
	directoryTableBase &= ~0xf;

	ULONGLONG pageOffset = virtualAddress & ~(~0ul << PAGE_OFFSET_SIZE);
	ULONGLONG pte = ((virtualAddress >> 12) & (0x1ffll));
	ULONGLONG pt = ((virtualAddress >> 21) & (0x1ffll));
	ULONGLONG pd = ((virtualAddress >> 30) & (0x1ffll));
	ULONGLONG pdp = ((virtualAddress >> 39) & (0x1ffll));

	SIZE_T readsize = 0;
	ULONGLONG pdpe = 0;
	ReadPhysicalAddress((PVOID)(directoryTableBase + 8 * pdp), &pdpe, sizeof(pdpe), &readsize);
	if (~pdpe & 1)
		return 0;

	ULONGLONG pde = 0;
	ReadPhysicalAddress((PVOID)((pdpe & PMASK) + 8 * pd), &pde, sizeof(pde), &readsize);
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pdpe & (1ULL << 7))
		return (pdpe & 0x000FFFFFC0000000ULL) |
		(virtualAddress & 0x3FFFFFFF);

	/* 2MB large page */
	if (pde & (1ULL << 7)) {
		return (pde & 0x000FFFFFFFE00000ULL) |
			(virtualAddress & 0x1FFFFF);
	}

	ULONGLONG pteAddr = 0;
	ReadPhysicalAddress((PVOID)((pde & PMASK) + 8 * pt), &pteAddr, sizeof(pteAddr), &readsize);
	if (~pteAddr & 1)
		return 0;

	ReadPhysicalAddress((PVOID)((pteAddr & PMASK) + 8 * pte), &virtualAddress, sizeof(virtualAddress), &readsize);
	virtualAddress &= PMASK;

	if (!virtualAddress)
		return 0;

	LOG("[+]Finish TranslateLinearAddress\n");
	return virtualAddress + pageOffset;

}


//
NTSTATUS ReadProcessMemory(HANDLE pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read)
{	
	PEPROCESS pProcess = NULL;
	if (pid == 0) return STATUS_UNSUCCESSFUL;

	NTSTATUS NtRet = PsLookupProcessByProcessId(pid, &pProcess);
	if (NtRet != STATUS_SUCCESS) return NtRet;

	ULONG_PTR process_dirbase = GetProcessCr3(pProcess);
	ObDereferenceObject(pProcess);

	SIZE_T CurOffset = 0;
	SIZE_T TotalSize = size;
	while (TotalSize)
	{

		ULONGLONG CurPhysAddr = TranslateLinearAddress(process_dirbase, (ULONG64)Address + CurOffset);
		if (!CurPhysAddr) return STATUS_UNSUCCESSFUL;

		ULONG64 ReadSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
		SIZE_T BytesRead = 0;
		NtRet = ReadPhysicalAddress((PVOID)CurPhysAddr, (PVOID)((ULONG64)AllocatedBuffer + CurOffset), ReadSize, &BytesRead);
		TotalSize -= BytesRead;
		CurOffset += BytesRead;
		if (NtRet != STATUS_SUCCESS) break;
		if (BytesRead == 0) break;
	}

	LOG("[+]Finish ReadProcessMemory\n");
	*read = CurOffset;
	return NtRet;
}

NTSTATUS ReadVirtual(ULONGLONG dirbase, ULONGLONG address, UCHAR* buffer, SIZE_T size, SIZE_T* read)
{
	ULONGLONG paddress = TranslateLinearAddress(dirbase, address);
	return ReadPhysicalAddress((PVOID)paddress, buffer, size, read);
}

NTSTATUS WriteVirtual(ULONGLONG dirbase, ULONGLONG address, UCHAR* buffer, SIZE_T size, SIZE_T* written)
{
	ULONGLONG paddress = TranslateLinearAddress(dirbase, address);
	return WritePhysicalAddress((PVOID)paddress, buffer, size, written);
}


NTSTATUS WriteProcessMemory(int pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written)
{
	PEPROCESS pProcess = NULL;
	if (pid == 0) return STATUS_UNSUCCESSFUL;

	NTSTATUS NtRet = PsLookupProcessByProcessId((HANDLE)pid, &pProcess);
	if (NtRet != STATUS_SUCCESS) return NtRet;

	ULONG_PTR process_dirbase = GetProcessCr3(pProcess);
	ObDereferenceObject(pProcess);

	SIZE_T CurOffset = 0;
	SIZE_T TotalSize = size;
	while (TotalSize)
	{
		ULONGLONG CurPhysAddr = TranslateLinearAddress(process_dirbase, (ULONG64)Address + CurOffset);
		if (!CurPhysAddr) return STATUS_UNSUCCESSFUL;

		ULONG64 WriteSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
		SIZE_T BytesWritten = 0;
		NtRet = WritePhysicalAddress((PVOID)CurPhysAddr, (PVOID)((ULONG64)AllocatedBuffer + CurOffset), WriteSize, &BytesWritten);
		TotalSize -= BytesWritten;
		CurOffset += BytesWritten;
		if (NtRet != STATUS_SUCCESS) break;
		if (BytesWritten == 0) break;
	}

	*written = CurOffset;
	return NtRet;
}