#define GL_SILENCE_DEPRECATION
#include "libraries/glad/glad.h"
#include "libraries/GLFW/glfw3.h"
#include <iostream>
#include "shader.h"
#include "sphere.h"
#include "model.h"
#include "filesystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libraries/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libraries/stb_image_write.h"
#define TINYEXR_IMPLEMENTATION
#include "libraries/tinyexr/tinyexr.h"


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// window size
unsigned int SCR_WIDTH = 1000;
unsigned int SCR_HEIGHT = 1000;

// FPS counter
double lastTime = glfwGetTime();
int nbFrames = 0;

// object 
float objectRadius = 2.5f;

// Camera
float radius = 6.5f;
float verticalAngle = 0.0f;
float horizontalAngle = 0.0f;
float cameraSpeed = 0.03f; // camera speed per frame
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, radius);
float fov = 45.0f;
// Near and Far clipping planes
float nearPlane = 0.5f;
float farPlane = 20.0f;

float leftBoundary = -2.5f;
float rightBoundary = 2.5f; 
float bottomBoundary = -2.5f;
float topBoundary = 2.5f;

// MSAA (0 for none)
int MSAA_SampleCount = 0;

// Framebuffers
GLuint FramebufferName;
GLuint depthTextures[2];
GLuint normalTextures[2];
GLuint vertexTextures[2];

GLuint hdrFBO;
GLuint colorBuffer;

//for testing
bool DoOnce = true;


//light direction
glm::vec3 lightDirections[] = {
        glm::vec3(1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f,  -1.0f, -1.6f),
    };
glm::vec3 lightRadiances[] = {
        glm::vec3(20.0f, 20.0f, 20.0f),
        glm::vec3(10.0f, 10.0f, 10.0f),
    };
glm::mat4 lightSpaceMatrices[2];




// MVP matrices
// glm::mat4 modelMatrix = glm::mat4(1.0f);    // Calculate model matrix
glm::mat4 viewMatrix = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Calculate view matrix
glm::mat4 projectionMatrix = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, nearPlane, farPlane); // Calculate projection matrix


// set up color buffer for HDR rendering
void setupColorBuffer()
{
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glEnable(GL_MULTISAMPLE);

    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorBuffer);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MSAA_SampleCount, GL_RGB32F, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorBuffer, 0);
    

    // set up depth buffer
    // create and bind a renderbuffer object for depth and stencil attachment
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SampleCount, GL_DEPTH_COMPONENT32F, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // check for framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void rendertoHDR(Shader &shader, Model &model)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

        // glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        // pass normal and vertex textures to the shader
        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, vertexTextures[i]);
            shader.setInt("vertexTextures[" + std::to_string(i) + "]", i);
        }

        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i + 2);
            glBindTexture(GL_TEXTURE_2D, normalTextures[i]);
            shader.setInt("normalTextures[" + std::to_string(i) + "]", i + 2);
        }


        // MVP matrices to be used in the vertex shader
        shader.setMat4("projection", projectionMatrix);
        shader.setMat4("view", viewMatrix);
        // set model matrix to identity matrix
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        shader.setMat4("model", modelMatrix);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

        shader.setFloat("farPlane", nearPlane);
        shader.setFloat("nearPlane", farPlane);

        // set light properties
        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            shader.setVec3("lightDirections[" + std::to_string(i) + "]", lightDirections[i]);
            shader.setVec3("lightRadiances[" + std::to_string(i) + "]", lightRadiances[i]);
            shader.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
        }
        shader.setInt("numLights", sizeof(lightDirections)/sizeof(lightDirections[0]));
        shader.setVec3("eyePos", cameraPos);

        // Pass screen resolution to the shader
        shader.setVec2("resolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        
        // draw object
        model.Draw(shader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void rendertoSDR(Shader &shader, Model &model)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

        // glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        // pass normal and vertex textures to the shader
        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, vertexTextures[i]);
            shader.setInt("vertexTextures[" + std::to_string(i) + "]", i);
        }

        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i + 2);
            glBindTexture(GL_TEXTURE_2D, normalTextures[i]);
            shader.setInt("normalTextures[" + std::to_string(i) + "]", i + 2);
        }


        // MVP matrices to be used in the vertex shader
        shader.setMat4("projection", projectionMatrix);
        shader.setMat4("view", viewMatrix);
        // set model matrix to identity matrix
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        shader.setMat4("model", modelMatrix);
        shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

        shader.setFloat("farPlane", nearPlane);
        shader.setFloat("nearPlane", farPlane);

        // set light properties
        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            shader.setVec3("lightDirections[" + std::to_string(i) + "]", lightDirections[i]);
            shader.setVec3("lightRadiances[" + std::to_string(i) + "]", lightRadiances[i]);
            shader.setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
        }
        shader.setInt("numLights", sizeof(lightDirections)/sizeof(lightDirections[0]));
        shader.setVec3("eyePos", cameraPos);

        // Pass screen resolution to the shader
        shader.setVec2("resolution", glm::vec2(SCR_WIDTH, SCR_HEIGHT));
        
        // draw object
        model.Draw(shader);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// use tinyexr to save HDR image
