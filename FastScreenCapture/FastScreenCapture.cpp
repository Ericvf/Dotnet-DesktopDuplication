#include "FastScreenCapture.h"

HRESULT Initialize(int pOutputNum, int width, int height) {
	isInitialized = false;
	HRESULT hr = S_OK;

	if (!deviceInitialized) {
		hr = InitDevice();

		if (FAILED(hr))
			return hr;
	}

	if (outputWidth != width || outputHeight != height)
	{
		outputWidth = width;
		outputHeight = height;

		hr = InitResources();
		if (FAILED(hr))
			return hr;
	}

	//if (pOutputNum != outputNum)
	{
		outputNum = pOutputNum;
		hr = InitDuplication(pOutputNum);
		if (FAILED(hr))
			return hr;
	}

	isInitialized = true;
	return hr;
}

HRESULT InitDevice() {

	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (deviceContext)
	{
		deviceContext->Release();
		deviceContext = nullptr;
	}

	HRESULT hr = D3D11CreateDevice(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D11_CREATE_DEVICE_DEBUG,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&device,
		NULL,
		&deviceContext);

	if (FAILED(hr))
		return hr;

	// Create the sampler state
	D3D11_SAMPLER_DESC SampDesc;
	RtlZeroMemory(&SampDesc, sizeof(SampDesc));
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = device->CreateSamplerState(&SampDesc, &pointSamplerState);
	if (FAILED(hr))
		return hr;

	// Create vertex shader
	UINT Size = ARRAYSIZE(g_VS);
	hr = device->CreateVertexShader(g_VS, Size, nullptr, &VertexShader);
	if (FAILED(hr))
		return hr;

	UINT NumElements = ARRAYSIZE(Layout);
	hr = device->CreateInputLayout(Layout, NumElements, g_VS, Size, &InputLayout);
	if (FAILED(hr))
		return hr;

	deviceContext->IASetInputLayout(InputLayout);

	// Create pixel shader
	Size = ARRAYSIZE(g_PS);
	hr = device->CreatePixelShader(g_PS, Size, nullptr, &PixelShader);
	if (FAILED(hr))
		return hr;

	// Create vertex buffer
	D3D11_BUFFER_DESC BufferDesc;
	RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	RtlZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = Vertices;

	hr = device->CreateBuffer(&BufferDesc, &InitData, &VertexBuffer);
	if (FAILED(hr))
		return hr;

	deviceInitialized = true;
	return hr;
}

HRESULT InitDuplication(int outputNum) {

	IDXGIDevice* dxgiDevice = nullptr;
	HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
	if (FAILED(hr))
		return hr;

	IDXGIAdapter* dxgiAdapter = nullptr;
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapter));
	if (FAILED(hr))
		return hr;

	dxgiDevice->Release();

	IDXGIOutput* dxgiOutput = nullptr;
	hr = dxgiAdapter->EnumOutputs(outputNum, &dxgiOutput);
	if (FAILED(hr))
		return hr;

	dxgiAdapter->Release();

	IDXGIOutput1* dxgiOutput1 = nullptr;
	hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&dxgiOutput1));
	if (FAILED(hr))
		return hr;

	dxgiOutput->Release();

	if (outputDuplication)
	{
		outputDuplication->Release();
		outputDuplication = nullptr;
	}

	dxgiOutput1->DuplicateOutput(device, &outputDuplication);
	if (FAILED(hr))
		return hr;

	dxgiOutput1->Release();
	return hr;
}

