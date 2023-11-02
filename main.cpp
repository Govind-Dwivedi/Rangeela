#include <iostream>
#include <math.h>
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <common/loadShader.hpp>

bool firstmouse = true, surveyMode = false, translateMode = false;
double lastX, lastY;
float horizontalAngle = 3.14f, verticaleAngle = 0.0f;
float width = 1200, height = 700;
int max_vertices = 100;
bool isVertexSelected[100] = {false};
glm::vec3 avg_normal = glm::vec3(0.0f, 0.0f, 0.0f);
int noOfVertices = 8, noOfSelectedVertices = 0, connection[100][100];

glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
glm::mat4 View = glm::lookAt(
    glm::vec3(4, 3, -3),
    glm::vec3(0, 0, 0),
    glm::vec3(0, 1, 0));
glm::mat4 Model = glm::mat4(1.0f);

static GLfloat g_vertex_buffer_data[300];
GLfloat cube_vertices[] = {
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, 0.5f
};

static GLfloat g_color_buffer_data[300];
GLfloat cube_color[] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
};

GLuint vertexbuffer, colorbuffer, programID, MatrixID, lineIndicesbuffer, indexbuffer, line_indices;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        surveyMode = !surveyMode;
        if (surveyMode)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            translateMode = false;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstmouse = true;
        }
    }
    else if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        if (!translateMode)
        {
            surveyMode = false;
            firstmouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        translateMode = !translateMode;
    }
}

void changeColor(int vertex)
{
    int v = vertex * 3;
    if (isVertexSelected[vertex])
    {
        g_color_buffer_data[v] = 1.0f;
        g_color_buffer_data[v + 1] = 0.0f;
        g_color_buffer_data[v + 2] = 0.0f;
    }
    else
    {
        g_color_buffer_data[v] = 0.0f;
        g_color_buffer_data[v + 1] = 0.0f;
        g_color_buffer_data[v + 2] = 0.0f;
    }
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
}

void updateAvgNormal(int vertex){
    if(isVertexSelected[vertex]){
        avg_normal.x = avg_normal.x*noOfSelectedVertices + g_vertex_buffer_data[3 * vertex];
        avg_normal.y = avg_normal.y*noOfSelectedVertices + g_vertex_buffer_data[3 * vertex + 1];
        avg_normal.z = avg_normal.z*noOfSelectedVertices + g_vertex_buffer_data[3 * vertex + 2];
        noOfSelectedVertices++;
    }
    else{
        avg_normal.x = avg_normal.x*noOfSelectedVertices - g_vertex_buffer_data[3 * vertex];
        avg_normal.y = avg_normal.y*noOfSelectedVertices - g_vertex_buffer_data[3 * vertex + 1];
        avg_normal.z = avg_normal.z*noOfSelectedVertices - g_vertex_buffer_data[3 * vertex + 2];
        noOfSelectedVertices--;
    }
    if(noOfSelectedVertices != 0){
        avg_normal.x = avg_normal.x/noOfSelectedVertices;
        avg_normal.y = avg_normal.y/noOfSelectedVertices;
        avg_normal.z = avg_normal.z/noOfSelectedVertices;
    }
    else{
        avg_normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }
    avg_normal = glm::normalize(avg_normal);
}

