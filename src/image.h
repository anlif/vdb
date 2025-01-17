#include "cmap/inferno.h"
#include "shaders/image.h"

vdbTextureFormat
    VDB_RGBA32F = 0,
    VDB_RGBA8   = 1;
vdbTextureFilter
    VDB_LINEAR        = 0,
    VDB_LINEAR_MIPMAP = 1,
    VDB_NEAREST       = 2;
vdbTextureWrap
    VDB_CLAMP  = 0,
    VDB_REPEAT = 1;

struct image_t
{
    GLuint handle;
    int width;
    int height;
    int channels;
    vdbVec4 v_min;
    vdbVec4 v_max;
};

enum { MAX_IMAGES = 1024 };
static image_t images[MAX_IMAGES];

image_t *GetImage(int slot)
{
    assert(slot >= 0 && slot < MAX_IMAGES && "Tried to access an image beyond the available slots");
    return &images[slot];
}

GLenum TextureFormatToGL(vdbTextureFormat format)
{
    if (format == VDB_RGBA32F) return GL_RGBA32F;
    else if (format == VDB_RGBA8) return GL_RGBA8;
    assert(false && "Unrecognized vdbTextureFormat");
    return GL_RGBA;
}

GLenum TextureFilterToGL(vdbTextureFilter filter)
{
    if (filter == VDB_NEAREST) return GL_NEAREST;
    else if (filter == VDB_LINEAR) return GL_LINEAR;
    else if (filter == VDB_LINEAR_MIPMAP) return GL_LINEAR_MIPMAP_LINEAR;
    assert(false && "Unrecognized vdbTextureFilter");
    return GL_NEAREST;
}

GLenum TextureWrapToGL(vdbTextureWrap wrap)
{
    if (wrap == VDB_CLAMP) return GL_CLAMP_TO_EDGE;
    else if (wrap == VDB_REPEAT) return GL_REPEAT;
    assert(false && "Unrecognized vdbTextureWrap");
    return GL_CLAMP_TO_EDGE;
}

void vdbSetTextureParameters(vdbTextureFilter filter, vdbTextureWrap wrap)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filter == VDB_LINEAR_MIPMAP) ? GL_LINEAR : TextureFilterToGL(filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureFilterToGL(filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     TextureWrapToGL(wrap));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     TextureWrapToGL(wrap));
}

GLuint TexImage2D(
    const void *data,
    int width,
    int height,
    GLenum data_format,
    GLenum data_type = GL_UNSIGNED_BYTE,
    GLenum mag_filter = GL_LINEAR,
    GLenum min_filter = GL_LINEAR,
    GLenum wrap_s = GL_CLAMP_TO_EDGE,
    GLenum wrap_t = GL_CLAMP_TO_EDGE,
    GLenum internal_format = GL_RGBA)
{
    GLuint result = 0;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 internal_format,
                 width,
                 height,
                 0,
                 data_format,
                 data_type,
                 data);
    if (min_filter == GL_LINEAR_MIPMAP_LINEAR)
        glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return result;
}

