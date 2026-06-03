/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 *
 * Câmera FPS implementada como classe Camera.
 * Controles:
 *   W/A/S/D     → mover câmera
 *   Mouse       → rotacionar câmera (yaw/pitch)
 *   Scroll      → zoom (FOV)
 *   ESC         → fechar
 *   Setas       → mover objeto ativo em X/Z
 *   I/J         → mover objeto ativo em Y
 *   X/Y/Z       → girar objeto ativo
 *   [/]         → escalar objeto ativo
 *   N / TAB     → adicionar/alternar objeto ativo
 */

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    float yaw;
    float pitch;
    float fov;
    float movementSpeed;
    float mouseSensitivity;
    float deltaTime;
    float lastFrame;

    Camera(glm::vec3 pos = glm::vec3(0.0f, 2.0f, 7.0f), float startYaw = -90.0f, float startPitch = 0.0f, float speed = 5.0f, float sensitivity = 0.05f, float startFov = 45.0f)
        : position(pos), yaw(startYaw), pitch(startPitch),
        fov(startFov), movementSpeed(speed), 
        mouseSensitivity(sensitivity),
        deltaTime(0.0f), lastFrame(0.0f)
    {
        updateCameraVectors();
    }

    void updateDeltaTime()
    {
        float current = static_cast<float>(glfwGetTime());
        deltaTime = current - lastFrame;
        lastFrame = current;
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const
    {
        return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
    }

    void moveForward() { position += front * movementSpeed * deltaTime; }
    void moveBackward() { position -= front * movementSpeed * deltaTime; }
    void moveLeft() { position -= glm::normalize(glm::cross(front, up)) * movementSpeed * deltaTime; }
    void moveRight() { position += glm::normalize(glm::cross(front, up)) * movementSpeed * deltaTime; }

    void processMouseMovement(float xOffset, float yOffset)
    {
        yaw += xOffset * mouseSensitivity;
        pitch += yOffset * mouseSensitivity;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
        updateCameraVectors();
    }

    void processMouseScroll(float yOffset)
    {
        fov = glm::clamp(fov - yOffset, 1.0f, 45.0f);
    }

private:
    void updateCameraVectors()
    {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
int  setupShader();
int  setupGeometry();

struct PhongMaterial
{
    glm::vec3 ka = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 kd = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 ks = glm::vec3(0.5f, 0.5f, 0.5f);
    float ns = 32.0f;
    string mapKd;
};

struct MeshMaterialBatch
{
    GLint first = 0;
    GLsizei count = 0;
    PhongMaterial material;
    GLuint textureID = 0;
    bool useTexture = false;
};

bool loadPhongMaterialsFromMTL(const string&, unordered_map<string, PhongMaterial>&, vector<string>&);
int loadSimpleOBJ(const string&, int&, string&, bool&, PhongMaterial&);
GLuint loadTexture(const string&, int&, int&);
string trim(const string&);

const GLuint WIDTH = 1000, HEIGHT = 1000;
const GLsizei DEFAULT_CUBE_VERTEX_COUNT = 36;
const float MOVE_STEP  = 0.1f;
const float SCALE_STEP = 0.1f;
const float MIN_SCALE  = 0.2f;
const glm::vec3 LIGHT_POSITION(6.0f, 2.5f, 1.5f);
const glm::vec3 LIGHT_COLOR   (1.0f, 1.0f, 1.0f);
const float PHONG_AMBIENT_GAIN  = 0.45f;
const float PHONG_DIFFUSE_GAIN  = 1.25f;
const float PHONG_SPECULAR_GAIN = 2.8f;

GLsizei gMeshVertexCount = DEFAULT_CUBE_VERTEX_COUNT;
vector<MeshMaterialBatch> gMeshBatches;
vector<GLuint> gOwnedTextureIDs;
GLuint gFallbackTextureID = 0;

Camera gCamera;

bool  gFirstMouse = true;
float gLastX = WIDTH  / 2.0f;
float gLastY = HEIGHT / 2.0f;

const GLchar* vertexShaderSource = "#version 410\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 texCoord;\n"
"layout (location = 2) in vec3 normal;\n"
"layout (location = 3) in vec3 color;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec2 finalTexCoord;\n"
"out vec3 finalNormal;\n"
"out vec3 fragPos;\n"
"out vec3 finalColor;\n"
"void main()\n"
"{\n"
"   vec4 worldPos = model * vec4(position, 1.0);\n"
"   gl_Position = projection * view * worldPos;\n"
"   finalTexCoord = texCoord;\n"
"   fragPos = vec3(worldPos);\n"
"   mat3 normalMatrix = transpose(inverse(mat3(model)));\n"
"   finalNormal = normalize(normalMatrix * normal);\n"
"   finalColor = color;\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 410\n"
"in vec2 finalTexCoord;\n"
"in vec3 finalNormal;\n"
"in vec3 fragPos;\n"
"in vec3 finalColor;\n"
"uniform sampler2D texBuff;\n"
"uniform bool useTexture;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 camPos;\n"
"uniform vec3 matKa;\n"
"uniform vec3 matKd;\n"
"uniform vec3 matKs;\n"
"uniform float matNs;\n"
"uniform float ambientGain;\n"
"uniform float diffuseGain;\n"
"uniform float specularGain;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"   vec3 baseColor = useTexture ? texture(texBuff, finalTexCoord).rgb : finalColor;\n"
"   vec3 N = normalize(finalNormal);\n"
"   vec3 L = normalize(lightPos - fragPos);\n"
"   vec3 V = normalize(camPos - fragPos);\n"
"   vec3 ambient = ambientGain * matKa * lightColor;\n"
"   float diff = max(dot(N, L), 0.0);\n"
"   vec3 diffuse = diffuseGain * matKd * diff * lightColor;\n"
"   vec3 specular = vec3(0.0);\n"
"   if (diff > 0.0)\n"
"   {\n"
"       vec3 R = reflect(-L, N);\n"
"       float spec = pow(max(dot(V, R), 0.0), max(matNs * 1.5, 1.0));\n"
"       specular = specularGain * matKs * spec * lightColor;\n"
"   }\n"
"   vec3 result = (ambient + diffuse) * baseColor + specular;\n"
"   color = vec4(result, 1.0);\n"
"}\n\0";


enum RotationAxis { AXIS_NONE = 0, AXIS_X, AXIS_Y, AXIS_Z };

struct CubeInstance
{
    glm::vec3 position;
    float scale;
    RotationAxis rotationAxis;
};

vector<CubeInstance> cubes;
size_t activeCubeIndex = 0;

CubeInstance& activeCube() { return cubes[activeCubeIndex]; }

void addCube()
{
    const int columns = 3;
    const float spacing = 1.8f;
    const size_t index = cubes.size();
    CubeInstance c;
    c.position = glm::vec3((static_cast<float>(index % columns) - 1.0f) * spacing, 0.0f, -static_cast<float>(index / columns) * spacing);
    c.scale = 1.0f;
    c.rotationAxis = AXIS_NONE;
    cubes.push_back(c);
    activeCubeIndex = cubes.size() - 1;
}

void cycleActiveCube() { if (!cubes.empty()) activeCubeIndex = (activeCubeIndex + 1) % cubes.size(); }
void moveActiveCube(const glm::vec3& d) { if (!cubes.empty()) activeCube().position += d; }
void setActiveRotationAxis(RotationAxis a) { if (!cubes.empty()) activeCube().rotationAxis = a; }

void scaleActiveCube(float d)
{
    if (!cubes.empty())
    {
        activeCube().scale = max(MIN_SCALE, activeCube().scale + d);
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Solaire de Astora -- Felipe Nogueira Bergamo!", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL: " << glGetString(GL_VERSION)  << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    const unsigned char whitePixel[] = { 255, 255, 255, 255 };
    glGenTextures(1, &gFallbackTextureID);
    glBindTexture(GL_TEXTURE_2D, gFallbackTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint shaderID = setupShader();
    GLuint VAO = setupGeometry();
    glUseProgram(shaderID);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc  = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");
    GLint texBuffLoc = glGetUniformLocation(shaderID, "texBuff");
    GLint useTextureLoc = glGetUniformLocation(shaderID, "useTexture");
    GLint lightPosLoc = glGetUniformLocation(shaderID, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(shaderID, "lightColor");
    GLint camPosLoc = glGetUniformLocation(shaderID, "camPos");
    GLint matKaLoc = glGetUniformLocation(shaderID, "matKa");
    GLint matKdLoc = glGetUniformLocation(shaderID, "matKd");
    GLint matKsLoc = glGetUniformLocation(shaderID, "matKs");
    GLint matNsLoc = glGetUniformLocation(shaderID, "matNs");
    GLint ambientGainLoc = glGetUniformLocation(shaderID, "ambientGain");
    GLint diffuseGainLoc = glGetUniformLocation(shaderID, "diffuseGain");
    GLint specularGainLoc = glGetUniformLocation(shaderID, "specularGain");

    glUniform1i (texBuffLoc, 0);
    glUniform1i (useTextureLoc, 0);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(LIGHT_POSITION));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(LIGHT_COLOR));
    glUniform1f (ambientGainLoc, PHONG_AMBIENT_GAIN);
    glUniform1f (diffuseGainLoc, PHONG_DIFFUSE_GAIN);
    glUniform1f (specularGainLoc, PHONG_SPECULAR_GAIN);

    glEnable(GL_DEPTH_TEST);
    addCube();

    while (!glfwWindowShouldClose(window))
    {
        gCamera.updateDeltaTime();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) gCamera.moveForward();
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) gCamera.moveBackward();
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) gCamera.moveLeft();
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) gCamera.moveRight();

        glfwPollEvents();

        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = gCamera.getViewMatrix();
        glm::mat4 projection = gCamera.getProjectionMatrix(static_cast<float>(width) / static_cast<float>(height));

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(camPosLoc, 1, glm::value_ptr(gCamera.position));

        const float angle = static_cast<float>(glfwGetTime());
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);

        for (const CubeInstance& cube : cubes)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cube.position);
            if (cube.rotationAxis == AXIS_X) model = glm::rotate(model, angle, glm::vec3(1, 0, 0));
            else if (cube.rotationAxis == AXIS_Y) model = glm::rotate(model, angle, glm::vec3(0, 1, 0));
            else if (cube.rotationAxis == AXIS_Z) model = glm::rotate(model, angle, glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(cube.scale));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            for (const MeshMaterialBatch& batch : gMeshBatches)
            {
                glUniform3fv(matKaLoc, 1, glm::value_ptr(batch.material.ka));
                glUniform3fv(matKdLoc, 1, glm::value_ptr(batch.material.kd));
                glUniform3fv(matKsLoc, 1, glm::value_ptr(batch.material.ks));
                glUniform1f (matNsLoc, batch.material.ns);
                glUniform1i (useTextureLoc, batch.useTexture ? 1 : 0);
                glBindTexture(GL_TEXTURE_2D, batch.useTexture ? batch.textureID : gFallbackTextureID);
                glDrawArrays(GL_TRIANGLES, batch.first, batch.count);
            }
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    for (GLuint id : gOwnedTextureIDs)
        if (id) glDeleteTextures(1, &id);
    glDeleteVertexArrays(1, &VAO);
    if (gFallbackTextureID) glDeleteTextures(1, &gFallbackTextureID);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    (void)scancode; (void)mode;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }
    if (!(action == GLFW_PRESS || action == GLFW_REPEAT)) return;

    if (key == GLFW_KEY_X) setActiveRotationAxis(AXIS_X);
    else if (key == GLFW_KEY_Y) setActiveRotationAxis(AXIS_Y);
    else if (key == GLFW_KEY_Z) setActiveRotationAxis(AXIS_Z);
    else if (key == GLFW_KEY_LEFT) moveActiveCube(glm::vec3(-MOVE_STEP, 0, 0));
    else if (key == GLFW_KEY_RIGHT) moveActiveCube(glm::vec3( MOVE_STEP, 0, 0));
    else if (key == GLFW_KEY_UP) moveActiveCube(glm::vec3(0, 0, -MOVE_STEP));
    else if (key == GLFW_KEY_DOWN) moveActiveCube(glm::vec3(0, 0, MOVE_STEP));
    else if (key == GLFW_KEY_I) moveActiveCube(glm::vec3(0, MOVE_STEP, 0));
    else if (key == GLFW_KEY_J) moveActiveCube(glm::vec3(0, -MOVE_STEP, 0));
    else if (key == GLFW_KEY_LEFT_BRACKET) scaleActiveCube(-SCALE_STEP);
    else if (key == GLFW_KEY_RIGHT_BRACKET) scaleActiveCube(SCALE_STEP);
    else if (key == GLFW_KEY_N && action == GLFW_PRESS) addCube();
    else if (key == GLFW_KEY_TAB && action == GLFW_PRESS) cycleActiveCube();
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;
    float fx = static_cast<float>(xpos);
    float fy = static_cast<float>(ypos);

    if (gFirstMouse) { gLastX = fx; gLastY = fy; gFirstMouse = false; }

    gCamera.processMouseMovement(fx - gLastX, gLastY - fy);
    gLastX = fx;
    gLastY = fy;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window; (void)xoffset;
    gCamera.processMouseScroll(static_cast<float>(yoffset));
}

