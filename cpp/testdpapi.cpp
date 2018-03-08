// testdpapi.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <Windows.h>
#include <Wincrypt.h>
#pragma comment(lib, "Crypt32.lib")

typedef NTSTATUS(*RtlDecryptMemory)(PVOID Memory, ULONG MemoryLength, ULONG OptionFlags);

void DumpHex(const char *desc, const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	if(desc)
		printf("%s:\n", desc);
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

bool EncryptData(const byte *cbDataIn, const int nLen, const byte *key, const int lenKey, void **encData, int *encLen)
{
	if(!cbDataIn || 0 == nLen)
		return false;
	if(*encData)
		free(*encData);
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB BlobKey;
	DataIn.pbData = const_cast<BYTE *>(cbDataIn);    
	DataIn.cbData = nLen;
	if(key)
	{
		BlobKey.pbData = const_cast<BYTE *>(key);
		BlobKey.cbData = lenKey;
	}
	CRYPTPROTECT_PROMPTSTRUCT promp;
	promp.cbSize = sizeof(CRYPTPROTECT_PROMPTSTRUCT);
	promp.szPrompt  = L"���Լ���";
	promp.dwPromptFlags = CRYPTPROTECT_PROMPT_ON_PROTECT;
	promp.hwndApp = NULL;
	if(!CryptProtectData(&DataIn, L"��������", key ? &BlobKey:NULL, NULL, &promp, 0, &DataOut))
		return false;
	*encLen = DataOut.cbData;
	*encData = malloc(DataOut.cbData);
	memcpy(*encData, DataOut.pbData, DataOut.cbData);
	LocalFree(DataOut.pbData);
	return true;
}

bool DecryptData(const void *encData, const int encLen, const byte *key, const int lenKey, void **cbDataIn, int *nLen)
{
	if(!encData || 0 == encLen)
		return false;
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB BlobKey;
	LPWSTR pDescrOut =  NULL;
	if(key)
	{
		BlobKey.pbData = const_cast<BYTE *>(key);
		BlobKey.cbData = lenKey;
	}
	DataIn.pbData = (BYTE *)const_cast<void *>(encData);    
	DataIn.cbData = encLen;

	CRYPTPROTECT_PROMPTSTRUCT promp;
	promp.cbSize = sizeof(CRYPTPROTECT_PROMPTSTRUCT);
	promp.szPrompt  = L"���Խ���";
	promp.dwPromptFlags = CRYPTPROTECT_PROMPT_ON_UNPROTECT;
	promp.hwndApp = NULL;

	if (!CryptUnprotectData(&DataIn, &pDescrOut, key ? &BlobKey:NULL, NULL, &promp, 0, &DataOut))
		return false;
	
	*nLen = DataOut.cbData;
	*cbDataIn = malloc(DataOut.cbData);
	memcpy(*cbDataIn, DataOut.pbData, DataOut.cbData);
	LocalFree(DataOut.pbData);
	return true;
}

bool EncryptMemoryData(const byte *cbDataIn, const int nLen, void **encData, int *encLen)
{
	if(!cbDataIn || 0 == nLen)
		return false;
	DWORD dwMod = nLen % CRYPTPROTECTMEMORY_BLOCK_SIZE;
	*encLen = nLen;
	if(dwMod > 0)
		*encLen = *encLen + (CRYPTPROTECTMEMORY_BLOCK_SIZE - dwMod);	//���ȱ�����CRYPTPROTECTMEMORY_BLOCK_SIZE�ı���
	*encData = malloc(*encLen);
	ZeroMemory(*encData, *encLen);
	memcpy(*encData, cbDataIn, nLen);
	return (bool)CryptProtectMemory(*encData, *encLen, CRYPTPROTECTMEMORY_SAME_PROCESS);
}

bool DecryptMemoryData(byte *cbDataIn, int nLen)
{
	if(!cbDataIn || 0 == nLen)
		return false;
	return (bool)CryptUnprotectMemory(cbDataIn, nLen, CRYPTPROTECTMEMORY_SAME_PROCESS);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//ʹ��CryptProtectData����Ҫ����������ԭ��
	BYTE *pbDataInput =(BYTE *)"Hello World!";
	DWORD cbDataInput = strlen((char *)pbDataInput)+1;
	void *pEncrptData = NULL;
	int nEncryptLen;

	if(!EncryptData(pbDataInput, cbDataInput, NULL, 0, &pEncrptData, &nEncryptLen))
	{
		fprintf(stderr, "use CryptProtectData encrypt data error!\n");
		exit(-1);
	}
	DumpHex("CryptProtectData", pEncrptData, nEncryptLen);

	//ʹ��CryptUnprotectData������Ҫ��������������
	void *pDecryptData = NULL;
	int nDecryptLen;
	if(!DecryptData(pEncrptData, nEncryptLen, NULL, 0, &pDecryptData, &nDecryptLen))
	{
		fprintf(stderr, "use CryptUnprotectData decrypt encrypt data error!\n");
		exit(-1);
	}
	DumpHex("CryptUnprotectData", pDecryptData, nDecryptLen);
	free(pEncrptData);
	free(pDecryptData);

	
	char *pszOrgData = "ni hao shi jie!";
	void *pEncrptMemoryData = NULL;
	int nEncrypMemorytLen;
	//ʹ��CryptProtectMemory�����ڴ���������
	if(!EncryptMemoryData((byte *)pszOrgData, strlen(pszOrgData), &pEncrptMemoryData, &nEncrypMemorytLen))
	{
		fprintf(stderr, "use CryptProtectMemory encrypt data error!\n");
		exit(-1);
	}
	DumpHex("CryptProtectMemory", pEncrptMemoryData, nEncrypMemorytLen);

	//ʹ��CryptProtectMemory�����ڴ���ܵ���������
	if(!DecryptMemoryData((byte *)pEncrptMemoryData, nEncrypMemorytLen))
	{
		fprintf(stderr, "use CryptUnprotectMemory decrypt encrypt data error!\n");
		exit(-1);
	}
	DumpHex("CryptUnprotectMemory", pEncrptMemoryData, nEncrypMemorytLen);

	free(pEncrptMemoryData);
	SecureZeroMemory(pEncrptMemoryData, nEncrypMemorytLen);
	getchar();
	return 0;
}

