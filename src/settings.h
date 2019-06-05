#include <stdio.h>
#include <stdlib.h>

typedef int camera_type_t;
enum camera_type_
{
    VDB_CAMERA_DISABLED=0,
    VDB_CAMERA_PLANAR,
    VDB_CAMERA_TRACKBALL,
    VDB_CAMERA_TURNTABLE
};

typedef int camera_up_t;
enum camera_up_
{
    VDB_Z_UP=0,
    VDB_Y_UP,
    VDB_X_UP,
    VDB_Z_DOWN,
    VDB_Y_DOWN,
    VDB_X_DOWN,
};

enum { MAX_FRAME_SETTINGS = 1024 };
enum { VDB_MAX_RENDER_SCALE_DOWN = 3 };
enum { VDB_MAX_RENDER_SCALE_UP = 3 };

static const char *CameraTypeToStr(camera_type_t type)
{
    if (type == VDB_CAMERA_DISABLED) return "disabled";
    else if (type == VDB_CAMERA_PLANAR) return "planar";
    else if (type == VDB_CAMERA_TRACKBALL) return "trackball";
    else if (type == VDB_CAMERA_TURNTABLE) return "turntable";
    return "disabled";
}

static const char *CameraUpToStr(camera_up_t mode)
{
    if      (mode == VDB_Z_UP) return "z_up";
    else if (mode == VDB_Y_UP) return "y_up";
    else if (mode == VDB_X_UP) return "x_up";
    else if (mode == VDB_Z_DOWN) return "z_down";
    else if (mode == VDB_Y_DOWN) return "y_down";
    else if (mode == VDB_X_DOWN) return "x_down";
    return "z_up";
}

static const char *Mat4ToStr(vdbMat4 m)
{
    static char buffer[1024];
    sprintf(buffer, "[%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f]",
        m.data[ 0], m.data[ 1], m.data[ 2], m.data[ 3],
        m.data[ 4], m.data[ 5], m.data[ 6], m.data[ 7],
        m.data[ 8], m.data[ 9], m.data[10], m.data[11],
        m.data[12], m.data[13], m.data[14], m.data[15]);
    return buffer;
}

static const char *Vec4ToStr(vdbVec4 v)
{
    static char buffer[1024];
    sprintf(buffer, "[%f, %f, %f, %f]",
        v.x, v.y, v.z, v.w);
    return buffer;
}

struct camera_trackball_settings_t
{
    vdbMat4 R; // world to camera
    vdbVec4 T; // camera relative world in world
    float zoom;
};

struct camera_turntable_settings_t
{
    float angle_x;
    float angle_y;
    float radius;
};

struct frame_settings_t
{
    char *name;
    camera_type_t camera_type;
    camera_trackball_settings_t trackball;
    camera_turntable_settings_t turntable;

    // perspective projection parameters
    float y_fov;
    float min_depth;
    float max_depth;

    camera_up_t camera_up;
    bool grid_visible;
    float grid_scale;
    bool cube_visible;

    int render_scale_down;
    int render_scale_up;
};

struct global_camera_settings_t
{
    float mouse_sensitivity;
    float scroll_sensitivity;
    float move_speed_normal;
    float move_speed_slow;

    // proportional smoothing gains
    float Kp_zoom;
    float Kp_translate;
    float Kp_rotate;
};

struct window_settings_t
{
    int x,y,width,height;
};

struct settings_t
{
    global_camera_settings_t camera;
    window_settings_t window;
    frame_settings_t frames[MAX_FRAME_SETTINGS];
    int num_frames;
    bool never_ask_on_exit;
    bool show_main_menu;
    int font_size;
    bool can_idle;
    int auto_step_delay_ms;
    int dpi_scale;

    void LoadOrDefault(const char *filename);
    void Save(const char *filename);
};

static settings_t settings;
static frame_settings_t *GetFrameSettings(); // defined in vdb.cpp

void DefaultFrameSettings(frame_settings_t *fs)
{
    fs->camera_type = VDB_CAMERA_DISABLED;
    fs->trackball.R = vdbMatIdentity();
    fs->trackball.T = vdbVec4(0.0f, 0.0f, 0.0f, 1.0f);
    fs->trackball.zoom = 1.0f;
    fs->turntable.angle_x = 0.0f;
    fs->turntable.angle_y = 0.0f;
    fs->turntable.radius = 1.0f;
    fs->y_fov = 0.7f;
    fs->min_depth = 0.1f;
    fs->max_depth = 50.0f;
    fs->grid_visible = false;
    fs->grid_scale = 2.0f;
    fs->camera_up = VDB_Z_UP;
    fs->cube_visible = false;
    fs->render_scale_down = 0;
    fs->render_scale_up = 0;
}

