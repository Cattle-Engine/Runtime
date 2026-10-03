CE::Mesh gMesh;
CE::Transform3D default_transform;

CE::Texture invalid_tex;
CE::Material material;

CE::Shader balls_shader;
CE::Shader fractal_shader;
CE::Shader more_shader;

CE::Shader current_shader;

bool use_shaders = true;

float elapsedTime = 0.0f;

void imgui() {
    CE::ImGui::Begin("Shader changer!");

    if (CE::ImGui::Button("Balls")) {
        current_shader = balls_shader;
    }
    CE::ImGui::SameLine();
    if (CE::ImGui::Button("Fractal")) {
        current_shader = fractal_shader;
    }
    CE::ImGui::SameLine();
    if (CE::ImGui::Button("More")) {
        current_shader = more_shader;
    }

    if (CE::ImGui::Button("Quit")) {
        CE::Quit();
    }

    use_shaders = CE::ImGui::Checkbox("Use default fragment shader", use_shaders);

    CE::ImGui::End();
}

void main() {
    CE::MeshData mesh_data = CE::Graphics::Render3D::Primitives::CreateCube();
    gMesh = CE::Graphics::Render3D::CompileMesh(mesh_data);
    
    CE::Graphics::Shaders::CreateProgram(balls_shader);
    if (!CE::Graphics::Shaders::LoadStage(balls_shader,"trippy/balls.frag", CE::ShaderStage::Fragment, 0)) {
        CE::Quit();
    }
    CE::Graphics::Shaders::CompileShader(balls_shader);

    CE::Graphics::Shaders::CreateProgram(fractal_shader);
    if (!CE::Graphics::Shaders::LoadStage(fractal_shader,"trippy/fractal.frag", CE::ShaderStage::Fragment, 0)) {
        CE::Quit();
    }
    CE::Graphics::Shaders::CompileShader(fractal_shader);

    CE::Graphics::Shaders::CreateProgram(more_shader);
    if (!CE::Graphics::Shaders::LoadStage(more_shader,"trippy/more.frag", CE::ShaderStage::Fragment, 0)) {
        CE::Quit();
    }
    CE::Graphics::Shaders::CompileShader(more_shader);

    CE::Graphics::Materials::CreateMaterial(material, invalid_tex);

}

void update() {
    elapsedTime += CE::Performance::GetDeltaTime();

    if (use_shaders) {
        CE::Graphics::Shaders::BindShader(current_shader);
        CE::Graphics::Shaders::SetFloat("time", elapsedTime);
    }

    CE::Graphics::Render3D::DrawMesh(gMesh, default_transform, material, false);
}

