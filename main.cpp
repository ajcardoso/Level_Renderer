// Simple basecode showing how to create a window and attatch a d3d11surface
#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
// Ignore some GRAPHICS libraries we aren't going to use
#define GATEWARE_DISABLE_GDIRECTX12SURFACE // we have another template for this
#define GATEWARE_DISABLE_GRASTERSURFACE // we have another template for this
#define GATEWARE_DISABLE_GOPENGLSURFACE // we have another template for this
#define GATEWARE_DISABLE_GVULKANSURFACE // we have another template for this
// With what we want & what we don't defined we can include the API
#define GATEWARE_ENABLE_MATH // NEED THIS
#define GATEWARE_ENABLE_INPUT 


#include "../gateware-main/Gateware.h"
#include "FileIntoString.h"
#include "renderer.h"

// example rendering code (not Gateware code!)
// open some namespaces to compact the code a bit
using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;
// lets pop a window and use D3D11 to clear to a green screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GDirectX11Surface d3d11;
	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		// TODO: Part 1A 
		win.SetWindowName("Armando J. Cardoso - Final Project -  DirectX 11");

		float clr[] = { 0.0f, 0.0f, 0.5f, 0 }; // TODO: Part 1A (optional) background
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clr[2] += 0.01f; // move towards a cyan as they resize
		});
		win.Register(msgs);
		if (+d3d11.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		{
			Renderer renderer(win, d3d11);
			while (+win.ProcessWindowEvents())
			{
				IDXGISwapChain* swap;
				ID3D11DeviceContext* con;
				ID3D11RenderTargetView* view;
				ID3D11DepthStencilView* depth;
				if (+d3d11.GetImmediateContext((void**)&con) &&
					+d3d11.GetRenderTargetView((void**)&view) &&
					+d3d11.GetDepthStencilView((void**)&depth) &&
					+d3d11.GetSwapchain((void**)&swap))
				{
					con->ClearRenderTargetView(view, clr);
					con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0);
					renderer.UpdateCamera();
					renderer.Render();
					swap->Present(1, 0);
					// release incremented COM reference counts
					swap->Release();
					view->Release();
					depth->Release();
					con->Release();
				}
			}
		}
	}
	return 0; // that's all folks
}