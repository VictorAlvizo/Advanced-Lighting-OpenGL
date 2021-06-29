#include "Texture.h"

Texture::Texture(std::string texturePath) {
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char * data = stbi_load(texturePath.c_str(), &m_Width, &m_Height, &m_NRChannels, 0);

	if (data) {
		GLenum format;
		GLenum internalFormat;

		if (m_NRChannels == 1) {
			format = GL_RED;
			internalFormat = GL_RED;
		}
		else if (m_NRChannels == 3) {
			format = GL_RGB;
			internalFormat = GL_SRGB;
		}
		else if (m_NRChannels == 4) {
			format = GL_RGBA;
			internalFormat = GL_SRGB_ALPHA;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cout << "Error: Failed to load the texture at " << texturePath << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
}

Texture::Texture(std::vector<std::string> faces) {
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

	for (unsigned int i = 0; i < faces.size(); i++) {
		unsigned char * data = stbi_load(faces[i].c_str(), &m_Width, &m_Height, &m_NRChannels, 0);

		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else {
			std::cout << "Error: Failed to load cubemap texture at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Texture::Texture(unsigned int internalFormat, unsigned int format, unsigned int winWidth, unsigned int winHeight, unsigned int type, unsigned int textureFilter, bool wrap, unsigned int textureWrap, void * data) {
	glGenTextures(1, &m_TextureID);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, winWidth, winHeight, 0, format, type, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureFilter);

	if (wrap) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrap);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
	glDeleteTextures(1, &m_TextureID);
}

void Texture::Bind(int textureUnit) {
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

void Texture::UnBind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::BindCubemap(int textureUnit) {
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);
}

void Texture::UnbindCubemap() {
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}