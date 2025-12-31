#include "File.h"
#include "Platform.h"
#include <iostream>

#ifdef PLATFORM_LINUX
#include <fstream>
#include <filesystem>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <vector>
#include <cstring>

namespace fs = std::filesystem;

// Helper to convert wstring to string
static std::string WStrToStr(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    size_t size_needed = wcstombs(nullptr, wstr.c_str(), 0);
    if (size_needed == (size_t)-1) return "";
    std::vector<char> str(size_needed + 1);
    wcstombs(str.data(), wstr.c_str(), size_needed + 1);
    return std::string(str.data());
}

static std::wstring StrToWStr(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    try {
        return converter.from_bytes(str);
    } catch (...) {
        return L"";
    }
}
#endif

std::unordered_map<std::wstring, std::wstring> CFile::FileMap;

CFile::CFile()
{
	_handle = INVALID_HANDLE_VALUE;
}

CFile::~CFile()
{
	Close();
}

std::wstring CFile::Find(std::wstring path, std::wstring file)
{
#ifdef PLATFORM_LINUX
    std::string sPath = WStrToStr(path);
    std::string sFile = WStrToStr(file);
    
    if (sPath.empty()) sPath = ".";
    // Normalize slashes
    std::replace(sPath.begin(), sPath.end(), '\\', '/');

    try {
        if (!fs::exists(sPath)) {
             std::cerr << "CFile::Find: Path " << sPath << " does not exist." << std::endl;
             return L"";
        }

        // Iterative search using recursive_directory_iterator is easier but the original code
        // implements manual recursion. We can use recursive_directory_iterator.
        // But we need to be careful about matching the file name case-insensitively.
        
        // std::cerr << "CFile::Find searching for " << sFile << " in " << sPath << std::endl;
        
        for (const auto& entry : fs::recursive_directory_iterator(sPath, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // std::cerr << "Checking " << filename << std::endl;
                if (strcasecmp(filename.c_str(), sFile.c_str()) == 0) {
                     std::string foundPath = entry.path().string();
                     std::cerr << "CFile::Find: Found " << sFile << " at " << foundPath << std::endl;
                     return StrToWStr(foundPath);
                }
            }
        }
        std::cerr << "CFile::Find: Could not find " << sFile << " in " << sPath << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "CFile::Find error: " << ex.what() << std::endl;
    } catch (...) {
        // Ignore permission errors etc
    }
    return L"";
#else
	// Enumerate files, return if file is found
	// Enumerate folders, search each one
	std::wstring foundFile;

	WIN32_FIND_DATA fd;
	HANDLE hFF = FindFirstFile((path + L"*").c_str(), &fd);
    if (hFF != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring enumeratedFile = fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if ((enumeratedFile != L".") && (enumeratedFile != L".."))
                {
                    foundFile = Find(path + enumeratedFile + L"\\", file);

                    if (foundFile.size() > 0)
                    {
                        break;
                    }
                }
            }
            else
            {
                if (enumeratedFile == file)
                {
                    foundFile = path + file;
                    break;
                }
            }
        } while (FindNextFile(hFF, &fd));

        FindClose(hFF);
    }

	return foundFile;
#endif
}

BOOL CFile::Open(LPCWSTR fileName, Mode mode, Sharing share)
{
#ifdef PLATFORM_LINUX
	std::wstring realFile = fileName;
	if (mode == Mode::Read)
	{
		std::unordered_map<std::wstring, std::wstring>::iterator it = FileMap.find(fileName);
		if (it != FileMap.end())
		{
			realFile = it->second;
		}
		else
		{
            std::string sFileName = WStrToStr(fileName);
            std::replace(sFileName.begin(), sFileName.end(), '\\', '/');

			if (fs::exists(sFileName))
			{
				// File exists, add to map
				FileMap[fileName] = fileName;
			}
			else
			{
				// File does not exist in expected path, perform a recursive search
                // Use current directory as base
				std::wstring path = L".";
				std::wstring file = fileName;
                
                // Extract filename from path if it has directory components
                size_t lastSlash = sFileName.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    file = StrToWStr(sFileName.substr(lastSlash + 1));
                }

				realFile = Find(path, file);

                if (!realFile.empty())
				    FileMap[fileName] = realFile;
                else 
                    realFile = fileName; // Try original anyway
			}
		}
	}

    std::string sRealFile = WStrToStr(realFile);
    std::replace(sRealFile.begin(), sRealFile.end(), '\\', '/');

	// Creation c = (mode == Mode::Read) ? Creation::OpenExisting : (mode == Mode::Write) ? Creation::CreateAlways : Creation::OpenAlways;
	
    std::ios_base::openmode openMode = std::ios_base::binary;
    if (mode == Mode::Read) openMode |= std::ios_base::in;
    if (mode == Mode::Write) openMode |= std::ios_base::out | std::ios_base::trunc;

    std::fstream* fs = new std::fstream();
    fs->open(sRealFile, openMode);

    if (fs->is_open()) {
        _handle = (HANDLE)fs;
        return TRUE;
    } else {
        delete fs;
        // std::cerr << "Failed to open file: " << sRealFile << std::endl;
        _handle = INVALID_HANDLE_VALUE;
        return FALSE;
    }
