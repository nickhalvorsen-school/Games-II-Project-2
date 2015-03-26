#include "d3dApp.h"
#include "Line.h"
#include "Box.h"
#include "Quad.h"
#include "Object.h"
#include "Axis.h"
#include "Pyramid.h"
#include "RenderInfo.h"
#include "ComplexGeometry.h"
#include "Triangle.h"
#include "audio.h"
#include "Player.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <ctime>
#include <random>
#include <sstream>

class App : public D3DApp {
public:
	App(HINSTANCE hInstance);
	~App();

	void initApp();
	void onResize();
	void updateScene(float dt);
	void drawScene(); 

private:
	void buildFX();
	void buildVertexLayouts();

	Audio audio;

	//Geometry
	Line line;
	Box box;
	Box bouncerBox;
	Quad quad;
	Pyramid pyramid;
	Triangle triangle;

	//Models
	//ComplexGeometry wing;

	//Objects
	Axis axis;
	Player player;

	Object sideBouncerL;
	Object sideBouncerR;
	Object topBouncer;
	Object bottomBouncer;

	RenderInfo ri;
	ID3D10Effect* mFX;
	ID3D10InputLayout* mVertexLayout;

	float mTheta;
	float mPhi;
	float mx, my, mz;

	std::uniform_real_distribution<float> randomScaleDistribution;
	std::mt19937 generator;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
/*	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif
*/
	App theApp(hInstance);
	theApp.initApp();
	return theApp.run();
}

App::App(HINSTANCE hInstance):D3DApp(hInstance), mFX(0), mVertexLayout(0), randomScaleDistribution(0.25f, 2.25f) {
	ri.mTech = 0;
	ri.mfxWVPVar = 0;
	D3DXMatrixIdentity(&ri.mView);
	D3DXMatrixIdentity(&ri.mProj);

	mTheta = 0;
	mPhi = M_PI*0.40f;
	mx = 0;
	my = 0;
	mz = 0;
}

App::~App() {
	if( md3dDevice ) md3dDevice->ClearState();

	ReleaseCOM(mFX);
	ReleaseCOM(mVertexLayout);
}

void App::initApp() {
	D3DApp::initApp();
	buildFX();
	buildVertexLayouts();
	audio.initialize();

	//Geometry
	line.init(md3dDevice, WHITE);
	box.init(md3dDevice, WHITE);
	bouncerBox.init(md3dDevice, WHITE);
	quad.init(md3dDevice, WHITE);
	pyramid.init(md3dDevice, WHITE);
	triangle.init(md3dDevice, WHITE);

	//Complex Geometry
	//wing.init(&triangle);
	//wing.addChild(&triangle, Vector3(0, 1, 0), Vector3(0, 0, 0), Vector3(1, 1, 1));
	//wing.addChild(&quad, Vector3(0, 0, 0), Vector3(0, 0, ToRadian(90)), Vector3(1, 1, 1));
	//wing.addChild(&quad, Vector3(0, 0, 0), Vector3(ToRadian(-90), 0, 0), Vector3(1, 1, 1));
	//wing.addChild(&quad, Vector3(0, 0, 1), Vector3(ToRadian(-90), ToRadian(45), 0), Vector3(1.4142, 1, 1));

	//Objects
	axis.init(&line);
	player.init(&box, Vector3(0, 0, 0));
	player.setColor(0.5f, 0.9f, 0.4f, 1.0f);
	player.setRotation(Vector3(0, -90 * M_PI / 180, 0));
	player.setScale(Vector3(0.5, 0.5, 0.5));
	sideBouncerL.init(&bouncerBox, Vector3(-GAME_WIDTH, 0, 0));
	sideBouncerL.setScale(Vector3(1, 20, 1));
	sideBouncerL.setColor(.9f, .4f, .4f, 1.0f);
	sideBouncerR.init(&bouncerBox, Vector3(GAME_WIDTH, 0, 0));
	sideBouncerR.setScale(Vector3(1, 20, 1));
	sideBouncerR.setColor(.9f, .4f, .4f, 1.0f);
	topBouncer.init(&bouncerBox, Vector3(0, GAME_TOP, 0));
	topBouncer.setScale(Vector3(20, 1, 1));
	topBouncer.setColor(.9f, .4f, .4f, 1.0f);
	bottomBouncer.init(&bouncerBox, Vector3(0, GAME_BOTTOM, 0));
	bottomBouncer.setScale(Vector3(20, 1, 1));
	bottomBouncer.setColor(.9f, .4f, .4f, 1.0f);
	
}

void App::onResize() {
	D3DApp::onResize();
	float aspect = (float)mClientWidth/mClientHeight;
	D3DXMatrixPerspectiveFovLH(&ri.mProj, 0.25f*PI, aspect, 1.0f, 1000.0f);
}