int setupShader()
{
    GLint  success;
    GLchar infoLog[512];

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vs, 512, NULL, infoLog); cerr << "VERTEX: " << infoLog << endl; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fs, 512, NULL, infoLog); cerr << "FRAGMENT: " << infoLog << endl; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(prog, 512, NULL, infoLog); cerr << "PROGRAM: " << infoLog << endl; }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

string trim(const string& v)
{
    const size_t b = v.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    return v.substr(b, v.find_last_not_of(" \t\r\n") - b + 1);
}

bool loadPhongMaterialsFromMTL(const string& path,
    unordered_map<string, PhongMaterial>& out, vector<string>& order)
{
    ifstream f(path);
    if (!f.is_open()) return false;

    struct Rec { string name; PhongMaterial mat; };
    vector<Rec> recs;
    Rec cur; bool hasCur = false;

    auto flush = [&]() { if (hasCur && !cur.name.empty()) recs.push_back(cur); };

    string line;
    while (getline(f, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line); string w; ss >> w;

        if (w == "newmtl") { flush(); cur = Rec(); string r; getline(ss, r); cur.name = trim(r); hasCur = true; }
        else if (w == "Ka" && hasCur) ss >> cur.mat.ka.x >> cur.mat.ka.y >> cur.mat.ka.z;
        else if (w == "Kd" && hasCur) ss >> cur.mat.kd.x >> cur.mat.kd.y >> cur.mat.kd.z;
        else if (w == "Ks" && hasCur) ss >> cur.mat.ks.x >> cur.mat.ks.y >> cur.mat.ks.z;
        else if (w == "Ns" && hasCur) ss >> cur.mat.ns;
        else if (w == "map_Kd" && hasCur) { string r; getline(ss, r); cur.mat.mapKd = trim(r); }
    }
    flush();
    if (recs.empty()) return false;

    out.clear(); order.clear();
    for (const Rec& r : recs) if (!r.name.empty()) { out[r.name] = r.mat; order.push_back(r.name); }
    return !order.empty();
}

