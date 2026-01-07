#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "utils/world_manager.h"
#include "render/shader.h"
#include "utils/texture_manager.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <glfw-3.1.2/deps/GL/glext.h>
#include <algorithm>

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
static glm::vec3 lightIntensity(5e6f, 5e6f, 5e6f);

// Mouse movement
static float yaw = -90.0f;
static float pitch = 0.0f;
static bool firstMouse = true;
static float lastX = 512.0f;
static float lastY = 384.0f;
static float sensitivity = 0.1f;

struct SnowParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float rotation;
    float rotationSpeed;
    float life; // For fade in/out if we want
};

class SimpleSnowSystem {
private:
    struct SimpleParticle {
        glm::vec3 position;
        float size;
    };

    std::vector<SimpleParticle> particles;
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint programID;
    GLuint mvpMatrixID;

    std::mt19937 rng;

public:
    SimpleSnowSystem() : vertexArrayID(0), vertexBufferID(0), programID(0) {
        rng.seed(std::random_device{}());
    }

    void initialize(int count = 1000) {
        particles.resize(count);

        // Initialize with random positions
        std::uniform_real_distribution<float> distX(-5000.0f, 5000.0f);
        std::uniform_real_distribution<float> distY(0.0f, 3000.0f);
        std::uniform_real_distribution<float> distZ(-5000.0f, 5000.0f);
        std::uniform_real_distribution<float> distSize(1.0f, 3.0f);

        for (auto& p : particles) {
            p.position = glm::vec3(distX(rng), distY(rng), distZ(rng));
            p.size = distSize(rng);
        }

        // Simple shader - we'll use the same as skybox for now
        programID = LoadShadersFromFile("../scene/shaders/skybox.vert", "../scene/shaders/skybox.frag");
        if (programID == 0) {
            // Create a super simple shader inline
            const char* vertexShaderSource = R"(
                #version 330 core
                layout(location = 0) in vec3 position;
                layout(location = 1) in float size;
                uniform mat4 MVP;
                uniform float pointSize;
                void main() {
                    gl_Position = MVP * vec4(position, 1.0);
                    gl_PointSize = size * pointSize;
                }
            )";

            const char* fragmentShaderSource = R"(
                #version 330 core
                out vec4 FragColor;
                void main() {
                    // Make circular points
                    vec2 coord = gl_PointCoord - vec2(0.5);
                    if(length(coord) > 0.5) discard;
                    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
                }
            )";

            // Compile shaders
            GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
            glCompileShader(vertexShader);

            GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
            glCompileShader(fragmentShader);

            programID = glCreateProgram();
            glAttachShader(programID, vertexShader);
            glAttachShader(programID, fragmentShader);
            glLinkProgram(programID);

            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
        }

        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        GLuint pointSizeID = glGetUniformLocation(programID, "pointSize");

        // Create buffers
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(SimpleParticle),
                     particles.data(), GL_DYNAMIC_DRAW);

        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleParticle), (void*)0);

        // Size attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(SimpleParticle),
                             (void*)offsetof(SimpleParticle, size));

        glBindVertexArray(0);
    }

    void update(float deltaTime) {
        std::uniform_real_distribution<float> distVelY(-30.0f, -10.0f);
        std::uniform_real_distribution<float> distVelXZ(-1.0f, 1.0f);

        for (auto& p : particles) {
            // Simple downward movement with slight drift
            p.position.y += distVelY(rng) * deltaTime;
            p.position.x += distVelXZ(rng) * deltaTime * 10.0f;
            p.position.z += distVelXZ(rng) * deltaTime * 10.0f;

            // Reset if below ground
            if (p.position.y < -100.0f) {
                std::uniform_real_distribution<float> distX(-5000.0f, 5000.0f);
                std::uniform_real_distribution<float> distY(2000.0f, 3000.0f);
                std::uniform_real_distribution<float> distZ(-5000.0f, 5000.0f);
                p.position = glm::vec3(distX(rng), distY(rng), distZ(rng));
            }
        }

        // Update buffer
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(SimpleParticle),
                       particles.data());
    }

    void render(const glm::mat4& vp) {
        glUseProgram(programID);
        glBindVertexArray(vertexArrayID);

        // Enable blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Enable point sprites and set size
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_POINT_SPRITE);

        // Set uniforms
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &vp[0][0]);
        glUniform1f(glGetUniformLocation(programID, "pointSize"),
                    5.0f * (windowHeight / 1080.0f)); // Scale with window size

        // Draw
        glDrawArrays(GL_POINTS, 0, particles.size());

        // Clean up
        glDisable(GL_POINT_SPRITE);
        glDisable(GL_PROGRAM_POINT_SIZE);
        glDisable(GL_BLEND);

        glBindVertexArray(0);
    }

    void cleanup() {
        if (vertexBufferID) glDeleteBuffers(1, &vertexBufferID);
        if (vertexArrayID) glDeleteVertexArrays(1, &vertexArrayID);
        if (programID) glDeleteProgram(programID);
    }
};

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

		programID = LoadShadersFromFile("../scene/shaders/skybox.vert", "../scene/shaders/skybox.frag");
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
	double lastFrameTime = glfwGetTime();
	int frameCount = 0;

    // Camera setup
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	WorldManager worldManager;

	SimpleSnowSystem snowSystem;
	snowSystem.initialize(1500);

	Skybox skybox;
	skybox.initialize(glm::vec3(0, 2000, 0), glm::vec3(7500, 7500, 7500));

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera/view matrix
        glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
        glm::mat4 vp = projectionMatrix * viewMatrix;

    	float currentTime = static_cast<float>(glfwGetTime());
    	float deltaTime = static_cast<float>(currentTime - lastFrameTime);
    	lastFrameTime = currentTime;
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

    	worldManager.render(vp, lightPosition, lightIntensity, eye_center);
    	worldManager.update(eye_center, deltaTime, currentTime);

    	snowSystem.update(deltaTime);

    	glm::vec3 lookDirection = glm::normalize(lookat - eye_center);
    	glm::vec3 cameraRight = glm::normalize(glm::cross(lookDirection, up));
    	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraRight, lookDirection));

    	snowSystem.render(vp);

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