// modified from tinyexr/examples/rgbe2exr/rgbe2exr.cc
void saveHDRImage(const char *filename, const float *colorBuffer)
{
    EXRHeader header;
    InitEXRHeader(&header);
    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 3;

    std::vector<float> images[3];
    images[0].resize(SCR_WIDTH * SCR_HEIGHT);
    images[1].resize(SCR_WIDTH * SCR_HEIGHT);
    images[2].resize(SCR_WIDTH * SCR_HEIGHT);

    // Split RGBRGBRGB... into R, G and B layer
        // and flip image vertically
    for (size_t y = 0; y < SCR_HEIGHT; y++)
{
    for (size_t x = 0; x < SCR_WIDTH; x++)
    {
        size_t flipped_index = (SCR_HEIGHT - y - 1) * SCR_WIDTH + x;
        size_t original_index = y * SCR_WIDTH + x;

        images[0][flipped_index] = colorBuffer[3 * original_index + 0];
        images[1][flipped_index] = colorBuffer[3 * original_index + 1];
        images[2][flipped_index] = colorBuffer[3 * original_index + 2];
    }
}

    float *image_ptr[3];
    image_ptr[0] = &(images[2].at(0)); // B
    image_ptr[1] = &(images[1].at(0)); // G
    image_ptr[2] = &(images[0].at(0)); // R

    image.images = (unsigned char **)image_ptr;
    image.width = SCR_WIDTH;
    image.height = SCR_HEIGHT;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
    // Must be (A)BGR order, since most of EXR viewers expect this channel order.
    strncpy(header.channels[0].name, "B", 255);
    header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255);
    header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255);
    header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
    for (int i = 0; i < header.num_channels; i++)
    {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }

    const char *err;
    int ret = SaveEXRImageToFile(&image, &header, filename, &err);
    if (ret != TINYEXR_SUCCESS)
    {
        fprintf(stderr, "Save EXR err: %s\n", err);
        return;
    }
    printf("Saved exr file. [ %s ] \n", filename);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}



void setupDepthBuffer(GLuint &texture)
{
    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT32F, SCR_HEIGHT, SCR_WIDTH, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);

    glDrawBuffer(GL_NONE); // No color buffer is drawn to.

    // Always check that our framebuffer is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer not complete\n");
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void setupNormalBuffer(GLuint &texture)
{
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB32F, SCR_HEIGHT, SCR_WIDTH, 0,GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glDrawBuffer(GL_COLOR_ATTACHMENT0); // No color buffer is drawn to.

    // Always check that our framebuffer is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer not complete\n");
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void cleanupNormalBuffer(GLuint &texture) {
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &FramebufferName);
}

void setupVertexBuffer(GLuint &texture)
{
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGB32F, SCR_HEIGHT, SCR_WIDTH, 0,GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // Generate mipmaps
    // glGenerateMipmap(GL_TEXTURE_2D);

    glDrawBuffer(GL_COLOR_ATTACHMENT0); // No color buffer is drawn to.

    // Always check that our framebuffer is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer not complete\n");
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void cleanupVertexBuffer(GLuint &texture) {
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &FramebufferName);
}

