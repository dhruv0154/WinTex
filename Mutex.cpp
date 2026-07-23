#include "Mutex.h"
#include "Utilities.h"
#include <chrono>

CMutex::CMutex()
{
	_lockCount = 0;
}

CMutex::~CMutex()
{
}

bool CMutex::Lock(int timeout)
{
    if (_mutex.try_lock_for(std::chrono::milliseconds(timeout))) {
        _lockCount++;
        return true;
    }
    return false;
}

void CMutex::Release()
{
    _lockCount--;
    _mutex.unlock();
}
