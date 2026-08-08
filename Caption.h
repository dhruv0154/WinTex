#pragma once

class CCaption
{
public:
	CCaption() { _frame = -1; _text = nullptr; _tex = false; _processed = false; }
	CCaption(int frame, const char* text, bool tex) { _frame = frame; _text = text; _tex = tex; _processed = false; }
	~CCaption() { }

	int Frame() { return _frame; }
	const char* Text() { return _text; }
	bool Tex() { return _tex; }
	bool Processed() { return _processed; }
	void SetProcessed(bool processed) { _processed = processed; }

protected:
	int _frame;
	const char* _text;
	bool _tex;
	bool _processed;
};
