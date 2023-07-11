// This is a sample of how to load a level in a object oriented fashion.
// Feel free to use this code as a base and tweak it for your needs.

// This reads .h2b files which are optimized binary .obj+.mtl files
#include "h2bParser.h"

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// Pipeline/State Objects
struct PipelineHandles
{
	ID3D11DeviceContext* context;
	ID3D11RenderTargetView* targetView;
	ID3D11DepthStencilView* depthStencil;
};

// Uniform/ShaderVariable Buffer
struct SceneData
{
	GW::MATH::GVECTORF sunAmbient, cameraPos;
	GW::MATH::GVECTORF lightDirc, lightColor;// lighting info
	GW::MATH::GMATRIXF vMatrix, pMatrix; // viewing info
};

struct MeshData
{
	GW::MATH::GMATRIXF wMatrix;
	// connect to the h2b material
	H2B::ATTRIBUTES h2b_attrib;
};

class Model {
	// Name of the Model in the GameLevel (useful for debugging)
	std::string name;
	// Shader variables needed by this model. 
	// Loads and stores CPU model data from .h2b file
	H2B::Parser cpuModel; // reads the .h2b format

	GW::MATH::GMATRIXF world;// TODO: Add matrix/light/etc vars..
	
public:
	// TODO: API Rendering vars here (unique to this model)

	// Vertex Buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	
	// Vertex/Pixel Shaders
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;
	
	// Index Buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> microsoftIndexBuffer;
	
	// mesh data buffer
	Microsoft::WRL::ComPtr<ID3D11Buffer> meshDataBuffer;

	GW::MATH::GMATRIXF view;		  // view matrix
	GW::MATH::GMATRIXF projection;	  // projection matrix
	GW::MATH::GMATRIXF rotatedWorld;  // rotate world

	// structs 
	SceneData _sceneData;			  // struct accessors
	MeshData _meshData;				  // struct accessors

	inline void SetName(std::string modelName) {
		name = modelName;
	}
	inline void SetWorldMatrix(GW::MATH::GMATRIXF worldMatrix) {
		world = worldMatrix;
		_meshData.wMatrix = world;
	}
	bool LoadModelDataFromDisk(const char* h2bPath) {
		// if this succeeds "cpuModel" should now contain all the model's info
		return cpuModel.Parse(h2bPath);
	}
	bool UploadModelData2GPU(ID3D11Device* creator) /*specific API device for loading*/{
		// TODO: Use chosen API to upload this model's graphics data to GPU
		
		InitializePipeline(creator);

		CreateVertexBuffer(creator, cpuModel.vertices.data(), sizeof(H2B::VERTEX) * cpuModel.vertexCount);
		
		CreateIndexBuffer(creator, cpuModel.indices.data(), sizeof(unsigned int) * cpuModel.indexCount);

		CreateMeshBuffer(creator);

		return true; 

	}
	void InitializePipeline(ID3D11Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);

