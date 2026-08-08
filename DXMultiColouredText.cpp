#include "DXMultiColouredText.h"
#include "Globals.h"
#include "ConstantBuffers.h"
#include <algorithm>
#include <cmath>

void CDXMultiColouredText::CMCWordList::Add(const char* text, int chars, float pixels, int col)
{
    CMCWordList* pWL = new CMCWordList(text, chars, pixels, col);
    if (_next == nullptr) _next = pWL;
    if (_last != nullptr) _last->SetNext(pWL);
    _last = pWL;
}

CDXMultiColouredText::CMCWordList::CMCWordList(const char* text, int chars, float pixels, int col) : CWordList(text, chars, pixels)
{
    Colour = col;
}

CDXMultiColouredText::CDXMultiColouredText() : CDXText()
{
}

CDXMultiColouredText::~CDXMultiColouredText()
{
}

void CDXMultiColouredText::SetColours(int colour1, int colour2, int colour3, int colour4)
{
    _colour1 = colour1;
    _colour2 = colour2;
    _colour3 = colour3;
    _colour4 = colour4;
}

void CDXMultiColouredText::Render(float x, float y, float z)
{
    if (_vertexBuffer == nullptr || _printableCharacters <= 0) return;

    uint32_t stride = sizeof(MULTICOLOURED_FONT_VERTEX);
    uint32_t offset = 0;
    dx.SetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
    dx.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11ShaderResourceView* pRV = TexFont.GetTextureRV();
    
    float16 wm = Math::Translation(std::floor(x), -std::floor(y), z);

    CShaders::SelectMultiColouredFontShaderPD();
    CConstantBuffers::SetWorld(dx, &wm);
    dx.SetShaderResources(0, 1, &pRV);
    
    dx.Draw(_printableCharacters * 6, 0);
}

void CDXMultiColouredText::SetText(const char* text, Rect rect)
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;
    CMCWordList wl;
    const char* scan = text;
    const char* start = scan;
    float pixels = 0.0f;
    _printableCharacters = 0;

    float* pWidths = TexFont.Widths();
    int currentCol = _colour1;

    while (true)
    {
        char c = *(scan++);
        if (c == 0) break;
        else if (c == 0x20 || c == '<' || c == '~' || c == '>' || c == '@')
        {
            int len = static_cast<int>(scan - start - 1);
            if (len > 0)
            {
                wl.Add(reinterpret_cast<const char*>(start), len, pixels, currentCol);
            }
            start = scan;
            pixels = 0.0f;

            if (c == '<')      currentCol = _colour2;
            else if (c == '~') currentCol = _colour3;
            else if (c == '>' || c == '@') currentCol = _colour1;
        }
        else if (c > 0x20 && c <= 0x7f)
        {
            pixels += static_cast<float>(pWidths[c - 0x20]) * pConfig->FontScale;
            _printableCharacters++;
        }
    }

    int len = static_cast<int>(scan - start - 1);
    if (len > 0)
    {
        wl.Add(reinterpret_cast<const char*>(start), len, pixels, currentCol);
    }

    if (_printableCharacters > 0)
    {
        std::vector<MULTICOLOURED_FONT_VERTEX> pVB(6 * _printableCharacters);

        _lines = 0;
        CMCWordList* pWL = static_cast<CMCWordList*>(wl.Next());
        float maxw = rect.Right - rect.Left;
        float pixelsLeft = 0.0f;
        CMCWordList* print = nullptr;
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
                pWL = static_cast<CMCWordList*>(pWL->Next());
                wordsInLine = 1;
            }

            while (pWL != nullptr && (pixelsLeft - (pWL->Pixels() + pWidths[0])) > -0.01f)
            {
                wordsInLine++;
                pixelsLeft -= pWL->Pixels() + pWidths[0];
                pWL = static_cast<CMCWordList*>(pWL->Next());
            }

            float sx = rect.Left;

            while (print != nullptr && wordsInLine-- > 0)
            {
                float r = print->Red();
                float g = print->Green();
                float b = print->Blue();
                float4 col = { r, g, b, 1.0f };

                sx = std::floor(sx);
                for (int c = 0; c < print->Chars(); c++)
                {
                    char ch = reinterpret_cast<const char*>(print->Text())[c];
                    if (ch >= 0x20 && ch <= 0x7f)
                    {
                        ch -= 0x20;
                        float fx = pWidths[static_cast<int>(ch)];
                        float x1 = fcw * static_cast<float>(ch);
                        float x2 = x1 + fx / 3584.0f;
                        float scaledFx = fx * pConfig->FontScale;

                        pVB[cix].position = { sx, sy, -1.5f };
                        pVB[cix].texture  = { x1, y1 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx + scaledFx, sy, -1.5f };
                        pVB[cix].texture  = { x2, y1 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx + scaledFx, sy - fh, -1.5f };
                        pVB[cix].texture  = { x2, y2 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx, sy, -1.5f };
                        pVB[cix].texture  = { x1, y1 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx + scaledFx, sy - fh, -1.5f };
                        pVB[cix].texture  = { x2, y2 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx, sy - fh, -1.5f };
                        pVB[cix].texture  = { x1, y2 };
                        pVB[cix++].colour = col;

                        sx += scaledFx;
                    }
                }

                sx += pWidths[0];
                print = static_cast<CMCWordList*>(print->Next());
            }

            if (_width < (sx - pWidths[0] - rect.Left))
            {
                _width = sx - pWidths[0] - rect.Left;
            }

            sy -= fh;
            wordsInLine = 0;
            pixelsLeft = 0.0f;
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(MULTICOLOURED_FONT_VERTEX) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "MultiColouredText");
    
    }
}

