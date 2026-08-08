#pragma once

#include <string>
#include <list>
#include "Hint.h"
#include "DXMultiColouredText.h"
#include "Structs.h"

class CHintCategory
{
public:
	CHintCategory(int categoryIndex, std::string title);
	~CHintCategory();

	void AddHint(int hintIndex, std::string text);

	void Prepare(int colBlack, int colBlue, int colCategory, int colOrange, int colGreen, int colScore, int colHighlight, int colShade, Rect directoryRect, Rect categoryRect, Rect hintRect);
	void Render(float x, float y, bool directory);

	std::list<CHint*> Hints;

	float Width() { return _categoryText.Width(); }
	void Reset();

private:
	int _hintCategoryIndex;
	std::string _title;

	CDXMultiColouredText _categoryText;
	CDXText _categoryTextBlack;
};