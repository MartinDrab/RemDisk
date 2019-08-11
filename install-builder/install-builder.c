
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>



#define RESOURCE_PREFIX_X86			L"X86_"
#define RESOURCE_PREFIX_X64			L"X64_"
#define RESOURCE_APPNAME			L"APPNAME"
#define RESOURCE_DESCRIPTION		L"DESCRIPTION"
#define RESOURCE_FILELIST			L"FILES"
#define RESOURCE_INFLIST			L"INFS"



static DWORD _AddFile(HANDLE hUpdate, BOOL x64, const wchar_t *aFileName)
{
	DWORD ret = 0;
	HANDLE hFile = NULL;
	wchar_t fileName[MAX_PATH];
	wchar_t resourceName[MAX_PATH];
	LARGE_INTEGER fileSize;
	void *buffer = NULL;

	if (x64) {
		swprintf(fileName, sizeof(fileName) / sizeof(fileName[0]), L"%ls\\%ls", L"x64\\", aFileName);
		swprintf(resourceName, sizeof(resourceName) / sizeof(resourceName[0]), L"%ls%ls", RESOURCE_PREFIX_X64, aFileName);
	} else {
		swprintf(fileName, sizeof(fileName) / sizeof(fileName[0]), L"%ls\\%ls", L"x86\\", aFileName);
		swprintf(resourceName, sizeof(resourceName) / sizeof(resourceName[0]), L"%ls%ls", RESOURCE_PREFIX_X86, aFileName);
	}

	fprintf(stderr, "Adding file %ls as %ls...\n", fileName, resourceName);
	hFile = CreateFileW(aFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		ret = GetLastError();
		fprintf(stderr, "CreateFileW: %u\n", ret);
		goto Exit;
	}

	if (!GetFileSizeEx(hFile, &fileSize)) {
		ret = GetLastError();
		fprintf(stderr, "GetFileSizeEx: %u\n", ret);
		goto CloseFile;
	}

	buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileSize.LowPart);
	if (buffer == NULL) {
		ret = GetLastError();
		fprintf(stderr, "HeapAlloc: %u\n", ret);
		goto CloseFile;
	}

	if (!ReadFile(hFile, buffer, fileSize.LowPart, &fileSize.LowPart, NULL)) {
		ret = GetLastError();
		fprintf(stderr, "ReadFile: %u\n", ret);
		goto FreeBuffer;
	}

	if (!UpdateResourceW(hUpdate, resourceName, RT_RCDATA, 1033, buffer, fileSize.LowPart)) {
		ret = GetLastError();
		fprintf(stderr, "UpdateResourceW: %u\n", ret);
		goto FreeBuffer;
	}

FreeBuffer:
	HeapFree(GetProcessHeap(), 0, buffer);
CloseFile:
	CloseHandle(hFile);
Exit:
	return ret;
}



int wmain(int argc, wchar_t *argv[])
{
	int ret = 0;
	HANDLE hUpdate = NULL;

	if (argc != 2) {
		fprintf(stderr, "Usage: install-builder <installer>\n");
		return -1;
	}

	hUpdate = BeginUpdateResourceW(argv[1], FALSE);
	if (hUpdate == NULL) {
		ret = GetLastError();
		fprintf(stderr, "BeginUpdateResourceW: %u\n", ret);
		return ret;
	}

	EndUpdateResourceW(hUpdate, ret != 0);

	return 0;
}