void App::updateScene(float dt) {
	D3DApp::updateScene(dt);

	if (GetAsyncKeyState(VK_ESCAPE)) exit(0);

	if (GetAsyncKeyState('R')) player.resetPos();

	if (GetAsyncKeyState(VK_SPACE)) player.thrust(dt);

	if (GetAsyncKeyState(VK_UP)) player.thrustUp(dt);
	if (GetAsyncKeyState(VK_LEFT)) player.rotateLeft(dt);
	if (GetAsyncKeyState(VK_RIGHT)) player.rotateRight(dt);

	Vector3 oldPlayerPosition = player.getPosition();

	player.update(dt);
	sideBouncerL.update(dt);
	sideBouncerR.update(dt);
	topBouncer.update(dt);
	bottomBouncer.update(dt);

	/* collision with side walls */
	if (abs(player.getPosition().x) + player.getScale().x > GAME_WIDTH)
	{
		player.setVelocity(Vector3(player.getVelocity().x * -.8, player.getVelocity().y * .95, player.getVelocity().z));
		player.setPosition(oldPlayerPosition);
	}

	/* collision with bottom area */
	if (player.getPosition().y - player.getScale().y < GAME_BOTTOM)
	{
		player.setVelocity(Vector3(player.getVelocity().x, -player.getVelocity().y, player.getVelocity().z));

		if (player.getVelocity().y < 7 && player.getVelocity() > 0)
			player.setVelocity(Vector3(player.getVelocity().x, 7, player.getVelocity().z));

		player.setPosition(oldPlayerPosition);
	}

	/* collision with top area */
	if (player.getPosition().y + player.getScale().y > GAME_TOP)
	{
		player.setVelocity(Vector3(player.getVelocity().x, -player.getVelocity().y, player.getVelocity().z));
		player.setPosition(oldPlayerPosition);
	}

	/* don't let the player go too fast */
	if (abs(player.getVelocity().x) > VELOCITY_LIMIT)
		player.setVelocity(Vector3(player.getVelocity().x * .99, player.getVelocity().y, player.getVelocity().z));

	if (abs(player.getVelocity().y) > VELOCITY_LIMIT)
		player.setVelocity(Vector3(player.getVelocity().x, player.getVelocity().y * .99, player.getVelocity().z));

	if (abs(player.getVelocity().y) > VELOCITY_LIMIT)
		player.setVelocity(Vector3(player.getVelocity().x, player.getVelocity().y, player.getVelocity().z * .99));



	// Update angles based on input to orbit camera around box.
	if(GetAsyncKeyState('A') & 0x8000)	mTheta -= 2.0f*dt;
	if(GetAsyncKeyState('D') & 0x8000)	mTheta += 2.0f*dt;
	if(GetAsyncKeyState('W') & 0x8000)	mPhi -= 2.0f*dt;
	if(GetAsyncKeyState('S') & 0x8000)	mPhi += 2.0f*dt;

	// Restrict the angle mPhi.
	if( mPhi < 0.1f )	mPhi = 0.1f;
	if( mPhi > PI-0.1f)	mPhi = PI-0.1f;

	// Convert Spherical to Cartesian coordinates: mPhi measured from +y
	// and mTheta measured counterclockwise from -z.
	float d = 4;
	float x =  5.0f*sinf(mPhi)*sinf(mTheta) * d;
	float z = -5.0f*sinf(mPhi)*cosf(mTheta) * d;
	float y =  5.0f*cosf(mPhi) * d;

	// Build the view matrix.
	D3DXVECTOR3 pos(x, y, z);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&ri.mView, &pos, &target, &up);
}

void App::drawScene() {
	D3DApp::drawScene();
	mClearColor = D3DXCOLOR(107.0f / 255.0f, 123.0f / 255.0f, 164.0f / 255.0f, 1.0f);

	// Restore default states, input layout and primitive topology 
	// because mFont->DrawText changes them.  Note that we can 
	// restore the default states by passing null.
	md3dDevice->OMSetDepthStencilState(0, 0);
	float blendFactors[] = {0.0f, 0.0f, 0.0f, 0.0f};
	md3dDevice->OMSetBlendState(0, blendFactors, 0xffffffff);
    md3dDevice->IASetInputLayout(mVertexLayout);

	//Draw Axis
	axis.draw(&ri);

	//Draw Player
	player.draw(&ri);

	sideBouncerL.draw(&ri);
	sideBouncerR.draw(&ri);
	topBouncer.draw(&ri);
	bottomBouncer.draw(&ri);


	//Draw text to screen

	std::wostringstream outs;
	outs.precision(3);
	outs << "Controls:\n"
		<< "Turn: Left/Right\n"
		<< "Up: Up\n" 
		<< "Thrust: Space\n"
		<< "Reset: R\n";
	mFrameStats.clear();
	mFrameStats.append(outs.str());

	// We specify DT_NOCLIP, so we do not care about width/height of the rect.
	RECT R = {5, 5, 0, 0};
	mFont->DrawText(0, mFrameStats.c_str(), -1, &R, DT_NOCLIP, WHITE);
	mSwapChain->Present(0, 0);
}

void App::buildFX() {
	DWORD shaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
	#if defined( DEBUG ) || defined( _DEBUG )
		shaderFlags |= D3D10_SHADER_DEBUG;
		shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
	#endif
 
	ID3D10Blob* compilationErrors = 0;
	HRESULT hr = 0;
	hr = D3DX10CreateEffectFromFile(L"shader.fx", 0, 0, "fx_4_0", shaderFlags, 0, md3dDevice, 0, 0, &mFX, &compilationErrors, 0);
	if(FAILED(hr)) {
		if( compilationErrors ) {
			MessageBoxA(0, (char*)compilationErrors->GetBufferPointer(), 0, 0);
			ReleaseCOM(compilationErrors);
		}
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX10CreateEffectFromFile", true);
	} 

	ri.mTech = mFX->GetTechniqueByName("ColorTech");
	
	ri.mfxWVPVar = mFX->GetVariableByName("gWVP")->AsMatrix();
	ri.mfxColorVar = mFX->GetVariableByName("colorOverride")->AsVector();
}

void App::buildVertexLayouts()
{
	// Create the vertex input layout.
	D3D10_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
    D3D10_PASS_DESC PassDesc;
    ri.mTech->GetPassByIndex(0)->GetDesc(&PassDesc);
    HR(md3dDevice->CreateInputLayout(vertexDesc, 2, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &mVertexLayout));
}
 