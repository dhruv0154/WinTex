#include "DXText.h"
#include "Globals.h"
#include "Utilities.h"
#include "DXScreen.h"
#include "ConstantBuffers.h"
#include <algorithm>
#include <cmath>
#include <cstring>

CDXText::CDXText()
{
	_vertexBuffer = nullptr;

	// TODO: Allow tab character
	// TODO: Compress multiple spaces? (This is currently done, but should be optional)

	_lines = 0;

	_printableCharacters = 0;
	_width = 0.0f;

	// Default colours, white without border
	_colour1 = DefaultCaptionColour1;
	_colour2 = DefaultCaptionColour2;
	_colour3 = DefaultCaptionColour3;
	_colour4 = DefaultCaptionColour4;
}

CDXText::~CDXText()
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }
}

void CDXText::Render(float x, float y, float z)
{
    if (_vertexBuffer == nullptr || _printableCharacters <= 0) return;

    uint32_t stride = sizeof(TEXTURED_VERTEX_ORTHO);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11ShaderResourceView* pRV = TexFont.GetTextureRV();
    
    float16 wm = Math::Translation(std::floor(x), -std::floor(y), z);

    CShaders::SelectTexFontShader();
    CDXFont::SelectFontColour(_colour1, _colour2, _colour3, _colour4);
    CConstantBuffers::SetWorld(dx, &wm);
    dx.SetShaderResources(0, 1, &pRV);
    
    dx.Draw(_printableCharacters * 6, 0);
}

CDXText::CWordList::CWordList()
{
    _text = nullptr;
    _chars = 0;
    _pixels = 0.0f;
    _next = nullptr;
    _last = nullptr;
}

CDXText::CWordList::CWordList(const char* text, int chars, float pixels)
{
    _text = text;
    _chars = chars;
    _pixels = pixels;
    _next = nullptr;
    _last = nullptr;
}

void CDXText::CWordList::Add(const char* text, int chars, float pixels)
{
    CWordList* pWL = new CWordList(text, chars, pixels);
    if (_next == nullptr) _next = pWL;
    if (_last != nullptr) _last->_next = pWL;
    _last = pWL;
}

CDXText::CWordList::~CWordList()
{
    while (_next != nullptr)
    {
        CWordList* next = _next;
        _next = next->_next;
        next->_next = nullptr;
        delete next;
    }
}

CDXText::CWordList* CDXText::CWordList::Next()
{
    return _next;
}

float CDXText::CWordList::Pixels() const
{
    return _pixels;
}

const char* CDXText::CWordList::Text() const
{
    return _text;
}

int CDXText::CWordList::Chars() const
{
    return _chars;
}

void CDXText::SetText(const wchar_t* text, Alignment alignment)
{
    _wstring = text;

    Rect rc;
    rc.Top = 0.0f;
    rc.Left = 10.0f;
    rc.Bottom = static_cast<float>(-dx.GetHeight());
    rc.Right = static_cast<float>(dx.GetWidth() - 10.0f);
    SetText(text, rc, alignment);
}

void CDXText::SetText(const char* text, Alignment alignment)
{
    _string = text;

    Rect rc;
    rc.Top = 0.0f;
    rc.Left = 10.0f;
    rc.Bottom = static_cast<float>(-dx.GetHeight());
    rc.Right = static_cast<float>(dx.GetWidth() - 10.0f);
    SetText(text, rc, alignment);
}