void vdbLoadImage(int slot,
                  const void *data,
                  int width,
                  int height,
                  GLenum data_format,
                  GLenum data_type,
                  GLenum mag_filter = GL_LINEAR,
                  GLenum min_filter = GL_LINEAR,
                  GLenum wrap_s = GL_CLAMP_TO_EDGE,
                  GLenum wrap_t = GL_CLAMP_TO_EDGE,
                  GLenum internal_format = GL_RGBA)
{
    image_t *image = GetImage(slot);
    image->width = width;
    image->height = height;
    if (!image->handle)
        glGenTextures(1, &image->handle);
    glBindTexture(GL_TEXTURE_2D, image->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    if (min_filter == GL_LINEAR_MIPMAP_LINEAR)
        glGenerateMipmap(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 internal_format,
                 width,
                 height,
                 0,
                 data_format,
                 data_type,
                 data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void vdbLoadImageUint8(int slot, const void *data, int width, int height, int channels)
{
    assert(channels >= 1 && channels <= 4 && "'channels' must be 1,2,3 or 4");
    if      (channels == 1) vdbLoadImage(slot, data, width, height, GL_RED, GL_UNSIGNED_BYTE);
    else if (channels == 2) vdbLoadImage(slot, data, width, height, GL_RG, GL_UNSIGNED_BYTE);
    else if (channels == 3) vdbLoadImage(slot, data, width, height, GL_RGB, GL_UNSIGNED_BYTE);
    else if (channels == 4) vdbLoadImage(slot, data, width, height, GL_RGBA, GL_UNSIGNED_BYTE);
    GetImage(slot)->channels = channels;
}

void vdbLoadImageFloat32(int slot, const void *data, int width, int height, int channels)
{
    assert(channels >= 1 && channels <= 4 && "'channels' must be 1,2,3 or 4");
    if      (channels == 1) vdbLoadImage(slot, data, width, height, GL_RED, GL_FLOAT);
    else if (channels == 2) vdbLoadImage(slot, data, width, height, GL_RG, GL_FLOAT);
    else if (channels == 3) vdbLoadImage(slot, data, width, height, GL_RGB, GL_FLOAT);
    else if (channels == 4) vdbLoadImage(slot, data, width, height, GL_RGBA, GL_FLOAT);
    GetImage(slot)->channels = channels;
}

void vdbDrawImage(int slot,
    float x, float y,
    float w, float h,
    vdbTextureFilter filter,
    vdbTextureWrap wrap,
    vdbVec4 v_min,
    vdbVec4 v_max)
{
    static GLuint program = LoadShaderFromMemory(shader_image_vs, shader_image_fs);
    assert(program);
    static GLint attrib_quad_pos  = glGetAttribLocation(program, "quad_pos");
    static GLint uniform_pvm      = glGetUniformLocation(program, "pvm");
    static GLint uniform_sampler0 = glGetUniformLocation(program, "sampler0");
    static GLint uniform_sampler1 = glGetUniformLocation(program, "sampler1");
    static GLint uniform_vmin     = glGetUniformLocation(program, "vmin");
    static GLint uniform_vmax     = glGetUniformLocation(program, "vmax");
    static GLint uniform_is_mono  = glGetUniformLocation(program, "is_mono");
    static GLint uniform_is_cmap  = glGetUniformLocation(program, "is_cmap");
    static GLint uniform_im_pos   = glGetUniformLocation(program, "im_pos");
    static GLint uniform_im_size  = glGetUniformLocation(program, "im_size");

    // upload 1D colormap as 2D texture of height 1
    static GLuint color_map_tex = TexImage2D(
        cmap_inferno,
        cmap_inferno_length,
        1,
        GL_RGB,
        GL_FLOAT,
        GL_LINEAR, GL_LINEAR,
        GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE,
        GL_RGBA);

    glUseProgram(program);

    // primary texture
    // glActiveTexture(GL_TEXTURE0);
    vdbBindImage(slot, filter, wrap, v_min, v_max);
    glUniform1i(uniform_sampler0, 0);

    // colormap texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color_map_tex);
    glUniform1i(uniform_sampler1, 1);

    // other uniforms
    float pvm[4*4];
    vdbGetPVM(pvm);
    UniformMat4fv(uniform_pvm, 1, pvm);
    glUniform1i(uniform_is_mono, GetImage(slot)->channels == 1 ? 1 : 0);
    glUniform1i(uniform_is_cmap, 0);
    glUniform2f(uniform_im_pos, x, y);
    glUniform2f(uniform_im_size, w, h);
    UniformVec4(uniform_vmin, v_min);
    UniformVec4(uniform_vmax, v_max);

    // generate buffers
    static GLuint vao = 0;
    static GLuint vbo = 0;
    if (!vao)
    {
        static float position[] = { 0,0, 1,0, 1,1, 1,1, 0,1, 0,0 };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);
    }
    assert(vao);
    assert(vbo);

    // draw
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(attrib_quad_pos);
    glVertexAttribPointer(attrib_quad_pos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // cleanup
    glDisableVertexAttribArray(attrib_quad_pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void vdbBindImage(int slot, vdbTextureFilter filter, vdbTextureWrap wrap, vdbVec4 v_min, vdbVec4 v_max)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GetImage(slot)->handle);
    vdbSetTextureParameters(filter, wrap);
    GetImage(slot)->v_min = v_min;
    GetImage(slot)->v_max = v_max;
}

void vdbUnbindImage()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}
