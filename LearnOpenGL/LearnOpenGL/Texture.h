#pragma once
#include <glad/glad.h>
#include <iostream>
#include <string>
#include <vector>
#include "stb_image.h"

class Texture {
public:
	Texture(std::string texturePath);
	Texture(std::vector<std::string> faces); //For cube maps
	//For framebuffer textures
	Texture(unsigned int internalFormat, unsigned int format, unsigned int winWidth, unsigned int winHeight, unsigned int type, unsigned int textureFilter, bool wrap, unsigned int textureWrap, void * data = nullptr);
	~Texture();

	void Bind(int textureUnit = 0);
	void UnBind();

	void BindCubemap(int textureUnit = 0);
	void UnbindCubemap();

	inline int getWidth() const { return m_Width; }
	inline int getHeight() const { return m_Height; }
	inline int getNRChannels() const { return m_NRChannels; }
	unsigned int getProgram() const { return m_TextureID; }

private:
	unsigned int m_TextureID;

	int m_Width, m_Height, m_NRChannels;
};