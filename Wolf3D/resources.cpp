#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "resources.hpp"
#include "utils.hpp"

static GLuint CreateTexture(GLuint format, const unsigned char* data, int w, int h)
{
	GLuint texture = 0;

	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	return texture;
}

Texture LoadTexture(const char* filename)
{
	int width, height, n;
	
	unsigned char* data = stbi_load(filename, &width, &height, &n, 4);
	if (!data)
	    CRASH("Failed to load texture '%s'\n", filename);

	GLuint id = CreateTexture(GL_RGBA, data, width, height);

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

Level LoadLevel(const char* filename)
{
    FILE* file = fopen(filename, "r");

    if(!file)
        CRASH("Failed to open level file '%s'\n", filename);

    int w, h;    
    fscanf(file, "%d %d", &w, &h);

	int* tiles = (int*)malloc(sizeof(int) * w * h);

    for(int y = 0; y < h; ++y)
    {
        for(int x = 0; x < w; ++x)
        {
            int tile;
            fscanf(file, "%d", &tile);

            tiles[x + y * w] = tile;
        }
    }

    Level level;

    level.tiles = tiles;
    level.width = w;
    level.height = h;

    return level;
}

void DestroyLevel(Level& level)
{
    free(level.tiles);
}
