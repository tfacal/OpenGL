// Copyright (C) 2020 Emilio J. Padrón
// Released as Free Software under the X11 License
// https://spdx.org/licenses/X11.html

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

// GLM library to deal with matrix operations
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>               // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::perspective
#include <glm/gtc/type_ptr.hpp>

#include "textfile_ALT.h"

int gl_width = 640;
int gl_height = 480;

void glfw_window_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void render(double, GLuint *vaos[]);
void draw(const GLfloat vertex_positions[], int size, GLuint *vao);

GLuint shader_program = 0;                          // shader program to set render pipeline
GLint model_location, view_location, proj_location; // Uniforms for transformation matrices

GLint material_ambient_location, material_diffuse_location, material_shininess_location, material_specular_location; // Uniforms para material
GLint light_pos_location, light_ambient_location, light_diffuse_location, light_specular_location;                   // Uniforms para la luz
GLint light_pos_location_second, light_ambient_location_second, light_diffuse_location_second, light_specular_location_second;                   // Uniforms para la luz
GLint normal_to_world_location;                                                                                      // Uniform normal matrix
GLint view_pos_location;                                                                                             // Posicion camara

// Shader names
const char *vertexFileName = "spinningcube_withlight_vs.glsl";
const char *fragmentFileName = "spinningcube_withlight_fs.glsl";

// Camera
glm::vec3 camera_pos(0.0f, 0.0f, 3.0f);
glm::vec3 pos_pyramid(1.0f, 0.0f, 0.0f);

// Lighting
glm::vec3 light_pos(0.0f, 0.0f, 2.0f);
glm::vec3 light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse(0.5f, 0.5f, 0.5f);
glm::vec3 light_specular(1.0f, 1.0f, 1.0f);

glm::vec3 light_pos_second(0.0f, 0.0f, 2.0f);
glm::vec3 light_ambient_second(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse_second(0.5f, 0.5f, 0.5f);
glm::vec3 light_specular_second(1.0f, 1.0f, 1.0f);

// Material
glm::vec3 material_ambient(1.0f, 0.5f, 0.31f);
glm::vec3 material_diffuse(1.0f, 0.5f, 0.31f);
glm::vec3 material_specular(0.5f, 0.5f, 0.5f);
const GLfloat material_shininess = 32.0f;

int main()
{
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit())
  {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(gl_width, gl_height, "My spinning cube", NULL, NULL);
  if (!window)
  {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwSetWindowSizeCallback(window, glfw_window_size_callback);
  glfwMakeContextCurrent(window);

  // start GLEW extension handler
  // glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte *vendor = glGetString(GL_VENDOR);                        // get vendor string
  const GLubyte *renderer = glGetString(GL_RENDERER);                    // get renderer string
  const GLubyte *glversion = glGetString(GL_VERSION);                    // version as a string
  const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION); // version as a string
  printf("Vendor: %s\n", vendor);
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", glversion);
  printf("GLSL version supported %s\n", glslversion);
  printf("Starting viewport: (width: %d, height: %d)\n", gl_width, gl_height);

  // Enable Depth test: only draw onto a pixel if fragment closer to viewer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // set a smaller value as "closer"

  // Vertex Shader
  char *vertex_shader = textFileRead(vertexFileName);

  // Fragment Shader
  char *fragment_shader = textFileRead(fragmentFileName);

  // Shaders compilation
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  free(vertex_shader);
  glCompileShader(vs);

  int success;
  char infoLog[512];
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vs, 512, NULL, infoLog);
    printf("ERROR: Vertex Shader compilation failed!\n%s\n", infoLog);

    return (1);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  free(fragment_shader);
  glCompileShader(fs);

  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fs, 512, NULL, infoLog);
    printf("ERROR: Fragment Shader compilation failed!\n%s\n", infoLog);

    return (1);
  }

  // Create program, attach shaders to it and link it
  shader_program = glCreateProgram();
  glAttachShader(shader_program, fs);
  glAttachShader(shader_program, vs);
  glLinkProgram(shader_program);

  glValidateProgram(shader_program);
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
    printf("ERROR: Shader Program linking failed!\n%s\n", infoLog);

    return (1);
  }

  // Release shader objects
  glDeleteShader(vs);
  glDeleteShader(fs);

  // Cube to be rendered
  //
  //          0        3
  //       7        4 <-- top-right-near
  // bottom
  // left
  // far ---> 1        2
  //       6        5
  //
  const GLfloat vertex_positions[] = {
      -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, // 1
      -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f,  // 0
      0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f,  // 2

      0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f,  // 3
      0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, // 2
      -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, // 0

      0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f, // 2
      0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f,  // 3
      0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f,  // 5

      0.25f, 0.25f, 0.25f, 1.0f, 0.0f, 0.0f,  // 4
      0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, // 5
      0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, // 3

      0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f,  // 5
      0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f,   // 4
      -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, // 6

      -0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f,  // 7
      -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, // 6
      0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f,   // 4

      -0.25f, -0.25f, 0.25f, -1.0f, 0.0f, 0.0f,  // 6
      -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f,   // 7
      -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, // 1

      -0.25f, 0.25f, -0.25f, -1.0f, 0.0f, 0.0f,  // 0
      -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, // 1
      -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f,   // 7

      0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f,  // 2
      0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f,   // 5
      -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, // 1

      -0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f,  // 6
      -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, // 1
      0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f,   // 5

      0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f,  // 4
      0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, // 3
      -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, // 7

      -0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, // 0
      -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f,  // 7
      0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f   // 3
  };

