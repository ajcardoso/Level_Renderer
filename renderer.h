#include <d3dcompiler.h>	// required for compiling shaders on the fly, consider pre-compiling instead
#include "load_object_oriented.h"
#pragma comment(lib, "d3dcompiler.lib") 

// Creation, Rendering & Cleanup
class Renderer
{
	// Class that holds all level objects
	Level_Objects level_obj;
	Model models;
	SceneData _sceneData;			  // struct accessors

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;

	GW::MATH::GVECTORF _lightDir;     // light direction vector

	GW::MATH::GVECTORF _lightColor;   // light color vector

	GW::MATH::GMATRIXF view;		  // view matrix
	GW::MATH::GMATRIXF projection;	  // projection matrix
	GW::MATH::GMATRIXF world;		  // world matrix
	GW::MATH::GMATRIXF rotatedWorld;  // rotate world

	// scene buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneDataBuffer;

	// timer
	std::chrono::high_resolution_clock::time_point lastTime;
	float deltaTime;

	// controller input
	GW::INPUT::GInput gInput;

	// level select
	bool isLevelSwaped = false;
	bool whichLevel = true;
	float _NumPad1 = 0.0f;
	float _NumPad2 = 0.0f;



public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		win = _win;
		d3d = _d3d;
		
		gInput.Create(win);
		
		// COMMENT OUT IF YOU WANT LEVEL 1 TO NOT POPULATE FIRST
		GW::SYSTEM::GLog gLog;
		gLog.Create("errorLog.txt");
		gLog.EnableConsoleLogging(true); // shows all loaded items
		level_obj.LoadLevel("../GameLevel.txt","../Models", gLog.Relinquish());
		
		// UNCOMMENT IF YOU WANT LEVEL 2 TO POPULATE FIRST
		//GW::SYSTEM::GLog gLog2;
		//
		//gLog2.Create("errorLog2.txt");
		//gLog2.EnableConsoleLogging(true); // shows all loaded items
		//level_obj.LoadLevel("../GameLevel2.txt", "../Models2", gLog2.Relinquish());
		

		IntializeGraphics();
	}

private:
	//constructor helper functions
	void IntializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);


		level_obj.UploadLevelToGPU(creator);

		ViewMatrixBuilder();

		ProjectionMatrixBuilder();

		LightVecBuilder();
		
		CreateSceneBuffer(creator);

		// free temporary handle
		creator->Release();
	}


public:
	void Render()
	{
		// Select level but the level needs to be rendered prior to switching
		SelectLevel();

		PipelineHandles curHandles = GetCurrentPipelineHandles();

		SetRenderTargets(curHandles);
				
		D3D11_MAPPED_SUBRESOURCE sceneMap = { 0 };
		curHandles.context->Map(sceneDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sceneMap);
		memcpy(sceneMap.pData, &_sceneData, sizeof(_sceneData));
		curHandles.context->Unmap(sceneDataBuffer.Get(), 0);

		// easier way to map and unmap information ^^^ proof above works as well ^^^
		//curHandles.context->UpdateSubresource(sceneDataBuffer.Get(), 0, nullptr, &_sceneData, 0, 0);


		curHandles.context->VSSetConstantBuffers(0, 1, sceneDataBuffer.GetAddressOf());
		curHandles.context->PSSetConstantBuffers(0, 1, sceneDataBuffer.GetAddressOf());

		level_obj.RenderLevel(curHandles);

		ReleasePipelineHandles(curHandles);
	}

