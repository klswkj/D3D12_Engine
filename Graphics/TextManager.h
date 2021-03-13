#pragma once

#include "Color.h"
#include "Vector.h"

namespace custom
{
	class GraphicsContext;
}

namespace TextManager
{
	void Initialize();
	void Shutdown();

	class Font;
}

class TextContext
{
public:
	TextContext(custom::GraphicsContext* const pGraphicsContext, const uint8_t commandIndex, const float canvasWidth = 1920.0f, const float canvasHeight = 1080.0f);

	// Put settings back to the defaults.
	void ResetSettings();

	// Control various text properties

	// Choose a font from the Fonts folder.  Previously loaded fonts are cached in memory.
	void SetFont(const std::wstring& fontName, const float textSize = 0.0f);

	void SetViewSize(const float viewWidth, const float viewHeight);

	void SetTextSize(const float pixelHeight);

	inline void ResetCursor(const float x, const float y) 
	{
		m_LeftMargin = x;
		m_TextPosX = x;
		m_TextPosY = y;
	}

	void SetLeftMargin(const float x) { m_LeftMargin = x; }
	void SetCursorX(const float x) { m_TextPosX = x; }
	void SetCursorY(const float y) { m_TextPosY = y; }
	void NewLine() { m_TextPosX = m_LeftMargin; m_TextPosY += m_LineHeight; }
	float GetLeftMargin() const { return m_LeftMargin; }
	float GetCursorX() const { return m_TextPosX; }
	float GetCursorY() const { return m_TextPosY; }

	custom::GraphicsContext* GetGraphicsContext() const { return m_pGraphicsContext; }
	uint8_t GetCommandIndex() const { return m_CommandIndex; }

	// Turn on or off drop shadow.
	void EnableDropShadow(const bool enable);

	// Adjust shadow parameters.
	void SetShadowOffset(const float xPercent, const float yPercent);
	void SetShadowParams(const float Opacity, const float Width);

	// Set the color and transparency of text.
	void SetColor(const custom::Color& _Color);

	// Get the amount to advance the Y position to begin a new line
	float GetVerticalSpacing();

	void Begin(const bool EnableHDR = false);
	void End();

	// Draw a string
	void DrawString(const std::wstring& str);
	void DrawString(const std::string& str);

	// A more powerful function which formats text like printf().  Very slow by comparison, so use it
	// only if you're going to format text anyway.
	void DrawFormattedString(const wchar_t* format, ...);
	void DrawFormattedString(const char* format, ...);

private:
	struct TextVert;

	UINT FillVertexBuffer(TextVert volatile* verts, const char* str, const size_t stride, const size_t sLength);

	__declspec(align(16)) struct VertexShaderParams
	{
		Math::Vector4 ViewportTransform;
		float NormalizeX, NormalizeY, TextSize;
		float Scale, DstBorder;
		uint32_t SrcBorder;
	};

	__declspec(align(16)) struct PixelShaderParams
	{
		custom::Color TextColor;
		float ShadowOffsetX, ShadowOffsetY;
		float ShadowHardness;        // More than 1 will cause aliasing
		float ShadowOpacity;        // Should make less opaque when making softer
		float HeightRange;
	};

	void SetRenderState();

	// 16 Byte structure to represent an entire glyph in the text vertex buffer
	__declspec(align(16)) struct TextVert
	{
		float X, Y;                // Upper-left glyph position in screen space
		uint16_t U, V, W, H;    // Upper-left glyph UV and the width in texture space
	};

	custom::GraphicsContext* const m_pGraphicsContext;
	uint8_t m_CommandIndex;

	const TextManager::Font* m_CurrentFont;
	VertexShaderParams m_VSParams;
	PixelShaderParams m_PSParams;
	bool m_VSConstantBufferIsStale; // Tracks when the CB needs updating
	bool m_PSConstantBufferIsStale; // Tracks when the CB needs updating
	bool m_TextureIsStale;
	bool m_EnableShadow;
	float m_LeftMargin;
	float m_TextPosX;
	float m_TextPosY;
	float m_LineHeight;
	float m_ViewWidth;  // Width of the drawable area
	float m_ViewHeight; // Height of the drawable area
	float m_ShadowOffsetX; // Percentage of the font's TextSize should the shadow be offset
	float m_ShadowOffsetY; // Percentage of the font's TextSize should the shadow be offset
	BOOL m_HDR;

	CRITICAL_SECTION m_CS;
};