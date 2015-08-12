#pragma once
#include "modules/gx2/gx2.h"
#include "modules/gx2/gx2_draw.h"
#include "modules/gx2/gx2_context.h"
#include "modules/gx2/gx2_texture.h"
#include "modules/gx2/gx2_shaders.h"
#include "hostlookup.h"
#include "dx12.h"
#include "dx12_heap.h"

struct DXScanBufferData;
struct DXColorBufferData;
struct DXDepthBufferData;

struct DXState {
   DXState()
   {

   }

   static const UINT FrameCount = 2;

   // DX Basics
   ComPtr<IDXGISwapChain3> swapChain;
   ComPtr<ID3D12Device> device;
   ComPtr<ID3D12Resource> renderTargets[FrameCount];
   ComPtr<ID3D12CommandAllocator> commandAllocator[FrameCount];
   ComPtr<ID3D12CommandQueue> commandQueue;
   ComPtr<ID3D12RootSignature> rootSignature;
   DXHeap *srvHeap;
   DXHeap *rtvHeap;
   ComPtr<ID3D12PipelineState> pipelineState;
   ComPtr<ID3D12GraphicsCommandList> commandList;
   DXHeapItemPtr scanbufferRtv[FrameCount];
   DXHeapItemPtr curScanbufferRtv;
   D3D12_VIEWPORT viewport;
   D3D12_RECT scissorRect;
   ComPtr<ID3D12Resource> vertexBuffer;
   D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

   // DX Synchronization objects.
   UINT frameIndex;
   HANDLE fenceEvent;
   ComPtr<ID3D12Fence> fence;
   UINT swapCount;

   // Emulator Objects
   DXScanBufferData *tvScanBuffer;
   DXScanBufferData *drcScanBuffer;

};

extern DXState gDX;

namespace dx {

   void initialise();
   void renderScanBuffers();

   DXColorBufferData * getColorBuffer(GX2ColorBuffer *buffer);
   DXDepthBufferData * getDepthBuffer(GX2DepthBuffer *buffer);

   void _beginFrame();
   void _endFrame();

}