#pragma once
#include <glad/glad.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "Vendor/glm/glm.hpp"
#include "Vendor/glm/gtc/matrix_transform.hpp"
#include "Vendor/glm/gtc/type_ptr.hpp"

class Shader {
public:
	Shader(std::string vertexPath, std::string fragmentPath);
	Shader(std::string vertexPath, std::string geometryPath, std::string fragmentPath);
	~Shader();

	void Bind();
	void UnBind();

	void SetInt(std::string name, int value);
	void SetFloat(std::string name, float value);
	void SetVec2(std::string name, glm::vec2 vec2);
	void SetVec3(std::string name, glm::vec3 vec3);
	void SetVec4(std::string name, glm::vec4 vec4);
	void SetMat4(std::string name, glm::mat4 mat4);

	inline unsigned int getProgram() const { return m_ProgramID; }

private:
	std::string ReadShader(std::string filePath);
	unsigned int CreateShader(unsigned int type, const char * src);

	unsigned int GetUniLocation(std::string uniformName);

	unsigned int m_ProgramID;

	std::unordered_map<std::string, unsigned int> m_UniformCache;
};