#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

namespace Graphics 
{
struct 
{
    GLint width = 0, height = 0;
    GLint tex_width = 0, tex_height = 0;
    GLFWwindow* window = nullptr;
    GLuint shaderProgram = 0;
    GLuint tex = 0;

    const GLfloat square_coords[12]
    {
        0, 0, 
        1, 0, 
        0, 1,

        1, 0, 
        1, 1, 
        0, 1
    };
} g_attrs;

GLuint loadInShader(GLenum const &shaderType, char const *fname) {
   std::vector<char> buffer;
   std::ifstream in;
   in.open(fname, std::ios::binary);

   if (in.is_open()) {
      in.seekg(0, std::ios::end);
      size_t const &length = in.tellg();

      in.seekg(0, std::ios::beg);

      buffer.resize(length + 1);
      in.read(&buffer[0], length);
      in.close();
      buffer[length] = '\0';
   } else {
      std::cerr << "Unable to open " << fname << std::endl;
      exit(-1);
   }

   GLchar const *src = &buffer[0];

   GLuint shader = glCreateShader(shaderType);
   glShaderSource(shader, 1, &src, NULL);
   glCompileShader(shader);
   GLint test;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &test);

   if (!test) {
      std::cerr << "Shader compilation failed with this message:" << std::endl;
      std::vector<char> compilationLog(512);
      glGetShaderInfoLog(shader, compilationLog.size(), NULL, &compilationLog[0]);
      std::cerr << &compilationLog[0] << std::endl;
      glfwTerminate();
      exit(-1);
   }

   return shader;
}

void initShaders (char const* vertLoc, char const* fragLoc)
{   
    g_attrs.shaderProgram = glCreateProgram();

    auto vertShader = loadInShader(GL_VERTEX_SHADER, vertLoc);
    auto fragShader = loadInShader(GL_FRAGMENT_SHADER, fragLoc);

    glAttachShader(g_attrs.shaderProgram, vertShader);
    glAttachShader(g_attrs.shaderProgram, fragShader);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    glLinkProgram(g_attrs.shaderProgram);
    glUseProgram(g_attrs.shaderProgram);
}

int setup (int const width, int const height, int const tex_width, int const tex_height)
{
    g_attrs.width = width; 
    g_attrs.height = height;
    g_attrs.tex_width = tex_width; 
    g_attrs.tex_height = tex_height;
    if(!glfwInit()) 
    {
        std::cerr << "Failed to init GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    g_attrs.window = glfwCreateWindow(g_attrs.width, g_attrs.height, "dust!", NULL, NULL);
    if(!g_attrs.window) 
    {
        std::cerr << "Failed to initialize window" << std::endl;
        return 1;
    }
    glfwMakeContextCurrent(g_attrs.window);

    glewExperimental = GL_TRUE;
    if(glewInit() != 0) 
    {
        std::cerr << "Failed to initialize glew" << std::endl;
        exit(1);
    }
    // We get an INVALID_ENUM error from glew sometimes.
    // It's not a problem, just wipe the error checking:
    glGetError(); 

    // Setup shaders
    initShaders("Shaders/vert.glsl", "Shaders/frag.glsl");

    // Setup 2d texture
    glGenTextures( 1, &g_attrs.tex );
    glBindTexture( GL_TEXTURE_2D, g_attrs.tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    // Send square verts to buffer
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint buff;
    glGenBuffers(1, &buff);
    glBindBuffer(GL_ARRAY_BUFFER, buff);
    glBufferData(GL_ARRAY_BUFFER,
            12*sizeof(GLfloat), g_attrs.square_coords, GL_STATIC_DRAW);

    GLint tex_coord_loc{glGetAttribLocation(g_attrs.shaderProgram, "tex_coord")};
    glVertexAttribPointer(tex_coord_loc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(tex_coord_loc);

//    glCullFace(GL_BACK);

    return 0;
}

void draw (float const* frame)
{
    GLfloat const color [4] {1.0f, 0.0f, 0.0f, 1.0f};
    glClearBufferfv(GL_COLOR, 0.0f, color);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RED, g_attrs.tex_width, g_attrs.tex_height, 0, GL_RED, GL_FLOAT, frame );

    glDrawArrays(GL_TRIANGLES, 0, 6); 
    glfwPollEvents();
    glfwSwapBuffers(g_attrs.window);
}

int get_width () {return g_attrs.width;}
int get_height () {return g_attrs.height;}
};
