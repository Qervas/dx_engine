#pragma once

#include "../RHI/CubemapTexture.h"
#include "../RHI/Buffer.h"
#include "../RHI/Shader.h"
#include "../RHI/RootSignature.h"
#include "../RHI/PipelineState.h"
#include <DirectXMath.h>
#include <memory>

class GraphicsDevice;
class CommandList;
class DescriptorHeap;
class Camera;

// Constant buffer structure for skybox shader
struct SkyboxConstants
{
    DirectX::XMMATRIX inverseViewProjection;
    DirectX::XMFLOAT3 cameraPosition;
    float exposure;
};

class Skybox
{
public:
    Skybox(GraphicsDevice* device);
    ~Skybox();

    // Initialize with a cubemap texture
    bool Initialize(CubemapTexture* cubemap, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat);

    // Render the skybox
    void Render(CommandList* commandList, DescriptorHeap* srvHeap, Camera* camera);

    // Accessors
    CubemapTexture* GetCubemap() const { return m_cubemap; }
    void SetCubemap(CubemapTexture* cubemap) { m_cubemap = cubemap; }
    void SetExposure(float exposure) { m_exposure = exposure; }
    float GetExposure() const { return m_exposure; }

private:
    GraphicsDevice* m_device = nullptr;
    CubemapTexture* m_cubemap = nullptr;

    std::unique_ptr<Shader> m_vertexShader;
    std::unique_ptr<Shader> m_pixelShader;
    std::unique_ptr<RootSignature> m_rootSignature;
    std::unique_ptr<PipelineState> m_pipelineState;
    std::unique_ptr<Buffer> m_constantBuffer;

    float m_exposure = 1.0f;
};
