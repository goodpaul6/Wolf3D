#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <gl3w.h>

#include "resources.hpp"

#include "draw.hpp"

static const int CIRCLE_POINTS = 30;

static float ViewWidth = 640.0f;
static float ViewHeight = 480.0f;

static struct 
{
    float r = 1.0f, g = 1.0f, b = 1.0f;
} DrawColor;

static struct
{
    Shader shader;

    GLuint viewSizeLoc = 0;
    GLuint colorLoc = 0;

    GLuint vertexArray = 0;
    GLuint vbo = 0;
} Shape;

static struct
{
    Shader shader;

    GLuint viewSizeLoc = 0;
    GLuint texLoc = 0;

    GLuint vertexArray = 0;
    GLuint vbo = 0;
} Sprite;

static struct
{
    Shader shader;

    GLuint viewSizeLoc = 0;
    GLuint texLoc = 0;
    GLuint tintLoc = 0;

    GLuint vertexArray = 0;
    GLuint vbo = 0;
} Text;

static void SetupShapeShader()
{
    glUseProgram(Shape.shader.id);

    glUniform2f(Shape.viewSizeLoc, ViewWidth, ViewHeight);
    glUniform3f(Shape.colorLoc, DrawColor.r, DrawColor.g, DrawColor.b);
}

static void SetupSpriteShader()
{
    glUseProgram(Sprite.shader.id);

    glUniform2f(Sprite.viewSizeLoc, ViewWidth, ViewHeight);
    glUniform1i(Sprite.texLoc, 0);
}

static void SetupTextShader()
{
    glUseProgram(Text.shader.id);

    glUniform2f(Text.viewSizeLoc, ViewWidth, ViewHeight);
    glUniform1i(Text.texLoc, 0);
    glUniform3f(Text.tintLoc, DrawColor.r, DrawColor.g, DrawColor.b);
}