void CDXMultiColouredText::SetTextPD(const char* text, Rect rect)
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;
    CMCWordList wl;
    const char* scan = text;
    const char* start = scan;
    float pixels = 0.0f;
    _printableCharacters = 0;

    float* pWidths = TexFont.Widths();
    int currentCol = _colour1;

    while (true)
    {
        char c = *(scan++);
        if (c == 0) break;
        else if (c == 0x20 || c == '^' || c == 0xa)
        {
            int len = static_cast<int>(scan - start - 1);
            if (len > 0)
            {
                wl.Add(reinterpret_cast<const char*>(start), len, pixels, currentCol);
            }

            if (c == '^')
            {
                // Next byte determines colour
				//currentCol = _colour2;
				//.-?
                int cix = static_cast<int>(*(scan++));
                if (cix == 0x2d)      currentCol = _colour1;
                else if (cix == 0x2e) currentCol = _colour2;
                else if (cix == 0x3f) currentCol = _colour3;
            }
            else if (c == 0xa)
            {
                wl.Add(nullptr, 0, 0.0f, 0);
            }

            start = scan;
            pixels = 0.0f;
        }
        else if (c > 0x20 && c <= 0x7f)
        {
            pixels += static_cast<float>(pWidths[c - 0x20]) * pConfig->FontScale;
            _printableCharacters++;
        }
    }

    int len = static_cast<int>(scan - start - 1);
    if (len > 0)
    {
        wl.Add(reinterpret_cast<const char*>(start), len, pixels, currentCol);
    }

    if (_printableCharacters > 0)
    {
        std::vector<MULTICOLOURED_FONT_VERTEX> pVB(6 * _printableCharacters);

        _lines = 0;
        CMCWordList* pWL = static_cast<CMCWordList*>(wl.Next());
        float maxw = rect.Right - rect.Left;
        float pixelsLeft = 0.0f;
        CMCWordList* print = nullptr;
        int wordsInLine = 0;

        float sy = 0.0f;
        int cix = 0;
        float fcw = 1.0f / 224.0f;
        float y1 = TexFont.Y1();
        float y2 = TexFont.Y2();
        float fh = TexFont.Height() * pConfig->FontScale;

        while (pWL != nullptr)
        {
            if (pWL->Text() == nullptr)
            {
                sy -= fh;
                pWL = static_cast<CMCWordList*>(pWL->Next());
                continue;
            }

            if (pWL->Pixels() > pixelsLeft)
            {
                _lines++;
                pixelsLeft = maxw - pWL->Pixels();
                print = pWL;
                pWL = static_cast<CMCWordList*>(pWL->Next());
                wordsInLine = 1;
            }

            while (pWL != nullptr && pWL->Text() != nullptr && (pixelsLeft - (pWL->Pixels() + pWidths[0])) > -0.01f)
            {
                wordsInLine++;
                pixelsLeft -= pWL->Pixels() + pWidths[0];
                pWL = static_cast<CMCWordList*>(pWL->Next());
            }

            float sx = rect.Left;

            while (print != nullptr && wordsInLine-- > 0)
            {
                float r = print->Red();
                float g = print->Green();
                float b = print->Blue();
                float4 col = { r, g, b, 1.0f };

                sx = std::floor(sx);
                for (int c = 0; c < print->Chars(); c++)
                {
                    char ch = reinterpret_cast<const char*>(print->Text())[c];
                    if (ch >= 0x20 && ch <= 0x7f)
                    {
                        ch -= 0x20;
                        float fx = pWidths[static_cast<int>(ch)];
                        float x1 = fcw * static_cast<float>(ch);
                        float x2 = x1 + fx / 3584.0f;
                        float scaledFx = fx * pConfig->FontScale;

                        pVB[cix].position = { sx, sy, -1.5f };
                        pVB[cix].texture  = { x1, y1 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx + scaledFx, sy, -1.5f };
                        pVB[cix].texture  = { x2, y1 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx + scaledFx, sy - fh, -1.5f };
                        pVB[cix].texture  = { x2, y2 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx, sy, -1.5f };
                        pVB[cix].texture  = { x1, y1 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx + scaledFx, sy - fh, -1.5f };
                        pVB[cix].texture  = { x2, y2 };
                        pVB[cix++].colour = col;

                        pVB[cix].position = { sx, sy - fh, -1.5f };
                        pVB[cix].texture  = { x1, y2 };
                        pVB[cix++].colour = col;

                        sx += scaledFx;
                    }
                }

                sx += pWidths[0];
                print = static_cast<CMCWordList*>(print->Next());
            }

            if (_width < (sx - pWidths[0] - rect.Left))
            {
                _width = sx - pWidths[0] - rect.Left;
            }

            sy -= fh;
            wordsInLine = 0;
            pixelsLeft = 0.0f;
        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(MULTICOLOURED_FONT_VERTEX) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "MultiColouredText");
    }
}

