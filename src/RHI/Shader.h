#pragma once

#include "../Core/Types.h"
#include <d3d12.h>
#include <string>
#include <vector>

class Shader
{
public:
    Shader();
    ~Shader();

    // Compile from file
    bool CompileFromFile(const std::wstring& filePath, const std::string& entryPoint, const std::string& target);

    // Get shader bytecode
    D3D12_SHADER_BYTECODE GetBytecode() const;
    const void* GetBufferPointer() const { return m_bytecode.data(); }
    size_t GetBufferSize() const { return m_bytecode.size(); }

private:
    std::vector<uint8_t> m_bytecode;
};