namespace settings_parser
{
    static bool IsAlphaNumeric(char c)
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9');
    }

    static void ParseBlank(const char **c)
    {
        while (**c == ' ' || **c == '\n' || **c == '\r' || **c == '\t')
            *c = *c + 1;
    }

    static void ClampInt(int *x, int x_min, int x_max)
    {
        if (*x < x_min) *x = x_min;
        if (*x > x_max) *x = x_max;
    }

    static bool ParseInt(const char **c, int *x, int x_min=0, int x_max=0)
    {
        ParseBlank(c);
        int b;
        if (1 == sscanf(*c, "%d%n", x, &b))
        {
            if (x_min != x_max)
                ClampInt(x, x_min, x_max);
            *c = *c + b;
            return true;
        }
        return false;
    }

    static bool ParseFloat(const char **c, float *x)
    {
        ParseBlank(c);
        int b;
        if (1 == sscanf(*c, "%f%n", x, &b))
        {
            *c = *c + b;
            return true;
        }
        return false;
    }

    static bool ParseFloatToInt(const char **c, int *x, int x_min, int x_max)
    {
        ParseBlank(c);
        int b;
        float f;
        if (1 == sscanf(*c, "%f%n", &f, &b))
        {
            *x = (int)f;
            ClampInt(x, x_min, x_max);
            *c = *c + b;
            return true;
        }
        return false;
    }

    static bool ParseComma(const char **c)
    {
        ParseBlank(c);
        if (**c == ',')
        {
            *c = *c + 1;
            return true;
        }
        return false;
    }

    static bool ParseString(const char **c, const char *match)
    {
        ParseBlank(c);
        const char *a = *c;
        const char *b = match;
        while (*a && *b)
        {
            if (*a != *b)
                return false;
            a++;
            b++;
        }
        if (*b) return false;
        *c = a;
        return true;
    }

    static bool ParseKey(const char **c, const char *match)
    {
        ParseBlank(c);
        if (!ParseString(c, match)) return false;
        ParseBlank(c);
        if (**c != '=') return false;
        *c = *c + 1;
        return true;
    }

    static bool ParseBool(const char **c, bool *x)
    {
        ParseBlank(c);
             if (**c == '0')           { *x = false; *c = *c + 1; return true; }
        else if (**c == '1')           { *x = true; *c = *c + 1; return true; }
        else if (ParseString(c, "False")) { *x = false; return true; }
        else if (ParseString(c, "True"))  { *x = true; return true; }
        else if (ParseString(c, "false")) { *x = false; return true; }
        else if (ParseString(c, "true"))  { *x = true; return true; }
        return false;
    }

    static bool ParseCameraType(const char **c, camera_type_t *type)
    {
        ParseBlank(c);
        if      (ParseString(c, "disabled"))  { *type = VDB_CAMERA_DISABLED; return true; }
        else if (ParseString(c, "planar"))    { *type = VDB_CAMERA_PLANAR; return true; }
        else if (ParseString(c, "trackball")) { *type = VDB_CAMERA_TRACKBALL; return true; }
        else if (ParseString(c, "turntable")) { *type = VDB_CAMERA_TURNTABLE; return true; }
        return false;
    }

    static bool ParseCameraUp(const char **c, camera_up_t *up)
    {
        ParseBlank(c);
        if      (ParseString(c, "z_up"))   { *up = VDB_Z_UP; return true; }
        else if (ParseString(c, "y_up"))   { *up = VDB_Y_UP; return true; }
        else if (ParseString(c, "x_up"))   { *up = VDB_X_UP; return true; }
        else if (ParseString(c, "z_down")) { *up = VDB_Z_DOWN; return true; }
        else if (ParseString(c, "y_down")) { *up = VDB_Y_DOWN; return true; }
        else if (ParseString(c, "x_down")) { *up = VDB_X_DOWN; return true; }
        return false;
    }

    static bool ParseInt2(const char **c, int *x, int *y)
    {
        ParseBlank(c);
        if (!ParseInt(c, x)) return false;
        if (!ParseComma(c)) return false;
        if (!ParseInt(c, y)) return false;
        return true;
    }

    static bool ParseMat4(const char **c, vdbMat4 *x)
    {
        ParseBlank(c);
        for (int i = 0; i < 16; i++)
        {
            if (i > 0 && !ParseComma(c)) return false;
            if (!ParseFloat(c, &x->data[i])) return false;
        }
        return true;
    }

    static bool ParseVec4(const char **c, vdbVec4 *x)
    {
        ParseBlank(c);
        if (!ParseFloat(c, &x->x)) return false;
        if (!ParseComma(c))        return false;
        if (!ParseFloat(c, &x->y)) return false;
        if (!ParseComma(c))        return false;
        if (!ParseFloat(c, &x->z)) return false;
        if (!ParseComma(c))        return false;
        if (!ParseFloat(c, &x->w)) return false;
        return true;
    }
}