void InitDraw(float viewWidth, float viewHeight)
{
    SetViewSize(viewWidth, viewHeight);

    Shape.shader = LoadShader("shaders/shape.vert", "shaders/shape.frag");
    
    Shape.viewSizeLoc = glGetUniformLocation(Shape.shader.id, "viewSize");
    Shape.colorLoc = glGetUniformLocation(Shape.shader.id, "color");

    glGenVertexArrays(1, &Shape.vertexArray);
    glGenBuffers(1, &Shape.vbo);

    glBindVertexArray(Shape.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Shape.vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

    Sprite.shader = LoadShader("shaders/sprite.vert", "shaders/sprite.frag");

    Sprite.viewSizeLoc = glGetUniformLocation(Sprite.shader.id, "viewSize");
    Sprite.texLoc = glGetUniformLocation(Sprite.shader.id, "tex");

    glGenVertexArrays(1, &Sprite.vertexArray);
    glGenBuffers(1, &Sprite.vbo);

    glBindVertexArray(Sprite.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Sprite.vbo); 

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

    Text.shader = LoadShader("shaders/text.vert", "shaders/text.frag");

    Text.viewSizeLoc = glGetUniformLocation(Text.shader.id, "viewSize");
    Text.texLoc = glGetUniformLocation(Text.shader.id, "tex");
    Text.tintLoc = glGetUniformLocation(Text.shader.id, "tint");

    glGenVertexArrays(1, &Text.vertexArray);
    glGenBuffers(1, &Text.vbo);

    glBindVertexArray(Text.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, Text.vbo); 

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
}

void SetViewSize(float width, float height)
{
    ViewWidth = width;
    ViewHeight = height;
}

void SetDrawColor(float r, float g, float b)
{
    DrawColor.r = r;
    DrawColor.g = g;
    DrawColor.b = b;
}

void SetDrawColorNorm(float r, float g, float b, float scale)
{
    DrawColor.r = r / scale;
    DrawColor.g = g / scale;
    DrawColor.b = b / scale;
}

void DrawRect(float x, float y, float w, float h)
{
    const float data[] =
    {
        x, y,
        x + w, y,
        x + w, y + h,
        x, y + h,
        x, y
    };

    SetupShapeShader();

    glBindBuffer(GL_ARRAY_BUFFER, Shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

    glBindVertexArray(Shape.vertexArray);

    glDrawArrays(GL_LINE_STRIP, 0, sizeof(data) / (sizeof(float) * 2));
}

void FillRect(float x, float y, float w, float h)
{
    const float data[] =
    {
        x, y,
        x + w, y,
        x + w, y + h,

        x + w, y + h,
        x, y + h,
        x, y
    };

    SetupShapeShader();

    glBindBuffer(GL_ARRAY_BUFFER, Shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

    glBindVertexArray(Shape.vertexArray);

    glDrawArrays(GL_TRIANGLES, 0, sizeof(data) / (sizeof(float) * 2));
}

void FillCircle(float x, float y, float radius)
{
    // For each point plus centre
    // plus one extra point at the end to close
    // the triangle fan
    float data[(CIRCLE_POINTS + 1) * 2 + 2] = { x, y };

    const float radsPerPoint = (float)(M_PI * 2) / CIRCLE_POINTS;
    
    for(int i = 0; i <= CIRCLE_POINTS; ++i)
    {
        float xx = cosf(i * radsPerPoint) * radius;
        float yy = sinf(i * radsPerPoint) * radius;

        data[2 + i * 2] = x + xx;
        data[2 + i * 2 + 1] = y + yy;
    }

    SetupShapeShader();

    glBindBuffer(GL_ARRAY_BUFFER, Shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

    glBindVertexArray(Shape.vertexArray);

    glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(data) / (sizeof(float) * 2));
}

void DrawQuad(const Texture& texture,
              float x, float y, float w, float h,
              float u1, float v1, float u2, float v2)
{
    const float data[] =
    {
        x, y,                  u1, v1,
        x + w, y,              u2, v1,
        x + w, y + h,          u2, v2,

        x + w, y + h,          u2, v2,
        x, y + h,              u1, v2,
        x, y,                  u1, v1
    };
    
    SetupSpriteShader();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.id);

    glBindBuffer(GL_ARRAY_BUFFER, Sprite.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_DYNAMIC_DRAW);

    glBindVertexArray(Sprite.vertexArray);

    glDrawArrays(GL_TRIANGLES, 0, sizeof(data) / (sizeof(float) * 4));
}

void DrawFrame(const Texture& texture,
               float x, float y, float w, float h,
               int fw, int fh, int frame)
{
    int columns = texture.width / fw;

    int u = frame % columns;
    int v = frame / columns;

    float normFw = fw / (float)texture.width;
    float normFh = fh / (float)texture.height;

    DrawQuad(texture, x, y, w, h, u * normFw, v * normFh,
            (u + 1) * normFw, (v + 1) * normFh);
}

void FillText(const Font& font, float x, float y, const char* text)
{
	float startX = x;
	size_t len = strlen(text);
	
	int ascent, descent, lg;
	stbtt_GetFontVMetrics(&font.info, &ascent, &descent, &lg);
	
	float scale = stbtt_ScaleForPixelHeight(&font.info, font.height);

    auto dataSize = sizeof(float) * 4 * 6 * len;
    auto data = (float*)malloc(dataSize);

	for (size_t i = 0; i < len; ++i)
	{
		if (text[i] == '\n')
		{
			x = startX;
			y += (ascent - descent + lg) * scale;
			continue;
		}

		float y0 = font.glyphs[text[i]].y0;
		float actualY = y + ascent * scale;

		stbtt_aligned_quad quad;
		stbtt_GetBakedQuad(font.glyphs, FONT_BITMAP_WIDTH, FONT_BITMAP_HEIGHT, text[i], &x, &actualY, &quad, 1);

		const float quadData[] = {
			quad.x0, quad.y0, quad.s0, quad.t0,
			quad.x1, quad.y0, quad.s1, quad.t0,
			quad.x1, quad.y1, quad.s1, quad.t1,

			quad.x1, quad.y1, quad.s1, quad.t1,
			quad.x0, quad.y1, quad.s0, quad.t1,
			quad.x0, quad.y0, quad.s0, quad.t0
		};

        memcpy(&data[4 * 6 * i], quadData, sizeof(float) * 4 * 6);
    }

    SetupTextShader();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.texture.id);

    glBindBuffer(GL_ARRAY_BUFFER, Text.vbo);
    glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_DYNAMIC_DRAW);

    glBindVertexArray(Text.vertexArray);

    glDrawArrays(GL_TRIANGLES, 0, dataSize / (sizeof(float) * 4));	
}

void DestroyDraw()
{
	glDeleteVertexArrays(1, &Shape.vertexArray);
	glDeleteBuffers(1, &Shape.vbo);

	glDeleteVertexArrays(1, &Sprite.vertexArray);
	glDeleteBuffers(1, &Sprite.vbo);

    glDeleteVertexArrays(1, &Text.vertexArray);
    glDeleteBuffers(1, &Text.vbo);
}