GLint findClosestVertexIntersection(const glm::vec3 &rayStart, const glm::vec3 &rayDir)
{
    GLint closestVertex = -1; // Initialize to an invalid value
    float minDistance = std::numeric_limits<float>::max();

    for (GLuint i = 0; i < sizeof(g_vertex_buffer_data); i += 3)
    {
        glm::vec3 vertex(g_vertex_buffer_data[i], g_vertex_buffer_data[i + 1], g_vertex_buffer_data[i + 2]);
        // Calculate the vector from the ray start to the vertex
        glm::vec3 rayToVertex = vertex - rayStart;
        // Calculate the distance along the ray direction to the intersection point
        float t = glm::dot(rayToVertex, rayDir);
        // Ensure the intersection point is in front of the ray
        if (t > 0.0f)
        {
            // Calculate the point of intersection on the ray
            glm::vec3 intersectionPoint = rayStart + t * rayDir;
            // Calculate the distance between the intersection point and the vertex
            float distance = glm::distance(vertex, intersectionPoint);
            // Update closestVertex and minDistance if this is the closest vertex
            if (distance < minDistance && distance <= 0.05f)
            {
                minDistance = distance;
                closestVertex = i / 3;
            }
        }
    }
    return closestVertex;
}
// Function to perform picking
void performPicking(int x, int y)
{
    // Get the mouse cursor's normalized device coordinates (NDC)
    int screenWidth, screenHeight;
    glfwGetWindowSize(glfwGetCurrentContext(), &screenWidth, &screenHeight);
    float mouseX = static_cast<float>(x) / static_cast<float>(screenWidth);
    float mouseY = 1.0f - static_cast<float>(y) / static_cast<float>(screenHeight); // Inverted Y-axis

    // Convert NDC to view space
    glm::vec4 rayStart_NDC(mouseX * 2.0f - 1.0f, mouseY * 2.0f - 1.0f, -1.0f, 1.0f);
    glm::vec4 rayEnd_NDC(mouseX * 2.0f - 1.0f, mouseY * 2.0f - 1.0f, 0.0f, 1.0f);

    glm::mat4 inverseProjectionMatrix = glm::inverse(Projection);
    glm::mat4 inverseViewMatrix = glm::inverse(View);

    glm::vec4 rayStart_World = inverseProjectionMatrix * rayStart_NDC;
    rayStart_World /= rayStart_World.w;
    glm::vec4 rayEnd_World = inverseProjectionMatrix * rayEnd_NDC;
    rayEnd_World /= rayEnd_World.w;

    // Convert ray to view space
    rayStart_World = inverseViewMatrix * rayStart_World;
    rayStart_World /= rayStart_World.w;
    rayEnd_World = inverseViewMatrix * rayEnd_World;
    rayEnd_World /= rayEnd_World.w;

    // Calculate the ray direction
    glm::vec3 rayDir_World(rayEnd_World - rayStart_World);
    rayDir_World = glm::normalize(rayDir_World);

    // Check for intersection with your model's vertices
    GLint selectedVertex = findClosestVertexIntersection(rayStart_World, rayDir_World);
    if (selectedVertex != -1)
    {
        isVertexSelected[selectedVertex] = !isVertexSelected[selectedVertex];
        updateAvgNormal(selectedVertex);
        changeColor(selectedVertex);
    }
}

glm::vec3 modelToScreen(glm::vec3 pos){
    glm::vec4 position = glm::vec4(pos, 1.0f);
    glm::mat4 mvp = Projection * View * Model;
    glm::vec4 clip = mvp * position;
    glm::vec4 ndc = clip / clip.w;
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glm::vec3 screen;
    screen.x = (ndc.x * 0.5 + 0.5) * viewport[2] + viewport[0];
    screen.y = (ndc.y * 0.5 + 0.5) * viewport[3] + viewport[1];
    screen.z = (ndc.z + 1) * 0.5;

    return screen;
}

glm::vec3 screenToModel(glm::vec3 screen, float w){
    glm::vec4 position, clip, ndc;
    glm::vec3 pos;
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    ndc.x = 2.0f * (screen.x - viewport[0]) / viewport[2] - 1.0f;
    // ndc.y = 1.0f - 2.0f*(screen.y - viewport[1])/viewport[3];
    ndc.y = 2.0f * (screen.y - viewport[1]) / viewport[3] - 1.0f;
    ndc.z = 2.0f * screen.z - 1.0f;

    clip = ndc * w;
    glm::mat4 inverseProjection = glm::inverse(Projection);
    glm::mat4 inverseView = glm::inverse(View);

    position = inverseProjection * clip;
    position = inverseView * position;

    pos.x = position.x;
    pos.y = position.y;
    pos.z = position.z;
    return pos;
}

glm::vec3 updatePos(int x, int y, glm::vec3 pos){
    glm::vec4 position = glm::vec4(pos, 1.0f);
    glm::mat4 mvp = Projection * View * Model;
    glm::vec4 clip = mvp * position;

    glm::vec4 ndc = clip / clip.w;
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    glm::vec3 screen;
    screen.x = (ndc.x * 0.5 + 0.5) * viewport[2] + viewport[0];
    screen.y = (ndc.y * 0.5 + 0.5) * viewport[3] + viewport[1];
    screen.z = (ndc.z + 1) * 0.5;
    
    // glm::vec3 screen1 = modelToScreen(pos);
    // std::cout<<"Screen inline = "<<glm::to_string(screen)<<std::endl;
    // std::cout<<"Screen by funtion = "<<glm::to_string(screen1)<<std::endl;

    screen.x = screen.x + x;
    screen.y = screen.y - y;
    // glm::vec3 pos1 = screenToModel(screen, clip.w);

    ndc.x = 2.0f * (screen.x - viewport[0]) / viewport[2] - 1.0f;
    // ndc.y = 1.0f - 2.0f*(screen.y - viewport[1])/viewport[3];
    ndc.y = 2.0f * (screen.y - viewport[1]) / viewport[3] - 1.0f;
    ndc.z = 2.0f * screen.z - 1.0f;

    clip = ndc * clip.w;
    glm::mat4 inverseProjection = glm::inverse(Projection);
    glm::mat4 inverseView = glm::inverse(View);

    position = inverseProjection * clip;
    position = inverseView * position;

    pos.x = position.x;
    pos.y = position.y;
    pos.z = position.z;
    // std::cout<<"Pos inline = "<<glm::to_string(pos)<<std::endl;
    // std::cout<<"Pos by funtion = "<<glm::to_string(pos1)<<std::endl;
    
    return pos;
}