private:

	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetImmediateContext((void**)&retval.context);
		d3d.GetRenderTargetView((void**)&retval.targetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencil);
		return retval;
	}
	void CreateSceneBuffer(ID3D11Device* creator) {
		D3D11_BUFFER_DESC bufferScene;
		bufferScene.Usage = D3D11_USAGE_DYNAMIC;
		bufferScene.ByteWidth = sizeof(_sceneData);
		bufferScene.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferScene.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferScene.MiscFlags = 0;
		creator->CreateBuffer(&bufferScene, nullptr, sceneDataBuffer.GetAddressOf());
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

public:
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here yet 
	}

	// reset scene
	void SceneBuffClear() {
		sceneDataBuffer.ReleaseAndGetAddressOf();
	}

	// helper functions for world
	void WorldMatrixBuilder(float _xTranslation=0, float _yTranslation=0, float _zTranslation=0, float yRotation=0, float xRotation=0, float _wTranslation = 0) {
		
		world = GW::MATH::GIdentityMatrixF;
		// rotatedWorld is reset to idenetity
		rotatedWorld = world;
		models._meshData.wMatrix = world;
	}

	// helper functions for view
	void ViewMatrixBuilder() {
		// camera vector
		GW::MATH::GVECTORF camera;
		camera.x = 1.25f;
		camera.y = 7.5;
		camera.z = -5.0f;
		camera.w = 0.0f;
		_sceneData.cameraPos = camera;
		// target position of the camera
		GW::MATH::GVECTORF targetPos;
		targetPos.x = 0.15f;
		targetPos.y = 0.75f;
		targetPos.z = 0.0f;
		targetPos.w = 0.0f;
		// Orientation of vector
		GW::MATH::GVECTORF orientation;
		orientation.x = 0.0f;
		orientation.y = 1.0f;
		orientation.z = 0.0f;
		orientation.w = 1.0f;
		GW::MATH::GMatrix::LookAtLHF(camera, targetPos, orientation, view);
		_sceneData.vMatrix = view;
	}

	// helper functions for projection
	void ProjectionMatrixBuilder() {
		// field of view
		float FOV = G_DEGREE_TO_RADIAN(65.0f);
		// near plane
		float nPlane = 0.1f;
		// far plane
		float fPlane = 100.0f;
		// aspect ratio
		float aspectRatio = 0.0f;
		d3d.GetAspectRatio(aspectRatio);
		GW::MATH::GMatrix::ProjectionDirectXLHF(FOV, aspectRatio, nPlane, fPlane, projection);
		_sceneData.pMatrix = projection;
	}

	// helper functions for lighting
	void LightVecBuilder() {
		_lightColor = { 0.9f,0.9f,1.0f,1.0f };
		_lightDir = { -1.0f, -1.0f, 2.0f, 0.0f };
		GW::MATH::GVector::NormalizeF(_lightDir, _lightDir);
		SetAmbientLight();
		_sceneData.lightColor = _lightColor;
		_sceneData.lightDirc = _lightDir;
	}

	//  setting sun ambient
	void SetAmbientLight() {
		_sceneData.sunAmbient = { 0.25f, 0.25f, 0.35f };
	}
	
	// camera controls
	void UpdateCamera() {

		std::chrono::high_resolution_clock::time_point _now = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds> (_now - lastTime).count() / 1000000.0f;
		lastTime = _now;

		GW::MATH::GMATRIXF _view;
		GW::MATH::GMatrix::InverseF(_sceneData.vMatrix, _view);

		// camera speed
		float cameraSpeed = 1.0f;

		float FPS = cameraSpeed * deltaTime;

		// keyboard controls

		// space
		float spaceBar = 0.0f;
		gInput.GetState(G_KEY_SPACE, spaceBar);

		// left shift
		float leftShiftBtn = 0.0f;
		gInput.GetState(G_KEY_LEFTSHIFT, leftShiftBtn);

		// testing shift btn
		//if (leftShiftBtn > 0)
		//{
		//	std::cout << "GREATER";
		//}
		
		// WASD Key
		
		// w btn
		float _wKey = 0.0f;
		gInput.GetState(G_KEY_W, _wKey);

		// a btn
		float _aKey = 0.0f;
		gInput.GetState(G_KEY_A, _aKey);

		// s btn
		float _sKey = 0.0f;
		gInput.GetState(G_KEY_S, _sKey);

		// d btn
		float _dKey = 0.0f;
		gInput.GetState(G_KEY_D, _dKey);

		// rotation controls
		float mouseX = 0.0f;
		float mouseY = 0.0f;
		gInput.GetMousePosition(mouseX, mouseY);

		float changeMouseX = 0.0f;
		float changeMouseY = 0.0f;
		GW::GReturn result = gInput.GetMouseDelta(changeMouseX, changeMouseY);

		float changeX = _dKey - _aKey;
		float changeY = spaceBar - leftShiftBtn;
		float changeZ = _wKey - _sKey;

		// window height
		unsigned int height;
		win.GetHeight(height);

		// window width
		unsigned int width;
		win.GetWidth(width);

		// pitch 
		float pitch = (65.0f * changeMouseY / (float)height) + (3.14159f * deltaTime) * -1; // inverse controls mul by -1

		// aspect ratio
		float aspectRatio = 0.0f;
		d3d.GetAspectRatio(aspectRatio);

		// yaw
		float yaw = (65.0f * changeMouseX / (float)width) + (3.14159f * deltaTime); // no inverse controls


		GW::MATH::GVECTORF translationVec = { changeX * FPS, changeY * FPS, changeZ * FPS };

		GW::MATH::GMatrix::TranslateLocalF(_view, translationVec, _view);

		if (G_PASS(result) && result != GW::GReturn::REDUNDANT)
		{
			// local rotation on x
			GW::MATH::GMatrix::RotateXLocalF(_view, G_DEGREE_TO_RADIAN(pitch), _view);

			// store the position of the camera
			GW::MATH::GVECTORF pos = _view.row4;

			// global on y
			GW::MATH::GMatrix::RotateYGlobalF(_view, G_DEGREE_TO_RADIAN(yaw), _view);

			// restore the position to the camera
			_view.row4 = pos;
		}

		// update the camera's position in the _sceneData variable
		_sceneData.cameraPos = _view.row4;;

		GW::MATH::GMatrix::InverseF(_view, _sceneData.vMatrix);

	}

	void SelectLevel() {
		
		GW::SYSTEM::GLog _glog;

		gInput.GetState(G_KEY_NUMPAD_1, _NumPad1);
		gInput.GetState(G_KEY_NUMPAD_2, _NumPad2);
		_glog.Create("errorLog.txt");
		_glog.EnableConsoleLogging(true); // shows all loaded items
		
		// once you click the num_pad_1 give it 3 secs to clear everything and render new level
		if (_NumPad1 > 0.0f)
		{
			if (isLevelSwaped)
			{
				isLevelSwaped = !isLevelSwaped;
				level_obj.LoadLevel("../GameLevel.txt", "../Models", _glog.Relinquish());	
			}
			SceneBuffClear();

			IntializeGraphics();
		}
		
		// once you click the num_pad_2 give it 3 secs to clear everything and render new level
		if (_NumPad2 > 0.0f)
		{
			if (!isLevelSwaped)
			{
				isLevelSwaped = !isLevelSwaped;
				level_obj.LoadLevel("../GameLevel2.txt", "../Models2", _glog.Relinquish());
			}
			SceneBuffClear();

			IntializeGraphics();
		}


	}
};
