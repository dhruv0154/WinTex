#pragma once

#include <mutex>

class CMutex
{
public:
	CMutex();
	~CMutex();

	bool Lock(int timeout = 1000000);
	void Release();

protected:

    std::recursive_timed_mutex _mutex;
	int _lockCount;
};