void CDXText::SetText(const char* text, Rect rect, Alignment alignment)
{
    _string = text;

    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;
    _width = 0.0f;

    CWordList wl;
    const char* scan = text;
    const char* start = scan;
    float pixels = 0.0f;
    _printableCharacters = 0;

    float* pWidths = TexFont.Widths();
    float spaceWidth = pWidths[0] * pConfig->FontScale;

    while (true)
    {
        char c = *(scan++);
        if (c == 0) break;
        else if (c == 0x20 || c == 0xa || c == 0xd)
        {
            int len = static_cast<int>(scan - start - 1);
            if (len > 0)
            {
                wl.Add(start, len, pixels);
            }

            if (c == 0xa || c == 0xd)
            {
                wl.Add(nullptr, 0, 0.0f);
            }

            start = scan;
            pixels = 0.0f;
        }
        else if (c > 0x20 && static_cast<unsigned char>(c) <= 0xff)
        {
            pixels += static_cast<float>(pWidths[c - 0x20]) * pConfig->FontScale;
            _printableCharacters++;
        }
    }

    int len = static_cast<int>(scan - start - 1);
    if (len > 0)
    {
        wl.Add(start, len, pixels);
    }

    if (_printableCharacters > 0)
    {
        std::vector<TEXTURED_VERTEX_ORTHO> pVB(6 * _printableCharacters);

        _lines = 0;
        CWordList* pWL = wl.Next();
        float maxw = rect.Right - rect.Left;
        float pixelsLeft = 0.0f;
        CWordList* print = nullptr;
        int wordsInLine = 0;

        float sy = 0.0f;
        int cix = 0;
        float fcw = 1.0f / 224.0f;
        float y1 = TexFont.Y1();
        float y2 = TexFont.Y2();
        float fh = TexFont.Height() * pConfig->FontScale;

        while (pWL != nullptr)
        {
            if (pWL->Pixels() == 0.0f)
            {
                _lines++;
                pWL = pWL->Next();
            }
            else
            {
                if (pWL->Pixels() > pixelsLeft)
                {
                    _lines++;
                    pixelsLeft = maxw - pWL->Pixels();
                    print = pWL;
                    pWL = pWL->Next();
                    wordsInLine = 1;
                }

                while (pWL != nullptr && pWL->Pixels() > 0.0f && (pixelsLeft - (pWL->Pixels() + spaceWidth)) > -0.01f)
                {
                    wordsInLine++;
                    pixelsLeft -= pWL->Pixels() + spaceWidth;
                    pWL = pWL->Next();
                }

                float sx = std::floor(rect.Left);
                float justifyadjust = 0.0f;
                switch (alignment)
                {
                    case Alignment::Left: break;
                    case Alignment::Center:
                        sx = pixelsLeft / 2.0f;
                        break;
                    case Alignment::Right:
                        sx = maxw - (pixelsLeft + (wordsInLine > 1 ? spaceWidth : 0.0f));
                        break;
                    case Alignment::Justify:
                        justifyadjust = (wordsInLine > 1 && pWL != nullptr) ? pixelsLeft / static_cast<float>(wordsInLine - 1) : 0.0f;
                        break;
                    case Alignment::JustifyAlways:
                        justifyadjust = (wordsInLine > 1) ? pixelsLeft / static_cast<float>(wordsInLine - 1) : 0.0f;
                        break;
                }

                if (pWL != nullptr && pWL->Pixels() == 0.0f)
                {
                    justifyadjust = 0.0f;
                }

                while (print != nullptr && wordsInLine-- > 0)
                {
                    for (int c = 0; c < print->Chars(); c++)
                    {
                        char ch = print->Text()[c];
                        if (ch >= 0x20 && static_cast<unsigned char>(ch) <= 0x7f)
                        {
                            ch -= 0x20;
                            float fx = pWidths[static_cast<int>(ch)];
                            float x1 = fcw * static_cast<float>(ch);
                            float x2 = x1 + fx / 3584.0f;

                            fx *= pConfig->FontScale;

                            float rsx = std::floor(sx);
                            pVB[cix].position = { rsx, sy, -1.5f };
                            pVB[cix++].texture = { x1, y1 };
                            pVB[cix].position = { rsx + fx, sy, -1.5f };
                            pVB[cix++].texture = { x2, y1 };
                            pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                            pVB[cix++].texture = { x2, y2 };

                            pVB[cix].position = { rsx, sy, -1.5f };
                            pVB[cix++].texture = { x1, y1 };
                            pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                            pVB[cix++].texture = { x2, y2 };
                            pVB[cix].position = { rsx, sy - fh, -1.5f };
                            pVB[cix++].texture = { x1, y2 };

                            sx += fx;
                        }
                    }

                    sx += spaceWidth + justifyadjust;
                    print = print->Next();
                }

                if (_width < (sx - spaceWidth - justifyadjust - rect.Left))
                {
                    _width = sx - spaceWidth - justifyadjust - rect.Left;
                }
            }

            sy -= TexFont.Height() * pConfig->FontScale;
            wordsInLine = 0;
            pixelsLeft = 0.0f;
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "Text");
    
    }
}