void rendertoNormalTexture(Shader &shader, Model &model, int index, glm::vec3 lightDir, GLuint texture)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    shader.use();

    //TODO probably do not need to do this again
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    // glEnable(GL_DEPTH_TEST);
    // MVP matrices to be used in the vertex shader
    glm::mat4 lightView = glm::lookAt(radius*normalize(lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // float leftBoundary = -2.5f, rightBoundary = 2.5f, bottomBoundary = -2.5f, topBoundary = 2.5f;
    // calculate the length of the light direction vector
    float distToCamera = glm::length(radius*normalize(lightDir));
    glm::mat4 lightProjection = glm::ortho(leftBoundary, rightBoundary, bottomBoundary, topBoundary, nearPlane, farPlane);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    lightSpaceMatrices[index] = lightSpaceMatrix;
    // set model matrix to identity matrix
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    shader.setMat4("model", modelMatrix);
    shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

    // Enable face culling
    glEnable(GL_CULL_FACE);
    // Cull back faces
    glCullFace(GL_BACK);

    // draw object
    model.Draw(shader);


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (DoOnce)
    {
    // For testing
    GLfloat* pixels_float = new GLfloat[SCR_HEIGHT * SCR_WIDTH * 3];
    unsigned char* pixels = new unsigned char[SCR_HEIGHT * SCR_WIDTH * 3];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels_float);
    for (int i = 0; i < SCR_HEIGHT * SCR_WIDTH * 3; i++)
    {
        // cout << pixels_float[i] << " ";
        pixels[i] = static_cast<unsigned char>(pixels_float[i] * 255.0f);
    }

    // Save texture data to PNG using stb_image_write library
    std::stringstream filename;
    filename << "output/normalTexture" << texture << ".png";
    std::string fullPath = FileSystem::getPath(filename.str());
    stbi_write_png(fullPath.c_str(), SCR_WIDTH, SCR_HEIGHT, 3, pixels, 0);
    }

}

void rendertoVertexTexture(Shader &shader, Model &model, int index, glm::vec3 lightDir, GLuint texture)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    shader.use();

    //TODO probably do not need to do this again
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    // glEnable(GL_DEPTH_TEST);
    // MVP matrices to be used in the vertex shader
    glm::mat4 lightView = glm::lookAt(radius*normalize(lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // float leftBoundary = -2.5f, rightBoundary = 2.5f, bottomBoundary = -2.5f, topBoundary = 2.5f;
    // calculate the length of the light direction vector
    float distToCamera = glm::length(radius*normalize(lightDir));
    glm::mat4 lightProjection = glm::ortho(leftBoundary, rightBoundary, bottomBoundary, topBoundary, nearPlane, farPlane);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    lightSpaceMatrices[index] = lightSpaceMatrix;
    // set model matrix to identity matrix
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    shader.setMat4("model", modelMatrix);
    shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

    // Enable face culling
    glEnable(GL_CULL_FACE);
    // Cull front faces
    glCullFace(GL_BACK);

    // draw object
    model.Draw(shader);

    // Reset to default culling mode after drawing
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (DoOnce)
    {
    // For testing
    GLfloat* pixels_float = new GLfloat[SCR_HEIGHT * SCR_WIDTH * 3];
    unsigned char* pixels = new unsigned char[SCR_HEIGHT * SCR_WIDTH * 3];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels_float);
    for (int i = 0; i < SCR_HEIGHT * SCR_WIDTH * 3; i++)
    {
        // cout << pixels_float[i] << " ";
        pixels[i] = static_cast<unsigned char>(pixels_float[i] * 255.0f/5.0);
    }

    // Save texture data to PNG using stb_image_write library
    std::stringstream filename;
    filename << "output/vertexTexture" << texture << ".png";

    std::string fullPath = FileSystem::getPath(filename.str());
    stbi_write_png(fullPath.c_str(), SCR_WIDTH, SCR_HEIGHT, 3, pixels, 0);
    }
}

