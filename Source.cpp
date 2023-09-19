#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "camera.h"
#include <iostream>
#include "meshes.h"

namespace {

    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
float gLastX = WINDOW_WIDTH / 2.0f;
float gLastY = WINDOW_HEIGHT / 2.0f;
bool gFirstMouse = true;

// timing
float gDeltaTime = 0.0f; // time between current frame and last frame
float gLastFrame = 0.0f;

}



const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;
    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aPos;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;
    in vec3 FragPos;
    in vec3 Normal;

    uniform sampler2D texture1;
    uniform vec3 lightDir; // directional light
    uniform vec3 lightColor; // light color direction

    void main()
    {
        vec3 ambient = 0.2 * texture(texture1, TexCoord).rgb;
        vec3 norm = normalize(Normal);
        vec3 lightDirNormalized = normalize(lightDir);
        float diff = max(dot(norm, lightDirNormalized), 0.0);

        // Increase the intensity of the light by multiplying with a factor
        float intensity = 3.0; // You can adjust this factor as needed
        vec3 diffuse = diff * intensity * lightColor * texture(texture1, TexCoord).rgb;

        vec3 result = ambient + diffuse;
        FragColor = vec4(result, 1.0);
    }
)";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* texturePath);

const int CYLINDER_SEGMENTS = 50;
const float CYLINDER_RADIUS = 0.5f;
const float CYLINDER_HEIGHT = 1.0f;

float cylinderVertices[(CYLINDER_SEGMENTS + 1) * 2 * 5];
float planeVertices[] = {
    // pos              // texture coords
    -2.0f, -0.5f, -2.0f,    0.0f, 0.0f,
     2.0f, -0.5f, -2.0f,    1.0f, 0.0f,
     2.0f, -0.5f,  2.0f,    1.0f, 1.0f,
    -2.0f, -0.5f,  2.0f,    0.0f, 1.0f
};

unsigned int planeIndices[] = {
    0, 1, 2,
    0, 2, 3
};


void setupCylinderVertices()
{
    float segmentAngle = 2.0f * glm::pi<float>() / static_cast<float>(CYLINDER_SEGMENTS);
    int vertexIndex = 0;

    for (int i = 0; i <= CYLINDER_SEGMENTS; ++i)
    {
        float x = CYLINDER_RADIUS * cos(i * segmentAngle);
        float z = CYLINDER_RADIUS * sin(i * segmentAngle);

        // lower vertex
        cylinderVertices[vertexIndex++] = x;
        cylinderVertices[vertexIndex++] = -CYLINDER_HEIGHT / 2.0f;
        cylinderVertices[vertexIndex++] = z;
        cylinderVertices[vertexIndex++] = static_cast<float>(i) / CYLINDER_SEGMENTS;
        cylinderVertices[vertexIndex++] = 0.0f;

        // upper vert
        cylinderVertices[vertexIndex++] = x;
        cylinderVertices[vertexIndex++] = CYLINDER_HEIGHT / 2.0f;
        cylinderVertices[vertexIndex++] = z;
        cylinderVertices[vertexIndex++] = static_cast<float>(i) / CYLINDER_SEGMENTS;
        cylinderVertices[vertexIndex++] = 1.0f;
    }
}

const int SPHERE_SEGMENTS = 50;
const int SPHERE_STACKS = 50;
const float SPHERE_RADIUS = 0.5f;

float sphereVertices[(SPHERE_SEGMENTS + 1) * (SPHERE_STACKS + 1) * 5];
unsigned int sphereIndices[SPHERE_SEGMENTS * SPHERE_STACKS * 6];

void setupSphereVertices()
{
    int vertexIndex = 0;
    for (int i = 0; i <= SPHERE_STACKS; ++i)
    {
        float phi = glm::pi<float>() * static_cast<float>(i) / SPHERE_STACKS;
        for (int j = 0; j <= SPHERE_SEGMENTS; ++j)
        {
            float theta = 2.0f * glm::pi<float>() * static_cast<float>(j) / SPHERE_SEGMENTS;

            float x = SPHERE_RADIUS * sin(phi) * cos(theta);
            float y = SPHERE_RADIUS * cos(phi);
            float z = SPHERE_RADIUS * sin(phi) * sin(theta);

            sphereVertices[vertexIndex++] = x;
            sphereVertices[vertexIndex++] = y;
            sphereVertices[vertexIndex++] = z;
            sphereVertices[vertexIndex++] = static_cast<float>(j) / SPHERE_SEGMENTS;
            sphereVertices[vertexIndex++] = static_cast<float>(i) / SPHERE_STACKS;
        }
    }

    int index = 0;
    for (int i = 0; i < SPHERE_STACKS; ++i)
    {
        for (int j = 0; j < SPHERE_SEGMENTS; ++j)
        {
            int topLeft = i * (SPHERE_SEGMENTS + 1) + j;
            int bottomLeft = (i + 1) * (SPHERE_SEGMENTS + 1) + j;

            sphereIndices[index++] = topLeft;
            sphereIndices[index++] = bottomLeft;
            sphereIndices[index++] = topLeft + 1;

            sphereIndices[index++] = topLeft + 1;
            sphereIndices[index++] = bottomLeft;
            sphereIndices[index++] = bottomLeft + 1;
        }
    }
}

int main()
{
    setupSphereVertices();
    setupCylinderVertices();


    Camera camera(glm::vec3(1.0f, 1.0f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);


    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D SCENE TEXTURED COGNATA", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex shader compilation failed:\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment shader compilation failed:\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "Shader program linking failed:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int cylinderVAO, cylinderVBO;
    glGenVertexArrays(1, &cylinderVAO);
    glGenBuffers(1, &cylinderVBO);

    glBindVertexArray(cylinderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cylinderVertices), cylinderVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int planeVAO, planeVBO, planeEBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Cube vertices and VAO setup
    const float CUBE_SIZE = 0.5f;

    float cubeVertices[] = {
        // positions          // normals           // texture coords
        // Front face
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,

        // Back face
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,

        // Left face
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

        // Right face
        CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,

        // Top face
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
        -CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,

        // Bottom face
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
        -CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
    };

    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    glm::mat4 view = glm::lookAt(glm::vec3(1.0f, 1.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    unsigned int texture = loadTexture("texture.jpg");
    double lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(window);  // Process other inputs (e.g., ESC key)

 

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::vec3 lightDirection(-1.0f, -1.0f, -1.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, glm::value_ptr(lightDirection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        glm::mat4 planeModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(planeModel));

        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glm::mat4 cylinderModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cylinderModel));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(cylinderVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (CYLINDER_SEGMENTS + 1) * 2);
        glBindVertexArray(0);

        glm::mat4 cubeModel = glm::mat4(1.0f);
        cubeModel = glm::translate(cubeModel, glm::vec3(-1.5f, 0.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &planeEBO);

    glDeleteVertexArrays(1, &cylinderVAO);
    glDeleteBuffers(1, &cylinderVBO);

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

unsigned int loadTexture(const char* texturePath)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


