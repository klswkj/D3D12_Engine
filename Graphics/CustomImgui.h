#pragma once

namespace ImGui
{
    void CenteredSeparator(float width = 0);

    // Create a centered separator right after the current item.
    // Eg.: 
    // ImGui::PreSeparator(10);
    // ImGui::Text("Section VI");
    // ImGui::SameLineSeparator();
    void SameLineSeparator(float width = 0);

    // Create a centered separator which can be immediately followed by a item
    void PreSeparator(float width);

    // The value for width is arbitrary. But it looks nice.
    void TextSeparator(const char* text, float pre_width = 10.0f);
}