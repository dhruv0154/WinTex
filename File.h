#pragma once

#include <unordered_map>
#include <string>
#include <fstream>
#include <cstdint>

class CFile
{
public:
	CFile();
	~CFile();

	enum class Mode
	{
		Read,
		Write
	};

	enum class SeekMethod
	{
		Begin,
		Current,
		End
	};

	bool Open(const std::string& Name, Mode mode = Mode::Read);
	void Close();
	uint32_t Seek(uint32_t distance, SeekMethod method = SeekMethod::Begin);
	int Read(LPBYTE pBuffer, int length);
	int Write(LPBYTE pBuffer, int length);
	uint32_t Size();

	static bool Exists(const std::string& fileName);

protected:
	std::fstream _stream;

	static std::unordered_map<std::string, std::string> FileMap;
	static std::string Find(std::string path, std::string file);
};
