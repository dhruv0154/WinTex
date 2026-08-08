#include "PDSubLocation.h"

CPDSubLocation::CPDSubLocation(int parent, int id, int travelIndex, int indicatorX, int indicatorY, int nameX, int nameY, int colourIndex, int area, int dmapId, int mapId, int startupPosition, int locationId, int type, int unknown, std::string text) : CSubLocation(parent, text)
{
	Id = id;
	TravelIndex = travelIndex;
	IndicatorX = indicatorX;
	IndicatorY = indicatorY;
	NameX = nameX;
	NameY = nameY;
	ColourIndex = colourIndex;
	DMapId = dmapId;
	Area = area;
	MapId = mapId;
	StartupPosition = startupPosition;
	LocationId = locationId;
	Type = type;
	Unknown = unknown;
}

bool CPDSubLocation::HitTest(int x, int y)
{
	return (x >= IconHitBox.Left && x < IconHitBox.Right && y >= IconHitBox.Top && y < IconHitBox.Bottom) || 
	       (x >= NameHitBox.Left && x < NameHitBox.Right && y >= NameHitBox.Top && y < NameHitBox.Bottom);
}