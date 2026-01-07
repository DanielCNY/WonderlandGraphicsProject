#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "utils/world_manager.h"
#include "render/shader.h"
#include "utils/texture_manager.h"
#include <iostream>
#include <iomanip>

static GLFWwindow *window;
static int windowWidth = 1920;
static int windowHeight = 1080;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void cursor_callback(GLFWwindow* window, double xpos, double ypos);

// Camera
static glm::vec3 eye_center(0.0f, 300.0f, 0.0f);
static glm::vec3 lookat(0.0f, 0.0f, 0.0f);
static glm::vec3 up(0.0f, 1.0f, 0.0f);

static float FoV = 60.0f;
static float zNear = 1.0f;
static float zFar = 20000.0f;

static glm::vec3 lightPosition(0.0f, 1000.0f, 0.0f);
static glm::vec3 lightIntensity(1.0f, 1.0f, 1.0f);
static glm::vec3 ambientLight(0.2f, 0.2f, 0.25f);

// Mouse movement
static float yaw = -90.0f;
static float pitch = 0.0f;
static bool firstMouse = true;
static float lastX = 512.0f;
static float lastY = 384.0f;
static float sensitivity = 0.1f;

struct Skybox {
	glm::vec3 position;
	glm::vec3 scale;

	GLfloat vertex_buffer_data[72] = {
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,

		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
	};

	GLfloat color_buffer_data[72] = {
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 1.0f,
	};

	GLuint index_buffer_data[36] = {
		0, 3, 2,
		0, 2, 1,

		4, 7, 6,
		4, 6, 5,

		8, 11, 10,
		8, 10, 9,

		12, 15, 14,
		12, 14, 13,

		16, 19, 18,
		16, 18, 17,

		20, 23, 22,
		20, 22, 21,
	};

	GLfloat uv_buffer_data[48] = {
		0.75f, 0.667f,
		0.5f, 0.667f,
		0.5f, 0.334f,
		0.75f, 0.334f,

		0.25f, 0.667f,
		0.0f, 0.667f,
		0.0f, 0.334f,
		0.25f, 0.334f,

		1.0f, 0.667f,
		0.75f, 0.667f,
		0.75f, 0.334f,
		1.0f, 0.334f,

		0.5f, 0.667f,
		0.25f, 0.667f,
		0.25f, 0.334f,
		0.5f, 0.334f,

		0.5f, 0.0f,
		0.5f, 0.334f,
		0.25f, 0.334f,
		0.25f, 0.0f,

		0.5f, 1.0f,
		0.25f, 1.0f,
		0.25f, 0.667f,
		0.5f, 0.667f,
	};

	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint uvBufferID;
	GLuint textureID;

	GLuint mvpMatrixID;
	GLuint textureSamplerID;
	GLuint programID;

	void initialize(glm::vec3 position, glm::vec3 scale) {
		this->position = position;
		this->scale = scale;

		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		for (int i = 0; i < 72; ++i) color_buffer_data[i] = 1.0f;
		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		programID = LoadShadersFromFile("../scene/shaders/box.vert", "../scene/shaders/box.frag");
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		mvpMatrixID = glGetUniformLocation(programID, "MVP");

		TextureManager& tm = TextureManager::getInstance();
		textureID = tm.getTexture("../scene/textures/sky.png");

		textureSamplerID = glGetUniformLocation(programID,"textureSampler");
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

        glm::mat4 modelMatrix = glm::mat4();
		modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		glDrawElements(
			GL_TRIANGLES,
			36,
			GL_UNSIGNED_INT,
			(void*)0
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(programID);
	}
};

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
    glClearColor(0.2f, 0.25f, 0.5f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

	// FPS tracking
	double lastTime = glfwGetTime();
	int frameCount = 0;

    // Camera setup
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	WorldManager worldManager;

	Skybox skybox;
	skybox.initialize(glm::vec3(0, 2000, 0), glm::vec3(7500, 7500, 7500));

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera/view matrix
        glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
        glm::mat4 vp = projectionMatrix * viewMatrix;

    	double currentTime = glfwGetTime();
    	frameCount++;

    	// Update FPS every second
    	if (currentTime - lastTime >= 1.0)
    	{
    		double fps = frameCount / (currentTime - lastTime);

    		// Create window title with FPS
    		std::stringstream ss;
    		ss << "Wonderland Project | FPS: " << std::fixed << std::setprecision(1) << fps;
    		glfwSetWindowTitle(window, ss.str().c_str());

    		// Reset counters
    		frameCount = 0;
    		lastTime = currentTime;
    	}

    	glm::mat4 skyboxView = glm::mat4(glm::mat3(viewMatrix));
    	skybox.render(projectionMatrix * skyboxView);

    	worldManager.update(eye_center);
    	worldManager.render(vp, lightPosition, lightIntensity, ambientLight, eye_center);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
	skybox.cleanup();
    glfwTerminate();
    return 0;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	float speed = 40.0f;
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