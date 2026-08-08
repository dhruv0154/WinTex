#include "Location.h"
#include "SQZ.h"
#include "File.h"
#include "Configuration.h"
#include "DXScreen.h"
#include "Globals.h"
#include "DXText.h"
#include "GameBase.h"
#include "AnimationController.h"
#include "GameController.h"
#include "Utilities.h"
#include <cmath>
#include <tuple>
#include "LocationDataHeader.h"
#include <algorithm>
#include <string>
#include "IntersectionInfo.h"
#include <cstring>
#include <chrono>

bool CLocation::_loading = false;

#ifdef DEBUG
bool CLocation::_disableClipping = false;
bool CLocation::_renderTextured = true;
bool CLocation::_renderPoints = false;
bool CLocation::_renderLines = false;
bool CLocation::_renderPaths = false;
#endif

//#define CLOSE							
const double maxClipDistance = 2.0 / 3.0;
const double maxClipDistanceSquared = maxClipDistance * maxClipDistance;

#define OBJECT_FLAGS_HIDDEN				0x80000000
#define SUBOBJECT_FLAGS_TRANSPARENT		0x00000001
#define SUBOBJECT_FLAGS_TEXTURED		0x00000002
#define SUBOBJECT_FLAGS_SPRITE			0x00000004
#define SUBOBJECT_FLAGS_ALPHA			0x00000010
#define SUBOBJECT_FLAGS_BOTTOM			0x00000020
#define SUBOBJECT_FLAGS_TOP				0x00000080
#define SUBOBJECT_FLAGS_OBJECTID		0x00000200
#define SUBOBJECT_FLAGS_SINGLE_COLOUR	0x00000800
#define SUBOBJECT_FLAGS_HIDDEN			0x80000000

#define TEXTURE_FLAGS_LARGE				0x200

double DistanceSquared(DPoint p1, DPoint p2)
{
	double x = p1.X - p2.X;
	double z = p1.Z - p2.Z;
	return x * x + z * z;
}

DPoint Project(DPoint& p1, DPoint& p2, DPoint& p3)
{
	double px = p2.X - p1.X, pz = p2.Z - p1.Z, dAB = px * px + pz * pz;
	double u = ((p3.X - p1.X) * px + (p3.Z - p1.Z) * pz) / dAB;

	return DPoint{ p1.X + u * px , 0.0f, p1.Z + u * pz };
}

DPoint Normalize(DPoint v)
{
	double len = sqrt(v.X * v.X + v.Z * v.Z);
	v.X /= len;
	v.Z /= len;
	return v;
}

float CLocation::_x = 0.0f;
float CLocation::_y = 0.0f;
float CLocation::_z = 0.0f;
float CLocation::_angle1 = 0.0f;
float CLocation::_angle2 = 0.0f;
float CLocation::_y_player_adjustment = 0.0f;
float CLocation::_y_player_adjustment_min = 0.0f;
float CLocation::_y_player_adjustment_max = 0.0f;
float CLocation::_y_elevation = 0.0f;

CElevation* CLocation::_pCurrentElevation = NULL;

CLocation::CLocation()
{
	_loading = false;

	_pLocObjects = NULL;
	_pLocSubObjects = NULL;

	_currentLocationId = -1;

	_paths = NULL;
	_pathCount = 0;

	HitObject = -1;
	HitSubObject = -1;
	ObjectIndex = -1;

	_points = NULL;

	_texturedVertexBuffer = NULL;
	_texturedVerticeCount = 0;
	_visibilityChanged = false;

	_transparentVertexBuffer = NULL;
	_transparentVerticeCount = 0;
	_translationChanged = false;

	_x = 0.0f;
	_y = 0.0f;
	_z = 0.0f;
	_angle1 = 0.0f;
	_angle2 = 0.0f;
	_y_player_adjustment = 0.0f;
	_y_player_adjustment_min = 0.0f;
	_y_player_adjustment_max = 0.0f;
	_y_elevation = 0.0f;

	_pCurrentElevation = NULL;

	_spriteVertexBuffer = NULL;
	_spriteVerticeCount = 0;

	_ppObjects = NULL;
	_objectCount = 0;

	_objectMap = NULL;
	_objectMapCount = 0;

	memset(Animations, 0, MAX_ANIMATIONS * sizeof(Animation));

	_locationData = NULL;

#ifdef DEBUG
	_pathVertexBuffer = NULL;
	_pathIndexBuffer = NULL;
	_pathIndexCount = 0;
	_vertexBuffer = NULL;
	_indexBuffer = NULL;
	_verticeCount = 0;
	_indexCount = 0;
	_pIndexes = NULL;
#endif
}

CLocation::~CLocation()
{
	Clear();
}

