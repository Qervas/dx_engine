#include "IBL.h"
#include "../RHI/Device.h"
#include "../RHI/CommandList.h"
#include "../RHI/CommandQueue.h"
#include "../RHI/Buffer.h"
#include <cmath>
#include <algorithm>

static const float PI = 3.14159265359f;

IBL::IBL(GraphicsDevice* device)
    : m_device(device)
{
}

IBL::~IBL()
{
}

bool IBL::Generate(
    CubemapTexture* environmentMap,
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap)
{
    if (!GenerateBRDFLUT(commandList, commandQueue, srvHeap))
        return false;

    if (!GenerateIrradianceMap(environmentMap, commandList, commandQueue, srvHeap))
        return false;

    if (!GeneratePrefilteredMap(environmentMap, commandList, commandQueue, srvHeap))
        return false;

    return true;
}

bool IBL::GenerateBRDFLUT(
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap,
    uint32_t size)
{
    // Generate BRDF LUT data on CPU
    std::vector<uint8_t> lutData;
    GenerateBRDFLUTData(lutData, size);

    // Create texture
    m_brdfLUT = std::make_unique<Texture>(m_device);
    if (!m_brdfLUT->Create(size, size, DXGI_FORMAT_R8G8_UNORM, TextureUsage::ShaderResource))
    {
        return false;
    }

    // Calculate row pitch (256-byte aligned)
    uint32_t rowPitch = size * 2;  // RG format = 2 bytes per pixel
    uint32_t alignedRowPitch = (rowPitch + 255) & ~255;

    // Create upload buffer
    Buffer uploadBuffer(m_device);
    if (!uploadBuffer.Create(alignedRowPitch * size, 0, BufferUsage::Upload))
    {
        return false;
    }

    // Map and copy with proper alignment
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer.GetD3D12Resource()->Map(0, &readRange, &mappedData);

    for (uint32_t y = 0; y < size; ++y)
    {
        memcpy(static_cast<uint8_t*>(mappedData) + y * alignedRowPitch,
               lutData.data() + y * rowPitch,
               rowPitch);
    }

    uploadBuffer.GetD3D12Resource()->Unmap(0, nullptr);

    // Upload to GPU
    commandList->Reset();

    commandList->TransitionBarrier(
        m_brdfLUT->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = uploadBuffer.GetD3D12Resource();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint.Offset = 0;
    srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8_UNORM;
    srcLocation.PlacedFootprint.Footprint.Width = size;
    srcLocation.PlacedFootprint.Footprint.Height = size;
    srcLocation.PlacedFootprint.Footprint.Depth = 1;
    srcLocation.PlacedFootprint.Footprint.RowPitch = alignedRowPitch;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = m_brdfLUT->GetD3D12Resource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    commandList->TransitionBarrier(
        m_brdfLUT->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->Close();

    ID3D12CommandList* commandLists[] = { commandList->GetD3D12CommandList() };
    commandQueue->ExecuteCommandLists(commandLists, 1);
    commandQueue->Flush();

    // Create SRV
    DescriptorHandle srvHandle = srvHeap->Allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;

    m_device->GetD3D12Device()->CreateShaderResourceView(
        m_brdfLUT->GetD3D12Resource(),
        &srvDesc,
        srvHandle.cpu
    );

    m_brdfLUT->SetSRV(srvHandle);

    return true;
}

bool IBL::GenerateIrradianceMap(
    CubemapTexture* environmentMap,
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap,
    uint32_t size)
{
    if (!environmentMap)
        return false;

    // Generate irradiance data on CPU
    std::vector<std::vector<uint8_t>> faceData(6);
    GenerateIrradianceData(environmentMap, faceData, size);

    // Create cubemap texture
    m_irradianceMap = std::make_unique<CubemapTexture>(m_device);
    if (!m_irradianceMap->Create(size, DXGI_FORMAT_R8G8B8A8_UNORM, 1))
    {
        return false;
    }

    // Calculate row pitch (256-byte aligned)
    uint32_t rowPitch = size * 4;
    uint32_t alignedRowPitch = (rowPitch + 255) & ~255;
    uint32_t faceSize = alignedRowPitch * size;

    // Create upload buffer
    Buffer uploadBuffer(m_device);
    if (!uploadBuffer.Create(faceSize * 6, 0, BufferUsage::Upload))
    {
        return false;
    }

    // Map and copy
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer.GetD3D12Resource()->Map(0, &readRange, &mappedData);

    for (int face = 0; face < 6; ++face)
    {
        uint8_t* dstFaceStart = static_cast<uint8_t*>(mappedData) + face * faceSize;
        for (uint32_t row = 0; row < size; ++row)
        {
            memcpy(dstFaceStart + row * alignedRowPitch,
                   faceData[face].data() + row * rowPitch,
                   rowPitch);
        }
    }

    uploadBuffer.GetD3D12Resource()->Unmap(0, nullptr);

    // Upload to GPU
    commandList->Reset();

    commandList->TransitionBarrier(
        m_irradianceMap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    for (int face = 0; face < 6; ++face)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.pResource = uploadBuffer.GetD3D12Resource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Offset = face * faceSize;
        srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srcLocation.PlacedFootprint.Footprint.Width = size;
        srcLocation.PlacedFootprint.Footprint.Height = size;
        srcLocation.PlacedFootprint.Footprint.Depth = 1;
        srcLocation.PlacedFootprint.Footprint.RowPitch = alignedRowPitch;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.pResource = m_irradianceMap->GetD3D12Resource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = face;

        commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    commandList->TransitionBarrier(
        m_irradianceMap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->Close();

    ID3D12CommandList* commandLists[] = { commandList->GetD3D12CommandList() };
    commandQueue->ExecuteCommandLists(commandLists, 1);
    commandQueue->Flush();

    // Create SRV
    if (!m_irradianceMap->CreateSRV(srvHeap))
    {
        return false;
    }

    return true;
}

bool IBL::GeneratePrefilteredMap(
    CubemapTexture* environmentMap,
    CommandList* commandList,
    CommandQueue* commandQueue,
    DescriptorHeap* srvHeap,
    uint32_t size,
    uint32_t mipLevels)
{
    if (!environmentMap)
        return false;

    m_prefilteredMipLevels = mipLevels;

    // Generate prefiltered data on CPU
    std::vector<std::vector<std::vector<uint8_t>>> mipData(mipLevels);
    GeneratePrefilteredData(environmentMap, mipData, size, mipLevels);

    // Create cubemap texture with mip levels
    m_prefilteredMap = std::make_unique<CubemapTexture>(m_device);
    if (!m_prefilteredMap->Create(size, DXGI_FORMAT_R8G8B8A8_UNORM, mipLevels))
    {
        return false;
    }

    // Calculate total upload size
    uint32_t totalUploadSize = 0;
    for (uint32_t mip = 0; mip < mipLevels; ++mip)
    {
        uint32_t mipSize = size >> mip;
        if (mipSize < 1) mipSize = 1;
        uint32_t rowPitch = mipSize * 4;
        uint32_t alignedRowPitch = (rowPitch + 255) & ~255;
        totalUploadSize += alignedRowPitch * mipSize * 6;
    }

    // Create upload buffer
    Buffer uploadBuffer(m_device);
    if (!uploadBuffer.Create(totalUploadSize, 0, BufferUsage::Upload))
    {
        return false;
    }

    // Map and copy all mip levels
    void* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    uploadBuffer.GetD3D12Resource()->Map(0, &readRange, &mappedData);

    uint32_t currentOffset = 0;
    std::vector<uint32_t> mipOffsets(mipLevels);
    std::vector<uint32_t> mipAlignedRowPitches(mipLevels);

    for (uint32_t mip = 0; mip < mipLevels; ++mip)
    {
        uint32_t mipSize = size >> mip;
        if (mipSize < 1) mipSize = 1;
        uint32_t rowPitch = mipSize * 4;
        uint32_t alignedRowPitch = (rowPitch + 255) & ~255;
        uint32_t faceSize = alignedRowPitch * mipSize;

        mipOffsets[mip] = currentOffset;
        mipAlignedRowPitches[mip] = alignedRowPitch;

        for (int face = 0; face < 6; ++face)
        {
            uint8_t* dstFaceStart = static_cast<uint8_t*>(mappedData) + currentOffset + face * faceSize;
            for (uint32_t row = 0; row < mipSize; ++row)
            {
                memcpy(dstFaceStart + row * alignedRowPitch,
                       mipData[mip][face].data() + row * rowPitch,
                       rowPitch);
            }
        }

        currentOffset += faceSize * 6;
    }

    uploadBuffer.GetD3D12Resource()->Unmap(0, nullptr);

    // Upload to GPU
    commandList->Reset();

    commandList->TransitionBarrier(
        m_prefilteredMap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST
    );

    for (uint32_t mip = 0; mip < mipLevels; ++mip)
    {
        uint32_t mipSize = size >> mip;
        if (mipSize < 1) mipSize = 1;
        uint32_t faceSize = mipAlignedRowPitches[mip] * mipSize;

        for (int face = 0; face < 6; ++face)
        {
            D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
            srcLocation.pResource = uploadBuffer.GetD3D12Resource();
            srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            srcLocation.PlacedFootprint.Offset = mipOffsets[mip] + face * faceSize;
            srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srcLocation.PlacedFootprint.Footprint.Width = mipSize;
            srcLocation.PlacedFootprint.Footprint.Height = mipSize;
            srcLocation.PlacedFootprint.Footprint.Depth = 1;
            srcLocation.PlacedFootprint.Footprint.RowPitch = mipAlignedRowPitches[mip];

            D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
            dstLocation.pResource = m_prefilteredMap->GetD3D12Resource();
            dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dstLocation.SubresourceIndex = face * mipLevels + mip;

            commandList->GetD3D12CommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
        }
    }

    commandList->TransitionBarrier(
        m_prefilteredMap->GetD3D12Resource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->Close();

    ID3D12CommandList* commandLists[] = { commandList->GetD3D12CommandList() };
    commandQueue->ExecuteCommandLists(commandLists, 1);
    commandQueue->Flush();

    // Create SRV
    if (!m_prefilteredMap->CreateSRV(srvHeap))
    {
        return false;
    }

    return true;
}

// Math helpers
float IBL::RadicalInverse_VdC(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

void IBL::Hammersley(uint32_t i, uint32_t N, float& xi1, float& xi2)
{
    xi1 = float(i) / float(N);
    xi2 = RadicalInverse_VdC(i);
}

void IBL::ImportanceSampleGGX(float xi1, float xi2, float roughness, float Nx, float Ny, float Nz, float& Hx, float& Hy, float& Hz)
{
    float a = roughness * roughness;

    float phi = 2.0f * PI * xi1;
    float cosTheta = sqrtf((1.0f - xi2) / (1.0f + (a * a - 1.0f) * xi2));
    float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

    // Tangent space H
    float Htx = cosf(phi) * sinTheta;
    float Hty = sinf(phi) * sinTheta;
    float Htz = cosTheta;

    // Create tangent space basis
    float upX = fabsf(Nz) < 0.999f ? 0.0f : 1.0f;
    float upY = fabsf(Nz) < 0.999f ? 0.0f : 0.0f;
    float upZ = fabsf(Nz) < 0.999f ? 1.0f : 0.0f;

    // tangent = normalize(cross(up, N))
    float Tx = upY * Nz - upZ * Ny;
    float Ty = upZ * Nx - upX * Nz;
    float Tz = upX * Ny - upY * Nx;
    float tLen = sqrtf(Tx * Tx + Ty * Ty + Tz * Tz);
    if (tLen > 0.0001f) { Tx /= tLen; Ty /= tLen; Tz /= tLen; }

    // bitangent = cross(N, tangent)
    float Bx = Ny * Tz - Nz * Ty;
    float By = Nz * Tx - Nx * Tz;
    float Bz = Nx * Ty - Ny * Tx;

    // Transform H to world space
    Hx = Tx * Htx + Bx * Hty + Nx * Htz;
    Hy = Ty * Htx + By * Hty + Ny * Htz;
    Hz = Tz * Htx + Bz * Hty + Nz * Htz;

    float hLen = sqrtf(Hx * Hx + Hy * Hy + Hz * Hz);
    if (hLen > 0.0001f) { Hx /= hLen; Hy /= hLen; Hz /= hLen; }
}

float IBL::GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float IBL::GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

void IBL::IntegrateBRDF(float NdotV, float roughness, float& scale, float& bias)
{
    float Vx = sqrtf(1.0f - NdotV * NdotV);
    float Vy = 0.0f;
    float Vz = NdotV;

    float A = 0.0f;
    float B = 0.0f;

    const uint32_t SAMPLE_COUNT = 1024u;
    for (uint32_t i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float xi1, xi2;
        Hammersley(i, SAMPLE_COUNT, xi1, xi2);

        float Hx, Hy, Hz;
        ImportanceSampleGGX(xi1, xi2, roughness, 0.0f, 0.0f, 1.0f, Hx, Hy, Hz);

        // L = 2.0 * dot(V, H) * H - V
        float VdotH = Vx * Hx + Vy * Hy + Vz * Hz;
        float Lx = 2.0f * VdotH * Hx - Vx;
        float Ly = 2.0f * VdotH * Hy - Vy;
        float Lz = 2.0f * VdotH * Hz - Vz;

        float NdotL = (std::max)(Lz, 0.0f);
        float NdotH = (std::max)(Hz, 0.0f);
        VdotH = (std::max)(VdotH, 0.0f);

        if (NdotL > 0.0f)
        {
            float G = GeometrySmith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = powf(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    scale = A / float(SAMPLE_COUNT);
    bias = B / float(SAMPLE_COUNT);
}

void IBL::GenerateBRDFLUTData(std::vector<uint8_t>& data, uint32_t size)
{
    data.resize(size * size * 2);  // RG format

    for (uint32_t y = 0; y < size; ++y)
    {
        float roughness = (float(y) + 0.5f) / float(size);
        roughness = (std::max)(roughness, 0.01f);  // Avoid zero roughness

        for (uint32_t x = 0; x < size; ++x)
        {
            float NdotV = (float(x) + 0.5f) / float(size);
            NdotV = (std::max)(NdotV, 0.01f);  // Avoid zero NdotV

            float scale, bias;
            IntegrateBRDF(NdotV, roughness, scale, bias);

            uint32_t idx = (y * size + x) * 2;
            data[idx + 0] = static_cast<uint8_t>((std::min)(scale, 1.0f) * 255.0f);
            data[idx + 1] = static_cast<uint8_t>((std::min)(bias, 1.0f) * 255.0f);
        }
    }
}

// Helper to get direction from cubemap face and UV
static void GetCubemapDirection(int face, float u, float v, float& dx, float& dy, float& dz)
{
    // u, v are in [-1, 1]
    switch (face)
    {
    case 0: dx = 1.0f;  dy = -v;    dz = -u;    break;  // +X
    case 1: dx = -1.0f; dy = -v;    dz = u;     break;  // -X
    case 2: dx = u;     dy = 1.0f;  dz = v;     break;  // +Y
    case 3: dx = u;     dy = -1.0f; dz = -v;    break;  // -Y
    case 4: dx = u;     dy = -v;    dz = 1.0f;  break;  // +Z
    case 5: dx = -u;    dy = -v;    dz = -1.0f; break;  // -Z
    }

    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    dx /= len; dy /= len; dz /= len;
}

// Simple cubemap sampling (would need actual texture data access in real implementation)
// For now, we'll use a simplified sky gradient approximation
static void SampleEnvironment(float dx, float dy, float dz, float& r, float& g, float& b)
{
    // Approximate our procedural sky gradient
    float horizonR = 0.7f, horizonG = 0.8f, horizonB = 0.9f;
    float zenithR = 0.2f, zenithG = 0.4f, zenithB = 0.8f;
    float nadirR = 0.3f, nadirG = 0.25f, nadirB = 0.2f;

    if (dy >= 0.0f)
    {
        float t = dy * dy;
        r = horizonR + t * (zenithR - horizonR);
        g = horizonG + t * (zenithG - horizonG);
        b = horizonB + t * (zenithB - horizonB);
    }
    else
    {
        float t = dy * dy;
        r = horizonR + t * (nadirR - horizonR);
        g = horizonG + t * (nadirG - horizonG);
        b = horizonB + t * (nadirB - horizonB);
    }
}

void IBL::GenerateIrradianceData(CubemapTexture* env, std::vector<std::vector<uint8_t>>& faceData, uint32_t size)
{
    for (int face = 0; face < 6; ++face)
    {
        faceData[face].resize(size * size * 4);

        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                float u = (float(x) + 0.5f) / float(size) * 2.0f - 1.0f;
                float v = (float(y) + 0.5f) / float(size) * 2.0f - 1.0f;

                float Nx, Ny, Nz;
                GetCubemapDirection(face, u, v, Nx, Ny, Nz);

                // Convolve with cosine-weighted hemisphere
                float irradianceR = 0.0f, irradianceG = 0.0f, irradianceB = 0.0f;
                float sampleDelta = 0.1f;  // Coarser sampling for speed
                float nrSamples = 0.0f;

                for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta)
                {
                    for (float theta = 0.0f; theta < 0.5f * PI; theta += sampleDelta)
                    {
                        // Spherical to cartesian (in tangent space)
                        float sinTheta = sinf(theta);
                        float cosTheta = cosf(theta);
                        float Tx = sinTheta * cosf(phi);
                        float Ty = sinTheta * sinf(phi);
                        float Tz = cosTheta;

                        // Create tangent basis
                        float upX = fabsf(Nz) < 0.999f ? 0.0f : 1.0f;
                        float upY = 0.0f;
                        float upZ = fabsf(Nz) < 0.999f ? 1.0f : 0.0f;

                        float tangentX = upY * Nz - upZ * Ny;
                        float tangentY = upZ * Nx - upX * Nz;
                        float tangentZ = upX * Ny - upY * Nx;
                        float tLen = sqrtf(tangentX * tangentX + tangentY * tangentY + tangentZ * tangentZ);
                        if (tLen > 0.0001f) { tangentX /= tLen; tangentY /= tLen; tangentZ /= tLen; }

                        float bitangentX = Ny * tangentZ - Nz * tangentY;
                        float bitangentY = Nz * tangentX - Nx * tangentZ;
                        float bitangentZ = Nx * tangentY - Ny * tangentX;

                        // Transform to world space
                        float sampleDirX = tangentX * Tx + bitangentX * Ty + Nx * Tz;
                        float sampleDirY = tangentY * Tx + bitangentY * Ty + Ny * Tz;
                        float sampleDirZ = tangentZ * Tx + bitangentZ * Ty + Nz * Tz;

                        float sr, sg, sb;
                        SampleEnvironment(sampleDirX, sampleDirY, sampleDirZ, sr, sg, sb);

                        irradianceR += sr * cosTheta * sinTheta;
                        irradianceG += sg * cosTheta * sinTheta;
                        irradianceB += sb * cosTheta * sinTheta;
                        nrSamples += 1.0f;
                    }
                }

                irradianceR = PI * irradianceR / nrSamples;
                irradianceG = PI * irradianceG / nrSamples;
                irradianceB = PI * irradianceB / nrSamples;

                uint32_t idx = (y * size + x) * 4;
                faceData[face][idx + 0] = static_cast<uint8_t>((std::min)(irradianceR, 1.0f) * 255.0f);
                faceData[face][idx + 1] = static_cast<uint8_t>((std::min)(irradianceG, 1.0f) * 255.0f);
                faceData[face][idx + 2] = static_cast<uint8_t>((std::min)(irradianceB, 1.0f) * 255.0f);
                faceData[face][idx + 3] = 255;
            }
        }
    }
}

void IBL::GeneratePrefilteredData(CubemapTexture* env, std::vector<std::vector<std::vector<uint8_t>>>& mipData, uint32_t size, uint32_t mipLevels)
{
    for (uint32_t mip = 0; mip < mipLevels; ++mip)
    {
        uint32_t mipSize = size >> mip;
        if (mipSize < 1) mipSize = 1;

        float roughness = float(mip) / float(mipLevels - 1);

        mipData[mip].resize(6);

        for (int face = 0; face < 6; ++face)
        {
            mipData[mip][face].resize(mipSize * mipSize * 4);

            for (uint32_t y = 0; y < mipSize; ++y)
            {
                for (uint32_t x = 0; x < mipSize; ++x)
                {
                    float u = (float(x) + 0.5f) / float(mipSize) * 2.0f - 1.0f;
                    float v = (float(y) + 0.5f) / float(mipSize) * 2.0f - 1.0f;

                    float Nx, Ny, Nz;
                    GetCubemapDirection(face, u, v, Nx, Ny, Nz);

                    // For mip 0 (roughness=0), just sample directly
                    if (roughness < 0.01f)
                    {
                        float r, g, b;
                        SampleEnvironment(Nx, Ny, Nz, r, g, b);

                        uint32_t idx = (y * mipSize + x) * 4;
                        mipData[mip][face][idx + 0] = static_cast<uint8_t>((std::min)(r, 1.0f) * 255.0f);
                        mipData[mip][face][idx + 1] = static_cast<uint8_t>((std::min)(g, 1.0f) * 255.0f);
                        mipData[mip][face][idx + 2] = static_cast<uint8_t>((std::min)(b, 1.0f) * 255.0f);
                        mipData[mip][face][idx + 3] = 255;
                        continue;
                    }

                    // Importance sample for rougher surfaces
                    float prefilteredR = 0.0f, prefilteredG = 0.0f, prefilteredB = 0.0f;
                    float totalWeight = 0.0f;

                    // Fewer samples for speed
                    const uint32_t SAMPLE_COUNT = 64u;
                    for (uint32_t i = 0u; i < SAMPLE_COUNT; ++i)
                    {
                        float xi1, xi2;
                        Hammersley(i, SAMPLE_COUNT, xi1, xi2);

                        float Hx, Hy, Hz;
                        ImportanceSampleGGX(xi1, xi2, roughness, Nx, Ny, Nz, Hx, Hy, Hz);

                        // L = 2.0 * dot(N, H) * H - N
                        float NdotH = Nx * Hx + Ny * Hy + Nz * Hz;
                        float Lx = 2.0f * NdotH * Hx - Nx;
                        float Ly = 2.0f * NdotH * Hy - Ny;
                        float Lz = 2.0f * NdotH * Hz - Nz;

                        float NdotL = Nx * Lx + Ny * Ly + Nz * Lz;
                        if (NdotL > 0.0f)
                        {
                            float sr, sg, sb;
                            SampleEnvironment(Lx, Ly, Lz, sr, sg, sb);

                            prefilteredR += sr * NdotL;
                            prefilteredG += sg * NdotL;
                            prefilteredB += sb * NdotL;
                            totalWeight += NdotL;
                        }
                    }

                    if (totalWeight > 0.0f)
                    {
                        prefilteredR /= totalWeight;
                        prefilteredG /= totalWeight;
                        prefilteredB /= totalWeight;
                    }

                    uint32_t idx = (y * mipSize + x) * 4;
                    mipData[mip][face][idx + 0] = static_cast<uint8_t>((std::min)(prefilteredR, 1.0f) * 255.0f);
                    mipData[mip][face][idx + 1] = static_cast<uint8_t>((std::min)(prefilteredG, 1.0f) * 255.0f);
                    mipData[mip][face][idx + 2] = static_cast<uint8_t>((std::min)(prefilteredB, 1.0f) * 255.0f);
                    mipData[mip][face][idx + 3] = 255;
                }
            }
        }
    }
}
