#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "utils/world_manager.h"

#include <iostream>

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void cursor_callback(GLFWwindow* window, double xpos, double ypos);

// Camera
static glm::vec3 eye_center(0.0f, 150.0f, 0.0f);
static glm::vec3 lookat(0.0f, 0.0f, 0.0f);
static glm::vec3 up(0.0f, 1.0f, 0.0f);

static float FoV = 60.0f;
static float zNear = 1.0f;
static float zFar = 3000.0f;

// Mouse movement
static float yaw = -90.0f;
static float pitch = 0.0f;
static bool firstMouse = true;
static float lastX = 512.0f;
static float lastY = 384.0f;
static float sensitivity = 0.1f;

WorldManager worldManager;

int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Wonderland Project", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to open a GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_callback);

    // Load OpenGL
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0)
    {
        std::cerr << "Failed to initialize OpenGL context." << std::endl;
        return -1;
    }

    // Setup
    glClearColor(0.2f, 0.1f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Camera setup
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera/view matrix
        glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
        glm::mat4 vp = projectionMatrix * viewMatrix;

    	worldManager.update(eye_center);
    	worldManager.render(vp);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	float speed = 3.0f;
	glm::vec3 lookDirection = glm::normalize(lookat - eye_center);
	glm::vec3 rightOfDirection = glm::normalize(glm::cross(lookDirection, up));

	// Move forward/backward: w/s
	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 move = lookDirection * speed;
		eye_center += move;
		lookat += move;
	}
	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 move = lookDirection * speed;
		eye_center -= move;
		lookat -= move;
	}

	// Move left/right: a/d
	if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 move = rightOfDirection * speed;
		eye_center -= move;
		lookat -= move;
	}
	if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 move = rightOfDirection * speed;
		eye_center += move;
		lookat += move;
	}

	// Move up/down: space/tab
	if (key == GLFW_KEY_SPACE && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 move = glm::normalize(up) * speed;
		eye_center += move;
		lookat += move;
	}
	if (key == GLFW_KEY_TAB && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 move = glm::normalize(up) * speed;
		eye_center -= move;
		lookat -= move;
	}

	// Close window
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    	glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	static float lastX = 0, lastY = 0;

	// Handle first input
	if (firstMouse) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		lastX = width / 2.0f;
		lastY = height / 2.0f;
		glfwSetCursorPos(window, lastX, lastY);
		firstMouse = false;
		return;
	}
	
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	// Reset cursor to center
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	float centerX = width / 2.0f;
	float centerY = height / 2.0f;
	glfwSetCursorPos(window, centerX, centerY);
	lastX = centerX;
	lastY = centerY;
	
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Constrain Pitch
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	// Update lookat
	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	lookat = eye_center + glm::normalize(front);
}