bool CLocation::Load(int locationFileIndex)
{
	_loading = true;

	std::string file = CGameController::GetFileName(locationFileIndex);

	PointingChanged = true;

	// Load location entries
	CFile f;
	if (f.Open(file.c_str()))
	{
		int len = f.Size();
		_locationData = new uint8_t[len];
		if (_locationData != NULL)
		{
			f.Read(_locationData, len);
		}
		f.Close();
	}

	// Modify points
	ModifyLocationPoints(file);

	// Load path
	LoadPaths();

	// Load 3D data
	BinaryData bd3d2 = GetLocationData(4);
	uint8_t* p3d2 = bd3d2.Data;
	_verticeCount = GetInt(p3d2, 0, 4);
	_objectCount = GetInt(p3d2, 12, 4);

	// Quickly check which textures require transparency (flags & 0x10)
	for (int tc = 0; tc < _objectCount; tc++)
	{
		int objectOffset = GetInt(p3d2, 0x30 + tc * 4, 4) + 0x30;
		int subObjects = GetInt(p3d2, objectOffset + 12, 4);
		int nextSubOffset = objectOffset + 40;
		for (int ts = 0; ts < subObjects; ts++)
		{
			int thisSubOffset = nextSubOffset;
			nextSubOffset = GetInt(p3d2, nextSubOffset, 4) + 0x30;

			int points = GetInt(p3d2, thisSubOffset + 4, 4);
			int flags = GetInt(p3d2, thisSubOffset + 8, 4);
			int tex = GetInt(p3d2, thisSubOffset + 0x24, 4);

			if ((flags & SUBOBJECT_FLAGS_ALPHA) != 0 || points == 1)	// Flagged having alpha or is used as sprite
			{
				transparentTextures[tex] = true;
			}
			else
			{
				opaqueTextures[tex] = true;
			}
			processedTextures[tex] = true;
		}
	}

	LoadTextures();

	// Load vertices
	int moff = 0x30 + _objectCount * 4;
	_points = new Point[_verticeCount];
	for (int i = 0; i < _verticeCount; i++)
	{
		_points[i].x = ((float)GetInt(p3d2, moff, 4)) / 65536.0f;
		_points[i].y = ((float)GetInt(p3d2, moff + 4, 4)) / 65536.0f;
		_points[i].z = ((float)GetInt(p3d2, moff + 8, 4)) / 65536.0f;
		moff += 12;
	}

	// Prepare the visibility buffer
	for (int i = 0; i < sizeof(_visibilityBuffer.visibility) / sizeof(float4); i++)
	{
		_visibilityBuffer.visibility[i] = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// Prepare the translation buffer
	for (int i = 0; i < 256; i++)
	{
		_translationBuffer.translation[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	// Prepare the object mapping list
	_objectMapCount = static_cast<int>(_mapEntry->ObjectMap.size());
	_objectMap = new ObjectMap[_objectMapCount];
	std::vector<int>::iterator oit = _mapEntry->ObjectMap.begin();
	std::vector<int>::iterator oend = _mapEntry->ObjectMap.end();
	int ix = 0;
	while (oit != oend)
	{
		_objectMap[ix].id = *oit;
		_objectMap[ix].ObjectIndex = ((_objectMap[ix].id & 0x80000800) == 0) ? _objectMap[ix].id : -1;

		if (((*oit) & 0x88000800) == 0)
		{
			_objectMap[ix].VisibilityFloatPointers.push_back(&_visibilityBuffer.visibility[ix].x);
		}

		ix++;
		oit++;
	}

	// Improved object map to test proper visibilities
	_subObjectCount = GetInt(p3d2, 8, 4);
	_improvedObjectMap = new ObjectVisibilityMapping[_subObjectCount];

	// Extract 3D data
	int texturedTriangles = 0;
	int transparentTriangles = 0;
	int toalObjectCount = 0;

	int spriteCount = 0;

	Point lb1, lb2;
	lb1.x = 10000.0f;
	lb2.x = -lb1.x;
	lb1.y = lb1.x;
	lb2.y = -lb1.x;
	lb1.z = lb1.x;
	lb2.z = -lb1.x;

	int six = 0;

	_pLocObjects = new CLocationObject[_objectCount];
	_pLocSubObjects = new CLocationSubObject[GetInt(p3d2, 8, 4)];
	CLocationSubObject* pCurSubObjPtr = _pLocSubObjects;

	_ppObjects = new ModelObject * [_objectCount];
	for (int i = 0; i < _objectCount; i++)
	{
		_pLocObjects[i].pSubObjects = pCurSubObjPtr;

		int objectOffset = GetInt(p3d2, 0x30 + i * 4, 4) + 0x30;
		int type = (int)GetInt(p3d2, objectOffset, 4);

		_ppObjects[i] = new ModelObject();
		memset(_ppObjects[i], 0, sizeof(ModelObject));
		_ppObjects[i]->Index = i;

		float ominx = 10000.0f, omaxx = -ominx;
		float ominy = ominx, omaxy = -ominx;
		float ominz = ominx, omaxz = -ominx;

		if ((type & 0x80000000) != 0)
		{
			// Object visibility
			_visibilityBuffer.visibility[i].x = -1.0f;
		}

		int unkownOffset = GetInt(p3d2, objectOffset + 0x1c, 4);

		// Header is 10 longs
		// Pointer to next sub object is first long in object

		int subObjects = GetInt(p3d2, objectOffset + 12, 4);
		toalObjectCount += subObjects;
		int nextSubOffset = objectOffset + 40;

		_ppObjects[i]->SubObjectCount = subObjects;
		_ppObjects[i]->SubObjects = new ModelSubObject[subObjects];
		memset(_ppObjects[i]->SubObjects, 0, subObjects * sizeof(ModelSubObject));

		_pLocObjects[i].SubObjectCount = subObjects;

		for (int j = 0; j < subObjects; j++)
		{
			float sminx = 10000.0f, smaxx = -sminx;
			float sminy = sminx, smaxy = -sminx;
			float sminz = sminx, smaxz = -sminx;

			int thisSubOffset = nextSubOffset;
			nextSubOffset = GetInt(p3d2, nextSubOffset, 4) + 0x30;

			if (thisSubOffset != unkownOffset)
			{
				int points = GetInt(p3d2, thisSubOffset + 4, 4);
				int flags = GetInt(p3d2, thisSubOffset + 8, 4);
				int sid = GetInt(p3d2, thisSubOffset + 0xc, 4);
				int tex = GetInt(p3d2, thisSubOffset + 0x24, 4);
				if ((flags & SUBOBJECT_FLAGS_TEXTURED) == 0) tex = -1;

				_improvedObjectMap[six].ObjectIndex = i;
				_improvedObjectMap[six].SubObjectIndex = six;
				_improvedObjectMap[six].SubObjectId = sid;

				pCurSubObjPtr[j].Id = sid;
				pCurSubObjPtr[j].TextureIndex = tex;
				pCurSubObjPtr[j].VertexIndex = -1;
				pCurSubObjPtr[j].VertexCount = 0;

				if ((sid & 0x88000000) != 0)
				{
					int rid = (sid >> 16) & 0xffff;
					for (int z = 0; z < _objectMapCount; z++)
					{
						if (_objectMap[z].id == sid || ((rid & 0x800) != 0 && _objectMap[z].id == rid))
						{
							_objectMap[z].SubObjectIndices.push_back(six);
							_objectMap[z].VisibilityFloatPointers.push_back(&_visibilityBuffer.visibility[six].y);
						}
					}
				}

				if ((flags & SUBOBJECT_FLAGS_HIDDEN) != 0)
				{
					// Sub-object visibility
					_visibilityBuffer.visibility[six].y = -1.0f;
				}

				// Sub-object transparency indicator
				_visibilityBuffer.visibility[six].z = (flags & SUBOBJECT_FLAGS_ALPHA) ? 1.0f : 0.0f;

				_ppObjects[i]->SubObjects[j].ModelIndex = i;
				_ppObjects[i]->SubObjects[j].SubObjectIndex = six;
				_ppObjects[i]->SubObjects[j].Flags = flags;
				_ppObjects[i]->SubObjects[j].Texture = tex;
				_ppObjects[i]->SubObjects[j].Active = ((flags & SUBOBJECT_FLAGS_HIDDEN) == 0);
				_ppObjects[i]->SubObjects[j].ID = sid;
				_ppObjects[i]->SubObjects[j].PointCount = points;
				_ppObjects[i]->SubObjects[j].Points = new TLPoint[points];
				_ppObjects[i]->SubObjects[j].BoundingBox.X1 = 10000.0f;
				_ppObjects[i]->SubObjects[j].BoundingBox.Y1 = 10000.0f;
				_ppObjects[i]->SubObjects[j].BoundingBox.Z1 = 10000.0f;
				_ppObjects[i]->SubObjects[j].BoundingBox.X2 = -_ppObjects[i]->SubObjects[j].BoundingBox.X1;
				_ppObjects[i]->SubObjects[j].BoundingBox.Y2 = -_ppObjects[i]->SubObjects[j].BoundingBox.Y1;
				_ppObjects[i]->SubObjects[j].BoundingBox.Z2 = -_ppObjects[i]->SubObjects[j].BoundingBox.Z1;
				_ppObjects[i]->SubObjects[j].Triangles = (points > 2) ? new Triangle[points - 2] : NULL;

				if (points > 0)
				{
					CTextureGroup* stex = (tex >= 0) ? _allTextures.at(tex) : NULL;

					if (points == 1)
					{
						int subSprites = GetInt(p3d2, thisSubOffset + 0x38, 4);
						int spoio = thisSubOffset + 0x44 + subSprites * 32;
						int pointIndex = _objectCount + (GetInt(p3d2, thisSubOffset + 0x44 + subSprites * 32, 4) >> 4);
						float cx = _points[pointIndex].x;
						float cy = _points[pointIndex].y;
						float cz = _points[pointIndex].z;

						if (subSprites > 0)
						{
							float sw = (stex != NULL) ? stex->pTexture->Width() : 1.0f;
							float sh = (stex != NULL) ? stex->pTexture->Height() : 1.0f;
							float ssw = ((float)GetInt(p3d2, thisSubOffset + 0x30, 4)) / 65536.0f;
							float ssh = ((float)GetInt(p3d2, thisSubOffset + 0x34, 4)) / 65536.0f;

							float sww = ((float)GetInt(p3d2, thisSubOffset + 0x28, 4)) / 65536.0f;
							float swh = ((float)GetInt(p3d2, thisSubOffset + 0x2c, 4)) / 65536.0f;

							for (int ss = 0; ss < subSprites; ss++)
							{
								float subSpriteOffsetX = ((float)GetInt(p3d2, thisSubOffset + 0x44 + ss * 32, 4)) / 65536.0f;
								float subSpriteOffsetY = ((float)GetInt(p3d2, thisSubOffset + 0x48 + ss * 32, 4)) / 65536.0f;

								float subSpriteTexX1 = ((float)GetInt(p3d2, thisSubOffset + 0x54 + ss * 32, 4)) / 65536.0f;
								float subSpriteTexY1 = ((float)GetInt(p3d2, thisSubOffset + 0x58 + ss * 32, 4)) / 65536.0f;
								float subSpriteTexX2 = 1.0f + (((float)GetInt(p3d2, thisSubOffset + 0x5c + ss * 32, 4)) / 65536.0f);
								float subSpriteTexY2 = 1.0f + (((float)GetInt(p3d2, thisSubOffset + 0x60 + ss * 32, 4)) / 65536.0f);

								float subSpriteWidth = subSpriteTexX2 - subSpriteTexX1;
								float subSpriteHeight = subSpriteTexY2 - subSpriteTexY1;

								SpriteInfo si;
								si.TextureIndex = tex;
								si.P.x = cx;
								si.P.y = cy;
								si.P.z = cz;
								si.OX = (sww * subSpriteOffsetX) / ssw;
								si.OY = (swh * subSpriteOffsetY) / ssh;
								si.W = (sww * subSpriteWidth) / ssw;
								si.H = (swh * subSpriteHeight) / ssh;
								si.U1 = subSpriteTexX1 / sw;
								si.V1 = subSpriteTexY1 / sh;
								si.U2 = subSpriteTexX2 / sw;
								si.V2 = subSpriteTexY2 / sh;
								si.ObjectIndex = i;
								si.SubObjectIndex = six;
								si.SubObjectId = sid;

								if (stex != NULL) stex->SpriteInfos.push_back(si);

								spriteCount++;

								if ((si.P.x - si.W / 2) < sminx) sminx = si.P.x - si.W / 2;
								if ((si.P.x + si.W / 2) > smaxx) smaxx = si.P.x + si.W / 2;
								if ((si.P.y - si.H) < sminy) sminy = si.P.y - si.H;
								if (si.P.y > smaxy) smaxy = si.P.y;
								if ((si.P.z - si.W / 2) < sminz) sminz = si.P.z - si.W / 2;
								if ((si.P.z + si.W / 2) > smaxz) smaxz = si.P.z + si.W / 2;
							}
						}
						else
						{
							sminx = smaxx = cx;
							sminy = smaxy = cy;
							sminz = smaxz = cz;
						}
					}
					else if (points >= 3)
					{
						Object obj;
						obj.TextureIndex = tex;

						CTexture* pt = (stex != NULL) ? stex->pTexture : NULL;
						float tw = (pt != NULL) ? pt->Width() : 0.0f;
						float th = (pt != NULL) ? pt->Height() : 0.0f;

						std::vector<TLPoint> vpoints;
						for (int p = 0; p < points; p++)
						{
							TLPoint px = GetPoint(p3d2, thisSubOffset + 0x28, p, points, tw, th, _objectCount, i, six);
							px.SubObjectId = j;

#ifdef DEBUG
							int pix = GetInt(p3d2, thisSubOffset + 0x28 + p * 4, 4) >> 4;
							_indexes.push_back(pix);
#endif

							if ((flags & SUBOBJECT_FLAGS_SINGLE_COLOUR) != 0)
							{
								px.U = 0.0f;
								px.V = 0.0f;
							}

							_ppObjects[i]->SubObjects[j].Points[p] = px;

							vpoints.push_back(px);

							if (px.Point->x < sminx) sminx = px.Point->x;
							if (px.Point->x > smaxx) smaxx = px.Point->x;
							if (px.Point->y < sminy) sminy = px.Point->y;
							if (px.Point->y > smaxy) smaxy = px.Point->y;
							if (px.Point->z < sminz) sminz = px.Point->z;
							if (px.Point->z > smaxz) smaxz = px.Point->z;
						}

#ifdef DEBUG
						int pix = GetInt(p3d2, thisSubOffset + 0x28, 4) >> 4;
						_indexes.push_back(pix);
						_indexes.push_back(-1);	
#endif

						int startp = 0;
						int tri = points - 2;
						int pointsLeft = points;
						int trix = 0;
						while (pointsLeft > 2)
						{
							int prev1 = startp - 1;
							if (prev1 < 0) prev1 += pointsLeft;
							int prev2 = startp - 2;
							if (prev2 < 0) prev2 += pointsLeft;

							TLPoint p0 = vpoints.at(startp);
							TLPoint p1 = vpoints.at(prev1);
							TLPoint p2 = vpoints.at(prev2);

							Point v1;
							v1.x = p0.Point->x - p1.Point->x;
							v1.y = p0.Point->y - p1.Point->y;
							v1.z = p0.Point->z - p1.Point->z;

							float len = sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
							v1.x /= len;
							v1.y /= len;
							v1.z /= len;

							Point v2;
							v2.x = p1.Point->x - p2.Point->x;
							v2.y = p1.Point->y - p2.Point->y;
							v2.z = p1.Point->z - p2.Point->z;

							len = sqrt(v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);
							v2.x /= len;
							v2.y /= len;
							v2.z /= len;

							if (v1.x != v2.x || v1.y != v2.y || v1.z != v2.z)
							{
								Triangle t;
								t.ObjectId = i;
								t.SubObjectId = sid & 0xffff;
								t.Flags = _ppObjects[i]->SubObjects[j].Flags;
								t.P1 = p0;
								t.P2 = p2;
								t.P3 = p1;

								obj.Triangles.push_back(t);
								if (stex != NULL)
								{
									if ((flags & SUBOBJECT_FLAGS_TRANSPARENT) == 0)
									{
										stex->Triangles.push_back(t);
										texturedTriangles++;
										obj.VertexCount += 3;
									}
									else
									{
										stex->TransparentTriangles.push_back(t);
										transparentTriangles++;
									}
								}

								_ppObjects[i]->SubObjects[j].Triangles[trix++] = t;
								vpoints.erase(vpoints.begin() + prev1);
								pointsLeft--;
							}

							startp++;
							if (startp >= pointsLeft) startp -= pointsLeft;
						}

						_objects.push_back(obj);
					}
				}
			}

			if (sminx < ominx) ominx = sminx;
			if (sminy < ominy) ominy = sminy;
			if (sminz < ominz) ominz = sminz;
			if (smaxx > omaxx) omaxx = smaxx;
			if (smaxy > omaxy) omaxy = smaxy;
			if (smaxz > omaxz) omaxz = smaxz;

			_ppObjects[i]->SubObjects[j].BoundingBox.X1 = sminx;
			_ppObjects[i]->SubObjects[j].BoundingBox.X2 = smaxx;
			_ppObjects[i]->SubObjects[j].BoundingBox.Y1 = sminy;
			_ppObjects[i]->SubObjects[j].BoundingBox.Y2 = smaxy;
			_ppObjects[i]->SubObjects[j].BoundingBox.Z1 = sminz;
			_ppObjects[i]->SubObjects[j].BoundingBox.Z2 = smaxz;

			six++;

			if (nextSubOffset < thisSubOffset) break;
		}

		pCurSubObjPtr += subObjects;

		_ppObjects[i]->BoundingBox.X1 = ominx;
		_ppObjects[i]->BoundingBox.X2 = omaxx;
		_ppObjects[i]->BoundingBox.Y1 = ominy;
		_ppObjects[i]->BoundingBox.Y2 = omaxy;
		_ppObjects[i]->BoundingBox.Z1 = ominz;
		_ppObjects[i]->BoundingBox.Z2 = omaxz;

		if (ominx < lb1.x) lb1.x = ominx;
		if (omaxx > lb2.x) lb2.x = omaxx;
		if (ominy < lb1.y) lb1.y = ominy;
		if (omaxy > lb2.y) lb2.y = omaxy;
		if (ominz < lb1.z) lb1.z = ominz;
		if (omaxz > lb2.z) lb2.z = omaxz;
	}

#ifdef DEBUG
	_indexCount = _indexes.size();
	if (_indexCount > 0)
	{
		_pIndexes = new int[_indexCount];

		for (int i = 0; i < _indexCount; i++)
		{
			int cix = _indexes.at(i);
			if (cix >= 0) cix += _objectCount;
			_pIndexes[i] = cix;
		}

		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		indexBufferDesc.ByteWidth = sizeof(int) * _indexCount;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexData;
		indexData.pSysMem = _pIndexes;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		dx.CreateBuffer(&indexBufferDesc, &indexData, &_indexBuffer, "Location Lines");
	}
#endif

	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.ByteWidth = sizeof(Point) * _verticeCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem = _points;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	dx.CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer, "Location Vertices");

	_texturedVerticeCount = texturedTriangles * 3;
	TEXTURED_VERTEX* pTV = new TEXTURED_VERTEX[_texturedVerticeCount];
	_transparentVerticeCount = transparentTriangles * 3;
	COLOURED_VERTEX* pTTV = new COLOURED_VERTEX[_transparentVerticeCount];
	std::vector<CTextureGroup*>::iterator tit = _allTextures.begin();
	std::vector<CTextureGroup*>::iterator tend = _allTextures.end();
	int tix = 0;
	int ttix = 0;

	while (tit != tend)
	{
		CTextureGroup* pTex = *tit;

		int vertexStart = tix;

		pTex->TransparentVertexStart = ttix;

		std::vector<Triangle>::iterator trit = pTex->Triangles.begin();
		std::vector<Triangle>::iterator trend = pTex->Triangles.end();
		int curobid = -1;
		int cursubobid = -1;
		CLocationSubObject* pSub = NULL;
		while (trit != trend)
		{
			Triangle* pTri = &(*trit);
			if (pTri->ObjectId != curobid || pTri->SubObjectId != cursubobid)
			{
				pSub = NULL;

				CLocationObject* pOb = &_pLocObjects[pTri->ObjectId];
				for (int sx = 0; sx < pOb->SubObjectCount; sx++)
				{
					if ((pOb->pSubObjects[sx].Id & 0xffff) == pTri->SubObjectId)
					{
						pSub = &pOb->pSubObjects[sx];
						if (pSub->VertexIndex < 0)
						{
							pSub->VertexIndex = tix;
						}
						break;
					}
				}
			}

			float shaderParameter = (pTri->Flags & SUBOBJECT_FLAGS_ALPHA) != 0 ? 1.0f : 0.0f;

			if (pSub != NULL)
			{
				pSub->VertexCount += 3;
			}

			pTV[tix].position.x = pTri->P1.Point->x;
			pTV[tix].position.y = pTri->P1.Point->y;
			pTV[tix].position.z = pTri->P1.Point->z;
			pTV[tix].texture.x = pTri->P1.U;
			pTV[tix].texture.y = pTri->P1.V;
			pTV[tix].object.x = (float)pTri->P1.ObjectIndex;
			pTV[tix].object.y = (float)pTri->P1.SubObjectIndex;
			pTV[tix].objectParameters.x = shaderParameter;
			tix++;
			pTV[tix].position.x = pTri->P2.Point->x;
			pTV[tix].position.y = pTri->P2.Point->y;
			pTV[tix].position.z = pTri->P2.Point->z;
			pTV[tix].texture.x = pTri->P2.U;
			pTV[tix].texture.y = pTri->P2.V;
			pTV[tix].object.x = (float)pTri->P2.ObjectIndex;
			pTV[tix].object.y = (float)pTri->P2.SubObjectIndex;
			pTV[tix].objectParameters.x = shaderParameter;
			tix++;
			pTV[tix].position.x = pTri->P3.Point->x;
			pTV[tix].position.y = pTri->P3.Point->y;
			pTV[tix].position.z = pTri->P3.Point->z;
			pTV[tix].texture.x = pTri->P3.U;
			pTV[tix].texture.y = pTri->P3.V;
			pTV[tix].object.x = (float)pTri->P3.ObjectIndex;
			pTV[tix].object.y = (float)pTri->P3.SubObjectIndex;
			pTV[tix].objectParameters.x = shaderParameter;
			tix++;

			trit++;
		}

		int vertexCount = tix - vertexStart;
		if (vertexCount > 0)
		{
			pTex->Points.Add(vertexStart, vertexCount);
		}

		trit = pTex->TransparentTriangles.begin();
		trend = pTex->TransparentTriangles.end();
		while (trit != trend)
		{
			Triangle* pTri = &(*trit);

			float4 transparentColour = GetTransparentColour(file, pTri->P1.ObjectIndex, pTri->P1.SubObjectId);

			pTTV[ttix].position.x = pTri->P1.Point->x;
			pTTV[ttix].position.y = pTri->P1.Point->y;
			pTTV[ttix].position.z = pTri->P1.Point->z;
			pTTV[ttix].position.w = 1.0f;
			pTTV[ttix].colour = transparentColour;
			pTTV[ttix].object.x = (float)pTri->P1.ObjectIndex;
			pTTV[ttix].object.y = (float)pTri->P1.SubObjectIndex;
			pTTV[ttix].object.z = 0.0f;
			pTTV[ttix].object.w = 0.0f;
			ttix++;
			
			pTTV[ttix].position.x = pTri->P2.Point->x;
			pTTV[ttix].position.y = pTri->P2.Point->y;
			pTTV[ttix].position.z = pTri->P2.Point->z;
			pTTV[ttix].position.w = 1.0f;
			pTTV[ttix].colour = transparentColour;
			pTTV[ttix].object.x = (float)pTri->P2.ObjectIndex;
			pTTV[ttix].object.y = (float)pTri->P2.SubObjectIndex;
			pTTV[ttix].object.z = 0.0f;
			pTTV[ttix].object.w = 0.0f;
			ttix++;
			
			pTTV[ttix].position.x = pTri->P3.Point->x;
			pTTV[ttix].position.y = pTri->P3.Point->y;
			pTTV[ttix].position.z = pTri->P3.Point->z;
			pTTV[ttix].position.w = 1.0f;
			pTTV[ttix].colour = transparentColour;
			pTTV[ttix].object.x = (float)pTri->P3.ObjectIndex;
			pTTV[ttix].object.y = (float)pTri->P3.SubObjectIndex;
			pTTV[ttix].object.z = 0.0f;
			pTTV[ttix].object.w = 0.0f;
			ttix++;

			trit++;
		}

		pTex->TransparentVerticeCount = ttix - pTex->TransparentVertexStart;

		tit++;
	}

	D3D11_BUFFER_DESC triBufferDesc;
	triBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	triBufferDesc.ByteWidth = sizeof(TEXTURED_VERTEX) * _texturedVerticeCount;
	triBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	triBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	triBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA triData;
	triData.pSysMem = pTV;
	triData.SysMemPitch = 0;
	triData.SysMemSlicePitch = 0;

	dx.CreateBuffer(&triBufferDesc, &triData, &_texturedVertexBuffer, "Location Textured Triangles");

	delete[] pTV;

	if (_transparentVerticeCount > 0)
	{
		triBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		triBufferDesc.ByteWidth = sizeof(COLOURED_VERTEX) * _transparentVerticeCount;
		triBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		triBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		triBufferDesc.StructureByteStride = 0;

		triData.pSysMem = pTTV;
		triData.SysMemPitch = 0;
		triData.SysMemSlicePitch = 0;

		dx.CreateBuffer(&triBufferDesc, &triData, &_transparentVertexBuffer, "Location Transparent Triangles");
	}
	delete[] pTTV;

	if (spriteCount > 0)
	{
		_spriteVerticeCount = spriteCount * 3 * 2;
		pTV = new TEXTURED_VERTEX[_spriteVerticeCount];
		tit = _allTextures.begin();
		tend = _allTextures.end();
		tix = 0;
		while (tit != tend)
		{
			CTextureGroup* pTex = *tit;

			pTex->SpriteVertexStart = tix;

			std::vector<SpriteInfo>::iterator sit = pTex->SpriteInfos.begin();
			std::vector<SpriteInfo>::iterator send = pTex->SpriteInfos.end();
			while (sit != send)
			{
				SpriteInfo* spr = &(*sit);

				pTV[tix].position.x = spr->P.x + spr->OX - spr->W / 2;
				pTV[tix].position.y = spr->P.y + spr->OY;
				pTV[tix].position.z = spr->P.z;
				pTV[tix].texture.x = spr->U1;
				pTV[tix].texture.y = spr->V1;
				tix++;
				pTV[tix].position.x = spr->P.x + spr->OX + spr->W / 2;
				pTV[tix].position.y = spr->P.y + spr->OY + spr->H;
				pTV[tix].position.z = spr->P.z;
				pTV[tix].texture.x = spr->U2;
				pTV[tix].texture.y = spr->V2;
				tix++;
				pTV[tix].position.x = spr->P.x + spr->OX - spr->W / 2;
				pTV[tix].position.y = spr->P.y + spr->OY;
				pTV[tix].position.z = spr->P.z;
				pTV[tix].texture.x = spr->U2;
				pTV[tix].texture.y = spr->V1;
				tix++;

				pTV[tix].position.x = spr->P.x + spr->OX - spr->W / 2;
				pTV[tix].position.y = spr->P.y + spr->OY;
				pTV[tix].position.z = spr->P.z;
				pTV[tix].texture.x = spr->U1;
				pTV[tix].texture.y = spr->V1;
				tix++;
				pTV[tix].position.x = spr->P.x + spr->OX - spr->W / 2;
				pTV[tix].position.y = spr->P.y + spr->OY + spr->H;
				pTV[tix].position.z = spr->P.z;
				pTV[tix].texture.x = spr->U1;
				pTV[tix].texture.y = spr->V2;
				tix++;
				pTV[tix].position.x = spr->P.x + spr->OX - spr->W / 2;
				pTV[tix].position.y = spr->P.y + spr->OY + spr->H;
				pTV[tix].position.z = spr->P.z;
				pTV[tix].texture.x = spr->U2;
				pTV[tix].texture.y = spr->V2;
				tix++;

				sit++;
			}

			pTex->SpriteVerticeCount = tix - pTex->SpriteVertexStart;

			tit++;
		}

		triBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		triBufferDesc.ByteWidth = sizeof(TEXTURED_VERTEX) * _spriteVerticeCount;
		triBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		triBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		triBufferDesc.StructureByteStride = 0;

		triData.pSysMem = pTV;
		triData.SysMemPitch = 0;
		triData.SysMemSlicePitch = 0;

		dx.CreateBuffer(&triBufferDesc, &triData, &_spriteVertexBuffer, "Location Sprites");

		delete[] pTV;
	}

	if (pConfig->AlternativeMedia) {
		auto extensionlessName = std::string(file.begin(), file.end() - 3) + '\\';
		int textureIndex{ 0 };
		for (auto&& texture : _allTextures) {
			auto alternateName = extensionlessName + std::to_string(textureIndex) + ".png";
			if (CFile::Exists(alternateName.c_str())) {
				texture->pTexture->Init(alternateName.c_str());
			}
			++textureIndex;
		}
	}

	_visibilityChanged = true;
	_translationChanged = true;

	BinaryData animbd = GetLocationData(0);
	_locationAnimationCount = GetInt(animbd.Data, 0, 4);
	for (int i = 0; i < _locationAnimationCount && i < MAX_ANIMATIONS; i++)
	{
		int offset = GetInt(animbd.Data, 8 + i * 8, 4);
		int trigger = GetInt(animbd.Data, offset + 8, 4);
		if (trigger == 1)
		{
			StartIndexedAnimation(i);
		}
		else
		{
			int type = GetInt(animbd.Data, offset, 4);
			if (type == 16)
			{
				Elevations.push_back(new CElevation((Elevation*)(animbd.Data + offset)));
			}
		}
	}

	_loading = false;

	Animate();

	return true;
}

void CLocation::Clear()
{
	if (_paths != NULL)
	{
		delete[] _paths;
		_paths = NULL;
	}

	_pathCount = 0;

	if (_ppObjects != NULL)
	{
		for (int i = 0; i < _objectCount; i++)
		{
			if (_ppObjects[i]->SubObjects != NULL)
			{
				for (int j = 0; j < _ppObjects[i]->SubObjectCount; j++)
				{
					if (_ppObjects[i]->SubObjects[j].Points != NULL)
					{
						delete[] _ppObjects[i]->SubObjects[j].Points;
						_ppObjects[i]->SubObjects[j].Points = NULL;
					}

					if (_ppObjects[i]->SubObjects[j].Triangles != NULL)
					{
						delete[] _ppObjects[i]->SubObjects[j].Triangles;
						_ppObjects[i]->SubObjects[j].Triangles = NULL;
					}
				}

				delete[] _ppObjects[i]->SubObjects;
				_ppObjects[i]->SubObjects = NULL;
			}

			delete[] _ppObjects[i];
			_ppObjects[i] = NULL;
		}

		delete[] _ppObjects;
		_ppObjects = NULL;
	}

	if (_pLocObjects != NULL)
	{
		delete[] _pLocObjects;
		_pLocObjects = NULL;
	}

	if (_pLocSubObjects != NULL)
	{
		delete[] _pLocSubObjects;
		_pLocSubObjects = NULL;
	}

	if (_locationData != NULL)
	{
		delete[] _locationData;
		_locationData = NULL;
	}

#ifdef DEBUG
	if (_indexBuffer != NULL)
	{
		_indexBuffer->Release();
		_indexBuffer = NULL;
	}

	if (_pIndexes != NULL)
	{
		delete[] _pIndexes;
		_pIndexes = NULL;
	}

	_indexes.clear();
#endif

	if (_vertexBuffer != NULL)
	{
		_vertexBuffer->Release();
		_vertexBuffer = NULL;
	}

	if (_points != NULL)
	{
		delete[] _points;
		_points = NULL;
	}

	_verticeCount = 0;

	std::vector<CTextureGroup*>::iterator texit = _allTextures.begin();
	std::vector<CTextureGroup*>::iterator texend = _allTextures.end();
	while (texit != texend)
	{
		CTextureGroup* pTG = *texit;
		CTexture* pTex = pTG->pTexture;

		pTG->SpriteInfos.clear();
		pTG->TextureInfos.clear();
		for (auto it : pTG->Textures)
		{
			delete it;
		}

		delete pTex;
		delete pTG;
		texit++;
	}

	if (_texturedVertexBuffer != NULL)
	{
		_texturedVertexBuffer->Release();
		_texturedVertexBuffer = NULL;
	}

	if (_transparentVertexBuffer != NULL)
	{
		_transparentVertexBuffer->Release();
		_transparentVertexBuffer = NULL;
	}

	_allTextures.clear();
	_objects.clear();

	if (_spriteVertexBuffer != NULL)
	{
		_spriteVertexBuffer->Release();
		_spriteVertexBuffer = NULL;
	}
	_spriteVerticeCount = 0;

	if (_objectMap != NULL)
	{
		delete[] _objectMap;
		_objectMap = NULL;
	}
	_objectMapCount = 0;

	memset(Animations, 0, MAX_ANIMATIONS * sizeof(Animation));

	Elevations.clear();
}

void CLocation::LoadTextures()
{
	BinaryData bdPal = GetLocationData(2);
	BinaryData bdsqz = GetLocationData(5);
	BinaryData bdtex = CSQZ::Decompress(bdsqz.Data, bdsqz.Length);

	int poff = (bdPal.Length <= 0x300) ? 0 : bdPal.Length - 0x300;
	int palette[256];
	uint8_t* pPal = (uint8_t*)palette;
	for (int i = 0; i < 256; i++)
	{
		uint8_t r = bdPal.Data[i * 3 + 0 + poff];
		uint8_t g = bdPal.Data[i * 3 + 1 + poff];
		uint8_t b = bdPal.Data[i * 3 + 2 + poff];
		uint8_t a = 255;

		pPal[i * 4 + 0] = b;
		pPal[i * 4 + 1] = g;
		pPal[i * 4 + 2] = r;
		pPal[i * 4 + 3] = a;
	}

	uint8_t* tex = bdtex.Data;
	uint8_t* texend = tex + bdtex.Length;

	pPal[0] = 0;
	pPal[1] = 0;
	pPal[2] = 0;

	int hqSize = GetInt(tex, 0, 4);
	int lqSize = GetInt(tex, 4, 4);
	int texCount = GetInt(tex, 8, 4);
	int texPtr = 12 + texCount * 12;
	std::unordered_map<int, bool> animatedTextures;
	for (int t = 0; t < texCount; t++)
	{
		pPal[3] = (transparentTextures[t] || processedTextures.find(t) == processedTextures.end()) ? 0 : 255;

		int w = GetInt(tex, texPtr, 4);
		int h = GetInt(tex, texPtr + 4, 4);
		int subType = GetInt(tex, texPtr + 8, 4);

		CTextureGroup* stex = new CTextureGroup();
		stex->SourcePointer = tex + texPtr;

		texPtr += 12 + 4 * h;
		uint8_t* scan = tex + texPtr;

		if ((subType & 2) == 2)
		{
			animatedTextures[t] = true;
		}
		else
		{
			CTexture* pTex = new CTexture(&dx, w, h, scan, palette, (subType & SUBOBJECT_FLAGS_TRANSPARENT) == 1);

			bool transparent = transparentTextures[t];
			bool opaque = opaqueTextures[t];

			stex->pTexture = pTex;
			stex->Transparent = transparent;
			stex->Rotated = (subType == 1);
		}

		_allTextures.push_back(stex);

		texPtr += w * h;
	}

	BinaryData bdanim = GetLocationData(0);
	uint8_t* pAnim = bdanim.Data;
	int animcount = GetInt(pAnim, 0, 4);
	for (int i = 0; i < animcount; i++)
	{
		int offset = GetInt(pAnim, 8 + i * 8, 4);
		int type = GetInt(pAnim, offset, 4);
		if (type == 14)
		{
			int animated = GetInt(pAnim, offset + 12, 4);
			int baseTexture = GetInt(pAnim, offset + 16, 4);
			if (animatedTextures[animated] == true)
			{
				CTextureGroup* pBase = _allTextures.at(baseTexture);
				CTextureGroup* pATex = _allTextures.at(animated);
				uint8_t* pBaseImage = pBase->SourcePointer;
				uint8_t* pMod = pATex->SourcePointer;

				int w = GetInt(pBaseImage, 0, 4);
				int h = GetInt(pBaseImage, 4, 4);

				int mw = GetInt(pMod, 0, 4);
				int mh = GetInt(pMod, 4, 4);

				pMod += 12 + 4 * mh;
				pBaseImage += 12 + 4 * h;

				int v1 = GetInt(pMod, 0, 2);
				int width = GetInt(pMod, 2, 2);
				int height = GetInt(pMod, 4, 2);
				int flags = GetInt(pMod, 6, 2);
				int fbc = (flags & TEXTURE_FLAGS_LARGE) ? 4 : 2;

				uint8_t* pEnd = pMod + mw * mh;
				pMod += 8;
				while (pMod < pEnd)
				{
					int destination = 0;
					int bytesInFrame = GetInt(pMod, 0, fbc);
					pMod += fbc;
					while (bytesInFrame > 0)
					{
						int b = *(pMod++);
						bytesInFrame--;
						if ((b & 0x80) != 0)
						{
							destination += (b & 0x7f);
						}
						else
						{
							memcpy(pBaseImage + destination, pMod, b);
							destination += b;
							pMod += b;
							bytesInFrame -= b;
						}
					}

					pPal[3] = pBase->Transparent ? 0 : 255;

					CTexture* pTex = new CTexture(&dx, w, h, pBaseImage, palette, pBase->Rotated);
					pATex->Textures.push_back(pTex);
					pATex->Transparent = pBase->Transparent;
				}

				pATex->AnimatedTextureIndex = 0;
			}
		}
	}

	delete[] tex;
}

void CLocation::Render()
{
	if (_visibilityChanged)
	{
		CConstantBuffers::SetVisibility(dx, _visibilityBuffer);
		_visibilityChanged = false;
	}

	if (_translationChanged)
	{
		CConstantBuffers::SetTranslation(dx, _translationBuffer);
		_translationChanged = false;
	}

	CConstantBuffers::Setup3D(dx);
	dx.EnableZBuffer();

	UpdateY();

	float16 rm1 = Math::RotationX(_angle1);
	float16 rm2 = Math::RotationY(_angle2);
	float16 tm = Math::Translation(_x, _y, _z);

	float16 wm;
	wm = tm * rm2 * rm1;
	CConstantBuffers::SetWorld(dx, &wm);

#ifdef DEBUG
	if (_renderTextured)
	{
#endif
		RenderTextured();
#ifdef DEBUG
	}

	if (_renderLines) RenderLines();
	if (_renderPoints) RenderPoints();
	if (_renderPaths) RenderPath();
#endif

	CShaders::SelectOrthoShader();
	dx.DisableZBuffer();
	CConstantBuffers::Setup2D(dx);

	CDXText caption;

#ifdef DEBUG
	char xbuffer[40];
	snprintf(xbuffer, sizeof(xbuffer), "%.3g", -_x);
	char ybuffer[40];
	snprintf(ybuffer, sizeof(ybuffer), "%.3g", -_y);
	char zbuffer[40];
	snprintf(zbuffer, sizeof(zbuffer), "%.3g", -_z);
	char a1buffer[40];
	snprintf(a1buffer, sizeof(a1buffer), "%.3g", _angle1);
	char a2buffer[40];
	snprintf(a2buffer, sizeof(a2buffer), "%.3g", _angle2);

	char obuffer[40];
	snprintf(obuffer, sizeof(obuffer), "%d", HitObject);
	char sbuffer[40];
	snprintf(sbuffer, sizeof(sbuffer), "%x", HitSubObject);

	char oixbuffer[40];
	snprintf(oixbuffer, sizeof(oixbuffer), "%d", ObjectIndex);

	float dy = 40.0f;
	caption.SetText("X");
	caption.Render(0.0f, dy);
	caption.SetText("Y");
	caption.Render(0.0f, dy + 20.0f * pConfig->FontScale);
	caption.SetText("Z");
	caption.Render(0.0f, dy + 40.0f * pConfig->FontScale);
	caption.SetText("Pitch");
	caption.Render(0.0f, dy + 60.0f * pConfig->FontScale);
	caption.SetText("Yaw");
	caption.Render(0.0f, dy + 80.0f * pConfig->FontScale);
	caption.SetText("Object index");
	caption.Render(0.0f, dy + 120.0f * pConfig->FontScale);
	caption.SetText("Sub-object id");
	caption.Render(0.0f, dy + 140.0f * pConfig->FontScale);
	caption.SetText("Mapped object");
	caption.Render(0.0f, dy + 160.0f * pConfig->FontScale);

	caption.SetText(xbuffer);
	caption.Render(100.0f * pConfig->FontScale, dy);
	caption.SetText(ybuffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 20.0f * pConfig->FontScale);
	caption.SetText(zbuffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 40.0f * pConfig->FontScale);
	caption.SetText(a1buffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 60.0f * pConfig->FontScale);
	caption.SetText(a2buffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 80.0f * pConfig->FontScale);
	caption.SetText(obuffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 120.0f * pConfig->FontScale);
	caption.SetText(sbuffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 140.0f * pConfig->FontScale);
	caption.SetText(oixbuffer);
	caption.Render(100.0f * pConfig->FontScale, dy + 160.0f * pConfig->FontScale);
#endif

	if (!CAnimationController::HasAnim())
	{
		caption.SetText("Score:");
		caption.Render(0.0f, dx.GetHeight() - caption.Height() - 10.0f);
		char scorebuffer[40];
		snprintf(scorebuffer, sizeof(scorebuffer), "%d", CGameController::GetScore());
		caption.SetText(scorebuffer);
		caption.Render(60.0f * pConfig->FontScale, dx.GetHeight() - caption.Height() - 10.0f);
	}
}

void CLocation::RenderTextured()
{
	unsigned int stride = sizeof(TEXTURED_VERTEX);
	unsigned int offset = 0;
	dx.SetVertexBuffers(0, 1, &_texturedVertexBuffer, &stride, &offset);
	dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CShaders::SelectTextureShader();

	std::vector<CTextureGroup*>::iterator tit = _allTextures.begin();
	std::vector<CTextureGroup*>::iterator tend = _allTextures.end();
	while (tit != tend)
	{
		CTextureGroup* pT = *tit;

		if (pT->Points.Next != NULL)
		{
			CTexture* pTex = (pT->RealTexture != NULL) ? pT->RealTexture : pT->pTexture;
			ID3D11ShaderResourceView* pRV = pTex->GetTextureRV();
			dx.SetShaderResources(0, 1, &pRV);
			CPointList* pP = pT->Points.Next;
			while (pP != NULL)
			{
				dx.Draw(pP->Count, pP->First);
				pP = pP->Next;
			}
		}

		tit++;
	}

	stride = sizeof(TEXTURED_VERTEX);
	offset = 0;
	dx.SetVertexBuffers(0, 1, &_spriteVertexBuffer, &stride, &offset);
	dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	tit = _allTextures.begin();
	tend = _allTextures.end();
	while (tit != tend)
	{
		CTextureGroup* pT = *tit;
		if (pT->SpriteVerticeCount > 0)
		{
			CTexture* pTex = (pT->RealTexture != NULL) ? pT->RealTexture : pT->pTexture;
			ID3D11ShaderResourceView* pRV = pTex->GetTextureRV();
			dx.SetShaderResources(0, 1, &pRV);
			dx.Draw(pT->SpriteVerticeCount, pT->SpriteVertexStart);
		}

		tit++;
	}

	if (_transparentVerticeCount > 0)
	{
		stride = sizeof(COLOURED_VERTEX);
		dx.SetVertexBuffers(0, 1, &_transparentVertexBuffer, &stride, &offset);
		CShaders::SelectTransparentColourShader();
		dx.Draw(_transparentVerticeCount, 0);
	}
}

TLPoint CLocation::GetPoint(uint8_t* p3d2, int offset, int index, int points, float tw, float th, int objectCount, int object, int subObject)
{
	TLPoint p;
	p.ObjectIndex = object;
	p.SubObjectIndex = subObject;

	int pix = GetInt(p3d2, offset + 0 + index * 4, 4) >> 4;
	float fu = ((float)GetInt(p3d2, offset + 0 + points * 4 + index * 8, 4)) / 65536.0f;
	float fv = ((float)GetInt(p3d2, offset + 4 + points * 4 + index * 8, 4)) / 65536.0f;

	p.Point = &_points[pix + objectCount];
	p.U = std::min(std::max(0.0f, fu / tw), 1.0f);
	p.V = std::min(std::max(0.0f, fv / th), 1.0f);

	return p;
}

void CLocation::SetPosition(StartupPosition pos)
{
	_x = pos.X;
	_y = pos.Y;
	_y_player_adjustment = pos.InitialEyeLevel;
	_y_player_adjustment_min = pos.MinYAdj;
	_y_player_adjustment_max = pos.MaxYAdj;
	_y_elevation = pos.Elevation;

	_z = pos.Z;
	_angle1 = 0.0f;
	_angle2 = pos.Angle;

	_pCurrentElevation = NULL;
}

void CLocation::SetPosition(float x, float y, float z, float angle)
{
	_x = x;
	_y_player_adjustment = y;
	_z = z;
	_angle2 = angle;
}

double Dot(DPoint& p1, DPoint& p2)
{
	return p1.X * p2.X + p1.Z * p2.Z;
}

DPoint Normal(DPoint p)
{
	return { -p.Z, 0.0, p.X };
}

DPoint Subtract(DPoint& p1, DPoint& p2)
{
	DPoint p{ p1.X - p2.X, p1.Y - p2.Y, p1.Z - p2.Z };
	return p;
}

double LineSegmentsDistance(DPoint& p1, DPoint& p2, DPoint& p3, DPoint& p4, double& t, double& uc)
{
	DPoint u = Subtract(p2, p1);
	DPoint v = Subtract(p4, p3);
	DPoint w = Subtract(p1, p3);

	double a = Dot(u, u);
	double b = Dot(u, v);
	double c = Dot(v, v);
	double d = Dot(u, w);
	double e = Dot(v, w);
	double D = a * c - b * b;
	double sc, sN, sD = D;
	double tc, tN, tD = D;

	double SMALL_NUM = 0.00000001;

	if (D < SMALL_NUM)
	{
		sN = 0.0;
		sD = 1.0;
		tN = e;
		tD = c;
	}
	else
	{
		sN = (b * e - c * d);
		tN = (a * e - b * d);
		if (sN < 0.0)
		{
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if (sN > sD)
		{
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if (tN < 0.0)
	{
		tN = 0.0;
		if (-d < 0.0)
		{
			sN = 0.0;
		}
		else if (-d > a)
		{
			sN = sD;
		}
		else
		{
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD)
	{
		tN = tD;
		if ((-d + b) < 0.0)
		{
			sN = 0;
		}
		else if ((-d + b) > a)
		{
			sN = sD;
		}
		else
		{
			sN = (-d + b);
			sD = a;
		}
	}

	sc = (abs(sN) < SMALL_NUM ? 0.0 : sN / sD);
	tc = (abs(tN) < SMALL_NUM ? 0.0 : tN / tD);

	DPoint dP{ w.X + u.X * sc - v.X * tc, 0.0, w.Z + u.Z * sc - v.Z * tc };

	t = sc;
	uc = tc;

	return sqrt(Dot(dP, dP));
}

void CLocation::Move(float mx, float my, float mz, float tmx)
{
	if (mx != 0.0f || my != 0.0f || mz != 0.0f)
	{
		PointingChanged = true;
	}

	if (mx != 0.0f || mz != 0.0f)
	{
		double nx = _x - (mz * sin(_angle2) - mx * cos(_angle2));
		double nz = _z + (mz * cos(_angle2) + mx * sin(_angle2));

#ifdef DEBUG
		bool collision = !_disableClipping;
#else
		bool collision = true;
#endif

		DPoint pp1{ -_x, 0.0, -_z };
		DPoint pp2{ -nx, 0.0, -nz };

		for (int pass = 0; pass < 2 && collision; pass++)
		{
			collision = false;

			double closest_t = 10000.0;
			double closest_u = 0.0;
			double closest_dist = 10000.0;
			double closest_dp = 10000.0;
			IntersectionInfo closestIntersection;
			closestIntersection.LineSegmentP1 = DPoint{ 0.0, 0.0, 0.0 };
			closestIntersection.LineSegmentP2 = DPoint{ 0.0, 0.0, 0.0 };
			closestIntersection.ProjectionPoint = DPoint{ 0.0, 0.0, 0.0 };
			closestIntersection.ProjectionLength = 0.0;

			for (int i = 0; i < _pathCount; i++)
			{
				if (!_paths[i].enabled)
				{
					continue;
				}

				for (std::size_t p1 = 0; p1 < _paths[i].Points.size(); p1++)
				{
					auto p2 = p1 + 1;
					if (p2 == _paths[i].Points.size()) p2 = 0;

					DPoint pt1 = _paths[i].Points.at(p1);
					DPoint pt2 = _paths[i].Points.at(p2);

					DPoint currentLineNormal = Normalize(Normal(Subtract(pt2, pt1)));
					DPoint currentMovement = Normalize(Subtract(pp2, pp1));
					double current_dp = Dot(currentMovement, currentLineNormal);
					{
						double t = 0.0, u = 0.0;
						double d = LineSegmentsDistance(pp1, pp2, pt1, pt2, t, u);

						if (d < 0.5 && d < closest_dist)
						{
							{
								closestIntersection.LineSegmentP1 = pt1;
								closestIntersection.LineSegmentP2 = pt2;
								closest_t = t;
								closest_u = u;
								closest_dist = d;
								closest_dp = current_dp;
								collision = true;
							}
						}
					}
				}
			}

			if (collision)
			{
				DPoint mp = Project(closestIntersection.LineSegmentP1, closestIntersection.LineSegmentP2, pp2);
				DPoint pp = Project(closestIntersection.LineSegmentP1, closestIntersection.LineSegmentP2, pp1);

				double distanceToMovement = sqrt(DistanceSquared(pp1, pp2));
				double playerDistanceToLine = sqrt(DistanceSquared(pp1, pp));
				double movementDistanceToLine = sqrt(DistanceSquared(pp2, mp));

				double distanceDelta = playerDistanceToLine - movementDistanceToLine;
				double percentage = 1.0;

				if (playerDistanceToLine > 0.5 && movementDistanceToLine <= 0.5)
				{
					percentage = (distanceDelta - (0.5001 - movementDistanceToLine)) / distanceDelta;
				}
				else if (playerDistanceToLine <= 0.5 && movementDistanceToLine >= playerDistanceToLine)
				{
					percentage = 1.0;
				}
				else if (playerDistanceToLine < 0.5 && movementDistanceToLine < playerDistanceToLine)
				{
					nx = _x;
					nz = _z;
					break;
				}
				else
				{
					percentage = 0.0;
				}

				DPoint tp{ pp1.X + (pp2.X - pp1.X) * percentage, 0.0, pp1.Z + (pp2.Z - pp1.Z) * percentage };
				DPoint tpp = Project(closestIntersection.LineSegmentP1, closestIntersection.LineSegmentP2, tp);
				DPoint tmp = Subtract(mp, tpp);

				pp1.X = tp.X;
				pp1.Z = tp.Z;

				pp2.X = tp.X + tmp.X;
				pp2.Z = tp.Z + tmp.Z;

				nx = (pass == 0) ? -pp2.X : -tp.X;
				nz = (pass == 0) ? -pp2.Z : -tp.Z;

				DPoint ntp{ -nx, 0.0, -nz };
				DPoint npp = Project(closestIntersection.LineSegmentP1, closestIntersection.LineSegmentP2, ntp);
				double newDistance = sqrt(DistanceSquared(ntp, npp));

				double newClosestD = 10000.0;
				int newclosestpathindex = -1;
				int newclosestPathSubIndex = -1;
				for (int i = 0; i < _pathCount; i++)
				{
					if (_paths[i].enabled)
					{
						for (size_t p1 = 0; p1 < _paths[i].Points.size(); p1++)
						{
							auto p2 = p1 + 1;
							if (p2 == _paths[i].Points.size()) p2 = 0;

							DPoint xpt1 = _paths[i].Points.at(p1);
							DPoint xpt2 = _paths[i].Points.at(p2);

							DPoint xnpp = Project(xpt1, xpt2, ntp);
							double newd = sqrt(DistanceSquared(ntp, xnpp));
							if (newd < newClosestD)
							{
								newClosestD = newd;
								newclosestpathindex = i;
								newclosestPathSubIndex = static_cast<int>(p1);
							}
						}
					}
				}
			}
			else
			{
				double newClosestD = 10000.0;
				int newclosestpathindex = -1;
				int newclosestPathSubIndex = -1;
				DPoint ntp{ nx, 0.0, nz };
				for (int i = 0; i < _pathCount; i++)
				{
					if (_paths[i].enabled)
					{
						for (size_t p1 = 0; p1 < _paths[i].Points.size(); p1++)
						{
							auto p2 = p1 + 1;
							if (p2 == _paths[i].Points.size()) p2 = 0;

							DPoint xpt1 = _paths[i].Points.at(p1);
							DPoint xpt2 = _paths[i].Points.at(p2);

							DPoint xnpp = Project(xpt1, xpt2, ntp);
							double newd = sqrt(DistanceSquared(ntp, xnpp));
							if (newd < newClosestD)
							{
								newClosestD = newd;
							}
						}
					}
				}

				if (newClosestD < 0.5)
				{
					int ddddd = 0;
				}
			}
		}

		_x = (float)nx;
		_z = (float)nz;

#ifdef DEBUG
		if (!_disableClipping)
#endif
		{
			if (_pCurrentElevation != NULL)
			{
				if (_pCurrentElevation->IsPointInElevation(-_x, -_y_elevation, -_z))
				{
					_y_elevation = -_pCurrentElevation->GetElevationFromXZPosition(-_x, -_z);
				}
				else
				{
					_y_elevation = -_pCurrentElevation->GetClosestElevation(-_y_elevation);
				}

				_pCurrentElevation = NULL;
			}

			if (_pCurrentElevation == NULL)
			{
				for (auto el : Elevations)
				{
					if (el->IsPointInElevation(-_x, -_y_elevation, -_z))
					{
						_y_elevation = -el->GetElevationFromXZPosition(-_x, -_z);
						_pCurrentElevation = el;
						break;
					}
				}
			}
		}
	}

	if (my != 0.0f)
	{
		float miny = _y_player_adjustment_min;
		float maxy = _y_player_adjustment_max;
#ifdef DEBUG
		if (_disableClipping)
		{
			miny = -1000;
			maxy = 1000;
		}
#endif
		if (_y_player_adjustment <= maxy && _y_player_adjustment >= miny)
		{
			_y_player_adjustment += (my < 0.0f) ? std::max(my, miny - _y_player_adjustment) : std::min(my, maxy - _y_player_adjustment);
		}
	}

	UpdateSprites();
}

void CLocation::DeltaAngles(float angle1, float angle2)
{
	if (angle1 != 0.0f || angle2 != 0.0f)
	{
		PointingChanged = true;
	}

	_angle1 -= angle1;
	_angle2 -= angle2;

	while (_angle2 < 0.0)
	{
		_angle2 += twopi;
	}
	while (_angle2 > twopi)
	{
		_angle2 -= twopi;
	}

	if (_angle1 > (3.141592654f / 2.0f)) _angle1 = (3.141592654f / 2.0f);
	else if (_angle1 < -(3.141592654f / 2.0f)) _angle1 = -(3.141592654f / 2.0f);
}

void CLocation::LoadPaths()
{
	BinaryData bdpath = GetLocationData(1);

	uint8_t* ppath = bdpath.Data;

	_pathCount = GetInt(ppath, 0x1a, 2);
	_paths = new Path[_pathCount];

	std::vector<DPoint> points;
	int os = 0x80;

	int pathVerticeCount = GetInt(ppath, 0x18, 2);
	for (int v = 0; v < pathVerticeCount; v++)
	{
		float x = ((float)GetInt(ppath, os + 0, 4)) / 65536.0f;
		float y = ((float)GetInt(ppath, os + 4, 4)) / 65536.0f;
		float z = ((float)GetInt(ppath, os + 8, 4)) / 65536.0f;
		os += 12;

		DPoint p{ x, y, z };
		points.push_back(p);
	}

	for (int f = 0; f < _pathCount; f++)
	{
		if (GetInt(ppath, os, 4) == 0x45434146)
		{
			_paths[f].enabled = true;
			_paths[f].allowLeave = false;
			int pathId = GetInt(ppath, os + 14, 2);
			_paths[f].Id = (pathId != 0) ? pathId : f;

			int faceSize = GetInt(ppath, os + 4, 4);
			int vertCount = GetInt(ppath, os + 8, 4);

			for (int v = 0; v < vertCount; v++)
			{
				int pix = GetInt(ppath, os + 0x15 + v * 2, 2);
				DPoint pt = points.at(pix);
				_paths[f].Points.push_back(pt);
			}

			os += faceSize;
		}
		else break;
	}

#ifdef DEBUG
	Point* pPV = new Point[pathVerticeCount];
	if (pPV != NULL)
	{
		int pi = 0;
		for (auto pt : points)
		{
			pPV[pi].x = pt.X;
			pPV[pi].y = pt.Y;
			pPV[pi].z = pt.Z;
			pi++;
		}

		D3D11_BUFFER_DESC vertexBufferDesc;
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.ByteWidth = sizeof(Point) * pathVerticeCount;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		vertexBufferDesc.MiscFlags = 0;
		vertexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vertexData;
		vertexData.pSysMem = pPV;
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		dx.CreateBuffer(&vertexBufferDesc, &vertexData, &_pathVertexBuffer, "Path Vertices");

		delete pPV;
	}

	int faceCount = GetInt(ppath, 0x1a, 2);
	std::vector<int> lines;
	os = 0x80 + pathVerticeCount * 12;
	for (int f = 0; f < faceCount; f++)
	{
		if (GetInt(ppath, os, 4) == 0x45434146)
		{
			int faceSize = GetInt(ppath, os + 4, 4);
			int vertCount = GetInt(ppath, os + 8, 4);

			for (int v = 0; v < vertCount; v++)
			{
				int pix = GetInt(ppath, os + 0x15 + v * 2, 2);
				lines.push_back(pix);
			}

			if (vertCount > 2)
			{
				int pix = GetInt(ppath, os + 0x15, 2);
				lines.push_back(pix);
			}
			lines.push_back(-1);

			os += faceSize;
		}
		else break;
	}

	_pathIndexCount = lines.size();
	int* pPI = new int[_pathIndexCount];
	if (pPI != NULL)
	{
		for (int c = 0; c < _pathIndexCount; c++)
		{
			pPI[c] = lines.at(c);
		}

		D3D11_BUFFER_DESC indexBufferDesc;
		indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		indexBufferDesc.ByteWidth = sizeof(int) * _pathIndexCount;
		indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		indexBufferDesc.MiscFlags = 0;
		indexBufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA indexData;
		indexData.pSysMem = pPI;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		dx.CreateBuffer(&indexBufferDesc, &indexData, &_pathIndexBuffer, "Path Indexes");

		delete pPI;
	}
#endif
}

TLPoint CLocation::GetSpritePoint(uint8_t* p3d2, int offset, int index, int objectCount, int object, int subObject)
{
	TLPoint p;
	p.ObjectIndex = object;
	p.SubObjectIndex = subObject;

	float sprite_w = ((float)GetInt(p3d2, offset + 0, 4)) / 65536.0f;
	float sprite_h = ((float)GetInt(p3d2, offset + 4, 4)) / 65536.0f;

	int subSprites = GetInt(p3d2, offset + 0x10, 4);
	int pointIndex = GetInt(p3d2, offset + 0x1c + subSprites * 32, 2) >> 4;

	float u1 = ((float)GetInt(p3d2, offset + 0x2c, 4)) / 65536.0f;
	float v1 = ((float)GetInt(p3d2, offset + 0x30, 4)) / 65536.0f;
	float u2 = ((float)GetInt(p3d2, offset + 0x34, 4)) / 65536.0f;
	float v2 = ((float)GetInt(p3d2, offset + 0x38, 4)) / 65536.0f;

	int pix = pointIndex + objectCount;
	float cx = _points[pix].x;
	float cy = _points[pix].y;
	float cz = _points[pix].z;

	p.Point = new Point();

	if (index == 0)
	{
		p.Point->x = cx - sprite_w / 2;
		p.Point->y = cy - sprite_h;
		p.Point->z = cz;
		p.U = u2;
		p.V = v2;
	}
	else if (index == 1)
	{
		p.Point->x = cx + sprite_w / 2;
		p.Point->y = cy - sprite_h;
		p.Point->z = cz;
		p.U = u1;
		p.V = v2;
	}
	else if (index == 2)
	{
		p.Point->x = cx + sprite_w / 2;
		p.Point->y = cy;
		p.Point->z = cz;
		p.U = u1;
		p.V = v1;
	}
	else if (index == 3)
	{
		p.Point->x = cx - sprite_w / 2;
		p.Point->y = cy;
		p.Point->z = cz;
		p.U = u2;
		p.V = v1;
	}

	return p;
}

void CLocation::UpdateSprites()
{
	if (_spriteVertexBuffer != NULL)
	{
		D3D11_MAPPED_SUBRESOURCE subRes;
		memset(&subRes, 0, sizeof(subRes));
		dx.Map(_spriteVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes);

		TEXTURED_VERTEX* pTV = (TEXTURED_VERTEX*)subRes.pData;

		std::vector<CTextureGroup*>::iterator tit = _allTextures.begin();
		std::vector<CTextureGroup*>::iterator tend = _allTextures.end();
		int tix = 0;
		int tex = 0;
		while (tit != tend)
		{
			CTextureGroup* pTex = *tit;

			pTex->SpriteVertexStart = tix;

			std::vector<SpriteInfo>::iterator sit = pTex->SpriteInfos.begin();
			std::vector<SpriteInfo>::iterator send = pTex->SpriteInfos.end();

			while (sit != send)
			{
				SpriteInfo* spr = &(*sit);

				Point sv;
				sv.x = -_x - spr->P.x - _translationBuffer.translation[spr->ObjectIndex].x;
				sv.z = -_z - spr->P.z - _translationBuffer.translation[spr->ObjectIndex].z;
				float len = sqrt(sv.x * sv.x + sv.z * sv.z);
				if (len == 0.0f) len = 1.0f;
				sv.x /= len;
				sv.z /= len;

				float sy1 = spr->P.y - spr->H - spr->OY;
				float sy2 = spr->P.y - spr->OY;

				float x1 = spr->P.x - (spr->OX + spr->W) * sv.z;
				float x2 = spr->P.x - spr->OX * sv.z;
				float z1 = spr->P.z + (spr->OX + spr->W) * sv.x;
				float z2 = spr->P.z + spr->OX * sv.x;

				pTV[tix].position.x = x1;
				pTV[tix].position.y = sy1;
				pTV[tix].position.z = z1;
				pTV[tix].texture.x = spr->U2;
				pTV[tix].texture.y = spr->V2;
				pTV[tix].object.x = (float)spr->ObjectIndex;
				pTV[tix].object.y = (float)spr->SubObjectIndex;
				pTV[tix].objectParameters.x = 1.0f;
				tix++;
				pTV[tix].position.x = x2;
				pTV[tix].position.y = sy1;
				pTV[tix].position.z = z2;
				pTV[tix].texture.x = spr->U1;
				pTV[tix].texture.y = spr->V2;
				pTV[tix].object.x = (float)spr->ObjectIndex;
				pTV[tix].object.y = (float)spr->SubObjectIndex;
				pTV[tix].objectParameters.x = 1.0f;
				tix++;
				pTV[tix].position.x = x2;
				pTV[tix].position.y = sy2;
				pTV[tix].position.z = z2;
				pTV[tix].texture.x = spr->U1;
				pTV[tix].texture.y = spr->V1;
				pTV[tix].object.x = (float)spr->ObjectIndex;
				pTV[tix].object.y = (float)spr->SubObjectIndex;
				pTV[tix].objectParameters.x = 1.0f;
				tix++;

				pTV[tix].position.x = x1;
				pTV[tix].position.y = sy1;
				pTV[tix].position.z = z1;
				pTV[tix].texture.x = spr->U2;
				pTV[tix].texture.y = spr->V2;
				pTV[tix].object.x = (float)spr->ObjectIndex;
				pTV[tix].object.y = (float)spr->SubObjectIndex;
				pTV[tix].objectParameters.x = 1.0f;
				tix++;
				pTV[tix].position.x = x2;
				pTV[tix].position.y = sy2;
				pTV[tix].position.z = z2;
				pTV[tix].texture.x = spr->U1;
				pTV[tix].texture.y = spr->V1;
				pTV[tix].object.x = (float)spr->ObjectIndex;
				pTV[tix].object.y = (float)spr->SubObjectIndex;
				pTV[tix].objectParameters.x = 1.0f;
				tix++;
				pTV[tix].position.x = x1;
				pTV[tix].position.y = sy2;
				pTV[tix].position.z = z1;
				pTV[tix].texture.x = spr->U2;
				pTV[tix].texture.y = spr->V1;
				pTV[tix].object.x = (float)spr->ObjectIndex;
				pTV[tix].object.y = (float)spr->SubObjectIndex;
				pTV[tix].objectParameters.x = 1.0f;
				tix++;

				sit++;
			}

			pTex->SpriteVerticeCount = tix - pTex->SpriteVertexStart;

			tit++;
			tex++;
		}

		dx.Unmap(_spriteVertexBuffer, 0);
	}
}

bool CLocation::Intersect(Box& boundingBox, Point& from, Point& direction)
{
	float t1 = (boundingBox.X1 - from.x) * direction.x;
	float t2 = (boundingBox.X2 - from.x) * direction.x;
	float t3 = (boundingBox.Y1 - from.y) * direction.y;
	float t4 = (boundingBox.Y2 - from.y) * direction.y;
	float t5 = (boundingBox.Z1 - from.z) * direction.z;
	float t6 = (boundingBox.Z2 - from.z) * direction.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	return (tmax >= 0.0f && tmax >= tmin);
}

int CLocation::GetPickObject(int& objectId, int& subObjectId)
{
	if (_loading)
	{
		return -1;
	}

	PointingChanged = false;

	Point ld;
	ld.x = cos(_angle1) * sin(-_angle2);
	ld.y = sin(_angle1);
	ld.z = cos(_angle1) * cos(-_angle2);

	Point perp;
	perp.x = sin(-_angle2 - 3.141592654f / 2);
	perp.y = 0.0f;
	perp.z = cos(-_angle2 - 3.141592654f / 2);

	float len = sqrt(ld.x * ld.x + ld.y * ld.y + ld.z * ld.z);
	ld.x /= len;
	ld.y /= len;
	ld.z /= len;

	Point dir;
	dir.x = 1.0f / ld.x;
	dir.y = 1.0f / ld.y;
	dir.z = 1.0f / ld.z;

	float distance = 1000000.0f;

	for (int i = 0; i < _objectCount; i++)
	{
		if (_visibilityBuffer.visibility[i].x > 0.0f)
		{
			Point teye;
			teye.x = -_x - _translationBuffer.translation[i].x;
			teye.y = -_y - _translationBuffer.translation[i].y;
			teye.z = -_z - _translationBuffer.translation[i].z;

			if (Intersect(_ppObjects[i]->BoundingBox, teye, dir))
			{
				for (int j = 0; j < _ppObjects[i]->SubObjectCount; j++)
				{
					if (_ppObjects[i]->SubObjects[j].Flags != 0 && _visibilityBuffer.visibility[_ppObjects[i]->SubObjects[j].SubObjectIndex].y > 0.0f)
					{
						if (Intersect(_ppObjects[i]->SubObjects[j].BoundingBox, teye, dir))
						{
							Math::vec3 veye = {teye.x, teye.y, teye.z};
							Math::vec3 vdir = {ld.x, ld.y, ld.z};
							Math::vec3 v0, v1, v2, v3;
							float td = 10000.0f;

							if (_ppObjects[i]->SubObjects[j].PointCount == 1)
							{
								float y1 = _ppObjects[i]->SubObjects[j].BoundingBox.Y1;
								float y2 = _ppObjects[i]->SubObjects[j].BoundingBox.Y2;
								float cx = (_ppObjects[i]->SubObjects[j].BoundingBox.X1 + _ppObjects[i]->SubObjects[j].BoundingBox.X2) / 2.0f;
								float cz = (_ppObjects[i]->SubObjects[j].BoundingBox.Z1 + _ppObjects[i]->SubObjects[j].BoundingBox.Z2) / 2.0f;
								float w = _ppObjects[i]->SubObjects[j].BoundingBox.X2 - _ppObjects[i]->SubObjects[j].BoundingBox.X1;

								Point sv;
								sv.x = teye.x - cx;
								sv.z = teye.z - cz;
								float len = sqrt(sv.x * sv.x + sv.z * sv.z);
								if (len == 0.0f) len = 1.0f;
								sv.x /= len;
								sv.z /= len;

								Point pp;
								pp.x = sv.z;
								pp.z = -sv.x;

								float ax = pp.x * w / 2;
								float az = pp.z * w / 2;

								float x1 = cx - ax;
								float x2 = cx + ax;
								float z1 = cz - az;
								float z2 = cz + az;

								v0 = {x1, y1, z1};
								v1 = {x2, y1, z2};
								v2 = {x1, y2, z1};
								v3 = {x2, y2, z2};

								if (Math::TriangleTests::Intersects(veye, vdir, v0, v3, v2, td) || Math::TriangleTests::Intersects(veye, vdir, v0, v1, v3, td))
								{
									if (td <= distance)
									{
										objectId = i;
										subObjectId = _ppObjects[i]->SubObjects[j].ID;
										distance = td;
									}
								}
							}
							else
							{
								int triCount = _ppObjects[i]->SubObjects[j].PointCount - 2;
								for (int t = 0; t < triCount; t++)
								{
									v0 = {
										_ppObjects[i]->SubObjects[j].Triangles[t].P1.Point->x,
										_ppObjects[i]->SubObjects[j].Triangles[t].P1.Point->y,
										_ppObjects[i]->SubObjects[j].Triangles[t].P1.Point->z
									};
									v1 = {
										_ppObjects[i]->SubObjects[j].Triangles[t].P2.Point->x,
										_ppObjects[i]->SubObjects[j].Triangles[t].P2.Point->y,
										_ppObjects[i]->SubObjects[j].Triangles[t].P2.Point->z
									};
									v2 = {
										_ppObjects[i]->SubObjects[j].Triangles[t].P3.Point->x,
										_ppObjects[i]->SubObjects[j].Triangles[t].P3.Point->y,
										_ppObjects[i]->SubObjects[j].Triangles[t].P3.Point->z
									};

									if (Math::TriangleTests::Intersects(veye, vdir, v0, v1, v2, td))
									{
										if (td <= distance)
										{
											objectId = i;
											subObjectId = _ppObjects[i]->SubObjects[j].ID;
											distance = td;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	HitObject = objectId;
	HitSubObject = subObjectId;
	ObjectIndex = -1;
	if (HitObject >= 0)
	{
		std::vector<int>::iterator oit = _mapEntry->ObjectMap.begin();
		std::vector<int>::iterator oend = _mapEntry->ObjectMap.end();
		int ix = 0;
		while (oit != oend)
		{
			int oid = *oit;
			if (((HitSubObject & 0x80000000) != 0 && HitSubObject == oid) || ((HitSubObject & 0x8000000) != 0 && oid == (HitSubObject >> 16)) || ((HitSubObject & 0x88000000) == 0 && HitObject == oid))
			{
				ObjectIndex = ix;
				break;
			}

			oit++;
			ix++;
		}
	}

	return ObjectIndex;
}

void CLocation::StartMappedAnimation(int index)
{
	int mappedIndex = _mapEntry->AnimationMap.at(index);
	StartIndexedAnimation(mappedIndex);
}

void CLocation::StartIndexedAnimation(int index)
{
	BinaryData bd = GetLocationData(0);
	if (index >= 0 && index < _locationAnimationCount && index < MAX_ANIMATIONS)
	{
		if (Animations[index].Status == AnimationStatus::NotStarted || Animations[index].Status == AnimationStatus::Completed)
		{
			Animations[index].ObjectId = GetInt(bd.Data, index * 8 + 4, 4);
			uint8_t* pA = bd.Data + GetInt(bd.Data, index * 8 + 8, 4);
			uint8_t* pAE = (index < (_locationAnimationCount - 1)) ? bd.Data + GetInt(bd.Data, index * 8 + 16, 4) : bd.Data + bd.Length;

			int id = GetInt(pA, 4, 4);
			if (id == index)
			{
				Animations[index].Type = GetInt(pA, 0, 4);

				pA += 12;

				Animations[index].AnimDataPointerInit = pA;
				Animations[index].AnimDataPointerEnd = pAE;
				Animations[index].ConstantFrameDuration = 0;

				if (Animations[index].Type == 1)
				{
					Animations[index].Parameter = GetInt(pA, 0, 4);
					pA += 4;
				}
				else if (Animations[index].Type == 15)
				{
					Animations[index].Parameter = GetInt(pA, 0, 4);
					Animations[index].FrameDuration = Animations[index].ConstantFrameDuration = (uint32_t)(GetInt(pA, 4, 4) * TIMER_SCALE);
					pA += 8;
					Animations[index].AnimDataPointerInit = pA;
				}
				else
				{
					Animations[index].Parameter = (Animations[index].Type == 3) ? Animations[index].ObjectId : -1;
				}

				Animations[index].AnimDataPointer = pA;
				Animations[index].FrameCounter = 0;
				Animations[index].FrameDuration = 0;
				Animations[index].FrameTime = 0;
				Animations[index].ParentAnim = -1;
				Animations[index].Status = AnimationStatus::Running;
			}
		}
	}
}

void CLocation::StartIdAnimation(int index)
{
	int mappedId = _mapEntry->AnimationMap.at(index);

	BinaryData bd = GetLocationData(0);
	for (int index = 0; index < _locationAnimationCount && index < MAX_ANIMATIONS; index++)
	{
		int animId = GetInt(bd.Data, index * 8 + 4, 4);
		if (animId == mappedId)
		{
			if (Animations[index].Status == AnimationStatus::NotStarted || Animations[index].Status == AnimationStatus::Completed)
			{
				Animations[index].ObjectId = GetInt(bd.Data, index * 8 + 4, 4);
				uint8_t* pA = bd.Data + GetInt(bd.Data, index * 8 + 8, 4);
				uint8_t* pAE = (index < (_locationAnimationCount - 1)) ? bd.Data + GetInt(bd.Data, index * 8 + 16, 4) : bd.Data + bd.Length;

				int id = GetInt(pA, 4, 4);
				if (id == index)
				{
					Animations[index].Type = GetInt(pA, 0, 4);

					pA += 12;

					Animations[index].AnimDataPointerInit = pA;
					Animations[index].AnimDataPointerEnd = pAE;
					Animations[index].ConstantFrameDuration = 0;

					if (Animations[index].Type == 1)
					{
						Animations[index].Parameter = GetInt(pA, 0, 4);
						pA += 4;
					}
					else if (Animations[index].Type == 15)
					{
						Animations[index].Parameter = GetInt(pA, 0, 4);
						Animations[index].FrameDuration = Animations[index].ConstantFrameDuration = (uint32_t)(GetInt(pA, 4, 4) * TIMER_SCALE);
						pA += 8;
						Animations[index].AnimDataPointerInit = pA;
					}
					else
					{
						Animations[index].Parameter = (Animations[index].Type == 3) ? Animations[index].ObjectId : -1;
					}

					Animations[index].AnimDataPointer = pA;
					Animations[index].FrameCounter = 0;
					Animations[index].FrameDuration = 0;
					Animations[index].FrameTime = 0;
					Animations[index].ParentAnim = -1;
					Animations[index].Status = AnimationStatus::Running;
				}
			}

			break;
		}
	}
}

void CLocation::StopMappedAnimation(int index)
{
	if (index >= 0 && index < _locationAnimationCount && index < MAX_ANIMATIONS)
	{
		int mappedIndex = _mapEntry->AnimationMap.at(index);
		StopIndexedAnimation(mappedIndex);
	}
}

void CLocation::StopIndexedAnimation(int index)
{
	if (index >= 0 && index < _locationAnimationCount && index < MAX_ANIMATIONS)
	{
		Animations[index].Status = AnimationStatus::Completed;
	}
}

bool CLocation::IsAnimationFinished(int index)
{
	int mappedIndex = _mapEntry->AnimationMap.at(index);
	return (mappedIndex >= 0 && mappedIndex < _locationAnimationCount && mappedIndex < MAX_ANIMATIONS && Animations[mappedIndex].Status == AnimationStatus::Completed);
}

bool CLocation::IsIndexedAnimationFinished(int index)
{
	return (index >= 0 && index < _locationAnimationCount && index < MAX_ANIMATIONS && (Animations[index].Status == AnimationStatus::Completed || Animations[index].Status == AnimationStatus::NotStarted));
}

int CLocation::GetAnimationFrame(int index)
{
	int mappedIndex = _mapEntry->AnimationMap.at(index);
	return (mappedIndex >= 0 && mappedIndex < _locationAnimationCount && mappedIndex < MAX_ANIMATIONS) ? Animations[mappedIndex].FrameCounter : -1;
}

int CLocation::GetIndexedAnimationFrame(int index)
{
	return (index >= 0 && index < _locationAnimationCount && index < MAX_ANIMATIONS) ? Animations[index].FrameCounter : -1;
}

double CLocation::GetPlayerDistanceFromPoint(double x, double z)
{
	double dx = -_x - x;
	double dz = -_z - z;
	return dx * dx + dz * dz;
}

Point CLocation::GetPlayerPosition()
{
	Point p;
	p.x = -_x;
	p.y = _y_elevation + _y_player_adjustment;
	p.z = -_z;
	return p;
}

Point CLocation::GetUnadjustedPlayerPosition()
{
	Point p;
	p.x = -_x;
	p.y = _y_elevation;
	p.z = -_z;
	return p;
}

SpritePosInfo CLocation::GetSpriteInfo(int index)
{
	SpritePosInfo info;
	info.Position = Point{ 0.0f,0.0f,0.0f };
	info.Visible = false;

	if (index >= 0 && index < _objectCount)
	{
		BinaryData bd = GetLocationData(4);

		uint8_t* pObj = bd.Data + GetInt(bd.Data, 0x30 + index * 4, 4) + 0x30;

		int subObjects = GetInt(pObj, 12, 4);
		if (subObjects == 1)
		{
			uint8_t* pSub = pObj + 40;
			int flags = GetInt(pSub, 8, 4);

			if ((flags & SUBOBJECT_FLAGS_SPRITE) != 0)
			{
				int points = GetInt(pSub, 4, 4);
				if (points == 1)
				{
					int subSprites = GetInt(pSub, 0x38, 4);
					if (subSprites == 1)
					{
						int pointIndex = GetInt(pSub, 0x44 + subSprites * 32, 2) >> 4;
						info.Position = _points[pointIndex + _objectCount];

						info.Position.x += _translationBuffer.translation[index].x;
						info.Position.y += _translationBuffer.translation[index].y;
						info.Position.z += _translationBuffer.translation[index].z;

						info.Visible = (_visibilityBuffer.visibility[index].x == 1.0f);
					}
				}
			}
		}
	}

	return info;
}

void CLocation::ModifyLocationPoints(std::string file)
{
	if (file == "ALLEY.AP")
	{
		ModifyLocationPoints(238, 241, 0.0f, 0.1f, 0.0f);
		ModifyLocationPoints(270, 273, 0.0f, 0.15f, 0.0f);
		ModifyLocationPoints(1395, 1398, 0.0f, 0.1f, 0.0f);
	}
	else if (file == "ARBOR.AP")
	{
		ModifyLocationPoints(1023, 1028, -0.1f, 0.0f, 0.0f);
	}
	else if (file == "AV.AP")
	{
		ModifyLocationPoints(131, 134, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(203, 210, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "BEDROOM.AP")
	{
		ModifyLocationPoints(58, 61, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(3652, 3655, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(3837, 3840, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(4025, 4028, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(4210, 4213, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(4402, 4405, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(4591, 4594, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(4781, 4784, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(62, 69, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "CASTLE.AP")
	{
		ModifyLocationPoints(87, 90, -0.01f, 0.0f, 0.0f);
	}
	else if (file == "COLOFF.AP")
	{
		ModifyLocationPoints(20, 23, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(28, 31, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(36, 39, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(251, 254, 0.0f, 0.0f, -0.15f);
		ModifyLocationPoints(468, 571, 0.0f, 0.0f, -0.2f);
		ModifyLocationPoints(2921, 2924, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(2943, 2950, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2951, 2954, 0.0f, 0.02f, 0.0f);
		ModifyLocationPoints(2955, 2958, 0.0f, -0.1f, 0.0f);
	}
	else if (file == "COUNTESS.AP")
	{
		ModifyLocationPoints(50, 53, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(58, 65, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(70, 73, 0.0f, 0.015f, 0.0f);
		ModifyLocationPoints(82, 85, 0.0f, -0.02f, 0.0f);
		ModifyLocationPoints(86, 89, 0.0f, -0.01f, 0.0f);
		ModifyLocationPoints(591, 598, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(599, 667, 0.0f, 0.4f, 0.0f);
		ModifyLocationPoints(668, 707, 0.0f, 0.1f, 0.0f);
	}
	else if (file == "DUBOIS.AP")
	{
		ModifyLocationPoints(120, 131, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(132, 139, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(140, 147, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(148, 155, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(156, 159, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(160, 171, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(172, 187, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(192, 195, -0.005f, 0.0f, 0.005f);
		ModifyLocationPoints(260, 299, 0.0f, 0.0f, 0.3f);
		ModifyLocationPoints(475, 478, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(517, 520, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(589, 592, 0.0f, 0.01f, 0.0f);
	}
	else if (file == "GRSHALL.AP")
	{
		ModifyLocationPoints(613, 616, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(625, 628, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(637, 640, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(649, 652, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(524, 531, 0.0f, 0.0f, 0.01f);
	}
	else if (file == "HALL.AP")
	{
		ModifyLocationPoints(154, 157, -1.0f, 0.0f, 0.0f);
		ModifyLocationPoints(158, 161, 1.0f, 0.0f, 0.0f);
		ModifyLocationPoints(162, 167, 0.0f, 0.0f, 1.0f);
		ModifyLocationPoints(260, 263, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(380, 383, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(1782, 1785, 0.0f, 0.4f, 0.0f);

		ModifyLocationPoints(174, 175, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(178, 183, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(185, 186, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(189, 190, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(193, 194, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(197, 198, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(202, 203, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(206, 219, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(228, 239, 0.01f, 0.0f, 0.0f);
	}
	else if (file == "JACUZZI.AP")
	{
		ModifyLocationPoints(139, 142, -4.0f, 0.0f, 0.0f);
		ModifyLocationPoints(101, 104, 0.0f, -0.49f, 0.0f);
		ModifyLocationPoints(420, 423, 0.0f, -0.45f, 0.0f);
		ModifyLocationPoints(118, 119, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(121, 122, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "LIBRARY.AP")
	{
		ModifyLocationPoints(2495, 2498, -0.1f, 0.0f, 0.0f);
		ModifyLocationPoints(2499, 2502, -0.1f, 0.0f, 0.0f);
		ModifyLocationPoints(104, 107, 0.0f, 0.0f, -0.1f);
	}
	else if (file == "LIBHALL.AP")
	{
	}
	else if (file == "MAINSTRT.AP")
	{
		ModifyLocationPoints(2933, 2988, 0.0f, 0.6f, 0.0f);
		ModifyLocationPoints(2792, 2847, 0.0f, 0.6f, 0.0f);
		ModifyLocationPoints(3039, 3078, 0.0f, 0.6f, 0.0f);
		ModifyLocationPoints(3124, 3167, 0.0f, 0.6f, 0.0f);
		ModifyLocationPoints(3136, 3139, 0.0f, -0.005f, 0.0f);
		ModifyLocationPoints(3164, 3167, 0.0f, 0.2f, 0.0f);
		ModifyLocationPoints(3132, 3135, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(3047, 3050, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(3063, 3066, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2933, 2936, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2957, 2960, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2796, 2799, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2804, 2811, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2840, 2843, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(3192, 3195, 0.0f, -0.01f, 0.0f);
		ModifyLocationPoints(1708, 1717, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2304, 2305, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(2315, 2315, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(2321, 2321, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(281, 281, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(318, 318, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(320, 320, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(2160, 2162, 0.0f, 0.1f, 0.0f);
		ModifyLocationPoints(2163, 2165, 0.0f, 0.25f, 0.0f);
		ModifyLocationPoints(645, 738, 0.0f, 0.1f, 0.0f);
		ModifyLocationPoints(1367, 1370, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(2667, 2670, 0.0f, 0.0f, 1.5f);
	}
	else if (file == "MOONHALL.AP")
	{
		ModifyLocationPoints(1033, 1036, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(1045, 1048, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(1057, 1060, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(1069, 1072, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(1097, 1100, 0.01f, 0.0f, 0.0f);

		ModifyLocationPoints(655, 655, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(662, 662, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(636, 636, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(675, 675, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(786, 786, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(789, 789, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(301, 301, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(304, 304, 0.0f, 0.01f, 0.0f);
	}
	else if (file == "OBSERVE.AP")
	{
		ModifyLocationPoints(106, 113, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(881, 884, -0.02f, 0.0f, 0.0f);
	}
	else if (file == "PIANORM.AP")
	{
	}
	else if (file == "RADIO.AP")
	{
		ModifyLocationPoints(741, 746, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "RUSTY.AP")
	{
		ModifyLocationPoints(506, 506, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(508, 510, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "SAFEROOM.AP")
	{
		ModifyLocationPoints(104, 104, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(106, 106, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(109, 109, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(111, 111, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(418, 457, 0.0f, 0.1f, 0.0f);
		ModifyLocationPoints(476, 476, 0.0f, 0.15f, 0.0f);
	}
	else if (file == "SECRET.AP")
	{
		ModifyLocationPoints(81, 84, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "STUDY.AP")
	{
		ModifyLocationPoints(369, 372, 0.02f, 0.0f, 0.0f);
		ModifyLocationPoints(608, 611, -0.005f, 0.0f, 0.0f);
		ModifyLocationPoints(506, 509, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(61, 64, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(157, 160, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(1112, 1115, -0.005f, 0.0f, 0.0f);
		ModifyLocationPoints(2510, 2510, 0.0f, 0.0f, 0.022f);
		ModifyLocationPoints(2512, 2512, 0.0f, 0.0f, 0.022f);
		ModifyLocationPoints(2545, 2545, 0.0f, 0.0f, 0.022f);
		ModifyLocationPoints(2548, 2548, 0.0f, 0.0f, 0.022f);
	}
	else if (file == "TEXOFF.AP")
	{
		ModifyLocationPoints(68, 71, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(3410, 3413, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(2369, 2464, 0.0f, 0.235f, 0.0f);
		ModifyLocationPoints(3886, 3889, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(776, 779, 0.0f, -0.33f, 0.0f);
		ModifyLocationPoints(957, 960, 0.0f, -0.33f, 0.0f);
	}
	else if (file == "WAREHOUS.AP")
	{
	}
	else if (file == "R01VR.AP")
	{
		ModifyLocationPoints(510, 517, 0.0f, 0.0f, -0.1f);
		ModifyLocationPoints(1446, 1560, 0.0f, 0.03f, 0.0f);
		ModifyLocationPoints(4416, 4517, 0.0f, -0.15f, 0.0f);
		ModifyLocationPoints(82, 82, 0.0f, -0.15f, 0.0f);
	}
	else if (file == "R02VR.AP")
	{
	}
	else if (file == "R03VR.AP")
	{
	}
	else if (file == "R04VR.AP")
	{
		ModifyLocationPoints(207, 598, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(846, 851, 0.0f, 0.0f, 0.01f);
	}
	else if (file == "R05VR.AP")
	{
		ModifyLocationPoints(216, 219, -0.01f, 0.0f, 0.0f);
	}
	else if (file == "R06VR.AP")
	{
	}
	else if (file == "R07VR.AP")
	{
	}
	else if (file == "R08VR.AP")
	{
	}
	else if (file == "R09VR.AP")
	{
	}
	else if (file == "R11VR.AP")
	{
		ModifyLocationPoints(62, 63, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(72, 72, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(75, 75, 0.0f, 0.0f, 0.01f);
	}
	else if (file == "R12VR.AP")
	{
		ModifyLocationPoints(200, 205, 0.0f, 0.0f, -0.01f);
	}
	else if (file == "R15VR.AP")
	{
		ModifyLocationPoints(99, 111, 0.0f, 0.001f, 0.0f);
		ModifyLocationPoints(161, 164, 0.0f, 0.001f, 0.0f);
		ModifyLocationPoints(165, 168, 0.0f, 0.002f, 0.0f);
		ModifyLocationPoints(147, 154, 0.0f, -0.065f, 0.0f);
		ModifyLocationPoints(2855, 2874, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(320, 321, -0.03f, 0.0f, 0.0f);
		ModifyLocationPoints(324, 325, -0.03f, 0.0f, 0.0f);
	}
	else if (file == "R17VR.AP")
	{
	}
	else if (file == "R20VR.AP")
	{
	}
	else if (file == "R21VR.AP")
	{
	}
	else if (file == "R67VR.AP")
	{
		ModifyLocationPoints(272, 275, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(276, 279, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(280, 291, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(292, 303, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(637, 644, 0.0f, 0.0f, -0.01f);
		ModifyLocationPoints(1705, 1720, 0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(1721, 1736, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(2583, 2590, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(2673, 2680, 0.0f, 0.01f, 0.0f);
	}
	else if (file == "R68VR.AP")
	{
	}
	else if (file == "R69VR.AP")
	{
	}
	else if (file == "R70VR.AP")
	{
		ModifyLocationPoints(230, 233, 0.0f, 0.0f, 0.01f);
		ModifyLocationPoints(234, 237, -0.01f, 0.0f, 0.0f);
		ModifyLocationPoints(176, 177, 0.0f, 0.01f, 0.0f);
		ModifyLocationPoints(179, 181, 0.0f, 0.01f, 0.0f);
	}
	else if (file == "R71VR.AP")
	{
		ModifyLocationPoints(1037, 1195, 0.0f, 0.0f, 0.01f);
	}
}

void CLocation::ModifyLocationPoints(int startix, int endix, float x, float y, float z)
{
	BinaryData bd = GetLocationData(4);

	uint8_t* pData = bd.Data;
	int verticeCount = GetInt(pData, 0, 4);
	int objectCount = GetInt(pData, 12, 4);
	int modCount = endix - startix;

	startix += objectCount;

	uint8_t* pVertices = pData + 0x30 + objectCount * 4 + startix * 12;

	for (int i = 0; i <= modCount; i++)
	{
		float nx = x + ((float)GetInt(pVertices, 0, 4)) / 65536.0f;
		float ny = y + ((float)GetInt(pVertices, 4, 4)) / 65536.0f;
		float nz = z + ((float)GetInt(pVertices, 8, 4)) / 65536.0f;

		SetInt(pVertices, 0, (int)(nx * 65536.0f), 4);
		SetInt(pVertices, 4, (int)(ny * 65536.0f), 4);
		SetInt(pVertices, 8, (int)(nz * 65536.0f), 4);

		pVertices += 12;
	}
}

float4 CLocation::GetTransparentColour(std::string file, int objectId, int subObjectId)
{
	if (file == "ALLEY.AP")
	{
		return float4(1.0f, 1.0f, 1.0f, 0.4f);
	}
	else if (file == "CASTLE.AP")
	{
		return float4(0.0f, 0.0f, 0.6f, 0.4f);
	}
	else if (file == "JACUZZI.AP")
	{
		return float4(0.5f, 0.5f, 0.8f, 0.4f);
	}
	else if (file == "LIBRARY.AP")
	{
		return float4(1.0f, 1.0f, 1.0f, 0.1f);
	}
	else if (file == "RADIO.AP")
	{
		if (objectId == 22 && subObjectId == 16)
		{
			return float4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		return float4(0.3f, 0.3f, 0.7f, 0.4f);
	}
	else if (file == "SCHANZEE.AP")
	{
		return float4(0.0f, 0.0f, 0.0f, 0.5f);
	}
	else if (file == "SECRET.AP")
	{
		return float4(1.0f, 1.0f, 1.0f, 0.1f);
	}
	else if (file == "STUDY.AP")
	{
		return float4(1.0f, 1.0f, 1.0f, 0.08f);
	}
	else if (file == "R01VR.AP")
	{
		return float4(1.0f, 1.0f, 1.0f, 0.2f);
	}
	else if (file == "R15VR.AP")
	{
		return float4(0.2f, 0.2f, 0.2f, 0.5f);
	}

	return float4(0.3f, 0.3f, 0.7f, 0.4f);
}

BinaryData CLocation::GetLocationData(int index)
{
	BinaryData bd;

	int offset = GetInt(_locationData, 2 + index * 4, 4);
	int nextOffset = GetInt(_locationData, 6 + index * 4, 4);
	bd.Data = _locationData + offset;
	bd.Length = nextOffset - offset;

	return bd;
}

void CLocation::CTextureGroup::RemovePoints(int first, int count)
{
	Points.Remove(first, count);
}

void CLocation::CTextureGroup::AddPoints(int first, int count)
{
	Points.Add(first, count);
}

#ifdef DEBUG
void CLocation::MoveObject(float delta, bool X, bool Y, bool Z)
{
	if (HitObject != -1)
	{
		if (X)
		{
			_translationBuffer.translation[HitObject].x += delta;
		}
		if (Y)
		{
			_translationBuffer.translation[HitObject].y += delta;
		}
		if (Z)
		{
			_translationBuffer.translation[HitObject].z += delta;
		}
		_translationChanged = true;
	}
}
#endif

void CLocation::SetObjectVisibility(int objectId, bool visible)
{
	int id = _objectMap[objectId].id;
	if (id < 0x800)
	{
		ChangeVisibility(id, visible, false, "Script ");
	}
	else
	{
		for (int i = 0; i < _subObjectCount; i++)
		{
			if (((_improvedObjectMap[i].SubObjectId >> 16) & 0xffff) == id)
			{
				_visibilityBuffer.visibility[_improvedObjectMap[i].ObjectIndex].x = visible ? 1.0f : -1.0f;
			}
		}
	}
}

void CLocation::ChangeVisibility(int id, bool visible, bool setOnSubObjects, std::string header)
{
	bool found = false;
	if ((id & 0x80000000) != 0)
	{
		int objectId = (id >> 16) & 0x7fff;
		int subObjectId = id & 0xffff;
		for (int i = 0; i < _subObjectCount; i++)
		{
			if (_improvedObjectMap[i].ObjectIndex == objectId && (_improvedObjectMap[i].SubObjectId & 0xffff) == subObjectId)
			{
				_visibilityBuffer.visibility[_improvedObjectMap[i].SubObjectIndex].y = visible ? 1.0f : -1.0f;
				found = true;
			}
		}
	}
	else if ((id & 0x800) != 0)
	{
		for (int i = 0; i < _subObjectCount; i++)
		{
			if (((_improvedObjectMap[i].SubObjectId >> 16) & 0xffff) == id)
			{
				_visibilityBuffer.visibility[_improvedObjectMap[i].SubObjectIndex].y = visible ? 1.0f : -1.0f;
				found = true;
			}
		}
	}
	else
	{
		_visibilityBuffer.visibility[id].x = visible ? 1.0f : -1.0f;

		if (setOnSubObjects && false)
		{
			for (int i = 0; i < _subObjectCount; i++)
			{
				if (_improvedObjectMap[i].ObjectIndex == id && (_improvedObjectMap[i].SubObjectId & 0x80000000) == 0)
				{
					_visibilityBuffer.visibility[_improvedObjectMap[i].SubObjectIndex].y = visible ? 1.0f : -1.0f;
				}
			}
		}

		found = true;
	}

	_visibilityChanged = true;
}

void CLocation::Animate()
{
	if (!_loading)
	{
		uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		for (int i = 0; i < _locationAnimationCount && i < MAX_ANIMATIONS; i++)
		{
			if (Animations[i].Status == AnimationStatus::Running)
			{
				uint64_t frameTimeDiff = now - Animations[i].FrameTime;
				if (frameTimeDiff >= Animations[i].FrameDuration)
				{
					Animations[i].FrameTime = now;
					uint8_t* pA = Animations[i].AnimDataPointer;

					bool frameEnd = false;
					while (!frameEnd)
					{
						int p1 = GetInt(pA, 0, 4);
						pA += 4;
						if (p1 != -1)
						{
							switch (Animations[i].Type)
							{
								case 1:
								{
									int objectId = Animations[i].ObjectId;
									int subObjectId = Animations[i].Parameter;
									int newTexture = p1;

									CLocationObject* lo = &_pLocObjects[objectId];
									for (int soi = 0; soi < lo->SubObjectCount; soi++)
									{
										CLocationSubObject* pSub = &lo->pSubObjects[soi];
										if ((pSub->Id & 0xffff) == subObjectId)
										{
											CTextureGroup* pTG = _allTextures.at(pSub->TextureIndex);
											CTextureGroup* pNewT = _allTextures.at(newTexture);
											if (pTG->SpriteVerticeCount > 0)
											{
												pTG->RealTexture = pNewT->pTexture;
											}
											else
											{
												pTG->RemovePoints(pSub->VertexIndex, pSub->VertexCount);
												pNewT->AddPoints(pSub->VertexIndex, pSub->VertexCount);
												pSub->TextureIndex = newTexture;
											}

											break;
										}
									}

									Animations[i].FrameDuration = (uint32_t)(GetInt(pA, 0, 4) * TIMER_SCALE);
									pA += 4;
									frameEnd = true;
									break;
								}
								case 2:
								{
									float x = ((float)p1) / 65536.0f;
									float y = ((float)GetInt(pA, 0, 4)) / 65536.0f;
									float z = ((float)GetInt(pA, 4, 4)) / 65536.0f;
									Animations[i].FrameDuration = (uint32_t)(GetInt(pA, 8, 4) * TIMER_SCALE);
									pA += 12;
									_translationBuffer.translation[Animations[i].ObjectId].x += x;
									_translationBuffer.translation[Animations[i].ObjectId].y += y;
									_translationBuffer.translation[Animations[i].ObjectId].z += z;
									_translationChanged = true;
									PointingChanged = true;
									frameEnd = true;
									break;
								}
								case 3:
								{
									int objectToHide = Animations[i].Parameter;
									int objectToShow = p1;
									if (objectToShow >= 0)
									{
										Animations[i].FrameDuration = (uint32_t)(GetInt(pA, 0, 4) * TIMER_SCALE);
										pA += 4;

										if (objectToHide >= 0)
										{
											ChangeVisibility(objectToHide, false, false, "Animation ");
										}
										ChangeVisibility(objectToShow, true, false, "Animation ");

										Animations[i].Parameter = objectToShow;
										PointingChanged = true;
									}

									frameEnd = true;
									break;
								}
								case 4:
								{
									switch (p1)
									{
										case 1:
										{
											int objectId = GetInt(pA, 0, 4);
											int subObjectId = GetInt(pA, 4, 4);
											int newTexture = GetInt(pA, 8, 4);
											pA += 12;
											BinaryData bd3d2 = GetLocationData(4);
											uint8_t* p3d2 = bd3d2.Data;
											int objectOffset = GetInt(p3d2, 0x30 + objectId * 4, 4) + 0x30;

											int subObjects = GetInt(p3d2, objectOffset + 12, 4);
											int nextSubOffset = objectOffset + 40;
											for (int ts = 0; ts < subObjects; ts++)
											{
												int thisSubOffset = nextSubOffset;
												nextSubOffset = GetInt(p3d2, nextSubOffset, 4) + 0x30;

												int sid = GetInt(p3d2, thisSubOffset + 0xc, 4);
												if ((sid & 0xffff) == subObjectId)
												{
													int tex = GetInt(p3d2, thisSubOffset + 0x24, 4);
													CTextureGroup* pTG = _allTextures.at(tex);
													if (newTexture != tex)
													{
														CTextureGroup* pNewT = _allTextures.at(newTexture);
														pTG->RealTexture = pNewT->pTexture;
													}
													else
													{
														pTG->RealTexture = NULL;
													}
												}
											}

											break;
										}
										case 2:
										{
											int p1 = GetInt(pA, 0, 4);
											int p2 = GetInt(pA, 4, 4);
											int p3 = GetInt(pA, 8, 4);
											int p4 = GetInt(pA, 12, 4);
											int p5 = GetInt(pA, 16, 4);
											pA += 20;

											float x = ((float)p2) / 65536.0f;
											float y = ((float)p3) / 65536.0f;
											float z = ((float)p4) / 65536.0f;

											Animations[i].FrameDuration = (uint32_t)(p5 * TIMER_SCALE);
											_translationBuffer.translation[p1].x += x;
											_translationBuffer.translation[p1].y += y;
											_translationBuffer.translation[p1].z += z;
											_translationChanged = true;
											PointingChanged = true;
											frameEnd = true;
											break;
										}
										case 3:
										{
											int objectId = GetInt(pA, 0, 4);
											int visibility = GetInt(pA, 4, 4);
											pA += 8;

											ChangeVisibility(objectId, visibility, false, "Animation 4.3 ");

											PointingChanged = true;
											break;
										}
										case 5:
										{
											int objectId = GetInt(pA, 0, 4);
											int subObjectId = GetInt(pA, 4, 4);
											int visibility = GetInt(pA, 8, 4);
											pA += 12;

											ChangeVisibility(0x80000000 | (objectId << 16) | subObjectId, visibility, false, "Animation type 4.5 ");

											PointingChanged = true;
											break;
										}
										case 6:
										{
											int animix = GetInt(pA, 0, 4);
											pA += 4;
											StartIndexedAnimation(animix);
											break;
										}
										case 7:
										{
											int animix = GetInt(pA, 0, 4);
											pA += 4;
											StartIndexedAnimation(animix);
											Animations[animix].ParentAnim = i;
											Animations[i].Status = AnimationStatus::OnHold;
											frameEnd = true;
											break;
										}
										case 8:
										{
											int pid = GetInt(pA, 0, 4);
											for (int p = 0; p < _pathCount; p++)
											{
												if (_paths[p].Id == pid)
												{
													_paths[p].enabled = GetInt(pA, 4, 4);
													break;
												}
											}
											pA += 8;
											break;
										}
									}
									break;
								}
								case 14:
								{
									int  animation = p1;
									int texture = GetInt(pA, 0, 4);
									int duration = GetInt(pA, 4, 4);
									if (duration == 0)
									{
										duration = 3;
									}
									pA -= 4;
									CTextureGroup* pTG = _allTextures.at(animation);
									if (pTG->AnimatedTextureIndex >= pTG->Textures.size() - 1)
									{
										pTG->AnimatedTextureIndex = 0;
									}
									_allTextures.at(texture)->RealTexture = pTG->Textures.at(pTG->AnimatedTextureIndex++);
									Animations[i].FrameDuration = (uint32_t)(duration * TIMER_SCALE);
									frameEnd = true;
									break;
								}
								case 15:
								{
									int objectId = Animations[i].ObjectId;
									int subObjectId = Animations[i].Parameter;
									int newTexture = p1;

									BinaryData bd3d2 = GetLocationData(4);
									uint8_t* p3d2 = bd3d2.Data;
									int objectOffset = GetInt(p3d2, 0x30 + objectId * 4, 4) + 0x30;

									int subObjects = GetInt(p3d2, objectOffset + 12, 4);
									int nextSubOffset = objectOffset + 40;
									for (int ts = 0; ts < subObjects; ts++)
									{
										int thisSubOffset = nextSubOffset;
										nextSubOffset = GetInt(p3d2, nextSubOffset, 4) + 0x30;

										int sid = GetInt(p3d2, thisSubOffset + 0xc, 4);
										if ((sid & 0xffff) == subObjectId)
										{
											int tex = GetInt(p3d2, thisSubOffset + 0x24, 4);
											CTextureGroup* pTG = _allTextures.at(tex);
											CTextureGroup* pNewT = _allTextures.at(newTexture);
											pTG->RealTexture = pNewT->pTexture;
										}
									}

									Animations[i].FrameDuration = Animations[i].ConstantFrameDuration;
									if (pA >= Animations[i].AnimDataPointerEnd)
									{
										pA = Animations[i].AnimDataPointerInit;
									}

									frameEnd = true;
									break;
								}
								case 16:
								{
									break;
								}
							}
						}
						else
						{
							int cmd = GetInt(pA, 0, 4);
							pA += 4;
							switch (cmd)
							{
								case 0:
								{
									int p2 = GetInt(pA, 0, 4);
									int p3 = GetInt(pA, 4, 4);
									int p4 = GetInt(pA, 8, 4);
									if (p3 == -1)
									{
										p3 = p2;
									}

									SetInt(pA, 4, --p3, 4);

									pA += 12;
									if (p3 > 0)
									{
										pA -= (p4 + 4);
									}
									else
									{
										SetInt(pA, -8, -1, 4);
									}

									break;
								}
								case 1:
								{
									pA -= GetInt(pA, 0, 4);
									break;
								}
								case 2:
								{
									Animations[i].Status = AnimationStatus::Completed;
									if (Animations[i].ParentAnim >= 0)
									{
										Animations[Animations[i].ParentAnim].Status = AnimationStatus::Running;
									}
									frameEnd = true;
									break;
								}
								case 3:
								{
									Animations[i].FrameDuration = (uint32_t)(GetInt(pA, 0, 4) * TIMER_SCALE);
									pA += 4;
									frameEnd = true;
									break;
								}
							}
						}
					}

					Animations[i].AnimDataPointer = pA;
					Animations[i].FrameCounter++;
				}
			}
		}
	}
}