void translate(int x, int y)
{
    int n = 8;
    for (int i = 0; i < n; i++)
    {
        if (isVertexSelected[i])
        {
            glm::vec3 vertex = glm::vec3(g_vertex_buffer_data[3 * i], g_vertex_buffer_data[3 * i + 1], g_vertex_buffer_data[3 * i + 2]);
            vertex = updatePos(x, y, vertex);
            g_vertex_buffer_data[3 * i] = vertex.x;
            g_vertex_buffer_data[3 * i + 1] = vertex.y;
            g_vertex_buffer_data[3 * i + 2] = vertex.z;
            updateAvgNormal(i);
        }
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstmouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstmouse = false;
    }
    double diffX = xpos - lastX;
    double diffY = ypos - lastY;
    lastX = xpos;
    lastY = ypos;

    if (surveyMode)
    {
        float sensitivity = 0.2f;

        horizontalAngle += diffX * sensitivity;
        verticaleAngle += diffY * sensitivity;

        glm::vec3 position;
        position.x = 6.0 * cos(glm::radians(verticaleAngle)) * cos(glm::radians(horizontalAngle));
        position.y = 6.0 * sin(glm::radians(verticaleAngle));
        position.z = 6.0 * cos(glm::radians(verticaleAngle)) * sin(glm::radians(horizontalAngle));
        // position = glm::normalize(position);
        View = glm::lookAt(position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    }
    else if (translateMode)
    {
        translate(diffX, diffY);
    }
}

void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        performPicking(static_cast<int>(xpos), static_cast<int>(ypos));
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_REPEAT)
    {
        surveyMode = true;
        if (surveyMode)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            mouse_callback(window, xpos, ypos);
        }
        else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            surveyMode = false;
            if (surveyMode)
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            else
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

glm::vec3 getCameraPosition()

{
    glm::mat4 inverseView = glm::inverse(View);
    glm::vec3 position = glm::vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);
    return position;
}

