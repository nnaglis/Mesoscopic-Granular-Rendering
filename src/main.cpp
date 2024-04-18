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


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// window size
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// FPS counter
double lastTime = glfwGetTime();
int nbFrames = 0;

// Camera
float radius = 8.0f;
float verticalAngle = 0.0f;
float horizontalAngle = 0.0f;
float cameraSpeed = 0.03f; // camera speed per frame
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, radius);
float fov = 60.0f;
// Near and Far clipping planes
float nearPlane = 0.1f;
float farPlane = 10.0f;

GLuint FramebufferName = 0;

//light direction
glm::vec3 lightDir = glm::vec3(-1.0f,-1.0f,-0.0f);


// MVP matrices
// glm::mat4 modelMatrix = glm::mat4(1.0f);    // Calculate model matrix
glm::mat4 viewMatrix = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Calculate view matrix
glm::mat4 projectionMatrix = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, nearPlane, farPlane); // Calculate projection matrix


// TEST VERTICES
//create sphere
Sphere sphere(1.5f, 30, 30);
float planePositions[] = {
    1.0f,  0.0f, 1.0f,
    1.0f,  0.0f, -1.0f,
    -1.0f, 0.0f, -1.0f,
    -1.0f, 0.0f, 1.0f
};

float planeTextureCoords[] = {
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f
};

unsigned int planeIndices[] = {
    0, 1, 3,
    1, 2, 3
};

unsigned int sphereVAO, sphereVBO, sphereEBO, sphereTBO, sphereTexture;
unsigned int planeVAO, planeVBO, planeEBO, planeTBO, planeTexture;



// Registering a callback function that gets called each time the window is resized.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    // Calculate new aspect ratio
    float aspectRatio = (float)width / (float)height;

    projectionMatrix  = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    SCR_HEIGHT = height;
    SCR_WIDTH = width;
    
    // // Update the VBO with the new vertex data
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sphere.getVertexCount(), sphere.getVertices(), GL_STATIC_DRAW);
    }

// function declarations
void CalculateFrameRate(GLFWwindow* window);
void key_callback(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
unsigned int loadTexture(const char *path);
// void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


void setupPlane() {
    // Generate the VAO
    glGenVertexArrays(1, &planeVAO);
    glBindVertexArray(planeVAO);

    // Generate and set up the VBO
    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planePositions), planePositions, GL_STATIC_DRAW);

    // Specify the vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);  // Vertex positions

    // Generate and set up the EBO
    glGenBuffers(1, &planeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    // Generate and set up the TBO
    glGenBuffers(1, &planeTBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeTBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeTextureCoords), planeTextureCoords, GL_STATIC_DRAW);

    // Specify the texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);  // Texture coordinates

    // load texture
    planeTexture = loadTexture(FileSystem::getPath("resources/textures/solid-grey.jpg").c_str());

    // Unbind the VAO (optional)
    glBindVertexArray(0);
}

void renderPlane() {
    glm::mat4 model = glm::mat4(1.0f); // start with an identity matrix
    model = glm::scale(model, glm::vec3(200.0f, 1.0f, 200.0f)); // apply scaling
    glBindVertexArray(planeVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTexture);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}



void setupSphere() {
    // Generate the VAO
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    // Generate and set up the VBO
    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*sphere.getVertexCount(), sphere.getVertices(), GL_STATIC_DRAW);

    // Specify the vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);  // Vertex positions

    // Generate and set up the EBO
    glGenBuffers(1, &sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)* sphere.getIndexCount(), sphere.getIndices(), GL_STATIC_DRAW);

    // Generate and set up the TBO
    glGenBuffers(1, &sphereTBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereTBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*sphere.getTexCoordCount(), sphere.getTexCoords(), GL_STATIC_DRAW);

    // Specify the texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);  // Texture coordinates

    sphereTexture = loadTexture(FileSystem::getPath("resources/textures/container.jpg").c_str());

    // Unbind the VAO
    glBindVertexArray(0);
}

void renderSphere() {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, sphere.radius, 0.0f)); // apply translation    glBindVertexArray(sphereVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sphereTexture);
    // glBindTexture(GL_TEXTURE_2D, texture);
    glDrawElements(GL_TRIANGLES, sphere.getIndexCount(), GL_UNSIGNED_INT, 0);
    
}


unsigned int loadTexture(const char *path)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    std::cout << "Width: " << width << " Height: " << height << " nrChannels: " << nrChannels << std::endl;
    if (data)
    {
        // generate the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        // generate the mipmap
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    // free the image memory
    stbi_image_free(data);
    return texture;
}

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

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Reference", NULL, NULL);
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
    // The first two parameters of glViewport set the location of the lower left corner of the window.
    // The third and fourth parameter set the width and height of the rendering window in pixels, which we set equal to GLFW's window size.
    // multiple by 2 due to HiDPI display (otherwise image is not centered and too small)
    // glViewport(0, 0, 640*2, 480*2);

    // Registering the callback function on window resize to make sure OpenGL renders the image in the right size whenever the window is resized.
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    Shader ourShader(FileSystem::getPath("src/shaders/vertexShader.vs").c_str(), FileSystem::getPath("src/shaders/fragmentShader.fs").c_str());
    Shader FBOShader(FileSystem::getPath("src/shaders/FBOvertexShader2.vs").c_str(), FileSystem::getPath("src/shaders/FBOfragmentShader2.fs").c_str());
    

    
    // Linking the shaders
        ourShader.use();

    // Enable depth test
        glEnable(GL_DEPTH_TEST);
    
    setupPlane();
    setupSphere();

    // load models
    // -----------
    Model ourModel(FileSystem::getPath("resources/objects/grain_sphere.obj"));

    /* Loop until the user closes the window */

    // // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    // Depth texture. Slower than a depth buffer, but you can sample it later in your shader
    GLuint depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT16, 800, 600, 0,GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

    glDrawBuffer(GL_NONE); // No color buffer is drawn to.

    // Always check that our framebuffer is ok
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return 0;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // The render loop
    while (!glfwWindowShouldClose(window))
    {
        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
        
        ourShader.use();
        // glViewport(0, 0, 800, 600);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // MVP matrices to be used in the vertex shader
        ourShader.setMat4("projection", projectionMatrix);
        ourShader.setMat4("view", viewMatrix);
        
        // Drawing the sphere
            // renderSphere();

        // Drawing the plane
            // renderPlane();

        // Drawing the model
        glm::mat4 model = glm::mat4(1.0f); // start with an identity matrix
    // model = glm::scale(model, glm::vec3(200.0f, 1.0f, 200.0f)); // apply scaling
        ourShader.setMat4("model", model);
        ourShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        ourShader.setVec3("lightDir", normalize(lightDir));
        ourShader.setVec3("eyePos", cameraPos);
        // glBindVertexArray(planeVAO);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, planeTexture);
        ourModel.Draw(ourShader);


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
}