const GLfloat vertex_positions_pyramid[] = {
     // Base - Triángulo frontal
     0.0f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.0f,  // Vértice 0
     -0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // Vértice 1
     0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // Vértice 2

     // Triángulo frontal derecho
     0.0f, -0.25f, 0.0f, 1.0f, -1.0f, 0.0f, 0.5f, 0.0f,  // Vértice 0
     0.25f, -0.25f, 0.25f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // Vértice 2
     0.0f, 0.25f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // Vértice 3

     // Triángulo frontal izquierdo
     0.0f, -0.25f, 0.0f, -1.0f, -1.0f, 0.0f, 0.5f, 0.0f,  // Vértice 0
     -0.25f, -0.25f, 0.25f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // Vértice 1
     0.0f, 0.25f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,  // Vértice 3

     // Triángulo trasero
     -0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f,  // Vértice 1
     0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 1.0f, 1.0f, 0.0f,  // Vértice 2
     0.0f, 0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.5f, 1.0f,  // Vértice 3
 };

  // Vertex Array Object
  GLuint vao_cube;
  GLuint vao_pyramid;
  
  //draw cube
  draw(vertex_positions, sizeof(vertex_positions), &vao_cube);

  //draw pyramid
  draw(vertex_positions_pyramid, sizeof(vertex_positions_pyramid), &vao_pyramid);

  GLuint *vaos[] = {&vao_cube, &vao_pyramid};

  // Unbind vbo (it was conveniently registered by VertexAttribPointer)
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 1);

  // Unbind vao
  glBindVertexArray(0);

  // Uniforms
  // - Model matrix
  // - View matrix
  // - Projection matrix
  // - Normal matrix: normal vectors from local to world coordinates
  // - Camera position
  // - Light data
  // - Material data
  model_location = glGetUniformLocation(shader_program, "model");
  view_location = glGetUniformLocation(shader_program, "view");
  proj_location = glGetUniformLocation(shader_program, "projection");
  normal_to_world_location = glGetUniformLocation(shader_program, "normal_to_world");
  // [...]
  view_pos_location = glGetUniformLocation(shader_program, "view_pos");

  // light components
  light_pos_location = glGetUniformLocation(shader_program, "light.position");
  light_ambient_location = glGetUniformLocation(shader_program, "light.ambient");
  light_diffuse_location = glGetUniformLocation(shader_program, "light.diffuse");
  light_specular_location = glGetUniformLocation(shader_program, "light.specular");

  light_pos_location_second = glGetUniformLocation(shader_program, "light_second.position");
  light_ambient_location_second = glGetUniformLocation(shader_program, "light_second.ambient");
  light_diffuse_location_second = glGetUniformLocation(shader_program, "light_second.diffuse");
  light_specular_location_second = glGetUniformLocation(shader_program, "light_second.specular");

  // material components
  material_ambient_location = glGetUniformLocation(shader_program, "material.ambient");
  material_diffuse_location = glGetUniformLocation(shader_program, "material.diffuse");
  material_specular_location = glGetUniformLocation(shader_program, "material.specular");
  material_shininess_location = glGetUniformLocation(shader_program, "material.shininess");
    
  // Render loop
  while (!glfwWindowShouldClose(window))
  {

    processInput(window);

    render(glfwGetTime(), vaos);

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}