void CDXText::SetText(const wchar_t* text, Rect rect, Alignment alignment)
{
    _wstring = text;

    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;
    _width = 0.0f;

    CWordList wl;
    const wchar_t* scan = text;
    const wchar_t* start = scan;
    float pixels = 0.0f;
    _printableCharacters = 0;

    float* pWidths = TexFont.Widths();
    float spaceWidth = pWidths[0] * pConfig->FontScale;

    while (true)
    {
        char c = static_cast<char>(*(scan++) & 0xFF);
        if (c == 0) break;
        else if (c == 0x20)
        {
            int len = static_cast<int>(scan - start - 1);
            if (len > 0)
            {
                wl.Add(reinterpret_cast<const char*>(start), len, pixels);
            }
            start = scan;
            pixels = 0.0f;
        }
        else if (c > 0x20 && static_cast<unsigned char>(c) <= 0x7f)
        {
            pixels += static_cast<float>(pWidths[c - 0x20]) * pConfig->FontScale;
            _printableCharacters++;
        }
    }

    int len = static_cast<int>(scan - start - 1);
    if (len > 0)
    {
        wl.Add(reinterpret_cast<const char*>(start), len, pixels);
    }

    if (_printableCharacters > 0)
    {
        std::vector<TEXTURED_VERTEX_ORTHO> pVB(6 * _printableCharacters);

        _lines = 0;
        CWordList* pWL = wl.Next();
        float maxw = rect.Right - rect.Left;
        float pixelsLeft = 0.0f;
        CWordList* print = nullptr;
        int wordsInLine = 0;

        float sy = 0.0f;
        int cix = 0;
        float fcw = 1.0f / 224.0f;
        float y1 = TexFont.Y1();
        float y2 = TexFont.Y2();
        float fh = TexFont.Height() * pConfig->FontScale;

        while (pWL != nullptr)
        {
            if (pWL->Pixels() > pixelsLeft)
            {
                _lines++;
                pixelsLeft = maxw - pWL->Pixels();
                print = pWL;
                pWL = pWL->Next();
                wordsInLine = 1;
            }

            while (pWL != nullptr && (pixelsLeft - (pWL->Pixels() + spaceWidth)) > -0.01f)
            {
                wordsInLine++;
                pixelsLeft -= pWL->Pixels() + spaceWidth;
                pWL = pWL->Next();
            }

            float sx = rect.Left;
            float justifyadjust = 0.0f;
            switch (alignment)
            {
                case Alignment::Left: break;
                case Alignment::Center:
                    sx = pixelsLeft / 2.0f;
                    break;
                case Alignment::Right:
                    sx = maxw - (pixelsLeft + (wordsInLine > 1 ? spaceWidth : 0.0f));
                    break;
                case Alignment::Justify:
                    justifyadjust = (wordsInLine > 1 && pWL != nullptr) ? pixelsLeft / static_cast<float>(wordsInLine - 1) : 0.0f;
                    break;
                case Alignment::JustifyAlways:
                    justifyadjust = (wordsInLine > 1) ? pixelsLeft / static_cast<float>(wordsInLine - 1) : 0.0f;
                    break;
            }

            while (print != nullptr && wordsInLine-- > 0)
            {
                sx = std::floor(sx);

                for (int c = 0; c < print->Chars(); c++)
                {
                    char ch = static_cast<char>(reinterpret_cast<const wchar_t*>(print->Text())[c] & 0xFF);
                    if (ch >= 0x20 && static_cast<unsigned char>(ch) <= 0x7f)
                    {
                        ch -= 0x20;
                        float fx = pWidths[static_cast<int>(ch)];
                        float x1 = fcw * static_cast<float>(ch);
                        float x2 = x1 + fx / 3584.0f;

                        fx *= pConfig->FontScale;

                        float rsx = std::floor(sx);
                        pVB[cix].position = { rsx, sy, -1.5f };
                        pVB[cix++].texture = { x1, y1 };
                        pVB[cix].position = { rsx + fx, sy, -1.5f };
                        pVB[cix++].texture = { x2, y1 };
                        pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                        pVB[cix++].texture = { x2, y2 };

                        pVB[cix].position = { rsx, sy, -1.5f };
                        pVB[cix++].texture = { x1, y1 };
                        pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                        pVB[cix++].texture = { x2, y2 };
                        pVB[cix].position = { rsx, sy - fh, -1.5f };
                        pVB[cix++].texture = { x1, y2 };

                        sx += fx;
                    }
                }

                sx += spaceWidth + justifyadjust;
                print = print->Next();
            }

            if (_width < (sx - spaceWidth - justifyadjust - rect.Left))
            {
                _width = sx - spaceWidth - justifyadjust - rect.Left;
            }

            sy -= TexFont.Height() * pConfig->FontScale;
            wordsInLine = 0;
            pixelsLeft = 0.0f;
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "Text");
    }
}

