#include "RootSignature.h"
#include "Device.h"
#include <d3d12.h>
#include <windows.h>

RootSignature::RootSignature(GraphicsDevice* device)
    : m_device(device)
{
}

RootSignature::~RootSignature()
{
}

bool RootSignature::CreateEmpty()
{
    // Create a simple empty root signature (no parameters)
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"Empty Root Signature");
#endif

    return true;
}

bool RootSignature::CreateWithTextureAndCBV()
{
    // Root parameter 0: CBV for MVP matrices
    D3D12_ROOT_PARAMETER rootParams[2] = {};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;  // b0
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: Descriptor table for texture
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;  // t0
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static sampler
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 16;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;  // s0
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize textured root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create textured root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"Textured Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForPBR()
{
    // Root signature for PBR rendering
    // b0: MVP matrices (vertex shader)
    // b1: Material properties (pixel shader)
    // b2: Scene lighting data (pixel shader)
    // t0: Albedo texture (pixel shader)
    // s0: Linear sampler

    D3D12_ROOT_PARAMETER rootParams[4] = {};

    // Root parameter 0: CBV for MVP matrices (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: CBV for Material properties (b1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: CBV for Scene lighting (b2)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: Descriptor table for albedo texture (t0)
    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[3].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static sampler (s0)
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 16;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 4;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize PBR root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create PBR root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"PBR Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForPBRWithNormalMap()
{
    // Root signature for PBR rendering with normal mapping
    // b0: MVP matrices (vertex shader)
    // b1: Material properties (pixel shader)
    // b2: Scene lighting data (pixel shader)
    // t0: Albedo texture (pixel shader)
    // t1: Normal map texture (pixel shader)
    // s0: Linear sampler

    D3D12_ROOT_PARAMETER rootParams[5] = {};

    // Root parameter 0: CBV for MVP matrices (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: CBV for Material properties (b1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: CBV for Scene lighting (b2)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: Descriptor table for albedo texture (t0)
    D3D12_DESCRIPTOR_RANGE albedoRange = {};
    albedoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    albedoRange.NumDescriptors = 1;
    albedoRange.BaseShaderRegister = 0;  // t0
    albedoRange.RegisterSpace = 0;
    albedoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[3].DescriptorTable.pDescriptorRanges = &albedoRange;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 4: Descriptor table for normal map (t1)
    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;  // t1
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[4].DescriptorTable.pDescriptorRanges = &normalRange;
    rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static sampler (s0)
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 16;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 5;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize PBR with normal map root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create PBR with normal map root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"PBR with Normal Map Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForPBRWithShadows()
{
    // Root signature for PBR rendering with shadows
    // b0: MVP matrices (vertex shader)
    // b1: Material properties (pixel shader)
    // b2: Scene lighting data (pixel shader)
    // b3: Shadow constants (pixel shader)
    // t0: Albedo texture (pixel shader)
    // t1: Normal map texture (pixel shader)
    // t2: Shadow map texture (pixel shader)
    // s0: Linear sampler
    // s1: Shadow comparison sampler

    D3D12_ROOT_PARAMETER rootParams[7] = {};

    // Root parameter 0: CBV for MVP matrices (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: CBV for Material properties (b1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: CBV for Scene lighting (b2)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: CBV for Shadow constants (b3)
    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[3].Descriptor.ShaderRegister = 3;
    rootParams[3].Descriptor.RegisterSpace = 0;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 4: Descriptor table for albedo texture (t0)
    D3D12_DESCRIPTOR_RANGE albedoRange = {};
    albedoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    albedoRange.NumDescriptors = 1;
    albedoRange.BaseShaderRegister = 0;  // t0
    albedoRange.RegisterSpace = 0;
    albedoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[4].DescriptorTable.pDescriptorRanges = &albedoRange;
    rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 5: Descriptor table for normal map (t1)
    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;  // t1
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[5].DescriptorTable.pDescriptorRanges = &normalRange;
    rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 6: Descriptor table for shadow map (t2)
    D3D12_DESCRIPTOR_RANGE shadowRange = {};
    shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRange.NumDescriptors = 1;
    shadowRange.BaseShaderRegister = 2;  // t2
    shadowRange.RegisterSpace = 0;
    shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[6].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[6].DescriptorTable.pDescriptorRanges = &shadowRange;
    rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    // Sampler 0: Linear sampler for textures (s0)
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 16;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Sampler 1: Shadow comparison sampler (s1)
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].MipLODBias = 0.0f;
    samplers[1].MaxAnisotropy = 1;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[1].MinLOD = 0.0f;
    samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister = 1;
    samplers[1].RegisterSpace = 0;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 7;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = samplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize PBR with shadows root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create PBR with shadows root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"PBR with Shadows Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForPBRWithEnvironment()
{
    // Root signature for PBR rendering with shadows and environment reflections
    // b0: MVP matrices (vertex shader)
    // b1: Material properties (pixel shader)
    // b2: Scene lighting data (pixel shader)
    // b3: Shadow constants (pixel shader)
    // t0: Albedo texture (pixel shader)
    // t1: Normal map texture (pixel shader)
    // t2: Shadow map texture (pixel shader)
    // t3: Environment cubemap (pixel shader)
    // s0: Linear sampler
    // s1: Shadow comparison sampler

    D3D12_ROOT_PARAMETER rootParams[8] = {};

    // Root parameter 0: CBV for MVP matrices (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: CBV for Material properties (b1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: CBV for Scene lighting (b2)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: CBV for Shadow constants (b3)
    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[3].Descriptor.ShaderRegister = 3;
    rootParams[3].Descriptor.RegisterSpace = 0;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 4: Descriptor table for albedo texture (t0)
    D3D12_DESCRIPTOR_RANGE albedoRange = {};
    albedoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    albedoRange.NumDescriptors = 1;
    albedoRange.BaseShaderRegister = 0;  // t0
    albedoRange.RegisterSpace = 0;
    albedoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[4].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[4].DescriptorTable.pDescriptorRanges = &albedoRange;
    rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 5: Descriptor table for normal map (t1)
    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;  // t1
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[5].DescriptorTable.pDescriptorRanges = &normalRange;
    rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 6: Descriptor table for shadow map (t2)
    D3D12_DESCRIPTOR_RANGE shadowRange = {};
    shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRange.NumDescriptors = 1;
    shadowRange.BaseShaderRegister = 2;  // t2
    shadowRange.RegisterSpace = 0;
    shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[6].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[6].DescriptorTable.pDescriptorRanges = &shadowRange;
    rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 7: Descriptor table for environment cubemap (t3)
    D3D12_DESCRIPTOR_RANGE envRange = {};
    envRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    envRange.NumDescriptors = 1;
    envRange.BaseShaderRegister = 3;  // t3
    envRange.RegisterSpace = 0;
    envRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[7].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[7].DescriptorTable.pDescriptorRanges = &envRange;
    rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    // Sampler 0: Linear sampler for textures (s0)
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 16;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Sampler 1: Shadow comparison sampler (s1)
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].MipLODBias = 0.0f;
    samplers[1].MaxAnisotropy = 1;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[1].MinLOD = 0.0f;
    samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister = 1;
    samplers[1].RegisterSpace = 0;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 8;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = samplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize PBR with environment root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create PBR with environment root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"PBR with Environment Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForPBRWithIBL()
{
    // Root signature for PBR with Image-Based Lighting
    // b0: MVP matrices (vertex shader)
    // b1: Material properties (pixel shader)
    // b2: Scene lighting data (pixel shader)
    // b3: Shadow constants (pixel shader)
    // b4: IBL constants (pixel shader)
    // t0: Albedo texture
    // t1: Normal map texture
    // t2: Shadow map texture
    // t3: Environment cubemap
    // t4: Irradiance map (cubemap)
    // t5: Prefiltered environment map (cubemap)
    // t6: BRDF LUT (2D texture)
    // s0: Linear sampler
    // s1: Shadow comparison sampler

    D3D12_ROOT_PARAMETER rootParams[12] = {};

    // Root parameter 0: CBV for MVP matrices (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: CBV for Material properties (b1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: CBV for Scene lighting (b2)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: CBV for Shadow constants (b3)
    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[3].Descriptor.ShaderRegister = 3;
    rootParams[3].Descriptor.RegisterSpace = 0;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 4: CBV for IBL constants (b4)
    rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[4].Descriptor.ShaderRegister = 4;
    rootParams[4].Descriptor.RegisterSpace = 0;
    rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 5: Descriptor table for albedo texture (t0)
    D3D12_DESCRIPTOR_RANGE albedoRange = {};
    albedoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    albedoRange.NumDescriptors = 1;
    albedoRange.BaseShaderRegister = 0;
    albedoRange.RegisterSpace = 0;
    albedoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[5].DescriptorTable.pDescriptorRanges = &albedoRange;
    rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 6: Descriptor table for normal map (t1)
    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[6].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[6].DescriptorTable.pDescriptorRanges = &normalRange;
    rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 7: Descriptor table for shadow map (t2)
    D3D12_DESCRIPTOR_RANGE shadowRange = {};
    shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRange.NumDescriptors = 1;
    shadowRange.BaseShaderRegister = 2;
    shadowRange.RegisterSpace = 0;
    shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[7].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[7].DescriptorTable.pDescriptorRanges = &shadowRange;
    rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 8: Descriptor table for environment cubemap (t3)
    D3D12_DESCRIPTOR_RANGE envRange = {};
    envRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    envRange.NumDescriptors = 1;
    envRange.BaseShaderRegister = 3;
    envRange.RegisterSpace = 0;
    envRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[8].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[8].DescriptorTable.pDescriptorRanges = &envRange;
    rootParams[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 9: Descriptor table for irradiance map (t4)
    D3D12_DESCRIPTOR_RANGE irradianceRange = {};
    irradianceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    irradianceRange.NumDescriptors = 1;
    irradianceRange.BaseShaderRegister = 4;
    irradianceRange.RegisterSpace = 0;
    irradianceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[9].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[9].DescriptorTable.pDescriptorRanges = &irradianceRange;
    rootParams[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 10: Descriptor table for prefiltered map (t5)
    D3D12_DESCRIPTOR_RANGE prefilteredRange = {};
    prefilteredRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    prefilteredRange.NumDescriptors = 1;
    prefilteredRange.BaseShaderRegister = 5;
    prefilteredRange.RegisterSpace = 0;
    prefilteredRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[10].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[10].DescriptorTable.pDescriptorRanges = &prefilteredRange;
    rootParams[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 11: Descriptor table for BRDF LUT (t6)
    D3D12_DESCRIPTOR_RANGE brdfRange = {};
    brdfRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    brdfRange.NumDescriptors = 1;
    brdfRange.BaseShaderRegister = 6;
    brdfRange.RegisterSpace = 0;
    brdfRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[11].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[11].DescriptorTable.pDescriptorRanges = &brdfRange;
    rootParams[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    // Sampler 0: Linear sampler for textures (s0)
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 16;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Sampler 1: Shadow comparison sampler (s1)
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].MipLODBias = 0.0f;
    samplers[1].MaxAnisotropy = 1;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[1].MinLOD = 0.0f;
    samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister = 1;
    samplers[1].RegisterSpace = 0;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 12;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = samplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize PBR with IBL root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create PBR with IBL root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"PBR with IBL Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForPBRWithSSAO()
{
    // Root signature for PBR with Image-Based Lighting and SSAO
    // b0: MVP matrices (vertex shader)
    // b1: Material properties (pixel shader)
    // b2: Scene lighting data (pixel shader)
    // b3: Shadow constants (pixel shader)
    // b4: IBL constants (pixel shader)
    // t0: Albedo texture
    // t1: Normal map texture
    // t2: Shadow map texture
    // t3: Environment cubemap
    // t4: Irradiance map (cubemap)
    // t5: Prefiltered environment map (cubemap)
    // t6: BRDF LUT (2D texture)
    // t7: SSAO texture (2D texture)
    // s0: Linear sampler
    // s1: Shadow comparison sampler

    D3D12_ROOT_PARAMETER rootParams[13] = {};

    // Root parameter 0: CBV for MVP matrices (b0)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Root parameter 1: CBV for Material properties (b1)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 1;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 2: CBV for Scene lighting (b2)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[2].Descriptor.ShaderRegister = 2;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 3: CBV for Shadow constants (b3)
    rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[3].Descriptor.ShaderRegister = 3;
    rootParams[3].Descriptor.RegisterSpace = 0;
    rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 4: CBV for IBL constants (b4)
    rootParams[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[4].Descriptor.ShaderRegister = 4;
    rootParams[4].Descriptor.RegisterSpace = 0;
    rootParams[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 5: Descriptor table for albedo texture (t0)
    D3D12_DESCRIPTOR_RANGE albedoRange = {};
    albedoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    albedoRange.NumDescriptors = 1;
    albedoRange.BaseShaderRegister = 0;
    albedoRange.RegisterSpace = 0;
    albedoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[5].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[5].DescriptorTable.pDescriptorRanges = &albedoRange;
    rootParams[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 6: Descriptor table for normal map (t1)
    D3D12_DESCRIPTOR_RANGE normalRange = {};
    normalRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    normalRange.NumDescriptors = 1;
    normalRange.BaseShaderRegister = 1;
    normalRange.RegisterSpace = 0;
    normalRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[6].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[6].DescriptorTable.pDescriptorRanges = &normalRange;
    rootParams[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 7: Descriptor table for shadow map (t2)
    D3D12_DESCRIPTOR_RANGE shadowRange = {};
    shadowRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    shadowRange.NumDescriptors = 1;
    shadowRange.BaseShaderRegister = 2;
    shadowRange.RegisterSpace = 0;
    shadowRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[7].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[7].DescriptorTable.pDescriptorRanges = &shadowRange;
    rootParams[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 8: Descriptor table for environment cubemap (t3)
    D3D12_DESCRIPTOR_RANGE envRange = {};
    envRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    envRange.NumDescriptors = 1;
    envRange.BaseShaderRegister = 3;
    envRange.RegisterSpace = 0;
    envRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[8].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[8].DescriptorTable.pDescriptorRanges = &envRange;
    rootParams[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 9: Descriptor table for irradiance map (t4)
    D3D12_DESCRIPTOR_RANGE irradianceRange = {};
    irradianceRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    irradianceRange.NumDescriptors = 1;
    irradianceRange.BaseShaderRegister = 4;
    irradianceRange.RegisterSpace = 0;
    irradianceRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[9].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[9].DescriptorTable.pDescriptorRanges = &irradianceRange;
    rootParams[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 10: Descriptor table for prefiltered map (t5)
    D3D12_DESCRIPTOR_RANGE prefilteredRange = {};
    prefilteredRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    prefilteredRange.NumDescriptors = 1;
    prefilteredRange.BaseShaderRegister = 5;
    prefilteredRange.RegisterSpace = 0;
    prefilteredRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[10].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[10].DescriptorTable.pDescriptorRanges = &prefilteredRange;
    rootParams[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 11: Descriptor table for BRDF LUT (t6)
    D3D12_DESCRIPTOR_RANGE brdfRange = {};
    brdfRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    brdfRange.NumDescriptors = 1;
    brdfRange.BaseShaderRegister = 6;
    brdfRange.RegisterSpace = 0;
    brdfRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[11].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[11].DescriptorTable.pDescriptorRanges = &brdfRange;
    rootParams[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root parameter 12: Descriptor table for SSAO texture (t7)
    D3D12_DESCRIPTOR_RANGE ssaoRange = {};
    ssaoRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ssaoRange.NumDescriptors = 1;
    ssaoRange.BaseShaderRegister = 7;
    ssaoRange.RegisterSpace = 0;
    ssaoRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[12].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[12].DescriptorTable.pDescriptorRanges = &ssaoRange;
    rootParams[12].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    // Sampler 0: Linear sampler for textures (s0)
    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplers[0].MipLODBias = 0.0f;
    samplers[0].MaxAnisotropy = 16;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Sampler 1: Shadow comparison sampler (s1)
    samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplers[1].MipLODBias = 0.0f;
    samplers[1].MaxAnisotropy = 1;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplers[1].MinLOD = 0.0f;
    samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister = 1;
    samplers[1].RegisterSpace = 0;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 13;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 2;
    rootSigDesc.pStaticSamplers = samplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize PBR with SSAO root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create PBR with SSAO root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"PBR with SSAO Root Signature");
#endif

    return true;
}

bool RootSignature::CreateForSkybox()
{
    // Root signature for skybox rendering
    // b0: Skybox constants (camera inverse VP matrix, position, exposure) - visible to both VS and PS
    // t0: Cubemap texture (pixel shader)
    // s0: Linear sampler

    D3D12_ROOT_PARAMETER rootParams[2] = {};

    // Root parameter 0: CBV for skybox constants (b0) - visible to all stages
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Root parameter 1: Descriptor table for cubemap texture (t0)
    D3D12_DESCRIPTOR_RANGE cubemapRange = {};
    cubemapRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    cubemapRange.NumDescriptors = 1;
    cubemapRange.BaseShaderRegister = 0;  // t0
    cubemapRange.RegisterSpace = 0;
    cubemapRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &cubemapRange;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Static sampler (s0) - linear sampler with clamp addressing
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0.0f;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Root signature description - note: no input assembler needed for fullscreen triangle
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 2;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;  // No input layout needed

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
        }
        MessageBox(nullptr, L"Failed to serialize skybox root signature", L"Error", MB_OK);
        return false;
    }

    hr = m_device->GetD3D12Device()->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr))
    {
        MessageBox(nullptr, L"Failed to create skybox root signature", L"Error", MB_OK);
        return false;
    }

#if defined(_DEBUG)
    m_rootSignature->SetName(L"Skybox Root Signature");
#endif

    return true;
}