void draw(const GLfloat vertex_positions[], int size, GLuint *vao)
{
  // Vertex Array Object
  glGenVertexArrays(1, vao);
  glBindVertexArray(*vao);

  // Vertex Buffer Object (for vertex coordinates)
  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, vertex_positions, GL_STATIC_DRAW);

  // Vertex attributes
  // 0: vertex position (x, y, z)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)0);
  glEnableVertexAttribArray(0);

  // 1: vertex normals (x, y, z)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void *)(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);

  // Unbind vbo (it was conveniently registered by VertexAttribPointer)
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 1);

  // Unbind vao
  glBindVertexArray(0);
}

void render(double currentTime, GLuint *vaos[])
{
  float f = (float)currentTime * 0.3f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glViewport(0, 0, gl_width, gl_height);

  glUseProgram(shader_program);
  glBindVertexArray(*vaos[0]);

  glm::mat4 model_matrix, view_matrix, proj_matrix;
  glm::mat3 normal_matrix;

  model_matrix = glm::mat4(1.f);
  //model_matrix = glm::scale(model_matrix, glm::vec3(0.6f));
  view_matrix = glm::lookAt(camera_pos,                   // pos
                            glm::vec3(0.0f, 0.0f, 0.0f),  // target
                            glm::vec3(0.0f, 1.0f, 0.0f)); // up

  // model_matrix = glm::translate(model_matrix, glm::vec3(0.0f, 0.0f, 0.0f));

  // Moving cube
  // model_matrix = glm::rotate(model_matrix,
  //   [...]
  //
  model_matrix = glm::rotate(model_matrix,
                             glm::radians((float)currentTime * 45.0f),
                             glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix,
                             glm::radians((float)currentTime * 81.0f),
                             glm::vec3(1.0f, 0.0f, 0.0f));
  // Projection
  // proj_matrix = glm::perspective(glm::radians(50.0f),
  //   [...]
  proj_matrix = glm::perspective(glm::radians(50.0f),
                                 (float)gl_width / (float)gl_height,
                                 0.1f, 100.0f);

  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view_matrix));
  glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(proj_matrix));

  glUniform3fv(view_pos_location, 1, glm::value_ptr(camera_pos));

  //
  // Normal matrix: normal vectors to world coordinates
  normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
  glUniformMatrix3fv(normal_to_world_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

  glUniform3fv(material_ambient_location, 1, glm::value_ptr(material_ambient));
  glUniform3fv(material_diffuse_location, 1, glm::value_ptr(material_diffuse));
  glUniform3fv(material_specular_location, 1, glm::value_ptr(material_specular));
  glUniform1f(material_shininess_location, material_shininess);

  glUniform3fv(light_pos_location, 1, glm::value_ptr(light_pos));
  glUniform3fv(light_ambient_location, 1, glm::value_ptr(light_ambient));
  glUniform3fv(light_diffuse_location, 1, glm::value_ptr(light_diffuse));
  glUniform3fv(light_specular_location, 1, glm::value_ptr(light_specular));

  glUniform3fv(light_pos_location_second, 1, glm::value_ptr(light_pos_second));
  glUniform3fv(light_ambient_location_second, 1, glm::value_ptr(light_ambient_second));
  glUniform3fv(light_diffuse_location_second, 1, glm::value_ptr(light_diffuse_second));
  glUniform3fv(light_specular_location_second, 1, glm::value_ptr(light_specular_second));

  glDrawArrays(GL_TRIANGLES, 0, 36);

  glBindVertexArray(*vaos[1]);

  // Dibujamos piramide
  model_matrix = glm::mat4(1.f);
  model_matrix = glm::translate(model_matrix, pos_pyramid);
  //model_matrix = glm::scale(model_matrix, glm::vec3(0.5f));

  model_matrix = glm::rotate(model_matrix,
                          glm::radians((float)currentTime * 45.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix,
                          glm::radians((float)currentTime * 81.0f),
                          glm::vec3(1.0f, 0.0f, 0.0f));

  // Normal matrix: normal vectors to world coordinates
  normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  glUniformMatrix3fv(normal_to_world_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

  glDrawArrays(GL_TRIANGLES, 0, 15);
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

// Callback function to track window size and update viewport
void glfw_window_size_callback(GLFWwindow *window, int width, int height)
{
  gl_width = width;
  gl_height = height;
  printf("New viewport: (width: %d, height: %d)\n", width, height);
}