void rendertoDepthTexture(Shader &shader, Model &model, int index, glm::vec3 lightDir, GLuint texture)
{
    
    
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    shader.use();

    //TODO probably do not need to do this again
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    // glEnable(GL_DEPTH_TEST);
    // MVP matrices to be used in the vertex shader
    glm::mat4 lightView = glm::lookAt(radius*normalize(lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // float leftBoundary = -2.5f, rightBoundary = 2.5f, bottomBoundary = -2.5f, topBoundary = 2.5f;
    // calculate the length of the light direction vector
    float distToCamera = glm::length(radius*normalize(lightDir));
    glm::mat4 lightProjection = glm::ortho(leftBoundary, rightBoundary, bottomBoundary, topBoundary, nearPlane, farPlane);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    lightSpaceMatrices[index] = lightSpaceMatrix;
    // set model matrix to identity matrix
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    shader.setMat4("model", modelMatrix);
    shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));

    // Enable face culling
    glEnable(GL_CULL_FACE);
    // Cull front faces
    glCullFace(GL_BACK);

    // draw object
    model.Draw(shader);

    // Reset to default culling mode after drawing
    glCullFace(GL_BACK);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // For testing
    GLfloat* pixels_float = new GLfloat[SCR_HEIGHT * SCR_WIDTH];
    unsigned char* pixels = new unsigned char[SCR_HEIGHT * SCR_WIDTH];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels_float);
    for (int i = 0; i < SCR_HEIGHT * SCR_WIDTH; i++)
    {
        // cout << pixels_float[i] << " ";
        pixels[i] = static_cast<unsigned char>(pixels_float[i] * 255.0f);
    }

    // Save texture data to PNG using stb_image_write library
    std::stringstream filename;
    filename << "output/depthTexture" << texture << ".png";
    std::string fullPath = FileSystem::getPath(filename.str());
    stbi_write_png(fullPath.c_str(), SCR_WIDTH, SCR_HEIGHT, 1, pixels, 0);

    // stbi_write_png(FileSystem::getPath("output/depthTexture.png").c_str(), SCR_WIDTH, SCR_HEIGHT, 1, pixels, 0);
    // saveHDRImage(FileSystem::getPath("output/depthTexture.exr").c_str(), pixels_float);









}

// Registering a callback function that gets called each time the window is resized.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    // Calculate new aspect ratio
    float aspectRatio = (float)width / (float)height;
    projectionMatrix  = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    SCR_HEIGHT = height;
    SCR_WIDTH = width;
    DoOnce = true;
    cout << "Width: " << SCR_WIDTH << " Height: " << SCR_HEIGHT << endl;
    }

