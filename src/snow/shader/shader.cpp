#include "shader.hpp"
Shader::Shader(const ShaderType &t, const std::string &filename)
    : type(t), filename(filename) {}

void Shader::load_shader_from_file() {
  auto shdr_source = FileSystem::load_string_from_file(filename);
  const auto start = 0;

  const auto main_start = shdr_source.find("void main");
  process_includes(shdr_source, start, main_start);

  const auto ext_start = filename.find(".");
  auto filename_tagged = filename;
  filename_tagged.insert(ext_start, "_compiled");
  const auto path_end = filename.find_last_of("/");
  filename_tagged.insert(path_end + 1, "compiled/");

  FileSystem::write_to_file(shdr_source, filename_tagged);
  source = shdr_source;
}

void Shader::process_includes(std::string &file, unsigned long long start,
                              unsigned long long end_search) {
  while ((start = file.find("#include", start)) <= end_search) {
    // file path for includes is within quotation marks
    const auto path_start = file.find('\"', start);
    const auto path_end = file.find('\"', path_start + 1);
    auto path_inc = file.substr(path_start + 1, path_end - path_start - 1);
    std::cout << "Including " << path_inc << "\n";

    auto inc = FileSystem::load_string_from_file(path_inc);
    const auto eol = file.find('\n', path_end);
    // nested includes
    process_includes(inc, 0, inc.size());
    // replace source of file with include pragma
    file.replace(start, eol - start, inc.data());
    end_search += inc.size();
  }
}

void Shader::upload() {
  gl_create_id();
  if (!gl_compile()) {  // if shader could'nt get compiled
    gl_delete();        // delete shader handle
  };
}

/*
 * start of glFunction() calls
 */

Shader::~Shader() {}
void Shader::gl_delete() {
  if (uploaded) {
    glDeleteShader(id);
    uploaded = false;
  }
}

void Shader::gl_create_id() {
  // We handle the GLuint type by scoped enums of C++14 which should be safer
  GLuint gl_type;
  switch (type) {
    case ShaderType::VERTEX:
      gl_type = GL_VERTEX_SHADER;
      break;
    case ShaderType::TESS_EVAL:
      gl_type = GL_TESS_EVALUATION_SHADER;
      break;
    case ShaderType::TESS_CNTRL:
      gl_type = GL_TESS_CONTROL_SHADER;
      break;
    case ShaderType::GEOMETRY:
      gl_type = GL_GEOMETRY_SHADER;
      break;
    case ShaderType::FRAGMENT:
      gl_type = GL_FRAGMENT_SHADER;
      break;
    case ShaderType::COMPUTE:
      gl_type = GL_COMPUTE_SHADER;
      break;
  }

  // Note: If you segfault here you probably don't have a valid rendering
  // context and uploaded is not gonna get set.
  id = glCreateShader(gl_type);
  uploaded = true;
}

bool Shader::gl_compile() {
  auto sourceChars = source.c_str();

  glShaderSource(id, 1, &sourceChars, NULL);

  glCompileShader(id);

  GLint shaderStatus;
  glGetShaderiv(id, GL_COMPILE_STATUS, &shaderStatus);

  // If the shader failed to compile, display the info log and quit out
  bool compiled = (shaderStatus == GL_TRUE);
  if (!compiled) {
    GLint infoLogLength;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

    GLchar *strInfoLog = new GLchar[infoLogLength + 1];
    glGetShaderInfoLog(id, infoLogLength, NULL, strInfoLog);

    std::cerr << "shader compilation failed: " << strInfoLog << std::endl;
    delete[] strInfoLog;
  }
  return compiled;
}

