#include "raylib_extension.hpp"

// Put any random small function/class implementations here

void SetGuiTextProps(TextProps props)
{
    GuiSetFont(props.font);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(props.color));
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, props.h_align);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, props.v_align);
    GuiSetStyle(DEFAULT, TEXT_SIZE, props.size);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, props.line_spacing);
}

TextProps GetGuiTextProps()
{
    TextProps props;
    props.font = GuiGetFont();
    props.color = GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL));
    props.h_align = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT);
    props.v_align = GuiGetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL);
    props.size = GuiGetStyle(DEFAULT, TEXT_SIZE);
    props.line_spacing = GuiGetStyle(DEFAULT, TEXT_LINE_SPACING);

    return props;
}

void DrawGuiLabelShadow(Rectangle rect, std::string str, Vector2 offset, Color shadow_color)
{
    // Get current text properties so we can revert
    TextProps current_props = GetGuiTextProps();

    // Draw the text shadow
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(BLACK));
    GuiLabel(Rectangle{rect.x + offset.x, rect.y + offset.y, rect.width, rect.height}, str.c_str());

    // Draw set text on top of shadow
    SetGuiTextProps(current_props);
    GuiLabel(rect, str.c_str());
}

void DrawShadowedTexture(ShadowedTextureProps props)
{
    Rectangle shadow_dest = props.dest;
    shadow_dest.x += props.shadow_offset.x;
    shadow_dest.y += props.shadow_offset.y;

    DrawTexturePro(props.tex, props.src, shadow_dest, props.origin, props.rot, props.shadow_color);
    DrawTexturePro(props.tex, props.src, props.dest, props.origin, props.rot, props.tint);
}