// function declarations
void CalculateFrameRate(GLFWwindow* window);
void key_callback(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
// unsigned int loadTexture(const char *path);
// void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);



// unsigned int loadTexture(const char *path)
// {
//     unsigned int texture;
//     glGenTextures(1, &texture);
//     glBindTexture(GL_TEXTURE_2D, texture);
//     // set the texture wrapping parameters
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//     // set texture filtering parameters
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//     // load and generate the texture
//     int width, height, nrChannels;
//     unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
//     std::cout << "Width: " << width << " Height: " << height << " nrChannels: " << nrChannels << std::endl;
//     if (data)
//     {
//         // generate the texture
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
//         // generate the mipmap
//         glGenerateMipmap(GL_TEXTURE_2D);
//     }
//     else
//     {
//         std::cout << "Failed to load texture" << std::endl;
//     }
//     // free the image memory
//     stbi_image_free(data);
//     return texture;
// }

//function to 

int main(void)
{

    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    // Set OpenGL version to 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // Telling GLFW we want to use the core-profile means we’ll get access to a smaller subset of OpenGL features
    // without backwards-compatible features we no longer need.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // For MacOS
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH/2, SCR_HEIGHT/2, "OpenGL Reference", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // GlAD manages function pointers for OpenGL so we want to initialize GLAD before we call any OpenGL function.
    // important to call after we've set the current OpenGL context
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1; 
        }

    // Setting the viewport size
    // The first two parameters of glViewport set the location of the lower leftBoundary corner of the window.
    // The third and fourth parameter set the width and height of the rendering window in pixels, which we set equal to GLFW's window size.
    // multiple by 2 due to HiDPI display (otherwise image is not centered and too small)
    // glViewport(0, 0, 640*2, 480*2);

    // Registering the callback function on window resize to make sure OpenGL renders the image in the rightBoundary size whenever the window is resized.
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    Shader ourShader(FileSystem::getPath("src/shaders/vertexShader.vs").c_str(), FileSystem::getPath("src/shaders/model3.fs").c_str());
    Shader FBOShader(FileSystem::getPath("src/shaders/vertexShader2.vs").c_str(), FileSystem::getPath("src/shaders/FBOfragmentShader.fs").c_str());
    //normals
    Shader FBOShader2(FileSystem::getPath("src/shaders/vertexShader2.vs").c_str(), FileSystem::getPath("src/shaders/FBOfragmentShader2.fs").c_str());
    //vertices
    Shader FBOShader3(FileSystem::getPath("src/shaders/vertexShader2.vs").c_str(), FileSystem::getPath("src/shaders/FBOfragmentShader3.fs").c_str());
    

    // Query the maximum number of samples
    GLint maxSamples;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    std::cout << "Max MSAA samples: " << maxSamples << std::endl;

    
    // Linking the shaders
        ourShader.use();

    // Enable depth test
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        

    // load models
    // -----------
    Model ourModel(FileSystem::getPath("resources/objects/grain3.obj"));


    
    setupColorBuffer();

    // configure second post-processing framebuffer
    unsigned int intermediateFBO;
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    // create a color attachment texture
    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);
    

    

    //for each light source, render the scene depth to a texture
    for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
    {
        
        
        
        // float distToCamera = glm::length(radius*normalize(lightDirections[i]));
        // lightPlanes[i] = glm::vec2(distToCamera - objectRadius, distToCamera + objectRadius);
        setupNormalBuffer(normalTextures[i]);
        rendertoNormalTexture(FBOShader2, ourModel, i, lightDirections[i], normalTextures[i]);
        setupVertexBuffer(vertexTextures[i]);
        rendertoVertexTexture(FBOShader3, ourModel, i, lightDirections[i], vertexTextures[i]);
        setupDepthBuffer(depthTextures[i]);
        rendertoDepthTexture(FBOShader, ourModel, i, lightDirections[i], depthTextures[i]);
    }

    /* Loop until the user closes the window */
    // The render loop
    while (!glfwWindowShouldClose(window))
    {
        if (DoOnce)
        {
            
            //set depthMap texture in the shader to the depthTexture
            

            
            // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
            // Render to HDR buffer
            rendertoHDR(ourShader, ourModel);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, hdrFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);




            // Save HDR image
            float *pixelBuffer = new float[SCR_WIDTH * SCR_HEIGHT * 3];
            glBindTexture(GL_TEXTURE_2D, screenTexture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixelBuffer);
            // for (int i = 0; i < SCR_WIDTH * SCR_HEIGHT * 3; i++)
            // {
            //     cout << pixelBuffer[i] << " ";
            // }
            //unbind the texture
            glBindTexture(GL_TEXTURE_2D, 0);
            saveHDRImage(FileSystem::getPath("output/hdrOutput.exr").c_str(), pixelBuffer);
            DoOnce = false;

            // printf("Depth texture saved\n");
        }
        
        

        for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
        {
            // float distToCamera = glm::length(radius*normalize(lightDirections[i]));
            // lightPlanes[i] = glm::vec2(distToCamera - objectRadius, distToCamera + objectRadius);
            
            // setupNormalBuffer(normalTextures[i]);
            rendertoNormalTexture(FBOShader2, ourModel, i, lightDirections[i], normalTextures[i]);
            
            // setupVertexBuffer(vertexTextures[i]);
            rendertoVertexTexture(FBOShader3, ourModel, i, lightDirections[i], vertexTextures[i]);

            // bind textures to be used in the shader


        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        rendertoSDR(ourShader, ourModel);

        


        // glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
        // FBOShader.use();
        // //maybe?
        // glViewport(0, 0, 800, 600);

        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // // Drawing the model
        // glm::mat4 model = glm::mat4(1.0f); // start with an identity matrix
        // glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
        // glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        // FBOShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        // FBOShader.setMat4("model", model);

        // ourModel.Draw(FBOShader);
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, depthTexture);

        // GLfloat* pixels_float = new GLfloat[800 * 600]; // Assuming width and height are the dimensions of the texture
        // unsigned char* pixels = new unsigned char[800 * 600];
        // glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels_float);
        // for (int i = 0; i < 800 * 600; i++)
        // {
        //     // cout << pixels_float[i] << " ";
        //     pixels[i] = static_cast<unsigned char>(pixels_float[i] * 255.0f);
        // }

        // // Save texture data to PNG using stb_image_write library
        // stbi_write_png("depthTexture.png", 800, 600, 1, pixels, 0);

        /* Render here */
        // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        
    //     ourShader.use();
    //     // glViewport(0, 0, 800, 600);
    //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //     // MVP matrices to be used in the vertex shader
    //     ourShader.setMat4("projection", projectionMatrix);
    //     ourShader.setMat4("view", viewMatrix);
        
    //     // Drawing the sphere
    //         // renderSphere();

    //     // Drawing the plane
    //         // renderPlane();

    //     // Drawing the model
    //     glm::mat4 model = glm::mat4(1.0f); // start with an identity matrix
    // // model = glm::scale(model, glm::vec3(200.0f, 1.0f, 200.0f)); // apply scaling
    //     ourShader.setMat4("model", model);
    //     ourShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    //     ourShader.setVec3("lightDir", normalize(lightDir));
    //     ourShader.setVec3("eyePos", cameraPos);
    //     // glBindVertexArray(planeVAO);
    //     // glActiveTexture(GL_TEXTURE0);
    //     // glBindTexture(GL_TEXTURE_2D, planeTexture);
    //     ourModel.Draw(ourShader);


        //check for key input
        key_callback(window);
        //adjust point size
        // glPointSize(10.0f);

        //check for mouse scroll input
        glfwSetScrollCallback(window, scroll_callback);

        

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        //get key input (switched due to low performance of the callback function)
        // glfwSetKeyCallback(window, key_callback);

        /* Poll for and process events */
        // e.g. keyboard input, mouse movement, etc.
        glfwPollEvents();

        CalculateFrameRate(window);
    }
    // cleanup
    for (unsigned int i = 0; i < sizeof(lightDirections)/sizeof(lightDirections[0]); i++)
    {
        cleanupNormalBuffer(normalTextures[i]);
        cleanupVertexBuffer(vertexTextures[i]);
    }

    glfwTerminate();
    return 0;
}

