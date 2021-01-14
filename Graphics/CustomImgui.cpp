#include "stdafx.h"
#include "CustomImgui.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
    void CenteredSeparator(float width/* = 0*/)
    {
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems)
        {
            return;
        }

        ImGuiContext& g = *GImGui;

        // Horizontal Separator
        float x1, x2;
        if (window->DC.CurrentColumns == NULL && (width == 0))
        {
            ///x1 = window->Pos.x; // This fails with SameLine(); CenteredSeparator();
            // Nah, we have to detect if we have a sameline in a different way
            x1 = window->DC.CursorPos.x;
            x2 = x1 + window->Size.x;
        }
        else
        {
            // Start at the cursor
            x1 = window->DC.CursorPos.x;
            if (width != 0)
            {
                x2 = x1 + width;
            }
            else
            {
                x2 = window->ClipRect.Max.x;
                // Pad right side of columns (except the last one)
				if (window->DC.CurrentColumns && (window->DC.CurrentColumns->Current < window->DC.CurrentColumns->Count - 1))
				{
					x2 -= g.Style.ItemSpacing.x;
				}
            }
        }

        float y1 = window->DC.CursorPos.y + int(window->DC.CurrLineSize.y / 2.0f);
        float y2 = y1 + 1.0f;

        window->DC.CursorPos.x += width;

		if (!window->DC.GroupStack.empty())
		{
			x1 += window->DC.Indent.x;
		}

        const ImRect bb(ImVec2(x1, y1), ImVec2(x2, y2));
        ItemSize(ImVec2(0.0f, 0.0f));
        
        if (!ItemAdd(bb, NULL))
        {
            return;
        }

        window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Border));
    }

    void SameLineSeparator(float width/* = 0*/) 
    {
        ImGui::SameLine();
        CenteredSeparator(width);
    }

    // Create a centered separator which can be immediately followed by a item
    void PreSeparator(float width)
    {
        ImGuiWindow* window = GetCurrentWindow();
		if (window->DC.CurrLineSize.y == 0)
		{
			window->DC.CurrLineSize.y = ImGui::GetTextLineHeight();
		}

        CenteredSeparator(width);
        ImGui::SameLine();
    }

    // width value is arbitrary.
    void TextSeparator(const char* text, float pre_width/* = 10.0f*/)
    {
        ImGui::PreSeparator(pre_width);
        ImGui::Text(text);
        ImGui::SameLineSeparator();
    }
}