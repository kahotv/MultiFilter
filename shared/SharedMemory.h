#pragma once
class SharedMemory
{
public:
	SharedMemory(const std::wstring& sm_name, int maxsize);
	~SharedMemory();

	bool Write(const void* data, int size);
	bool Read(void* data, int size);
	template<typename T>
	bool Write(const T& obj)
	{
		return Write(&obj, sizeof(obj));
	}
	template<typename T>
	bool Read(T& obj)
	{
		return Read(&obj, sizeof(obj));
	}
private:
	bool Open(const std::wstring& sm_name, int maxsize);
	void Close();

private:

	std::wstring _name;
	bool _run;


	HANDLE _hMapFile;
	void* _lpData;
	const int _maxsize;
};

