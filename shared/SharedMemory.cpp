#include "stdafx.h"
#include "SharedMemory.h"

SharedMemory::SharedMemory(const std::wstring& sm_name, int maxsize) :
	_run(false),
	_maxsize(maxsize),
	_hMapFile(NULL),
	_lpData(NULL)
{
	printf("[SharedMemory] malloc %p\n", this);
	_run = Open(sm_name, maxsize);
}
SharedMemory::~SharedMemory()
{
	printf("[SharedMemory] free %p\n", this);
	Close();
}

bool SharedMemory::Write(const void* data, int size)
{
	//未启动
	if (!_run)
		_run = Open(_name, _maxsize);
	if (!_run)
		return false;

	//入参错误
	if (data == NULL || size <= 0)
		return false;

	//超长
	if (size > _maxsize - 4)
		return false;

	//错误
	if (_lpData == NULL)
		return false;

	uint32_t *sm_data_size = (uint32_t*)_lpData;
	*sm_data_size = size;

	memcpy((char*)_lpData + 4, data, size);

	printf("[SharedMemory] set succ data: %p, size: %d\n", data, size);

	return true;
}
bool SharedMemory::Read(void* data, int size)
{
	if (!_run) {
		_run = Open(_name, _maxsize);
		if (!_run)
			return false;
	}
	if (data == NULL || size <= 0)
		return false;
	if (size > _maxsize - 4)
		return false;
	if (_lpData == NULL)
		return false;

	//可读长度错误
	uint32_t *sm_data_size = (uint32_t*)_lpData;
	if (*sm_data_size != size)
		return false;

	memcpy(data, (const char*)_lpData + 4, size);

	printf("[SharedMemory] get succ data: %p, size: %d\n", data, size);
	return true;
}

bool SharedMemory::Open(const std::wstring& sm_name, int maxsize)
{
	//如果没打开过或者长度比上次的长
	HANDLE tmp = NULL;
	bool succ = false;
	do
	{
		SECURITY_ATTRIBUTES sa;
		SECURITY_DESCRIPTOR sd;
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, true, NULL, false);
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = &sd;
		sa.bInheritHandle = false;

		HANDLE tmp = CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE | SEC_COMMIT, 0, maxsize, sm_name.c_str());
		if (tmp <= 0)
		{
			printf("[SharedMemory] CreateFileMappingW error: %d | name: %ws, maxsize: %d\n", GetLastError(), sm_name.c_str(), maxsize);
			break;
		}

		VOID* tmpData = MapViewOfFile(tmp, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		if (tmpData <= 0)
		{
			printf("[SharedMemory] MapViewOfFile() error: %d | name: %ws, maxsize: %d\n", GetLastError(), sm_name.c_str(), maxsize);
			break;
		}
		printf("[SharedMemory] open() succ | name: %ws, maxsize: %d\n", sm_name.c_str(), maxsize);

		_hMapFile = tmp;
		_lpData = tmpData;
		succ = true;

	} while (false);

	if (!succ)
	{
		if (tmp > 0)
		{
			CloseHandle(tmp);
			tmp = NULL;
		}
	}

	return succ;

}
void SharedMemory::Close()
{
	if (_run)
	{
		_run = false;

		CloseHandle(_hMapFile);
		UnmapViewOfFile(_lpData);
		_hMapFile = NULL;
		_lpData = NULL;
	}
}