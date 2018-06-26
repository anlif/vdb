void vdbNote(float x, float y, const char *fmt, ...)
{
    char name[1024];
    sprintf(name, "vdb_tooltip_%d", vdb.note_index);
    va_list args;
    va_start(args, fmt);

    // Transform position to window coordinates
    vdbVec2 ndc = vdbModelToNDC(x,y,0.0f,1.0f);
    vdbVec2 win = vdbNDCToWindow(ndc.x,ndc.y);

    // Clamp tooltip to window
    // (Doesn't work yet)
    #if 0
    {
        char text[1024];
        sprintf(text, fmt, args);

        ImVec2 size = ImGui::CalcTextSize(text);

        if (x_win + size.x + 20.0f > vdb.window_w)
            x_win = vdb.window_w - size.x - 20.0f;

        if (y_win + size.y + 20.0f > vdb.window_h)
            y_win = vdb.window_h - size.y - 20.0f;
    }
    #endif

    ImGui::SetNextWindowPos(ImVec2(win.x, win.y));
    ImGui::Begin(name, 0, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextV(fmt, args);
    ImGui::End();
    va_end(args);
    vdb.note_index++;
}
void vdbClearColor(float r, float g, float b, float a)
{
    glClearColor(r,g,b,a);
    glClear(GL_COLOR_BUFFER_BIT);
}
void vdbClearDepth(float d)
{
    glClearDepth(d);
    glClear(GL_DEPTH_BUFFER_BIT);
}
void vdbLines(float width)
{
    glLineWidth(width);
    glBegin(GL_LINES);
}
void vdbPoints(float radius)
{
    glPointSize(radius);
    glBegin(GL_POINTS);
}
void vdbTriangles()
{
    glBegin(GL_TRIANGLES);
}
void vdbEnd()
{
    glEnd();
}
void vdbVertex(float x, float y, float z, float w) { glVertex4f(x,y,z,w); }
void vdbColor(float r, float g, float b, float a) { glColor4f(r,g,b,a); }
void vdbTexel(float u, float v) { glTexCoord2f(u,v); }
void vdbBlendNone() { glDisable(GL_BLEND); }
void vdbBlendAdd() { glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); }
void vdbBlendAlpha() { glEnable(GL_BLEND); glBlendEquation(GL_FUNC_ADD); glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE); }
void vdbDepthTest(bool enabled)
{
    if (enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}
void vdbDepthWrite(bool enabled)
{
    if (enabled) { glDepthMask(GL_TRUE); glDepthRange(0.0f, 1.0f); }
    else { glDepthMask(GL_FALSE); }
}
