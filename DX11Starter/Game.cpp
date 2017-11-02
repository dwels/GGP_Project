#include "Game.h"

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1000,			   // Width of the window's client area
		1000,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	// Initialize fields
	vertexBuffer = 0;
	indexBuffer = 0;
	vertexShader = 0;
	pixelShader = 0;

	material = 0;

	Cam = new Camera();

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
	
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Release any (and all!) DirectX objects
	// we've made in the Game class
	if (vertexBuffer) { vertexBuffer->Release(); }
	if (indexBuffer) { indexBuffer->Release(); }

	// Delete our simple shader objects, which
	// will clean up their own internal DirectX stuff
	delete vertexShader;
	delete pixelShader;
	delete Cam;
	delete material;
	delete material2;

	//Level1->~Level();
	delete level;
	delete player;

	checkerSRV->Release();
	rainbowSRV->Release();
	sampleState->Release();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateMatrices();
	//CreateBasicGeometry(); //deprecated

	light.AmbientColor = XMFLOAT4(0.1, 0.1, 0.1, 1.0);
	light.DiffuseColor = XMFLOAT4(1, 1, 1, 1);
	light.Direction = XMFLOAT3(1, -1, 0);

	light2.AmbientColor = XMFLOAT4(0.1, 0.1, 0.1, 1.0);
	light2.DiffuseColor = XMFLOAT4(1, 0, 0, 1);
	light2.Direction = XMFLOAT3(0, 0, 1);

	//create the level
	level = new Level(material);

	//set the level variables
	const int LANES = 10;
	Vertex verts[LANES * 2] = { }; //vertex array null w/ length = lanecount*2
	int inds[LANES * 6] = {}; // ind array w/ length = lanecount*6

	//generate the level
	level->genLevel(device, inds, verts, LANES, 8.0, 8, 75.0, material, material2);

	// Create Player
	this->player = new Player(level, material2, device);

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files using
// my SimpleShader wrapper for DirectX shader manipulation.
// - SimpleShader provides helpful methods for sending
//   data to individual variables on the GPU
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device, context);
	vertexShader->LoadShaderFile(L"VertexShader.cso");

	pixelShader = new SimplePixelShader(device, context);
	pixelShader->LoadShaderFile(L"PixelShader.cso");
	
	
	CreateWICTextureFromFile(device, context, L"Assets/Textures/checker.jpg", 0, &checkerSRV);
	CreateWICTextureFromFile(device, context, L"Assets/Textures/rainbow.png", 0, &rainbowSRV);

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, &sampleState);

	material = new Materials(pixelShader, vertexShader, checkerSRV, sampleState);
	material2 = new Materials(pixelShader, vertexShader, rainbowSRV, sampleState);
}



// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	Cam->ProjectMat(width,height);
}


// --------------------------------------------------------
// Creates the geometry we're going to draw - deprecated
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	Cam->ProjectMat(width, height);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();

	//timer for some of the transformations
	float sinTime = (sin(totalTime * 10.0f) + 2.0f) / 10.0f;

	//OnMouseDown(); //how do I do mouse controls?
	
	Cam->Update(deltaTime);

	//update game objects
	level->Update(deltaTime, totalTime);
	player->Update();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV, color);
	context->ClearDepthStencilView(
		depthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	pixelShader->SetData(
		"light",	//Name of the (eventual) variable in the shader 
		&light,		//address of the data to copy
		sizeof(DirectionalLight));	//size of data to copy

	pixelShader->SetData(
		"light2",	//Name of the (eventual) variable in the shader 
		&light2,		//address of the data to copy
		sizeof(DirectionalLight));	//size of data to copy

	pixelShader->CopyAllBufferData();

	//get vector of lanes
	std::vector<Lane*>* lanes = level->getLanes();

	//loop through lanes
	for each(Lane* lane in *lanes)
	{
		//draw each enemy
		for each(Enemy* enemy in *lane->getEnemies())
		{
			Entity* enemyEntity = enemy->getEntity();
			enemyEntity->Draw(context, Cam->GetViewMat(), Cam->GetProjectionMatrix(), enemyEntity->mesh, sampleState);
		}
		//draw each projectile
		for each(Projectile* proj in *lane->getProjectiles())
		{
			Entity* projEntity = proj->getEntity();
			projEntity->Draw(context, Cam->GetViewMat(), Cam->GetProjectionMatrix(), projEntity->mesh, sampleState);
		}
	}
	
	//draw the level
	Entity* levelEntity = level->getEntity();
	levelEntity->Draw(context, Cam->GetViewMat(), Cam->GetProjectionMatrix(), levelEntity->mesh, sampleState);

	//draw the player
	Entity* playerEntity = player->getEntity();
	playerEntity->Draw(context, Cam->GetViewMat(), Cam->GetProjectionMatrix(), playerEntity->mesh, sampleState);
	
	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);
}


#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...
	Cam->SetRot(0,0); //stop mouse movement when the button is released


	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	if (buttonState & 0x0001) {
		Cam->SetRot(y - prevMousePos.y, x - prevMousePos.x); //only moves the camera while the mouse button is pressed
	}

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
}
#pragma endregion