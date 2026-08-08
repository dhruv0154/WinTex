#include "HintCategory.h"
#include "GameController.h"

CHintCategory::CHintCategory(int categoryIndex, std::string title)
{
	_hintCategoryIndex = categoryIndex;
	_title = title;
}

CHintCategory::~CHintCategory()
{
	for (auto hint : Hints)
	{
		delete hint;
	}
	Hints.clear();
}

void CHintCategory::AddHint(int hintIndex, std::string text)
{
	Hints.push_back(new CHint(hintIndex, text));
}

void CHintCategory::Render(float x, float y, bool directory)
{
	if (directory)
	{
		_categoryTextBlack.Render(x, y);
	}
	else
	{
		_categoryText.Render(x, y);
	}
}

void CHintCategory::Prepare(int colBlack, int colBlue, int colCategory, int colOrange, int colGreen, int colScore, int colHighlight, int colShade, Rect directoryRect, Rect categoryRect, Rect hintRect)
{
	_categoryText.SetColours(colCategory, colCategory, colCategory);
	_categoryText.SetText(_title.c_str(), categoryRect);
	_categoryTextBlack.SetColours(colShade, colBlack, colBlack, colHighlight);
	_categoryTextBlack.SetText(_title.c_str(), directoryRect);
}

void CHintCategory::Reset()
{
	for (auto it : Hints)
	{
		it->SetState(-1);
	}
}