#else
	// Check map to get real file path
	std::wstring realFile = fileName;
	if (mode == Mode::Read)
	{
		std::unordered_map<std::wstring, std::wstring>::iterator it = FileMap.find(fileName);
		if (it != FileMap.end())
		{
			realFile = it->second;
		}
		else
		{
			if (Exists(fileName))
			{
				// File exists, add to map
				FileMap[fileName] = fileName;
			}
			else
			{
				// File does not exist in expected path, perform a recursive search
				std::wstring path = L"";
				std::wstring file = L"";
				size_t lastSeparator = realFile.find_last_of('\\');
				if (lastSeparator != -1)
				{
					path = realFile.substr(0, lastSeparator + 1);
					file = realFile.substr(lastSeparator + 1);
				}
				else
				{
					path = L".\\";
					file = realFile;
				}

				realFile = Find(path, file);

				FileMap[fileName] = realFile;
			}
		}
	}

	Creation c = (mode == Mode::Read) ? Creation::OpenExisting : (mode == Mode::Write) ? Creation::CreateAlways : Creation::OpenAlways;
	_handle = CreateFile(realFile.c_str(), (DWORD)mode, (DWORD)share, NULL, (DWORD)c, (DWORD)Flags::Normal, NULL);
	return (_handle != INVALID_HANDLE_VALUE);
#endif
}

void CFile::Close()
{
#ifdef PLATFORM_LINUX
    if (_handle != INVALID_HANDLE_VALUE)
    {
        std::fstream* fs = (std::fstream*)_handle;
        fs->close();
        delete fs;
        _handle = INVALID_HANDLE_VALUE;
    }
#else
	if (_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_handle);
		_handle = INVALID_HANDLE_VALUE;
	}
#endif
}

DWORD CFile::Seek(DWORD distance, SeekMethod method)
{
#ifdef PLATFORM_LINUX
    if (_handle == INVALID_HANDLE_VALUE) return 0;
    std::fstream* fs = (std::fstream*)_handle;
    
    std::ios_base::seekdir dir = std::ios_base::beg;
    if (method == SeekMethod::Current) dir = std::ios_base::cur;
    if (method == SeekMethod::End) dir = std::ios_base::end;
    
    fs->seekg(distance, dir);
    if (fs->fail()) fs->clear();
    
    return (DWORD)fs->tellg();
#else
	return SetFilePointer(_handle, distance, NULL, (DWORD)method);
#endif
}

int CFile::Read(LPBYTE pBuffer, int length)
{
#ifdef PLATFORM_LINUX
    if (_handle == INVALID_HANDLE_VALUE) return 0;
    std::fstream* fs = (std::fstream*)_handle;
    fs->read((char*)pBuffer, length);
    return (int)fs->gcount();
#else
	int read = 0;
	ReadFile(_handle, pBuffer, length, (LPDWORD)&read, NULL);
	return read;
#endif
}

int CFile::Write(LPBYTE pBuffer, int length)
{
#ifdef PLATFORM_LINUX
    if (_handle == INVALID_HANDLE_VALUE) return 0;
    std::fstream* fs = (std::fstream*)_handle;
    fs->write((char*)pBuffer, length);
    return fs->bad() ? 0 : length;
#else
	int written = 0;
	WriteFile(_handle, pBuffer, length, (LPDWORD)&written, NULL);
	return written;
#endif
}

DWORD CFile::Size()
{
#ifdef PLATFORM_LINUX
    if (_handle == INVALID_HANDLE_VALUE) return 0;
    std::fstream* fs = (std::fstream*)_handle;
    std::streampos current = fs->tellg();
    fs->seekg(0, std::ios::end);
    std::streampos end = fs->tellg();
    fs->seekg(current, std::ios::beg);
    fs->clear();
    return (DWORD)end;
#else
    LARGE_INTEGER li;
    ::GetFileSizeEx(_handle, &li);
    return li.LowPart;
#endif
}

BOOL CFile::Exists(LPCWSTR fileName)
{
#ifdef PLATFORM_LINUX
    std::string sFileName = WStrToStr(fileName);
    std::replace(sFileName.begin(), sFileName.end(), '\\', '/');
    return fs::exists(sFileName);
#else
	WIN32_FIND_DATA findFileData;
	HANDLE handle = FindFirstFile(fileName, &findFileData);
	BOOL found = (handle != INVALID_HANDLE_VALUE);
	if (found)
	{
		FindClose(handle);
	}

	return found;
#endif
}
