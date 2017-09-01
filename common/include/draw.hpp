#pragma once

// Immediate mode rendering utilities.
// All the Draw procedures update a vertex buffer and 
// use a predefined shader.

struct Texture;
struct Font;

void InitDraw(float viewWidth, float viewHeight);

void SetViewSize(float width, float height);

void SetDrawColor(float r, float g, float b);
// Normalize the color (r / scale, g / scale, b / scale)
void SetDrawColorNorm(float r, float g, float b, float scale = 255.0f);

void DrawRect(float x, float y, float w, float h);
void FillRect(float x, float y, float w, float h);
void FillCircle(float x, float y, float radius);

void DrawQuad(const Texture& texture, float x, float y, float w, float h, 
        float u1, float v1, float u2, float v2);

void DrawFrame(const Texture& texture, float x, float y, float w, float h,
        int fw, int fh, int frame);

void FillText(const Font& font, float x, float y, const char* text);

void DestroyDraw();