//change the name of the window to have the FPS
void CalculateFrameRate(GLFWwindow* window)
{
    // Measure speed
    double currentTime = glfwGetTime();
    nbFrames++;
    if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1 sec ago
        // printf and reset timer
        glfwSetWindowTitle(window, ("OpenGL Reference " + std::to_string(nbFrames) + " FPS").c_str());
        nbFrames = 0;
        lastTime += 1.0;
    }
}
// void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
inline void key_callback(GLFWwindow* window)

{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        verticalAngle += cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        verticalAngle -= cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        horizontalAngle -= cameraSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        horizontalAngle += cameraSpeed;
    }

// Calculate the new camera position using the angles and the radius
cameraPos.x = radius * cos(verticalAngle) * sin(horizontalAngle);
cameraPos.y = radius * sin(verticalAngle);
cameraPos.z = radius * cos(verticalAngle) * cos(horizontalAngle);

// Update the view matrix
viewMatrix = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// mouse callback function to change the radius of the camera
    //if the mouse is scrolled up, the radius is decreased
    //if the mouse is scrolled down, the radius is increased
inline void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset > 0)
    {
        radius -= 0.1f;
    }
    else
    {
        radius += 0.1f;
    }
    // Calculate the new camera position using the angles and the radius
    cameraPos.x = radius * cos(verticalAngle) * sin(horizontalAngle);
    cameraPos.y = radius * sin(verticalAngle);
    cameraPos.z = radius * cos(verticalAngle) * cos(horizontalAngle);

    // Update the view matrix
    viewMatrix = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // Update the projection matrix
    projectionMatrix = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, nearPlane, farPlane);

}