HRESULT InitResources() {
	// Create viewports
	ZeroMemory(&renderTargetViewport, sizeof(D3D11_VIEWPORT));
	renderTargetViewport.TopLeftX = 0;
	renderTargetViewport.TopLeftY = 0;
	renderTargetViewport.Width = (float)outputWidth;
	renderTargetViewport.Height = (float)outputHeight;

	// Create staging texture
	D3D11_TEXTURE2D_DESC stagingTextureDescription;
	ZeroMemory(&stagingTextureDescription, sizeof(stagingTextureDescription));
	stagingTextureDescription.Width = outputWidth;
	stagingTextureDescription.Height = outputHeight;
	stagingTextureDescription.MipLevels = 1;
	stagingTextureDescription.ArraySize = 1;
	stagingTextureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	stagingTextureDescription.Usage = D3D11_USAGE_STAGING;
	stagingTextureDescription.BindFlags = 0;
	stagingTextureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingTextureDescription.MiscFlags = 0;
	stagingTextureDescription.SampleDesc.Count = 1;
	stagingTextureDescription.SampleDesc.Quality = 0;

	if (stagingTexture)
	{
		stagingTexture->Release();
		stagingTexture = nullptr;
	}

	HRESULT hr = device->CreateTexture2D(&stagingTextureDescription, NULL, &stagingTexture);
	if (FAILED(hr))
		return hr;

	// Create render target texture
	D3D11_TEXTURE2D_DESC renderTargetTextureDesc;
	ZeroMemory(&renderTargetTextureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	renderTargetTextureDesc.Width = outputWidth;
	renderTargetTextureDesc.Height = outputHeight;
	renderTargetTextureDesc.MipLevels = 1;
	renderTargetTextureDesc.ArraySize = 1;
	renderTargetTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	renderTargetTextureDesc.SampleDesc.Count = 1;
	renderTargetTextureDesc.SampleDesc.Quality = 0;
	renderTargetTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	renderTargetTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	renderTargetTextureDesc.CPUAccessFlags = 0;
	renderTargetTextureDesc.MiscFlags = 0;

	if (renderTargetTexture)
	{
		renderTargetTexture->Release();
		renderTargetTexture = nullptr;
	}

	hr = device->CreateTexture2D(&renderTargetTextureDesc, NULL, &renderTargetTexture);
	if (FAILED(hr))
		return hr;

	// Create render target view
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	renderTargetViewDesc.Format = renderTargetTextureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	hr = device->CreateRenderTargetView(renderTargetTexture, &renderTargetViewDesc, &renderTargetView);
	if (FAILED(hr))
		return hr;

	// Create render target resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC renderTargetResourceViewDesc;
	renderTargetResourceViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	renderTargetResourceViewDesc.Texture2D.MipLevels = 1;
	renderTargetResourceViewDesc.Texture2D.MostDetailedMip = 0;
	renderTargetResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	if (renderTargetResourceView)
	{
		renderTargetResourceView->Release();
		renderTargetResourceView = nullptr;
	}

	hr = device->CreateShaderResourceView(renderTargetTexture, &renderTargetResourceViewDesc, &renderTargetResourceView);
	if (FAILED(hr))
		return hr;

	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(VS_CONSTANT_BUFFER), D3D11_BIND_CONSTANT_BUFFER);
	hr = device->CreateBuffer(
		&constantBufferDesc,
		nullptr,
		&ConstantBuffer
	);

	if (FAILED(hr))
		return hr;

	return hr;
}

void CaptureScreen(byte* imageData, bool sbs, bool hou) 
{
	if (!isInitialized)
		return;

	IDXGIResource* DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	HRESULT hr = outputDuplication->AcquireNextFrame(1, &FrameInfo, &DesktopResource);

	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
		return;

	if (FAILED(hr))
		return;

	// Acquire image
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&acquiredDesktopImage));
	if (FAILED(hr))
		return;

	DesktopResource->Release();
	DesktopResource = nullptr;

	// get acquired desc
	D3D11_TEXTURE2D_DESC acquiredTextureDescription;
	acquiredDesktopImage->GetDesc(&acquiredTextureDescription);

	// Create acquired image resource view 
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = acquiredTextureDescription.Format;
	shaderResourceViewDesc.Texture2D.MipLevels = acquiredTextureDescription.MipLevels;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = acquiredTextureDescription.MipLevels - 1;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

	ID3D11ShaderResourceView* shaderResourceView = nullptr;
	hr = device->CreateShaderResourceView(acquiredDesktopImage, &shaderResourceViewDesc, &shaderResourceView);
	if (FAILED(hr))
		return;

	float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	constantBufferData.isSBS = sbs ? 1.0f : 0;
	constantBufferData.isHOU = hou ? 1.0f : 0;

	deviceContext->UpdateSubresource(
		ConstantBuffer,
		0,
		NULL,
		&constantBufferData,
		0,
		0
	);

	// Set render target 
	deviceContext->RSSetViewports(1, &renderTargetViewport);
	deviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
	deviceContext->ClearRenderTargetView(renderTargetView, color);

	// Set device resources
	deviceContext->VSSetShader(VertexShader, nullptr, 0);
	deviceContext->PSSetShader(PixelShader, nullptr, 0);
	deviceContext->PSSetShaderResources(0, 1, &shaderResourceView);
	deviceContext->PSSetSamplers(0, 1, &pointSamplerState);
	deviceContext->PSSetConstantBuffers(0, 1, &ConstantBuffer);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

	// Draw render target
	deviceContext->Draw(NUMVERTICES, 0);

	shaderResourceView->Release();

	// Copy rendertarget to staging texture
	deviceContext->CopyResource(stagingTexture, renderTargetTexture);

	// Map the staging texture resource
	int subResource = 0;
	D3D11_MAPPED_SUBRESOURCE resource;
	ZeroMemory(&resource, sizeof(resource));
	hr = deviceContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0x0, &resource);

	// Copy texture data
	byte* outputReader = (byte*)imageData;
	byte* textureReader = (byte*)resource.pData;

	for (int y = 0; y < outputHeight; ++y)
	{
		memcpy(outputReader, textureReader, outputWidth * 4);
		outputReader += outputWidth * 4;
		textureReader += resource.RowPitch;
	}

	// Release map
	deviceContext->Unmap(stagingTexture, subResource);

	outputDuplication->ReleaseFrame();
}

void Clean() {

	outputDuplication->Release();
	outputDuplication = nullptr;

	deviceContext->Release();
	deviceContext = nullptr;

	device->Release();
	device = nullptr;
}