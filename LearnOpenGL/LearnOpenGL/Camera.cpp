#include "Camera.h"

Camera::Camera(glm::vec3 pos, glm::vec3 direction, glm::vec3 worldUp, float cameraSpeed, float windowWidth, float windowHeight) 
	:m_CameraPos(pos), m_CameraFront(direction), m_CameraUp(worldUp), m_CameraSpeed(cameraSpeed)
{
	m_CameraRight = glm::normalize(glm::cross(m_CameraUp, m_CameraFront));

	m_FirstMouse = true;
	m_Yaw = -90.0f;
	m_Pitch = 0.0f;
	m_LastX = windowWidth / 2.0f;
	m_LastY = windowHeight / 2.0f;
}

Camera::~Camera() {
	//holder
}

glm::mat4 Camera::UpdateCamera() {
	return glm::lookAt(m_CameraPos, m_CameraPos + m_CameraFront, m_CameraUp);
}

void Camera::Translate(Direction dir, float deltaTime) {
	float moveSpeed = m_CameraSpeed * deltaTime;

	switch (dir) {
	case Direction::FOWARD:
		m_CameraPos += moveSpeed * m_CameraFront;
		break;

	case Direction::BACKWARD:
		m_CameraPos -= moveSpeed * m_CameraFront;
		break;

	case Direction::RIGHT:
		m_CameraPos -= glm::normalize(glm::cross(m_CameraUp, m_CameraFront)) * moveSpeed;
		break;

	case Direction::LEFT:
		m_CameraPos += glm::normalize(glm::cross(m_CameraUp, m_CameraFront)) * moveSpeed;
		break;
	}
}

void Camera::MouseMovement(double xPos, double yPos) {
	if (m_FirstMouse) {
		m_FirstMouse = false;
		m_LastX = xPos;
		m_LastY = yPos;
	}

	float xOffset = xPos - m_LastX;
	float yOffset = m_LastY - yPos;
	m_LastX = xPos;
	m_LastY = yPos;

	xPos *= 0.1f;
	yPos *= 0.1f;

	m_Yaw += xOffset;
	m_Pitch += yOffset;

	if (m_Pitch > 89.0f) {
		m_Pitch = 89.0f;
	}else if(m_Pitch < -89.0f) {
		m_Pitch = -89.0f;
	}

	glm::vec3 direction;
	direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	direction.y = sin(glm::radians(m_Pitch));
	direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

	m_CameraFront = glm::normalize(direction);
}

void Camera::SetSpeed(float newSpeed) {
	m_CameraSpeed = newSpeed;
}
