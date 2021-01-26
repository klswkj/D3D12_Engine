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
	TextContext(custom::GraphicsContext& CmdContext, float CanvasWidth = 1920.0f, float CanvasHeight = 1080.0f);

	custom::GraphicsContext& GetCommandContext() const { return m_Context; }

	// Put settings back to the defaults.
	void ResetSettings();

	// Control various text properties

	// Choose a font from the Fonts folder.  Previously loaded fonts are cached in memory.
	void SetFont(const std::wstring& fontName, float TextSize = 0.0f);

	void SetViewSize(float ViewWidth, float ViewHeight);

	void SetTextSize(float PixelHeight);

	void ResetCursor(float x, float y);
	void SetLeftMargin(float x);
	void SetCursorX(float x);
	void SetCursorY(float y);
	void NewLine();
	float GetLeftMargin();
	float GetCursorX();
	float GetCursorY();

	// Turn on or off drop shadow.
	void EnableDropShadow(bool enable);

	// Adjust shadow parameters.
	void SetShadowOffset(float xPercent, float yPercent);
	void SetShadowParams(float Opacity, float Width);

	// Set the color and transparency of text.
	void SetColor(custom::Color _Color);

	// Get the amount to advance the Y position to begin a new line
	float GetVerticalSpacing();

	void Begin(bool EnableHDR = false);
	void End();

	// Draw a string
	void DrawString(const std::wstring& str);
	void DrawString(const std::string& str);

	// A more powerful function which formats text like printf().  Very slow by comparison, so use it
	// only if you're going to format text anyway.
	void DrawFormattedString(const wchar_t* format, ...);
	void DrawFormattedString(const char* format, ...);

private:

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

	void SetRenderState(void);

	// 16 Byte structure to represent an entire glyph in the text vertex buffer
	__declspec(align(16)) struct TextVert
	{
		float X, Y;                // Upper-left glyph position in screen space
		uint16_t U, V, W, H;    // Upper-left glyph UV and the width in texture space
	};

	UINT FillVertexBuffer(TextVert volatile* verts, const char* str, size_t stride, size_t slen);
	void DrawStringInternal(const std::string& str);
	void DrawStringInternal(const std::wstring& str);

	custom::GraphicsContext& m_Context;
	const TextManager::Font* m_CurrentFont;
	VertexShaderParams m_VSParams;
	PixelShaderParams m_PSParams;
	bool m_VSConstantBufferIsStale;    // Tracks when the CB needs updating
	bool m_PSConstantBufferIsStale;    // Tracks when the CB needs updating
	bool m_TextureIsStale;
	bool m_EnableShadow;
	float m_LeftMargin;
	float m_TextPosX;
	float m_TextPosY;
	float m_LineHeight;
	float m_ViewWidth;                // Width of the drawable area
	float m_ViewHeight;                // Height of the drawable area
	float m_ShadowOffsetX;            // Percentage of the font's TextSize should the shadow be offset
	float m_ShadowOffsetY;            // Percentage of the font's TextSize should the shadow be offset
	BOOL m_HDR;

	std::mutex m_mutex;
};