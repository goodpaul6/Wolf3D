#include <assert.h>
#include <stb_image.h>

#include "resources.hpp"
#include "graphics.hpp"
#include "utils.hpp"

static unsigned char FontDataBuffer[1 << 20];
static unsigned char TempFontImage[512 * 512];

static GLuint CreateTexture(GLuint internalFormat, GLuint format, const unsigned char* data, int w, int h)
{
	GLuint texture = 0;

	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, data);

	return texture;
}

Texture LoadTexture(const char* filename)
{
	int width, height, n;
	
	unsigned char* data = stbi_load(filename, &width, &height, &n, 4);
	if (!data)
	    CRASH("Failed to load texture '%s'\n", filename);

	GLuint id = CreateTexture(GL_RGBA, GL_RGBA, data, width, height);

	stbi_image_free(data);

    Texture texture;

    texture.id = id;
    texture.width = width;
    texture.height = height;

    return texture;
}

static GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource)
{ 
	GLuint vertexShader, fragmentShader;
	
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	GLint success = 0;

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	
	static char log[2048];

	if (success == GL_FALSE)
	{
		glGetShaderInfoLog(vertexShader, sizeof(log), NULL, log);
		CRASH("Vertex shader compilation failed: %s\n", log);
	}

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE)
	{
		glGetShaderInfoLog(fragmentShader, sizeof(log), NULL, log);
		CRASH("Fragment shader compilation failed: %s\n", log);
	}

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	
	if (success == GL_FALSE)
	{
		glGetProgramInfoLog(program, sizeof(log), NULL, log);
		CRASH("Shader program linking failed: %s\n", log);
	}

	return program;
}

static char* ReadEntireFile(const char* filename)
{
    FILE* file = fopen(filename, "rb");

    if(!file)
        CRASH("Failed to open file '%s' for reading\n", filename);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);

    rewind(file);

    char* str = (char*)malloc(size + 1);
    
    fread(str, 1, size, file);
    str[size] = '\0';

    fclose(file);

    return str;
}

Shader LoadShader(const char* vertexFilename, const char* fragmentFilename)
{
    char* vertexSource = ReadEntireFile(vertexFilename);
    char* fragmentSource = ReadEntireFile(fragmentFilename);

    Shader shader;

    shader.id = CreateShaderProgram(vertexSource, fragmentSource);

    free(vertexSource);
    free(fragmentSource);

    return shader;
}

Mesh LoadMesh(const char* filename)
{
    // x,y,z per vertex
    static float positions[MAX_LOAD_MESH_VERTICES][3];
    // u,v per vertex
    static float texCoords[MAX_LOAD_MESH_VERTICES][2];

    static ushort indices[MAX_LOAD_MESH_VERTICES];
    static Vertex vertices[MAX_LOAD_MESH_VERTICES];

    const char* ext = strrchr(filename, '.');
    
    // Only supports obj files
    assert(ext && strcmp(ext, ".obj") == 0); 

    FILE* file = fopen(filename, "rb");

    if(!file)
        CRASH("Failed to open mesh file '%s'\n", filename);

    int posCount = 0;
    int texCoordCount = 0;

    // TODO: Intelligent generation of indices
    // Right now we're basically not using index
    // buffer at all so indexCount == vertexCount
    int vertexCount = 0;

    while(true)
    {
        static char lineHeader[128];

        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break;

        if (strcmp(lineHeader, "v") == 0)
        {
            float pos[3];

            fscanf(file, "%f %f %f\n", &pos[0], &pos[1], &pos[2]);

            memcpy(positions[posCount++], pos, sizeof(pos));
        }
        else if (strcmp(lineHeader, "vt") == 0)
        {
            float texCoord[2];

            fscanf(file, "%f %f\n", &texCoord[0], &texCoord[1]);
            memcpy(texCoords[texCoordCount++], texCoord, sizeof(texCoord)); 
        }
        else if (strcmp(lineHeader, "f") == 0)
        {
            int pos[3], texCoord[3];
            
            fscanf(file, "%d/%d %d/%d %d/%d\n",
                &pos[0], &texCoord[0],
                &pos[1], &texCoord[1],
                &pos[2], &texCoord[2]);

            for (int i = 0; i < 3; ++i)
            {
                float x = positions[pos[i] - 1][0];
                float y = positions[pos[i] - 1][1];
                float z = positions[pos[i] - 1][2];
                float u = texCoords[texCoord[i] - 1][0];
                float v = texCoords[texCoord[i] - 1][1];
            
                if(vertexCount >= MAX_LOAD_MESH_VERTICES)
                    CRASH("Model exceeded maximum number of vertices\n");

                // HACK: Blender exports 0, 0 as top left of texture
                // whereas OpenGL expects 0, 0, as bottom left
                v = 1 - v;
            
                indices[vertexCount] = vertexCount;
                vertices[vertexCount] = { x, y, z, u, v };

                vertexCount += 1;
            }
        }
    }

    fclose(file);

    // TODO: Change vertexCount to indexCount once that's changed
    return CreateMesh(vertexCount, vertices, vertexCount, indices);
}

