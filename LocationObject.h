#pragma once

#include <cstdint>

class CLocationSubObject
{
public:
    CLocationSubObject();
    virtual ~CLocationSubObject() = default;

    int Id{0};
    int TextureIndex{0};
    int VertexIndex{0};
    int VertexCount{0};
};

class CLocationObject
{
public:
    CLocationObject();
    virtual ~CLocationObject();

    int SubObjectCount{0};
    CLocationSubObject* pSubObjects{nullptr};
};