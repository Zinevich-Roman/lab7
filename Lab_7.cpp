#include "Shader.h"
#include "Model.h"
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <matrix_transform.hpp>
#include <type_ptr.hpp>
#include <iostream>


const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct ObjectTransform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);


    struct {
        float min = -5.0f;
        float max = 5.0f;
    } xLimit;

    struct {
        float min = -5.0f;
        float max = 5.0f;
    } zLimit;
};

std::vector<ObjectTransform> objectTransforms;


float Cylinder_gradus = 0.0f;
float plecho_gradus = 0.0f;
float kyst_gradus = 0.0f;
float model_speed = 0.05f;
glm::vec3 plecho_center = glm::vec3();
glm::vec3 kyst_center = glm::vec3();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

auto rotAroundPoint(float rad, const glm::vec3& point, const glm::vec3& axis)
{

    auto t1 = glm::translate(glm::mat4(1), -point);
    auto r = glm::rotate(glm::mat4(1), rad, axis);
    auto t2 = glm::translate(glm::mat4(1), point);
    return t2 * r * t1;
}

glm::mat4 calculateModelMatrix(int index) {
    glm::mat4 model = glm::mat4(1.0f);

    

    if (index == 0) {
        model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
        return model;
    }

    if (index == 1) { // плечо
        model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
        model *= rotAroundPoint(glm::radians(plecho_gradus), plecho_center, glm::vec3(1.0f, 0.0f, 0.0f));
        return model;
    }

    if (index == 2) { // штука на основании
        model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
        return model;
    }

    if (index == 3) { // кисть
        model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
        model *= rotAroundPoint(glm::radians(plecho_gradus), plecho_center, glm::vec3(1.0f, 0.0f, 0.0f));
        model *= rotAroundPoint(glm::radians(kyst_gradus), kyst_center, glm::vec3(1.0f, 0.0f, 0.0f));
        return model;
    }
    if (index == 4) { // штука на основании
        model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
        return model;
    }
    if (index == 5) { // плечо
        model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
        model *= rotAroundPoint(glm::radians(plecho_gradus), plecho_center, glm::vec3(1.0f, 0.0f, 0.0f));
        return model;
    }
    //switch (index)
    //{
    //case 1: // штука на основании
    //    break;
    //case 3: // кисть
    //    model *= rotAroundPoint(glm::radians(plecho_gradus), glm::vec3(), glm::vec3(1.0f, 0.0f, 0.0f));
    //    break;
    //case 2: // штука на основании
    //    break;
    //case 0: // основание
    //    model *= rotAroundPoint(glm::radians(Cylinder_gradus), glm::vec3(), glm::vec3(0.0f, 1.0f, 0.0f));
    //    break;
    //}

    return model;
}

int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Model", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader("vertex_sheder.glsl", "fragment_shader.glsl");
    Model ourModel("manipulator.obj");

    plecho_center = ourModel.plecho_center;
    kyst_center = ourModel.kyst_center;

    objectTransforms.resize(4);


    objectTransforms[1].zLimit = { -0.5f, 0.25f };
    objectTransforms[2].xLimit = { -0.5f, 0.5f };
    objectTransforms[3].xLimit = { -1.0f, 0.5f };

    shader.use();
    shader.setVec3("light.position", glm::vec3(2.0f, 3.0f, 2.0f));
    shader.setVec3("light.ambient", glm::vec3(0.1f, 0.1f, 0.1f));
    shader.setVec3("light.diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
    shader.setVec3("light.specular", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setVec3("material.ambient", glm::vec3(1.0f, 0.1f, 0.1f));
    shader.setVec3("material.diffuse", glm::vec3(0.2f, 0.4f, 0.8f));
    shader.setVec3("material.specular", glm::vec3(0.8f, 0.8f, 0.8f));
    shader.setFloat("material.shininess", 32.0f);

    //printf("%f\t%f\t%f\n", plecho_center.x, plecho_center.y, plecho_center.z);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.5f, 0.5f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        shader.setVec3("viewPos", cameraPos);
        glm::mat4 projection = glm::perspective(glm::radians(fov),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);


        glm::mat4 model_transform = glm::mat4(1.0f);
        for (size_t i = 0; i < ourModel.meshTransforms.size(); ++i) {
            ourModel.meshTransforms[i] = calculateModelMatrix(i);
        }
        shader.setMat4("model", model_transform);

        ourModel.Draw(shader);
        //printf("%f\t%f\n", hotizontal_on_start, objectTransforms[3].rotation.x);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 90.0f)
        fov = 90.0f;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    float moveSpeed = 1.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        Cylinder_gradus += model_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        Cylinder_gradus -= model_speed;
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        plecho_gradus += model_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        plecho_gradus -= model_speed;
    }

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        kyst_gradus += model_speed;
    }
    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        kyst_gradus -= model_speed;
    }


    float Cylinder_gradus_ogr = 150.0f;
    if (Cylinder_gradus > Cylinder_gradus_ogr) Cylinder_gradus = Cylinder_gradus_ogr;
    if (Cylinder_gradus < -Cylinder_gradus_ogr) Cylinder_gradus = -Cylinder_gradus_ogr;

    float plecho_gradus_ogr_up = 60.0f;
    float plecho_gradus_ogr_down = -25.0f;
    if (plecho_gradus > plecho_gradus_ogr_up) plecho_gradus = plecho_gradus_ogr_up;
    if (plecho_gradus < plecho_gradus_ogr_down) plecho_gradus = plecho_gradus_ogr_down;

    float kyst_gradus_ogr_up = 75.0f;
    float kyst_gradus_ogr_down = -30.0f;
    if (kyst_gradus > kyst_gradus_ogr_up) kyst_gradus = kyst_gradus_ogr_up;
    if (kyst_gradus < kyst_gradus_ogr_down) kyst_gradus = kyst_gradus_ogr_down;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}