#include "shader.h"

#include "utils.h"

#include <SDL_log.h>
#include <cassert>

Shader::Shader() :
	_vertexID(0),
	_fragmentID(0),
	_programID(0) {}

Shader::~Shader() {
	glDeleteShader(_vertexID);
	glDeleteShader(_fragmentID);
  glDeleteProgram(_programID);
}

Shader::Shader(Shader&& other) noexcept :
	_vertexID(other._vertexID),
	_fragmentID(other._fragmentID),
	_programID(other._programID) {

	other._vertexID = 0;
	other._fragmentID = 0;
	other._programID = 0;
}

bool Shader::load(std::string vertexSource, std::string fragmentSource) {
  if(glIsShader(_vertexID) == GL_TRUE)
  	glDeleteShader(_vertexID);

  if(glIsShader(_fragmentID) == GL_TRUE)
    glDeleteShader(_fragmentID);

  if(glIsProgram(_programID) == GL_TRUE)
    glDeleteProgram(_programID);


  if(!compileShader(_vertexID, GL_VERTEX_SHADER, vertexSource))
    return false;

  if(!compileShader(_fragmentID, GL_FRAGMENT_SHADER, fragmentSource))
    return false;

  _programID = glCreateProgram();

  glAttachShader(_programID, _vertexID);
  glAttachShader(_programID, _fragmentID);

  glLinkProgram(_programID);

  GLint errorLink(0);
  glGetProgramiv(_programID, GL_LINK_STATUS, &errorLink);

  if(errorLink != GL_TRUE) {
    GLint sizeError(0);
    glGetProgramiv(_programID, GL_INFO_LOG_LENGTH, &sizeError);

    char *error = new char[sizeError + 1];

    glGetShaderInfoLog(_programID, sizeError, &sizeError, error);
    error[sizeError] = '\0';

    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "(%s,%s): Error in shader linking: %s",
			vertexSource.c_str(), fragmentSource.c_str(), error);

    delete[] error;
   	glDeleteProgram(_programID);

    return false;
  }

	return true;
}


bool Shader::compileShader(GLuint &shader, GLenum type, std::string const &source) {

  shader = glCreateShader(type);

  if(shader == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: could not create shader %s", source.c_str());
    return false;
  }

  std::string sourceCode = ut::textFileToString(source);

  const GLchar* str = sourceCode.c_str();

#ifdef GL_ES_VERSION_3_0
	const char *sources[2] = {"#version 300 es\nprecision highp float;\nprecision mediump sampler2DArray;\n", str };
#else
	const char *sources[2] = { "#version 330\n", str };
#endif

	glShaderSource(shader, 2, sources, 0);

  glCompileShader(shader);

  GLint compilationError(0);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compilationError);

  if(compilationError != GL_TRUE) {
    GLint errorLength(0);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &errorLength);

    char *error = new char[errorLength + 1];

    glGetShaderInfoLog(shader, errorLength, &errorLength, error);
    error[errorLength] = '\0';

    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: Error in shader compilation: %s", source.c_str(), error);

    delete[] error;
		glDeleteShader(shader);

		assert(false);

    return false;
  }

	else
    return true;
}
