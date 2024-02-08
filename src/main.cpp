#define GL_SILENCE_DEPRECATION
#include "libraries/glad/glad.h"
#include "libraries/GLFW/glfw3.h"
#include <iostream>
#include "shader.h"
#include "sphere.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libraries/stb_image.h"


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// window size
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// FPS counter
double lastTime = glfwGetTime();
int nbFrames = 0;

// Camera
float radius = 3.0f;
float verticalAngle = 0.0f;
float horizontalAngle = 0.0f;
float cameraSpeed = 0.03f; // camera speed per frame
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, radius);
float fov = 90.0f;
// Near and Far clipping planes
float nearPlane = 0.1f;
float farPlane = 100.0f;


// MVP matrices
glm::mat4 modelMatrix = glm::mat4(1.0f);    // Calculate model matrix
glm::mat4 viewMatrix = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Calculate view matrix
glm::mat4 projectionMatrix = glm::perspective(glm::radians(fov), (float)SCR_WIDTH/(float)SCR_HEIGHT, nearPlane, farPlane); // Calculate projection matrix


// TEST VERTICES
//create sphere
Sphere sphere(1.0f, 30, 30);


    unsigned int indices[] = {  // note that we start from 0!
        0, 2, 1,  // first Triangle
        1, 2, 3   // second Triangle
    };

unsigned int VBO;


// Registering a callback function that gets called each time the window is resized.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    // Calculate new aspect ratio
    float aspectRatio = (float)width / (float)height;

    projectionMatrix  = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    // // Update the VBO with the new vertex data
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(float) * sphere.getVertexCount(), sphere.getVertices(), GL_STATIC_DRAW);
    }

// function declarations
void CalculateFrameRate(GLFWwindow* window);
void key_callback(GLFWwindow* window);
// void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);


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
    Shader ourShader("/Users/naglisnaslenas/Documents/DTU/Thesis/code/Refference/LearnOpenGL_Xcode/src/shaders/vertexShader.vs", "/Users/naglisnaslenas/Documents/DTU/Thesis/code/Refference/LearnOpenGL_Xcode/src/shaders/fragmentShader.fs");

    // Load texture
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
    unsigned char *data = stbi_load("/Users/naglisnaslenas/Documents/DTU/Thesis/code/Refference/LearnOpenGL/resources/textures/container.jpg", &width, &height, &nrChannels, 0);
    std::cout << "Width: " << width << " Height: " << height << " nrChannels: " << nrChannels << std::endl;
    if (data)
    {
        // generate the texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        // generate the mipmap
        glGenerateMipmap(GL_TEXTURE_2D);
        // set the texture wrapping/filtering options (on the currently bound texture object)
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // // Set the uniform variable in the shader to the texture unit
        // glUniform1i(glGetUniformLocation(ourShader.ID, "tex"), 0);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    // free the image memory
    stbi_image_free(data);

    // Generating the vertex array object
        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

    // creating a Vertex buffer object
        glGenBuffers(1, &VBO);
        // binding the buffer type
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // sending the data to the buffer
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*sphere.getVertexCount(), sphere.getVertices(), GL_STATIC_DRAW);

    // generating an element buffer object
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        // binding the buffer type
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // sending the data to the buffer
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)* sphere.getIndexCount(), sphere.getIndices(), GL_STATIC_DRAW);

        // Setting the vertex attribute pointers for position data
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

    //creating a texture buffer object 
        unsigned int TBO;
        glGenBuffers(1, &TBO);
        // binding the buffer type
        glBindBuffer(GL_ARRAY_BUFFER, TBO);
        // sending the data to the buffer
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*sphere.getTexCoordCount(), sphere.getTexCoords(), GL_STATIC_DRAW);

        // Link the texture coordinates to the vertex shader
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);



    // Linking the shaders
        ourShader.use();

    // Enable depth test
        glEnable(GL_DEPTH_TEST);
    
    /* Loop until the user closes the window */
    // The render loop

    GLint modelMatrixLoc = glGetUniformLocation(ourShader.ID, "modelMatrix");
    GLint viewMatrixLoc = glGetUniformLocation(ourShader.ID, "viewMatrix");
    GLint projectionMatrixLoc = glGetUniformLocation(ourShader.ID, "projectionMatrix");

    while (!glfwWindowShouldClose(window))
    {
        //check for key input (for camera movement)
        key_callback(window);
        // MVP matrices to be used in the vertex shader
        glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(projectionMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));




        /* Render here */
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //set texture
        glBindTexture(GL_TEXTURE_2D, texture);

        // Drawing the square
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, sphere.getIndexCount(), GL_UNSIGNED_INT, 0);
    
        //adjust point size
        // glPointSize(10.0f);

        

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