#include <util.hpp>

// Put any random small function/class implementations here

void setGuiTextStyle(Font f, int color, int h_align, int v_align, int size, int spacing)
{
    GuiSetFont(f);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, color);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, h_align);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, v_align);
    GuiSetStyle(DEFAULT, TEXT_SIZE, size);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, spacing);
}