		CreateVertexInputLayout(creator, vsBlob);
	}
	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		HRESULT compilationResult =
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateVertexShader(vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreatePixelShader(psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;
	}
	// attributes
	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{
		// TODO: Part 1E 
		D3D11_INPUT_ELEMENT_DESC attributes[3];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		attributes[1].SemanticName = "LOCATION";
		attributes[1].SemanticIndex = 0;
		attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[1].InputSlot = 0;
		attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[1].InstanceDataStepRate = 0;

		attributes[2].SemanticName = "NORMAL";
		attributes[2].SemanticIndex = 0;
		attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[2].InputSlot = 0;
		attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[2].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}
	
	// mesh buffer
	void CreateMeshBuffer(ID3D11Device* creator) {

		D3D11_BUFFER_DESC bufferMesh = { 0 };
		bufferMesh.Usage = D3D11_USAGE_DEFAULT;
		bufferMesh.ByteWidth = sizeof(_meshData);
		bufferMesh.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferMesh.CPUAccessFlags = 0;
		bufferMesh.MiscFlags = 0;
		bufferMesh.StructureByteStride = 0;
		creator->CreateBuffer(&bufferMesh, nullptr, meshDataBuffer.GetAddressOf());

	}

	// index buffer
	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_BUFFER_DESC bufferIndex;
		bufferIndex.Usage = D3D11_USAGE_IMMUTABLE;
		bufferIndex.ByteWidth = sizeInBytes;
		bufferIndex.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferIndex.CPUAccessFlags = 0;
		bufferIndex.MiscFlags = 0;
		bufferIndex.StructureByteStride = 0;
		
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		
		creator->CreateBuffer(&bufferIndex, &bData, microsoftIndexBuffer.GetAddressOf());

		//CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
		//creator->CreateBuffer(&bDesc, &bData, microsoftIndexBuffer.GetAddressOf());
	}

	// vertex buffer
	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_BUFFER_DESC bufferVert;
		bufferVert.Usage = D3D11_USAGE_IMMUTABLE;
		bufferVert.ByteWidth = sizeInBytes;
		bufferVert.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferVert.CPUAccessFlags = 0;
		bufferVert.MiscFlags = 0;
		bufferVert.StructureByteStride = 0;
	
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		
		creator->CreateBuffer(&bufferVert, &bData, vertexBuffer.GetAddressOf());

		//CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		//creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	bool DrawModel(PipelineHandles curHandles) /*specific API command list or context*/{
		// TODO: Use chosen API to setup the pipeline for this model and draw it
		SetUpPipeline(curHandles);

		//D3D11_MAPPED_SUBRESOURCE meshMapping = { 0 }; 
		
		_meshData.wMatrix = world;
		for (int i = 0; i < cpuModel.meshCount; i++)	
		{
			//curHandles.context->Map(meshDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &meshMapping);
			//memcpy(meshMapping.pData, &_meshData, sizeof(_meshData));
			//curHandles.context->Unmap(meshDataBuffer.Get(), 0);

			_meshData.h2b_attrib = cpuModel.materials[cpuModel.meshes[i].materialIndex].attrib;

			// easier way to map and unmap information
			curHandles.context->UpdateSubresource(meshDataBuffer.Get(), 0, nullptr, &_meshData, 0, 0);
			
			curHandles.context->DrawIndexed(cpuModel.meshes[i].drawInfo.indexCount, cpuModel.meshes[i].drawInfo.indexOffset, 0);
		}	
		return true;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		SetVertexAndIndexBuffers(handles);
		SetShaders(handles);

		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		handles.context->VSSetConstantBuffers(1, 1, meshDataBuffer.GetAddressOf());
		handles.context->PSSetConstantBuffers(1, 1, meshDataBuffer.GetAddressOf());
	}

	void SetVertexAndIndexBuffers(PipelineHandles handles)
	{
		const UINT strides[] = { sizeof(H2B::VERTEX)};
		const UINT offsets[] = { 0 };
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
		handles.context->IASetIndexBuffer(microsoftIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	}


	//bool FreeResources(/*specific API device for unloading*/) {
	//	// TODO: Use chosen API to free all GPU resources used by this model

	//	return true;
	//}
	
	// Clears MESH/INDEX/VERTEX BUFFERS
	void Mesh_Vert_Index_BuffClear() { // thx mr. fernandez for the notice of release
		meshDataBuffer.ReleaseAndGetAddressOf();
		microsoftIndexBuffer.ReleaseAndGetAddressOf();
		vertexBuffer.ReleaseAndGetAddressOf();
	}
};


class Level_Objects {

	// store all our models
	std::list<Model> allObjectsInLevel;
	// TODO: This could be a good spot for any global data like cameras or lights

	GW::MATH::GVECTORF _lightDir;     // light direction vector
	GW::MATH::GVECTORF _lightColor;   // light color vector
	Model mod;
public:
	