int main()
{
    glewExperimental = true; // Needed for core profile
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window;
    window = glfwCreateWindow(width, height, "Cube", NULL, NULL);
    if (window == NULL)
    {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, key_callback);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLfloat cube_vertices[] = {
    0.5f, 0.5f, 0.5f,
    0.5f, 0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, 0.5f,
    -0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, 0.5f
    };

    GLfloat cube_color[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
    };

    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        3, 4, 7,
        4, 5, 6,
        4, 6, 7,
        1, 2, 5,
        2, 5, 6,
        0, 1, 4,
        1, 4, 5,
        2, 3, 6,
        3, 6, 7
    };

    unsigned int line_indices[] = {
        0, 1,
        1, 2,
        2, 3,
        0, 3,
        0, 4,
        4, 7,
        3, 7,
        4, 5,
        5, 6,
        6, 7,
        1, 5,
        2, 6
    };

    for(int i=0; i<24; i++){
        g_vertex_buffer_data[i] = cube_vertices[i];
        g_color_buffer_data[i] = cube_color[i];
    }

    for(int i=0; i<max_vertices; i++){
        for(int j=0; j<max_vertices; j++){
            connection[i][j] = -1;
        }
    }

    for(int i=0, size=sizeof(line_indices)/sizeof(int); i<size; i+=2){
        for(int j=0; j<max_vertices; j++){
            if(connection[line_indices[i]][j] == -1){
                connection[line_indices[i]][j] = line_indices[i+1];
                break;
            }
        }
        for(int j=0; j<max_vertices; j++){
            if(connection[line_indices[i+1]][j] == -1){
                connection[line_indices[i+1]][j] = line_indices[i];
                break;
            }
        }
    }

    int grid_end = 50, grid_stride = 1;
    int grid_vert_2sides = 4*(grid_end/grid_stride) + 2;             //grid_vert_2sides variable contains number od vertices in grid on two sides on front and back or laft side and right side
    int grid_array_len = grid_vert_2sides*6;
    static GLfloat gridBufferVertex[1212];

    for (int i=0; i<grid_vert_2sides; i++)
    {
        gridBufferVertex[3 * i] = -grid_end + grid_stride * (i / 2);
        gridBufferVertex[3 * i + 1] = 0;
        gridBufferVertex[3 * i + 2] = (1 - 2 * (i % 2)) * grid_end;
    }

    for (int i=0; i<grid_vert_2sides; i++)
    {
        gridBufferVertex[3*grid_vert_2sides + 3*i] = (1 - 2 * (i % 2)) * grid_end;
        gridBufferVertex[3*grid_vert_2sides + 3*i + 1] = 0;
        gridBufferVertex[3*grid_vert_2sides + 3*i + 2] = -grid_end + grid_stride * (i / 2);
    }

    static GLfloat gridColor[1212];
    for (int i = 0; i < grid_vert_2sides*2; i++){
        gridColor[3 * i] = 0.2f;
        gridColor[3 * i + 1] = 0.2f;
        gridColor[3 * i + 2] = 0.2f;
    }

    {
        int grid_center = 2*grid_end/grid_stride;
        for(int i=0; i<6; i++){
            if(i!=0 && i!=3){
                gridColor[3*grid_center+i] = 0.3f;
                gridColor[3*grid_vert_2sides + 3*grid_center + i] = 0.3f;
            }
            else{
                gridColor[3*grid_center+i] = 0.8f;
                gridColor[3*grid_vert_2sides + 3*grid_center + i] = 0.8f;
            }
        }
    }

    static GLfloat planeBufferColor[24];
    for (int i = 0; i < 24; i++)
        planeBufferColor[i] = 0.8f;

    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    GLuint indexbuffer;
    glGenBuffers(1, &indexbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLuint lineIndicesbuffer;
    glGenBuffers(1, &lineIndicesbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndicesbuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(line_indices), line_indices, GL_STATIC_DRAW);

    GLuint planeBuffer;
    glGenBuffers(1, &planeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, planeBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeBufferColor), planeBufferColor, GL_STATIC_DRAW);

    GLuint gridBuffer;
    glGenBuffers(1, &gridBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, gridBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridBufferVertex), gridBufferVertex, GL_STATIC_DRAW);

    GLuint gridColorBuffer;
    glGenBuffers(1, &gridColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, gridColorBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridColor), gridColor, GL_STATIC_DRAW);

    // GLuint gridProgram = LoadShaders("SimpleVertexShader.vertexshader", "gridFragShader.fragmentshader");
    GLuint programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");
    // GLuint CameraID = glGetUniformLocation(gridProgram, "camera");
    GLuint ModelViewID = glGetUniformLocation(programID, "modelView");
    GLuint ProjectionID = glGetUniformLocation(programID, "projection");
    
    //Model = glm::translate(Model, glm::vec3(1,1,1));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glViewport(0, 0, width, height);

    double lastTime = glfwGetTime();
    int nbFrames = 0;

    do
    {
        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0)
        {
            // std::cout<<1000.0/double(nbFrames)<<"ms/frame\t"<<double(nbFrames)<<"FPS\n";
            nbFrames = 0;
            lastTime += 1.0;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programID);
        // glm::vec3 cameraPos = getCameraPosition();
        //  std::cout<<"Camera = "<<glm::to_string(cameraPos)<<"\n";
        // glUniform3f(CameraID, cameraPos.x, cameraPos.y, cameraPos.z);

        // This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
        glm::mat4 modelViewMat = View * Model;
        glUniformMatrix4fv(ModelViewID, 1, GL_FALSE, &modelViewMat[0][0]);
        glUniformMatrix4fv(ProjectionID, 1, GL_FALSE, &Projection[0][0]);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, gridBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, gridColorBuffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glEnableVertexAttribArray(1);
        
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_LINES, 0, grid_array_len);

       // glUseProgram(programID);
 
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glBindBuffer(GL_ARRAY_BUFFER, planeBuffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexbuffer);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, 12 * 3, GL_UNSIGNED_INT, 0);

        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndicesbuffer);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_LINES, sizeof(line_indices) / sizeof(line_indices[0]), GL_UNSIGNED_INT, 0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        glfwSwapBuffers(window);
        glfwPollEvents();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteBuffers(1, &gridBuffer);
    glDeleteBuffers(1, &gridColorBuffer);
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &indexbuffer);
    glDeleteBuffers(1, &lineIndicesbuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteProgram(programID);
    glfwTerminate();
    return 0;
}