void CDXMultiColouredText::SetTextPD2(const char* text, Rect rect)
{
    if (_vertexBuffer != nullptr)
    {
        _vertexBuffer->Release();
        _vertexBuffer = nullptr;
    }

    _lines = 0;

    std::list<std::unique_ptr<CMCWord>> words;

    const char* scan = text;
    float pixels = 0.0f;
    _printableCharacters = 0;

    float* pWidths = TexFont.Widths();
    int currentCol = _colour1;

    auto pWord = std::make_unique<CMCWord>();

    while (true)
    {
        char c = *(scan++);
        if (c == 0 || c == 0x20 || c == 0xa)
        {
            if (pWord->Pixels > 0.0f)
            {
                words.push_back(std::move(pWord));
                pWord = std::make_unique<CMCWord>();
            }

            if (c == 0)
            {
                break;
            }
            else if (c == 0xa)
            {
                words.push_back(nullptr);
            }

            pixels = 0.0f;
        }
        else if (c == '^')
        {
            int cix = static_cast<int>(*(scan++));
            if (cix == 0x2d)      currentCol = _colour1;
            else if (cix == 0x2e) currentCol = _colour2;
            else if (cix == 0x3f) currentCol = _colour3;
        }
        else if (c > 0x20 && c <= 0x7f)
        {
            CMCChar mc;
            mc.Char = c;
            mc.Colour = currentCol;
            mc.Pixels = static_cast<float>(pWidths[c - 0x20]) * pConfig->FontScale;
            
            pWord->Pixels += mc.Pixels;
            pWord->Characters.push_back(mc);
            _printableCharacters++;
        }
    }

    if (_printableCharacters > 0)
    {
        std::vector<MULTICOLOURED_FONT_VERTEX> pVB(6 * _printableCharacters);

        _lines = 0;
        float sx = rect.Left;
        float sy = 0.0f;
        float fcw = 1.0f / 224.0f;
        float y1 = TexFont.Y1();
        float y2 = TexFont.Y2();
        float fh = TexFont.Height() * pConfig->FontScale;
        float pixelsLeft = 0.0f;
        float maxw = rect.Right - rect.Left;
        int cix = 0;

        for (const auto& pW : words)
        {
            if (!pW || pW->Pixels > pixelsLeft)
            {
                sx = rect.Left;
                sy -= fh;
                _lines++;
                pixelsLeft = maxw;
                
                if (!pW) continue;
            }

            for (const auto& c : pW->Characters)
            {
                float r = c.Red();
                float g = c.Green();
                float b = c.Blue();
                float4 col = { r, g, b, 1.0f };

                sx = std::floor(sx);
                char ch = c.Char;
                if (ch >= 0x20 && ch <= 0x7f)
                {
                    ch -= 0x20;
                    float fx = pWidths[static_cast<int>(ch)];
                    float x1 = fcw * static_cast<float>(ch);
                    float x2 = x1 + fx / 3584.0f;
                    float scaledFx = fx * pConfig->FontScale;

                    pVB[cix].position = { sx, sy, -1.5f };
                    pVB[cix].texture  = { x1, y1 };
                    pVB[cix++].colour = col;

                    pVB[cix].position = { sx + scaledFx, sy, -1.5f };
                    pVB[cix].texture  = { x2, y1 };
                    pVB[cix++].colour = col;

                    pVB[cix].position = { sx + scaledFx, sy - fh, -1.5f };
                    pVB[cix].texture  = { x2, y2 };
                    pVB[cix++].colour = col;

                    pVB[cix].position = { sx, sy, -1.5f };
                    pVB[cix].texture  = { x1, y1 };
                    pVB[cix++].colour = col;

                    pVB[cix].position = { sx + scaledFx, sy - fh, -1.5f };
                    pVB[cix].texture  = { x2, y2 };
                    pVB[cix++].colour = col;

                    pVB[cix].position = { sx, sy - fh, -1.5f };
                    pVB[cix].texture  = { x1, y2 };
                    pVB[cix++].colour = col;

                    sx += scaledFx;
                }
            }

            pixelsLeft -= (pW->Pixels + pWidths[0] * pConfig->FontScale);
            sx += pWidths[0] * pConfig->FontScale;

        }

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.ByteWidth = sizeof(MULTICOLOURED_FONT_VERTEX) * 6 * _printableCharacters;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA vData = {};
        vData.pSysMem = pVB.data();
        vData.SysMemPitch = 0;
        vData.SysMemSlicePitch = 0;

        dx.CreateBuffer(&vbDesc, &vData, &_vertexBuffer, "MultiColouredText");
    }
}
