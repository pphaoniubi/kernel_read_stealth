#pragma once


#define SPECIAL_CALL 1337


#pragma pack(push, 1)
typedef struct _INFORMATION {
	INT key;
	CHAR operation;
	HANDLE process_id;
	PVOID target;
	PVOID buffer;
	SIZE_T read;	//return_size
	SIZE_T size;

}INFORMATION, * PINFORMATION;
#pragma pack(pop)