void settings_t::LoadOrDefault(const char *filename)
{
    camera.mouse_sensitivity = 50.0f;
    camera.scroll_sensitivity = 5.0f;
    camera.move_speed_normal = 1.0f;
    camera.move_speed_slow = 0.5f;
    camera.Kp_zoom = 5.0f;
    camera.Kp_translate = 5.0f;
    camera.Kp_rotate = 10.0f;
    window.x = -1;
    window.y = -1;
    window.width = 1000;
    window.height = 600;
    dpi_scale = 100;
    never_ask_on_exit = false;
    show_main_menu = true;
    can_idle = false;
    num_frames = 0;
    auto_step_delay_ms = 250;
    font_size = (int)(VDB_DEFAULT_FONT_SIZE);

    char *data = NULL;
    {
        FILE *f = fopen(filename, "rb");
        if (!f) return;
        if (fseek(f, 0, SEEK_END)) { fclose(f); return; }
        int len = (int)ftell(f);
        if (len <= 0)        { fclose(f); return; }
        if (fseek(f, 0, SEEK_SET)) { fclose(f); return; }
        data = (char*)malloc(len + 1);
        if (!fread(data, 1, len, f)) { fclose(f); free(data); return; }
        data[len] = '\0';
        fclose(f);
    }
    const char **c = (const char **)&data;

    frame_settings_t *frame = NULL;

    while (**c)
    {
        using namespace settings_parser;
             if (ParseKey(c, "window_pos"))         ParseInt2(c, &window.x, &window.y);
        else if (ParseKey(c, "window_size"))        ParseInt2(c, &window.width, &window.height);
        else if (ParseKey(c, "never_ask_on_exit"))  ParseBool(c, &never_ask_on_exit);
        else if (ParseKey(c, "show_main_menu"))     ParseBool(c, &show_main_menu);
        else if (ParseKey(c, "mouse_sensitivity"))  ParseFloat(c, &camera.mouse_sensitivity);
        else if (ParseKey(c, "scroll_sensitivity")) ParseFloat(c, &camera.scroll_sensitivity);
        else if (ParseKey(c, "move_speed_normal"))  ParseFloat(c, &camera.move_speed_normal);
        else if (ParseKey(c, "move_speed_slow"))    ParseFloat(c, &camera.move_speed_slow);
        else if (ParseKey(c, "Kp_zoom"))            ParseFloat(c, &camera.Kp_zoom);
        else if (ParseKey(c, "Kp_translate"))       ParseFloat(c, &camera.Kp_translate);
        else if (ParseKey(c, "Kp_rotate"))          ParseFloat(c, &camera.Kp_rotate);
        else if (ParseKey(c, "font_size"))          ParseFloatToInt(c, &font_size, 6, 96);
        else if (ParseKey(c, "dpi_scale"))          ParseFloatToInt(c, &dpi_scale, 100, 200);
        else if (ParseKey(c, "can_idle"))           ParseBool(c, &can_idle);
        else if (ParseKey(c, "auto_step_delay_ms")) ParseInt(c, &auto_step_delay_ms);
        else if (ParseKey(c, "[frame]"))
        {
            if (num_frames == MAX_FRAME_SETTINGS)
            {
                frame = NULL;
                fprintf(stderr, "vdb: Reached max number of stored per-block settings. You should clean up your vdb.ini file!\n");
                continue;
            }
            assert(num_frames < MAX_FRAME_SETTINGS);
            frame = frames + (num_frames++);
            const char *name_begin = *c;
            while (**c && !(**c == '\n' || **c == '\r'))
                *c = *c + 1;
            const char *name_end = *c;
            size_t len = name_end - name_begin;
            frame->name = (char*)malloc(len + 1);
            memcpy(frame->name, name_begin, len);
            frame->name[len] = '\0';
            DefaultFrameSettings(frame);
        }
        else if (frame)
        {
                 if (ParseKey(c, "camera_type"))       ParseCameraType(c, &frame->camera_type);
            else if (ParseKey(c, "camera_angle_x"))    ParseFloat(c, &frame->turntable.angle_x);
            else if (ParseKey(c, "camera_angle_y"))    ParseFloat(c, &frame->turntable.angle_y);
            else if (ParseKey(c, "camera_radius"))     ParseFloat(c, &frame->turntable.radius);
            else if (ParseKey(c, "y_fov"))             ParseFloat(c, &frame->y_fov);
            else if (ParseKey(c, "min_depth"))         ParseFloat(c, &frame->min_depth);
            else if (ParseKey(c, "max_depth"))         ParseFloat(c, &frame->max_depth);
            else if (ParseKey(c, "grid_visible"))      ParseBool(c, &frame->grid_visible);
            else if (ParseKey(c, "grid_scale"))        ParseFloat(c, &frame->grid_scale);
            else if (ParseKey(c, "camera_up"))         ParseCameraUp(c, &frame->camera_up);
            else if (ParseKey(c, "cube_visible"))      ParseBool(c, &frame->cube_visible);
            else if (ParseKey(c, "render_scale_down")) ParseInt(c, &frame->render_scale_down, 0, VDB_MAX_RENDER_SCALE_DOWN);
            else if (ParseKey(c, "render_scale_up"))   ParseInt(c, &frame->render_scale_up, 0, VDB_MAX_RENDER_SCALE_UP);
        }
        else *c = *c + 1;
    }
}