void CDXText::SetTextUnmodified(const char* text)
{
    _string = text;

    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;
    _width = 0.0f;
    _printableCharacters = static_cast<int>(std::strlen(text));

    float* pWidths = TexFont.Widths();

    if (_printableCharacters > 0)
    {
        std::vector<TEXTURED_VERTEX_ORTHO> pVB(6 * _printableCharacters);

        float sy = 0.0f;
        float sx = 0.0f;
        int cix = 0;
        float fcw = 1.0f / 224.0f;
        float y1 = TexFont.Y1();
        float y2 = TexFont.Y2();
        float fh = TexFont.Height() * pConfig->FontScale;
        const char* scan = text;
        char ch;
        
        while ((ch = *scan++))
        {
            if (ch >= 0x20 && static_cast<unsigned char>(ch) <= 0x7f)
            {
                ch -= 0x20;
                float fx = pWidths[static_cast<int>(ch)];
                float x1 = fcw * static_cast<float>(ch);
                float x2 = x1 + fx / 3584.0f;

                fx *= pConfig->FontScale;

                float rsx = std::floor(sx);
                pVB[cix].position = { rsx, sy, -1.5f };
                pVB[cix++].texture = { x1, y1 };
                pVB[cix].position = { rsx + fx, sy, -1.5f };
                pVB[cix++].texture = { x2, y1 };
                pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                pVB[cix++].texture = { x2, y2 };

                pVB[cix].position = { rsx, sy, -1.5f };
                pVB[cix++].texture = { x1, y1 };
                pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                pVB[cix++].texture = { x2, y2 };
                pVB[cix].position = { rsx, sy - fh, -1.5f };
                pVB[cix++].texture = { x1, y2 };

                sx += fx;
            }
        }

        _width = sx;

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "Text");
    }
}

void CDXText::SetTextUnmodified(const wchar_t* text)
{
    _wstring = text;

    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;
    _width = 0.0f;
    _printableCharacters = static_cast<int>(_wstring.size());

    float* pWidths = TexFont.Widths();

    if (_printableCharacters > 0)
    {
        std::vector<TEXTURED_VERTEX_ORTHO> pVB(6 * _printableCharacters);

        float sy = 0.0f;
        float sx = 10.0f;
        int cix = 0;
        float fcw = 1.0f / 224.0f;
        float y1 = TexFont.Y1();
        float y2 = TexFont.Y2();
        float fh = TexFont.Height() * pConfig->FontScale;
        const wchar_t* scan = text;
        wchar_t wch;
        
        while ((wch = *scan++))
        {
            char ch = static_cast<char>(wch & 0xFF);
            if (ch >= 0x20 && static_cast<unsigned char>(ch) <= 0x7f)
            {
                ch -= 0x20;
                float fx = pWidths[static_cast<int>(ch)];
                float x1 = fcw * static_cast<float>(ch);
                float x2 = x1 + fx / 3584.0f;

                fx *= pConfig->FontScale;

                float rsx = std::floor(sx);
                pVB[cix].position = { rsx, sy, -1.5f };
                pVB[cix++].texture = { x1, y1 };
                pVB[cix].position = { rsx + fx, sy, -1.5f };
                pVB[cix++].texture = { x2, y1 };
                pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                pVB[cix++].texture = { x2, y2 };

                pVB[cix].position = { rsx, sy, -1.5f };
                pVB[cix++].texture = { x1, y1 };
                pVB[cix].position = { rsx + fx, sy - fh, -1.5f };
                pVB[cix++].texture = { x2, y2 };
                pVB[cix].position = { rsx, sy - fh, -1.5f };
                pVB[cix++].texture = { x1, y2 };

                sx += fx;
            }
        }

        _width = sx - 10.0f;

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(TEXTURED_VERTEX_ORTHO) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "Text");
    }
}

float CDXText::PixelWidth(const char* text)
{
    return TexFont.PixelWidth(text);
}

float CDXText::Width() const
{
    return _width;
}

void CDXText::Width(float w)
{
    _width = w;
}

float CDXText::Height() const
{
    return TexFont.Height() * static_cast<float>(_lines) * pConfig->FontScale;
}

void CDXText::SetColours(int colour)
{
    _colour1 = _colour4 = 0;
    _colour2 = _colour3 = colour;
}

void CDXText::SetColours(int colour1, int colour2)
{
    _colour1 = _colour4 = 0;
    _colour2 = colour1;
    _colour3 = colour2;
}

void CDXText::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    _colour1 = colour1;
    _colour2 = colour2;
    _colour3 = colour3;
    _colour4 = colour4;
}

void CDXText::ResetText()
{
    SetText(_wstring.c_str());
}
