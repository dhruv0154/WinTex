#pragma once
#include "SubLocation.h"
#include "Structs.h"
#include <string>

class CPDSubLocation : public CSubLocation
{
public:
	CPDSubLocation(int parent, int id, int travelIndex, int indicatorX, int indicatorY, int nameX, int nameY, int colourIndex, int area, int dmapId, int mapId, int startupPosition, int locationId, int type, int unknown, std::string text);

	int Id;
	int TravelIndex;
	int IndicatorX;
	int IndicatorY;
	int NameX;
	int NameY;
	int ColourIndex;
	int Area;
	int DMapId;
	int MapId;
	int StartupPosition;
	int LocationId;
	int Type;
	int Unknown;
	Rect IconHitBox;
	Rect NameHitBox;

	bool HitTest(int x, int y);
};