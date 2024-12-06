#include <atlbase.h>
#include <math.h>
#include <gdiplus.h>

#define TEXTURE_ERROR(msg)                         \
{                                                  \
	MessageBox(NULL, msg, "Error", MB_OK);         \
	return -1;                                     \
}                                                  

class TextureLoader
{
	ULONG_PTR gdiplusToken;

	bool ispowerof2(int num)
	{
		return !(num & (num - 1)) && num;
	}

public:

	TextureLoader(void)
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	}

	~TextureLoader(void)
	{
		Gdiplus::GdiplusShutdown(gdiplusToken);
	}


	int gettexture(const char *filename)
	{
		USES_CONVERSION;
		Gdiplus::Bitmap bm(T2W(filename));
		GLuint mytexture;

		const int width = bm.GetWidth();
		const int height = bm.GetHeight();    

		if (width <= 0) TEXTURE_ERROR("width <= 0: image probably couldn't be loaded");

		if (!ispowerof2(height)) TEXTURE_ERROR("Height must be a power of 2");          
		if (!ispowerof2(width)) TEXTURE_ERROR("Width must be a power of 2");

		int MaxTextureSize = 0; 
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);

		if (height > MaxTextureSize) TEXTURE_ERROR("Height is greater than maximum texture size");    
		if (width > MaxTextureSize) TEXTURE_ERROR("Width is greater than maximum texture size");

		unsigned char *imagedata = new unsigned char[width*height*3];

		Gdiplus::Color color;

		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
			{
				bm.GetPixel(x, y, &color);
				int red = color.GetR();
				int green = color.GetG();
				int blue = color.GetB();
				imagedata[y*width*3+x*3] = red;
				imagedata[y*width*3+x*3+1] = green;
				imagedata[y*width*3+x*3+2] = blue;
			}

			// Create The Texture
			glGenTextures(1, &mytexture);					
			glBindTexture(GL_TEXTURE_2D, mytexture);
			//glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imagedata);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_RGB, GL_UNSIGNED_BYTE, imagedata);

glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			delete[] imagedata;
			return mytexture;
	}




};