#include "Shader.h"

Shader::Shader(std::string vertexPath, std::string fragmentPath) {
	std::string vertexSrc = ReadShader(vertexPath);
	std::string fragmentSrc = ReadShader(fragmentPath);

	unsigned int vertexShader = CreateShader(GL_VERTEX_SHADER, vertexSrc.c_str());
	unsigned int fragmentShader = CreateShader(GL_FRAGMENT_SHADER, fragmentSrc.c_str());

	m_ProgramID = glCreateProgram();

	glAttachShader(m_ProgramID, vertexShader);
	glAttachShader(m_ProgramID, fragmentShader);
	glLinkProgram(m_ProgramID);

	int success;
	char infoLog[512];
	glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(m_ProgramID, 512, nullptr, infoLog);
		std::cout << "Error: Could not link the program, message \n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

Shader::Shader(std::string vertexPath, std::string geometryPath, std::string fragmentPath) {
	std::string vertexSrc = ReadShader(vertexPath);
	std::string geometrySrc = ReadShader(geometryPath);
	std::string fragmentSrc = ReadShader(fragmentPath);

	unsigned int vertexShader = CreateShader(GL_VERTEX_SHADER, vertexSrc.c_str());
	unsigned int geometryShader = CreateShader(GL_GEOMETRY_SHADER, geometrySrc.c_str());
	unsigned int fragmentShader = CreateShader(GL_FRAGMENT_SHADER, fragmentSrc.c_str());

	m_ProgramID = glCreateProgram();

	glAttachShader(m_ProgramID, vertexShader);
	glAttachShader(m_ProgramID, geometryShader);
	glAttachShader(m_ProgramID, fragmentShader);
	glLinkProgram(m_ProgramID);

	int success;
	char infoLog[512];
	glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(m_ProgramID, 512, nullptr, infoLog);
		std::cout << "Error: Could not link the program, message \n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);
}

Shader::~Shader() {
	glDeleteProgram(m_ProgramID);
}

void Shader::Bind() {
	glUseProgram(m_ProgramID);
}

void Shader::UnBind() {
	glUseProgram(0);
}

void Shader::SetInt(std::string name, int value) {
	glUniform1i(GetUniLocation(name), value);
}

void Shader::SetFloat(std::string name, float value) {
	glUniform1f(GetUniLocation(name), value);
}

void Shader::SetVec2(std::string name, glm::vec2 vec2) {
	glUniform2fv(GetUniLocation(name), 1, &vec2[0]);
}

void Shader::SetVec3(std::string name, glm::vec3 vec3) {
	glUniform3fv(GetUniLocation(name), 1, &vec3[0]);
}

void Shader::SetVec4(std::string name, glm::vec4 vec4) {
	glUniform4fv(GetUniLocation(name), 1, &vec4[0]);
}

void Shader::SetMat4(std::string name, glm::mat4 mat4) {
	glUniformMatrix4fv(GetUniLocation(name), 1, GL_FALSE, glm::value_ptr(mat4));
}

std::string Shader::ReadShader(std::string filePath) {
	std::ifstream shaderFile(filePath);
	std::stringstream ss;

	if (!shaderFile.is_open()) {
		std::cout << "Error: Could not open the shader file: " << filePath << std::endl;
		shaderFile.close();
		return "";
	}

	ss << shaderFile.rdbuf();
	shaderFile.close();

	return ss.str();
}

unsigned int Shader::CreateShader(unsigned int type, const char * src) {
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		std::string shaderType;
		if (type == GL_VERTEX_SHADER) {
			shaderType = "Vertex Shader";
		}else if(type == GL_FRAGMENT_SHADER){
			shaderType = "Fragment Shader";
		}
		else {
			shaderType = "Unknown Shader";
		}

		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cout << "Error: problem in the " << shaderType << " Error Information: \n" << infoLog << std::endl;
	}

	return shader;
}

unsigned int Shader::GetUniLocation(std::string uniformName) {
	if (m_UniformCache.find(uniformName) != m_UniformCache.end()) { //Uniform was found
		return m_UniformCache[uniformName];
	}

	int location = glGetUniformLocation(m_ProgramID, uniformName.c_str());

	if (location == -1) {
		std::cout << "Error: Could not find the uniform: " << uniformName << std::endl;
		return -1;
	}

	m_UniformCache[uniformName] = location;
	return location;
}
