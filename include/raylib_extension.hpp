#pragma once

#include "main.hpp"

struct TextProps
{
    Font font;
    Color color;
    int h_align;
    int v_align;
    int size;
    int line_spacing;
};

void SetGuiTextProps(TextProps props);

void DrawGuiLabelShadow(Rectangle rect, std::string str, Vector2 offset, Color shadow_color);

struct ShadowedTextureProps
{
    Texture2D tex;
    Rectangle src;
    Rectangle dest;
    Vector2 origin;
    float rot;
    Color tint;
    Vector2 shadow_offset;
    Color shadow_color;
};

void DrawShadowedTexture(ShadowedTextureProps props);