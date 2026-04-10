#pragma once

class Pipeline
{
public:
	void Init(ID3D12Device5* device);

	ID3D12PipelineState* GetStaticMeshPso() const { return m_staticMeshPso.Get(); }
	ID3D12PipelineState* GetDrawMapPso() const { return m_drawMapPso.Get(); }
	ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRootSign.Get(); }


private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateGlobalRootSignature(ID3D12Device* device);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> CreateStaticMeshPso(ID3D12Device* device);


private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_globalRootSign;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_staticMeshPso;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_drawMapPso;
};
