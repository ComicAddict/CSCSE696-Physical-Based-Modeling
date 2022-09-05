#include "shader.h"

Shader::Shader(const char* vertexPath, const char* fragPath) {

	std::string vertexCode;
	std::string fragCode;
	std::ifstream vertexShaderFile;
	std::ifstream fragShaderFile;
    vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		vertexShaderFile.open(vertexPath);
		fragShaderFile.open(fragPath);
		std::stringstream vertexShaderStream, fragShaderStream;

		vertexShaderStream << vertexShaderFile.rdbuf();
		fragShaderStream << fragShaderFile.rdbuf();

		vertexShaderFile.close();
		fragShaderFile.close();

		vertexCode = vertexShaderStream.str();
		fragCode = fragShaderStream.str();
	}
	catch(std::ifstream::failure e){
		std::cout << "ERROR: Shader File Reading Failed" << std::endl;
	}

	const char* vertexShaderCode = vertexCode.c_str();
	const char* fragShaderCode = fragCode.c_str();

	unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
    glCompileShader(vertexShader);

    int success;
    char errLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, errLog);
        std::cout << "ERROR: Shader Vertex Compilation Failed\n" << errLog << std::endl;
    }

    unsigned int fragShader;
    fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragShaderCode, NULL);
    glCompileShader(fragShader);

    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragShader, 512, NULL, errLog);
        std::cout << "ERROR: Shader Fragment Compilation Failed\n" << errLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragShader);
    glLinkProgram(ID);

    glGetShaderiv(ID, GL_LINK_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(ID, 512, NULL, errLog);
        std::cout << "ERROR: Shader Program Linking Failed\n" << errLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragShader);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setMat4(const std::string& name, glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}