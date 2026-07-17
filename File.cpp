#include "File.h"
#include "Globals.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <cstring>
#include <strings.h>

namespace fs = std::filesystem;

std::unordered_map<std::string, std::string> CFile::FileMap;

static std::string ResolvePath(const std::string& path) 
{
    if (!gamePath.empty() && path.find(gamePath) == std::string::npos) {
        fs::path p = fs::path(gamePath) / path;
        return p.string();
    }
    return path;
}

CFile::CFile()
{
}

CFile::~CFile()
{
	Close();
}

std::string CFile::Find(std::string path, std::string file)
{
    std::string sPath = ResolvePath(path);
    
    if (sPath.empty()) sPath = ".";

    try {
        if (!fs::exists(sPath)) {
            return "";
        }

        for (const auto& entry : fs::recursive_directory_iterator(sPath, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (strcasecmp(filename.c_str(), file.c_str()) == 0) {
                     std::string foundPath = entry.path().string();
                     return foundPath;
                }
            }
        }
    } catch (...) {}
    return "";
}

bool CFile::Open(const std::string& fileName, Mode mode)
{
	std::string realFile = fileName;
	if (mode == Mode::Read)
	{
		auto it = FileMap.find(fileName);
		if (it != FileMap.end())
		{
			realFile = it->second;
		}
		else
		{
			if (Exists(fileName))
			{
				FileMap[fileName] = fileName;
                realFile = FileMap[fileName];
			}
			else
			{
				std::string path = ".";
				std::string file = fileName;
                
                std::string sFileName = fileName;
                std::replace(sFileName.begin(), sFileName.end(), '\\', '/');
                size_t lastSlash = sFileName.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    file = sFileName.substr(lastSlash + 1);
                    path = sFileName.substr(0, lastSlash);
                }

				realFile = Find(path, file);
                if (!realFile.empty())
				    FileMap[fileName] = realFile;
                else 
                    realFile = fileName;
			}
		}
	}

	std::ios_base::openmode openMode = std::ios::binary;
	if (mode == Mode::Read) openMode |= std::ios::in;
	if (mode == Mode::Write) openMode |= (std::ios::out | std::ios::trunc);

	_stream.open(realFile, openMode);
	return _stream.is_open();
}

void CFile::Close()
{
	if (_stream.is_open())
	{
		_stream.close();
	}
}

uint32_t CFile::Seek(uint32_t distance, SeekMethod method)
{
	if (!_stream.is_open()) return 0;

	std::ios_base::seekdir dir;
	if (method == SeekMethod::Begin) dir = std::ios::beg;
    else if (method == SeekMethod::Current) dir = std::ios::cur;
    else dir = std::ios::end;

	_stream.seekg(distance, dir);
	_stream.seekp(distance, dir);

	return static_cast<uint32_t>(_stream.tellg());
}

int CFile::Read(uint8_t* pBuffer, int length)
{
	if (!_stream.is_open()) return 0;

	_stream.read(reinterpret_cast<char*>(pBuffer), length);

	return static_cast<int>(_stream.gcount());
}

int CFile::Write(uint8_t* pBuffer, int length)
{
	if (!_stream.is_open()) return 0;

	_stream.read(reinterpret_cast<char*>(pBuffer), length);

	return _stream.fail() ? 0 : length;
}

uint32_t CFile::Size()
{
	if (!_stream.is_open()) return 0;
	
	std::streampos currentPos = _stream.tellg();
	_stream.seekg(0, std::ios::end);

	uint32_t size = static_cast<uint32_t>(_stream.tellg());
	_stream.seekg(currentPos, std::ios::beg);

	return size;
}

bool CFile::Exists(const std::string& fileName)
{
    std::string path = ResolvePath(fileName);
    return fs::exists(path);
}
