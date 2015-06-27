#pragma once

#include <GL/glew.h>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <string>
#include <fstream>


class Shader {
public:

    Shader();
    Shader(Shader const &_shader);
    Shader(std::string _vertexSource, std::string _fragmentSource);
    ~Shader();

    Shader& operator=(Shader const &_shader);

    bool load();
    bool compileShader(GLuint &shader, GLenum type, std::string const &fichierSource);
    inline GLuint getProgramID() const {return programID;}


private:

    GLuint vertexID;
    GLuint fragmentID;
    GLuint programID;

    std::string vertexSource;
    std::string fragmentSource;
};