GLuint loadTexture(const string& filePath, int& width, int& height)
{
    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int ch = 0;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &ch, 0);
    if (!data)
    {
        cerr << "Falha ao carregar textura: " << filePath << endl;
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &id);
        return 0;
    }

    GLenum fmt = GL_RGB, ifmt = GL_RGB8;
    if (ch == 1) { fmt = GL_RED;  ifmt = GL_R8; }
    else if (ch == 4) { fmt = GL_RGBA; ifmt = GL_RGBA8; }

    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

int loadSimpleOBJ(const string& filePath, int& nVertices,
    string& outTexturePath, bool& outHasTexCoords, PhongMaterial& outMaterial)
{
    struct OBJIndex { int vertex = -1, uv = -1, normal = -1; };
    struct TriRecord { array<OBJIndex, 3> idx; string mat; };

    vector<glm::vec3> verts, norms;
    vector<glm::vec2> uvs;
    vector<TriRecord> tris;
    vector<GLfloat> buf;

    bool hasBounds = false;
    glm::vec3 minP(0), maxP(0);
    string mtlFile, curMat, firstMat;
    outTexturePath.clear(); outHasTexCoords = false;
    outMaterial = PhongMaterial();
    gMeshBatches.clear();

    ifstream in(filePath);
    if (!in.is_open()) { cerr << "Erro ao abrir: " << filePath << endl; return -1; }

    string line;
    while (getline(in, line))
    {
        istringstream ss(line); string w; ss >> w;
        if (w.empty() || w[0] == '#') continue;

        if (w == "v")
        {
            glm::vec3 v; ss >> v.x >> v.y >> v.z; verts.push_back(v);
            if (!hasBounds) { minP = maxP = v; hasBounds = true; }
            else { minP = glm::min(minP, v); maxP = glm::max(maxP, v); }
        }
        else if (w == "vt") { glm::vec2 t; ss >> t.x >> t.y; uvs.push_back(t); }
        else if (w == "vn") { glm::vec3 n; ss >> n.x >> n.y >> n.z; norms.push_back(n); }
        else if (w == "mtllib") { string r; getline(ss, r); mtlFile = trim(r); }
        else if (w == "usemtl") { string r; getline(ss, r); curMat = trim(r); if (firstMat.empty()) firstMat = curMat; }
        else if (w == "f")
        {
            vector<OBJIndex> face;
            string tok;
            while (ss >> tok)
            {
                OBJIndex idx;
                istringstream st(tok); string p;
                if (getline(st, p, '/') && !p.empty()) { int i = stoi(p); idx.vertex = i > 0 ? i-1 : i; }
                if (getline(st, p, '/') && !p.empty()) { int i = stoi(p); idx.uv = i > 0 ? i-1 : i; }
                if (getline(st, p) && !p.empty()) { int i = stoi(p); idx.normal = i > 0 ? i-1 : i; }
                face.push_back(idx);
            }
            if (face.size() < 3) continue;
            for (size_t i = 1; i+1 < face.size(); ++i)
                tris.push_back({ { face[0], face[i], face[i+1] }, curMat });
        }
    }

    if (verts.empty() || tris.empty()) { cerr << "OBJ vazio: " << filePath << endl; return -1; }

    glm::vec3 center(0);
    float scale = 1.0f;
    if (hasBounds)
    {
        center = (minP + maxP) * 0.5f;
        float ext = max({ maxP.x-minP.x, maxP.y-minP.y, maxP.z-minP.z });
        if (ext > 1e-6f) scale = 2.0f / ext;
    }

    unordered_map<string, PhongMaterial> matMap;
    vector<string> matOrder;
    filesystem::path mtlPath, objPath(filePath);
    bool matLoaded = false;

    if (!mtlFile.empty())
    {
        filesystem::path p = objPath.parent_path() / mtlFile;
        matLoaded = loadPhongMaterialsFromMTL(p.string(), matMap, matOrder);
        if (matLoaded) mtlPath = p;
    }
    if (!matLoaded)
    {
        filesystem::path p = objPath.parent_path() / (objPath.stem().string() + ".mtl");
        matLoaded = loadPhongMaterialsFromMTL(p.string(), matMap, matOrder);
        if (matLoaded) mtlPath = p;
    }

    auto resolveMat = [&](const string& name) -> PhongMaterial
    {
        auto it = matMap.find(name);
        if (it != matMap.end()) return it->second;
        it = matMap.find(firstMat);
        if (it != matMap.end()) return it->second;
        if (!matOrder.empty()) return matMap[matOrder.front()];
        return PhongMaterial();
    };

    unordered_map<string, GLuint> texCache;
    auto resolveTexID = [&](const PhongMaterial& m) -> GLuint
    {
        if (!matLoaded || m.mapKd.empty()) return 0;
        string nm = m.mapKd;
        replace(nm.begin(), nm.end(), '\\', '/');
        filesystem::path tp(nm);
        if (tp.is_relative()) tp = mtlPath.parent_path() / tp;
        else if (!filesystem::exists(tp)) tp = mtlPath.parent_path() / tp.filename();
        if (!filesystem::exists(tp))
        {
            filesystem::path fb = objPath.parent_path() / (objPath.stem().string() + ".png");
            if (filesystem::exists(fb)) tp = fb;
        }
        if (!filesystem::exists(tp)) return 0;
        string canon = tp.lexically_normal().string();
        auto it = texCache.find(canon);
        if (it != texCache.end()) return it->second;
        int tw = 0, th = 0;
        GLuint tid = loadTexture(canon, tw, th);
        texCache[canon] = tid;
        if (tid) gOwnedTextureIDs.push_back(tid);
        return tid;
    };

    auto appendVert = [&](const OBJIndex& idx, const glm::vec3& fn, const glm::vec3& col) -> bool
    {
        auto ri = [](int i, size_t n) { return i > 0 ? i-1 : i < 0 ? (int)n+i : -1; };
        int vi = ri(idx.vertex, verts.size());
        int ui = ri(idx.uv, uvs.size());
        int ni = ri(idx.normal, norms.size());
        if (vi < 0 || vi >= (int)verts.size()) return false;

        glm::vec2 uv(0);
        if (ui >= 0 && ui < (int)uvs.size()) { uv = uvs[ui]; outHasTexCoords = true; }

        glm::vec3 n = fn;
        if (ni >= 0 && ni < (int)norms.size()) n = norms[ni];
        float nl = glm::length(n);
        n = nl > 1e-6f ? n/nl : glm::vec3(0,0,1);

        glm::vec3 p = (verts[vi] - center) * scale;
        buf.push_back(p.x); buf.push_back(p.y); buf.push_back(p.z);
        buf.push_back(uv.x); buf.push_back(uv.y);
        buf.push_back(n.x); buf.push_back(n.y); buf.push_back(n.z);
        buf.push_back(col.r); buf.push_back(col.g); buf.push_back(col.b);
        return true;
    };

    string activeBatch;
    for (const TriRecord& tr : tris)
    {
        string key = tr.mat.empty() ? "__default__" : tr.mat;
        PhongMaterial m = resolveMat(tr.mat);

        if (gMeshBatches.empty() || key != activeBatch)
        {
            MeshMaterialBatch b;
            b.first = static_cast<GLint>(buf.size() / 11);
            b.count = 0;
            b.material = m;
            b.textureID = resolveTexID(m);
            b.useTexture = b.textureID != 0;
            gMeshBatches.push_back(b);
            activeBatch = key;
        }

        glm::vec3 fn(0, 0, 1);
        const auto& t = tr.idx;
        if (t[0].vertex >= 0 && t[1].vertex >= 0 && t[2].vertex >= 0
            && t[0].vertex < (int)verts.size()
            && t[1].vertex < (int)verts.size()
            && t[2].vertex < (int)verts.size())
        {
            glm::vec3 cn = glm::cross(verts[t[1].vertex]-verts[t[0].vertex], verts[t[2].vertex]-verts[t[0].vertex]);
            if (glm::length(cn) > 1e-6f) fn = glm::normalize(cn);
        }

        if (!appendVert(t[0],fn,m.kd) || !appendVert(t[1],fn,m.kd) || !appendVert(t[2],fn,m.kd))
        { cerr << "Falha ao processar face: " << filePath << endl; return -1; }
        gMeshBatches.back().count += 3;
    }

    if (gMeshBatches.empty()) { cerr << "OBJ sem lotes: " << filePath << endl; return -1; }
    outMaterial     = gMeshBatches.front().material;
    outHasTexCoords = outHasTexCoords && !uvs.empty();
    if (gMeshBatches.front().useTexture) outTexturePath = gMeshBatches.front().material.mapKd;

    GLuint VBO = 0, VAO = 0;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, buf.size()*sizeof(GLfloat), buf.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(5*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(8*sizeof(GLfloat)));
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = static_cast<int>(buf.size() / 11);
    return static_cast<int>(VAO);
}