	// Imports the default level txt format and creates a Model from each .h2b
	bool LoadLevel(	const char* gameLevelPath,
					const char* h2bFolderPath,
					GW::SYSTEM::GLog log) {
	
		if (allObjectsInLevel.size() > 0.0f)
		{
			mod.Mesh_Vert_Index_BuffClear();
		}
		
		// What this does:
		// Parse GameLevel.txt 
		// For each model found in the file...
			// Create a new Model class on the stack.
				// Read matrix transform and add to this model.
				// Load all CPU rendering data for this model from .h2b
			// Move the newly found Model to our list of total models for the level 

		log.LogCategorized("EVENT", "LOADING GAME LEVEL [OBJECT ORIENTED]");
		log.LogCategorized("MESSAGE", "Begin Reading Game Level Text File.");

		UnloadLevel();// clear previous level data if there is any
		GW::SYSTEM::GFile file;
		file.Create();
		if (-file.OpenTextRead(gameLevelPath)) {
			log.LogCategorized(
				"ERROR", (std::string("Game level not found: ") + gameLevelPath).c_str());
			return false;
		}
		char linebuffer[1024];
		while (+file.ReadLine(linebuffer, 1024, '\n'))
		{
			// having to have this is a bug, need to have Read/ReadLine return failure at EOF
			if (linebuffer[0] == '\0')
				break;
			if (std::strcmp(linebuffer, "MESH") == 0)
			{
				Model newModel;
				file.ReadLine(linebuffer, 1024, '\n');
				log.LogCategorized("INFO", (std::string("Model Detected: ") + linebuffer).c_str());
				// create the model file name from this (strip the .001)
				newModel.SetName(linebuffer);
				std::string modelFile = linebuffer;
				modelFile = modelFile.substr(0, modelFile.find_last_of("."));
				modelFile += ".h2b";

				// now read the transform data as we will need that regardless
				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(linebuffer, 1024, '\n');
					// read floats
					std::sscanf(linebuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				}
				std::string loc = "Location: X ";
				loc += std::to_string(transform.row4.x) + " Y " +
					std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);
				log.LogCategorized("INFO", loc.c_str());

				// Add new model to list of all Models
				log.LogCategorized("MESSAGE", "Begin Importing .H2B File Data.");
				modelFile = std::string(h2bFolderPath) + "/" + modelFile;
				newModel.SetWorldMatrix(transform);
				// If we find and load it add it to the level
				if (newModel.LoadModelDataFromDisk(modelFile.c_str())) {
					// add to our level objects, we use std::move since Model::cpuModel is not copy safe.
					allObjectsInLevel.push_back(std::move(newModel));
					log.LogCategorized("INFO", (std::string("H2B Imported: ") + modelFile).c_str());
				}
				else {
					// notify user that a model file is missing but continue loading
					log.LogCategorized("ERROR",
						(std::string("H2B Not Found: ") + modelFile).c_str());
					log.LogCategorized("WARNING", "Loading will continue but model(s) are missing.");
				}
				log.LogCategorized("MESSAGE", "Importing of .H2B File Data Complete.");
			}
		}
		log.LogCategorized("MESSAGE", "Game Level File Reading Complete.");
		// level loaded into CPU ram
		log.LogCategorized("EVENT", "GAME LEVEL WAS LOADED TO CPU [OBJECT ORIENTED]");
		return true;
	}
	// Upload the CPU level to GPU
	void UploadLevelToGPU(ID3D11Device* creator) /*pass handle to API device if needed*/{
		// iterate over each model and tell it to draw itself
		for (auto& e : allObjectsInLevel) {
			e.UploadModelData2GPU(creator);/*forward handle to API device if needed*/
		}
	}

	// Draws all objects in the level
	void RenderLevel(PipelineHandles _drawPipeLine) {
		// iterate over each model and tell it to draw itself
		for (auto &e : allObjectsInLevel) {
			e.DrawModel( _drawPipeLine);/*pass any needed global info.(ex:camera)*/
		}
	}
	// used to wipe CPU & GPU level data between levels
	bool UnloadLevel() {
		if (allObjectsInLevel.size() > 0)
		{
			allObjectsInLevel.clear();
			return true;
		}
		return false;
	}
	// *THIS APPROACH COMBINES DATA & LOGIC* 
	// *WITH THIS APPROACH THE CURRENT RENDERER SHOULD BE JUST AN API MANAGER CLASS*
	// *ALL ACTUAL GPU LOADING AND RENDERING SHOULD BE HANDLED BY THE MODEL CLASS* 
	// For example: anything that is not a global API object should be encapsulated.
};