Font LoadFont(const char* filename, float height)
{
    FILE* file = fopen(filename, "rb");

    if(!file)
        CRASH("Failed to open bitmap font file '%s'\n", filename);

    fread(FontDataBuffer, 1, 1 << 20, file);
    fclose(file);

    Font font;

    stbtt_InitFont(&font.info, FontDataBuffer, stbtt_GetFontOffsetForIndex(FontDataBuffer, 0));

    font.height = height;
    
    // TODO: Attempt to do more than ascii?
    stbtt_BakeFontBitmap(FontDataBuffer, 0, height, TempFontImage, FONT_BITMAP_WIDTH, FONT_BITMAP_HEIGHT, 32, 96, font.glyphs);

    font.texture.id = CreateTexture(GL_RED, GL_RED, TempFontImage, FONT_BITMAP_WIDTH, FONT_BITMAP_HEIGHT);
	font.texture.width = FONT_BITMAP_WIDTH;
	font.texture.height = FONT_BITMAP_HEIGHT;

    return font;
}

Level LoadLevel(const char* filename)
{
    FILE* file = fopen(filename, "r");

    if(!file)
        CRASH("Failed to open level file '%s'\n", filename);

    Level level;

    fscanf(file, "%d", &level.planeCount);

    level.planes = (Level::Plane*)malloc(sizeof(Level::Plane) * level.planeCount);

    for(int i = 0; i < level.planeCount; ++i)
    {
        Level::Plane& plane = level.planes[i];
        fscanf(file, "%d %d %d %d %d %d %d %d %d %d", &plane.o[0], &plane.o[1], &plane.o[2],
                                                      &plane.a[0], &plane.a[1], &plane.a[2],
                                                      &plane.b[0], &plane.b[1], &plane.b[2],
                                                      &plane.tile);
    }

    for(int i = 0; i < ET_COUNT; ++i)
        fscanf(file, "%d", &level.entityCount[i]);

    for(int i = 0; i < ET_COUNT; ++i)
    { 
        level.entities[i] = (EntityInfo*)malloc(sizeof(EntityInfo) * level.entityCount[i]); 
        for(int j = 0; j < level.entityCount[i]; ++j)
        {
            // Read an EntityInfo
            EntityInfo& info = level.entities[i][j];

            info.type = (EntityType)i;
				
            // Always present
            fscanf(file, "%f %f %f", &info.x, &info.y, &info.z);

            switch(info.type)
            {
                case ET_DOOR:
                case ET_PAINTING:
                {
                    fscanf(file, "%d", &info.dir);
                } break;

                case ET_ENEMY:
                {
                    fscanf(file, "%d", &info.health);
                    fscanf(file, "%f", &info.speed);
                } break;

                case ET_BOXCOLLIDER:
                {
                    fscanf(file, "%f %f %f %f %f %f", &info.minx, &info.miny, &info.minz,
                                                      &info.maxx, &info.maxy, &info.maxz);
                } break;
            }
        }
    }

    fclose(file);

    return level;
}

void DestroyFont(Font& font)
{
    // TODO: Destroy Texture
}

void DestroyLevel(Level& level)
{
    free(level.planes);

    for(int i = 0; i < ET_COUNT; ++i)
        free(level.entities[i]);
}