int setupGeometry()
{
    gOwnedTextureIDs.clear();
    gMeshBatches.clear();

    int nVertices = 0;
    string texturePath;
    bool hasTexCoords = false;
    PhongMaterial mat;

    int objVAO = loadSimpleOBJ("../assets/Modelos3D/astora.obj", nVertices, texturePath, hasTexCoords, mat);

    if (objVAO != -1 && nVertices > 0)
    {
        gMeshVertexCount = static_cast<GLsizei>(nVertices);
        return objVAO;
    }

    cerr << "Falha no OBJ. Usando cubo de fallback." << endl;
    gMeshVertexCount = DEFAULT_CUBE_VERTEX_COUNT;

    GLfloat verts[] = {
        -0.5f,-0.5f, 0.5f,1,0,0,  0.5f,-0.5f, 0.5f,1,0,0,  0.5f, 0.5f, 0.5f,1,0,0,
         0.5f, 0.5f, 0.5f,1,0,0, -0.5f, 0.5f, 0.5f,1,0,0, -0.5f,-0.5f, 0.5f,1,0,0,
         0.5f,-0.5f,-0.5f,0,1,0, -0.5f,-0.5f,-0.5f,0,1,0, -0.5f, 0.5f,-0.5f,0,1,0,
        -0.5f, 0.5f,-0.5f,0,1,0,  0.5f, 0.5f,-0.5f,0,1,0,  0.5f,-0.5f,-0.5f,0,1,0,
        -0.5f,-0.5f,-0.5f,0,0,1, -0.5f,-0.5f, 0.5f,0,0,1, -0.5f, 0.5f, 0.5f,0,0,1,
        -0.5f, 0.5f, 0.5f,0,0,1, -0.5f, 0.5f,-0.5f,0,0,1, -0.5f,-0.5f,-0.5f,0,0,1,
         0.5f,-0.5f, 0.5f,1,1,0,  0.5f,-0.5f,-0.5f,1,1,0,  0.5f, 0.5f,-0.5f,1,1,0,
         0.5f, 0.5f,-0.5f,1,1,0,  0.5f, 0.5f, 0.5f,1,1,0,  0.5f,-0.5f, 0.5f,1,1,0,
        -0.5f, 0.5f, 0.5f,1,0,1,  0.5f, 0.5f, 0.5f,1,0,1,  0.5f, 0.5f,-0.5f,1,0,1,
         0.5f, 0.5f,-0.5f,1,0,1, -0.5f, 0.5f,-0.5f,1,0,1, -0.5f, 0.5f, 0.5f,1,0,1,
        -0.5f,-0.5f,-0.5f,0,1,1,  0.5f,-0.5f,-0.5f,0,1,1,  0.5f,-0.5f, 0.5f,0,1,1,
         0.5f,-0.5f, 0.5f,0,1,1, -0.5f,-0.5f, 0.5f,0,1,1, -0.5f,-0.5f,-0.5f,0,1,1
    };

    const size_t stride = 6;
    const size_t count  = sizeof(verts) / sizeof(GLfloat);
    vector<GLfloat> fb;

    for (size_t i = 0; i + 3*stride <= count; i += 3*stride)
    {
        glm::vec3 p0(verts[i], verts[i+1], verts[i+2]);
        glm::vec3 p1(verts[i+stride], verts[i+stride+1], verts[i+stride+2]);
        glm::vec3 p2(verts[i+2*stride], verts[i+2*stride+1], verts[i+2*stride+2]);
        glm::vec3 n(0,0,1);
        glm::vec3 cn = glm::cross(p1-p0, p2-p0);
        if (glm::length(cn) > 1e-6f) n = glm::normalize(cn);

        for (size_t v = 0; v < 3; ++v)
        {
            size_t b = i + v*stride;
            fb.push_back(verts[b]); fb.push_back(verts[b+1]); fb.push_back(verts[b+2]);
            fb.push_back(0); fb.push_back(0);
            fb.push_back(n.x); fb.push_back(n.y); fb.push_back(n.z);
            fb.push_back(verts[b+3]); fb.push_back(verts[b+4]); fb.push_back(verts[b+5]);
        }
    }

    gMeshVertexCount = static_cast<GLsizei>(fb.size() / 11);
    MeshMaterialBatch batch;
    batch.first = 0; batch.count = gMeshVertexCount;
    gMeshBatches.push_back(batch);

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, fb.size()*sizeof(GLfloat), fb.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(5*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(8*sizeof(GLfloat)));
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}