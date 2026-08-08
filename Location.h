#pragma once

#include "Map.h"
#include <vector>
#include "LZ.h"
#include "Texture.h"
#include <unordered_map>
#include "DXText.h"
#include "Mutex.h"
#include "LocationObject.h"
#include "PointList.h"
#include "LocationSprite.h"
#include "LocationStructs.h"
#include "ObjectMap.h"
#include "Elevation.h"
#include "ShaderStructs.h"
#include <cstdint>

#define MAX_ANIMATIONS		100

class CLocation
{
public:
	CLocation();
	~CLocation();

	bool Load(int locationFileIndex);

	void Render();

	static void SetPosition(StartupPosition pos);
	static void SetPosition(float x, float y, float z, float angle);
	void Move(float mx, float my, float mz, float tmx);
	void DeltaAngles(float angle1, float angle2);

	bool PointingChanged;

	int GetPickObject(int& objectId, int& subObjectId);
	void SetObjectVisibility(int objectId, bool visible);

	void StartMappedAnimation(int index);
	void StartIndexedAnimation(int index);
	void StartIdAnimation(int index);
	void StopMappedAnimation(int index);
	void StopIndexedAnimation(int index);
	bool IsAnimationFinished(int index);
	bool IsIndexedAnimationFinished(int index);
	int GetAnimationFrame(int index);
	int GetIndexedAnimationFrame(int index);
	void Animate();

	double GetPlayerDistanceFromPoint(double x, double z);
	Point GetPlayerPosition();
	Point GetUnadjustedPlayerPosition();
	SpritePosInfo GetSpriteInfo(int index);

	static void SetMinY(float minY);
	static void SetMaxY(float maxY);

	CMapData* _mapEntry;
	//CDMap::DMapEntry _dmapEntry;

	void UpdateSprites();

	static float _x;
	static float _y;
	static float _z;
	static float _angle1;
	static float _angle2;
	static float _y_player_adjustment;
	static float _y_player_adjustment_min;
	static float _y_player_adjustment_max;
	static float _y_elevation;

#ifdef DEBUG
	void MoveObject(float delta, bool X, bool Y, bool Z);
#endif

protected:
	void UpdateY()
	{
		_y = _y_elevation + _y_player_adjustment;
	}

	static CElevation* _pCurrentElevation;

	int _currentLocationId;

	//CMutex _locationMutex;

	void Clear();

	uint8_t* _locationData;

	void LoadPaths();
	void LoadTextures();

	int _verticeCount;
	Point* _points;
	ID3D11Buffer* _vertexBuffer;

	static bool _loading;

	CLocationObject* _pLocObjects;
	CLocationSubObject* _pLocSubObjects;

	struct Object
	{
		int TextureIndex;
		std::vector<Triangle> Triangles;
		int VertexStart;
		int VertexCount;
	};

	std::vector<Object> _objects;

	ID3D11Buffer* _texturedVertexBuffer;
	int _texturedVerticeCount;
	ID3D11Buffer* _transparentVertexBuffer;
	int _transparentVerticeCount;

	void RenderTextured();

	TLPoint GetPoint(uint8_t* p3d2, int offset, int index, int points, float tw, float th, int objectCount, int object, int subObject);
	TLPoint GetSpritePoint(uint8_t* p3d2, int offset, int index, int objectCount, int object, int subObject);

	struct Sprite
	{
		TLPoint P1;
		TLPoint P2;
		TLPoint P3;
		TLPoint P4;
	};

	struct SpriteInfo
	{
		int TextureIndex;
		Point P;
		float OX;
		float OY;
		float W;
		float H;
		float U1;
		float V1;
		float U2;
		float V2;
		int ObjectIndex;
		int SubObjectIndex;
		int SubObjectId;
	};

	struct TextureInfo
	{
		int VertexStart;
		int VerticeCount;
	};

	class CTextureGroup
	{
	public:
		CTextureGroup()
		{
			pTexture = NULL;
			Transparent = false;
			TransparentVertexStart = 0;
			TransparentVerticeCount = 0;
			Rotated = false;
			AnimatedTextureIndex = -1;
			SourcePointer = NULL;
			RealTexture = NULL;

			SpriteVertexStart = 0;
			SpriteVerticeCount = 0;
		}

		bool Rotated;

		CTexture* pTexture;
		std::vector<CTexture*> Textures;
		CTexture* RealTexture;
		int AnimatedTextureIndex;
		uint8_t* SourcePointer;

		bool Transparent;
		std::vector<Triangle> Triangles;
		std::vector<Triangle> TransparentTriangles;
		
		int TransparentVertexStart;
		int TransparentVerticeCount;

		int SpriteVertexStart;
		int SpriteVerticeCount;

		std::vector<SpriteInfo> SpriteInfos;
		std::vector<TextureInfo> TextureInfos;

		CPointList Points;

		void RemovePoints(int first, int count);
		void AddPoints(int first, int count);
	};

	std::vector<CTextureGroup*> _allTextures;

	struct Path
	{
		int Id;
		std::vector<DPoint> Points;
		bool enabled;
		bool allowLeave;
	};

	Path* _paths;
	int _pathCount;

	ID3D11Buffer* _spriteVertexBuffer;
	int _spriteVerticeCount;

	std::unordered_map<int, bool> opaqueTextures;
	std::unordered_map<int, bool> transparentTextures;
	std::unordered_map<int, bool> processedTextures;

	ModelObject** _ppObjects;
	int _objectCount;
	int _subObjectCount;

	bool Intersect(Box& boundingBox, Point& from, Point& direction);

	int HitObject;
	int HitSubObject;
	int ObjectIndex;

	VisibilityBufferType _visibilityBuffer;
	TranslationBufferType _translationBuffer;
	ObjectMap* _objectMap;
	int _objectMapCount;
	bool _visibilityChanged;
	bool _translationChanged;

	void ModifyLocationPoints(std::string file);
	void ModifyLocationPoints(int startix, int endix, float x, float y, float z);

	float4 GetTransparentColour(std::string file, int objectId, int subObjectId);

	int _locationAnimationCount;
	Animation Animations[MAX_ANIMATIONS];

	BinaryData GetLocationData(int index);
	std::list<CElevation*> Elevations;

	ObjectVisibilityMapping* _improvedObjectMap;
	void ChangeVisibility(int id, bool visible, bool setOnSubObjects, std::string header);

#ifdef DEBUG
	void RenderPoints();
	void RenderLines();
	void RenderPath();
public:
	static bool _renderTextured;
	static bool _renderPoints;
	static bool _renderLines;
	static bool _renderPaths;
	static bool _disableClipping;

	ID3D11Buffer* _pathVertexBuffer;
	ID3D11Buffer* _pathIndexBuffer;
	int _pathIndexCount;

	ID3D11Buffer* _indexBuffer;
	int _indexCount;
	int* _pIndexes;
	std::vector<int> _indexes;
#endif
};