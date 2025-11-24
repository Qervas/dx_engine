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
