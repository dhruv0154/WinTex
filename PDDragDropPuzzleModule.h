#pragma once

#include "FullScreenModule.h"
#include "RawFont.h"
#include "PuzzlePiece.h"
#include "Structs.h"

class CPDDragDropPuzzleModule : public CFullScreenModule
{
public:
	CPDDragDropPuzzleModule(int puzzleIndex);
	~CPDDragDropPuzzleModule();

	virtual void Render();

protected:
	virtual void Initialize();

	CRawFont _pdRawFont;

	void RenderScreen();

	// Input related
	virtual void Cursor(float x, float y, bool relative);
	virtual void BeginAction();
	virtual void EndAction();
	virtual void Back();
	virtual void Cycle();
	virtual void Next();
	virtual void Prev();

	int _puzzleIndex;

	bool _hasBonusScore;
	int _scoreToAdd;
	int _bonusScore;
	int _bonusDropSpeed;
	int _timeOrMoves;
	int _timeOrFreeMoves;

	static const char* FileNames[];
	static int PuzzleFiles[];
	static int PuzzleEntries[];
	static int PuzzlePiecesCount[];
	static int PuzzleDataOffsets[];

	int _numberOfPieces;
	int _positionOffset;
	int _imageOffset;

	CPuzzlePiece* _selectedPiece;
	Point _pt;

	bool _completed;
	bool CheckCompleted();
	bool CheckPandoraPuzzleCompleted();
	bool CheckFiguresPuzzleCompleted();
	bool CheckTornNotePuzzleCompleted();
	bool CheckHolePunchPuzzleCompleted();
	bool CheckLabyrinthPuzzleCompleted();
	bool CheckDaggerPuzzleCompleted();
	bool CheckTornPhotoPuzzleCompleted();
	bool CheckLaptopPuzzleCompleted();

	bool _cheated;
};