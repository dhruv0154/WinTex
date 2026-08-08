#include "PDElevationModOverlay.h"
#include "Utilities.h"
#include <chrono>

CPDElevationModOverlay::CPDElevationModOverlay()
{
	_targetY = 0.0f;
	_speed = 0.0f;
	_lastUpdate = 0;
}

void CPDElevationModOverlay::Render()
{
	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	float diff = (float)(now - _lastUpdate);
	if (diff > 0.0f)
	{
		CLocation::_y_player_adjustment -= _speed / diff;
		if ((_speed > 0.0f && CLocation::_y_player_adjustment <= _targetY) || (_speed < 0.0f && CLocation::_y_player_adjustment >= _targetY))
		{
			BeginAction();
		}
		_lastUpdate = now;
	}
}

void CPDElevationModOverlay::BeginAction()
{
	CLocation::_y_player_adjustment = _targetY;
	pOverlay = nullptr;
}

void CPDElevationModOverlay::SetData(int p1, int p2)
{
	_targetY = -From12_4(p1);
	_speed = From12_4(p2) * 3;
	_lastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