void settings_t::Save(const char *filename)
{
    FILE *f = fopen(filename, "wb+");
    if (!f)
    {
        fprintf(stderr, "Failed to save settings.\n");
        return;
    }
    fprintf(f, "[vdb]\n");
    fprintf(f, "window_pos=%d,%d\n", window.x, window.y);
    fprintf(f, "window_size=%d,%d\n", window.width, window.height);
    fprintf(f, "never_ask_on_exit=%d\n", never_ask_on_exit);
    fprintf(f, "show_main_menu=%d\n", show_main_menu);
    fprintf(f, "mouse_sensitivity=%g\n", camera.mouse_sensitivity);
    fprintf(f, "scroll_sensitivity=%g\n", camera.scroll_sensitivity);
    fprintf(f, "move_speed_normal=%g\n", camera.move_speed_normal);
    fprintf(f, "move_speed_slow=%g\n", camera.move_speed_slow);
    fprintf(f, "Kp_zoom=%g\n", camera.Kp_zoom);
    fprintf(f, "Kp_translate=%g\n", camera.Kp_translate);
    fprintf(f, "Kp_rotate=%g\n", camera.Kp_rotate);
    fprintf(f, "font_size=%d\n", font_size);
    fprintf(f, "dpi_scale=%d\n", dpi_scale);
    fprintf(f, "can_idle=%d\n", can_idle);
    fprintf(f, "auto_step_delay_ms=%d\n", auto_step_delay_ms);
    for (int i = 0; i < num_frames; i++)
    {
        frame_settings_t *frame = frames + i;
        fprintf(f, "\n[frame]=%s\n", frame->name);
        if (frame->camera_type != VDB_CAMERA_DISABLED)
        {
            fprintf(f, "camera_type=%s\n", CameraTypeToStr(frame->camera_type));
            if (frame->camera_type != VDB_CAMERA_PLANAR)
            {
                fprintf(f, "y_fov=%g\n", frame->y_fov);
                fprintf(f, "min_depth=%g\n", frame->min_depth);
                fprintf(f, "max_depth=%g\n", frame->max_depth);
            }
            fprintf(f, "grid_visible=%d\n", frame->grid_visible ? 1 : 0);
            fprintf(f, "grid_scale=%g\n", frame->grid_scale);
            fprintf(f, "camera_up=%s\n", CameraUpToStr(frame->camera_up));
            fprintf(f, "cube_visible=%d\n", frame->cube_visible ? 1 : 0);
        }
        fprintf(f, "render_scale_down=%d\n", frame->render_scale_down);
        fprintf(f, "render_scale_up=%d\n", frame->render_scale_up);
    }
    fclose(f);
}
