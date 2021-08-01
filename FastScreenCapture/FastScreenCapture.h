#pragma once

#pragma comment (lib, "d3d11.lib")

#include <Windows.h>
#include <d3d11_2.h>
#include <DirectXMath.h>

#include "PixelShader.h"
#include "VertexShader.h"

using namespace DirectX;

extern "C" {
	__declspec(dllexport) HRESULT Initialize(int outputNum, int width, int height);
	__declspec(dllexport) void CaptureScreen(byte* imageData, bool sbs, bool hou);
	__declspec(dllexport) void Clean();
}

bool deviceInitialized = false;
bool isInitialized = false; 

HRESULT InitDevice();
HRESULT InitDuplication(int outputNum);
HRESULT InitResources();

typedef struct _VERTEX
{
	XMFLOAT3 Pos;
	XMFLOAT2 TexCoord;
} VERTEX;

__declspec(align(16))
struct VS_CONSTANT_BUFFER {
	float isSBS;
	float isHOU;
};

VS_CONSTANT_BUFFER constantBufferData;

int outputWidth = 0;
int outputHeight = 0;
int outputNum = -1;

ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
IDXGIOutputDuplication* outputDuplication;
ID3D11Texture2D* acquiredDesktopImage;

ID3D11VertexShader* VertexShader;
ID3D11PixelShader* PixelShader;
ID3D11InputLayout* InputLayout;
ID3D11Buffer* VertexBuffer;
ID3D11Buffer* ConstantBuffer;

ID3D11RenderTargetView* renderTargetView;
ID3D11Texture2D* renderTargetTexture;
ID3D11ShaderResourceView* renderTargetResourceView;
ID3D11SamplerState* pointSamplerState;
ID3D11Texture2D* stagingTexture;
D3D11_VIEWPORT renderTargetViewport;

#define NUMVERTICES 6
VERTEX Vertices[NUMVERTICES] =
{
	{ XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
	{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
	{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
	{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
	{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
	{ XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
};

const UINT Stride = sizeof(VERTEX);
const UINT Offset = 0;

// Create input layout
D3D11_INPUT_ELEMENT_DESC Layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};
