#include "Shader.h"
#include <d3dcompiler.h>
#include <windows.h>

#pragma comment(lib, "d3dcompiler.lib")

Shader::Shader()
{
}

Shader::~Shader()
{
}

bool Shader::CompileFromFile(const std::wstring& filePath, const std::string& entryPoint, const std::string& target)
{
    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompileFromFile(
        filePath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(),
        target.c_str(),
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "Shader Compilation Error", MB_OK);
        }
        else
        {
            MessageBox(nullptr, L"Failed to compile shader from file", L"Error", MB_OK);
        }
        return false;
    }

    // Copy bytecode
    m_bytecode.resize(shaderBlob->GetBufferSize());
    memcpy(m_bytecode.data(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

    return true;
}

D3D12_SHADER_BYTECODE Shader::GetBytecode() const
{
    D3D12_SHADER_BYTECODE bytecode;
    bytecode.pShaderBytecode = m_bytecode.data();
    bytecode.BytecodeLength = m_bytecode.size();
    return bytecode;
}
