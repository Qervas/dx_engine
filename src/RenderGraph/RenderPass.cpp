#include "RenderPass.h"

void RenderPass::AddInput(RGResourceHandle resource, ResourceAccess access, D3D12_RESOURCE_STATES state)
{
    m_inputs.push_back({ resource, access, state });
}

void RenderPass::AddOutput(RGResourceHandle resource, ResourceAccess access, D3D12_RESOURCE_STATES state)
{
    m_outputs.push_back({ resource, access, state });
}
