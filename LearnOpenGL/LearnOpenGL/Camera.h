#pragma once
#include <iostream>
#include "Vendor/glm/glm.hpp"
#include "Vendor/glm/gtc/matrix_transform.hpp"
#include "Vendor/glm/gtc/type_ptr.hpp"
#include "Vendor/glm/gtx/string_cast.hpp"

enum class Direction {
	FOWARD, BACKWARD, RIGHT, LEFT
};

class Camera
{
public:
	Camera(glm::vec3 pos, glm::vec3 direction, glm::vec3 worldUp, float cameraSpeed, float windowWidth, float windowHeight);
	~Camera();

	glm::mat4 UpdateCamera();

	void Translate(Direction dir, float deltaTime);
	void MouseMovement(double xPos, double yPos);
	void SetSpeed(float newSpeed);

	inline glm::vec3 getCameraPos() const { return m_CameraPos; }

private:
	glm::vec3 m_CameraPos;
	glm::vec3 m_CameraFront;
	glm::vec3 m_CameraUp;
	glm::vec3 m_CameraRight;

	float m_Pitch, m_Yaw;
	float m_LastX, m_LastY;

	float m_CameraSpeed;

	bool m_FirstMouse;
};