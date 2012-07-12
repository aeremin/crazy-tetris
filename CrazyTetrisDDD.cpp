#include <cstdlib>
#include "d3dApp/d3dApp.h"
#include "Primitives.h"
#include "Light.h"
#include "Vertex.h"
#include "VisualEffects.h"
#include "Engine.h"
#include "Declarations.h"
#include "DirectXConstants.h"


bool keyPressed(KeyboardKey key)
{
  return (GetAsyncKeyState(key) & 0x8000) != 0;
}

class CrazyTetrisApp : public D3DApp
{
public:
	CrazyTetrisApp(HINSTANCE hInstance);
	~CrazyTetrisApp();

	void initApp();
	void onResize();
	void updateScene(float dt);
	void drawScene(); 
  float fieldToWorldX(float fieldX);
  float fieldToWorldY(float fieldY);

private:
	void buildFX();
	void buildVertexLayouts();
  void buildBuffers();
	void buildLights();
	void buildRasterizerStates();
  void buildBlendingStates();
  void buildTextures();

  void setConstantBuffers();
 
private:
	
	Box mBox;
  Wall mWall;
  CubeInstance* cubeInstances;
  Game mGame;

  //Vertex layouts
  ID3D10InputLayout* cubesVertexLayout;
  ID3D10InputLayout* texturedVertexLayout;
	
	//Effects and techniques
  ID3D10Effect* FX;
	ID3D10EffectTechnique* techCubes;
  ID3D10EffectTechnique* techSemicubes;
  ID3D10EffectTechnique* techTextured;
	
  //Shader variables
	ID3D10EffectMatrixVariable* fxVPVar;
	ID3D10EffectMatrixVariable* fxWorldVar;
	ID3D10EffectMatrixVariable* fxGlobalRotationVar;
  
  ID3D10EffectScalarVariable* fxTimeVar;
	ID3D10EffectScalarVariable* fxLightTypeVar;
  ID3D10EffectScalarVariable* fxWaveProgressVar;
  ID3D10EffectScalarVariable* fxSemicubesProgressVar;
  ID3D10EffectScalarVariable* fxCUBE_SCALE_INVERTED;

  ID3D10EffectVariable*       fxLightVar;
	ID3D10EffectVariable*       fxEyePosVar;
  ID3D10EffectVariable*       fxColorDiffuseVar;
  ID3D10EffectVariable*       fxColorSpecularVar;

  ID3D10EffectShaderResourceVariable* fxDiffuseMapVar;


	//Rasterizer state - clean up
  ID3D10RasterizerState* rsSolidCullFront;
  ID3D10RasterizerState* rsSolidCullBack;
  ID3D10RasterizerState* rsSolidCullNone;
  ID3D10RasterizerState* rsForSemicubes;

 
  //Textures and texture views
  //ID3D10Texture2D            texBackWall;
  ID3D10ShaderResourceView* texBackWallRV;
  
  ID3D10Buffer* cubeInstancesBuffer;
	
  ID3D10BlendState* transparentBS;

	D3DXMATRIX mWorld;
	D3DXMATRIX mView;
	D3DXMATRIX mProj;
	D3DXMATRIX mWVP;
	D3DXMATRIX mVP;
  D3DXMATRIX mGlobalRotation;

	Light Lights[MAX_LIGHTS];
	
	D3DXVECTOR3 eyePosW;
  	

  float currTime;
  float mPhi, mTheta;
   
  float WORLD_FIELD_WIDTH;
  float WORLD_FIELD_HEIGHT;
  float CUBE_SCALE;

};

CrazyTetrisApp::CrazyTetrisApp(HINSTANCE hInstance)
: D3DApp(hInstance), FX(0), techCubes(0), techSemicubes(0),
  fxVPVar(0), currTime(0.f), mPhi(PI / 2.f), mTheta(0.f), 
  WORLD_FIELD_HEIGHT(3.), WORLD_FIELD_WIDTH(4.),
  CUBE_SCALE(min (WORLD_FIELD_WIDTH / (float) FIELD_WIDTH, WORLD_FIELD_HEIGHT / (float) FIELD_HEIGHT))
  
{
  D3DXMatrixIdentity(&mWorld);
	D3DXMatrixIdentity(&mView);
	D3DXMatrixIdentity(&mProj);
	D3DXMatrixIdentity(&mWVP);
	D3DXMatrixIdentity(&mVP); 
  D3DXMatrixIdentity(&mGlobalRotation); 

}

CrazyTetrisApp::~CrazyTetrisApp()
{
	if( md3dDevice )
		md3dDevice->ClearState();

	ReleaseCOM(FX);
	ReleaseCOM(cubesVertexLayout);
  //ReleaseCOM(texturedVertexLayout);
  ReleaseCOM(rsSolidCullFront);
  ReleaseCOM(rsSolidCullBack);
  ReleaseCOM(rsSolidCullNone);
  ReleaseCOM(rsForSemicubes);
  ReleaseCOM(transparentBS);
}

void CrazyTetrisApp::initApp()
{
	D3DApp::initApp();
	
  mBox.init(md3dDevice, CUBE_SCALE / 2.0f + MATH_EPS, 0.5f, 5);
  mWall.init(md3dDevice, CUBE_SCALE * FIELD_WIDTH * FIELD_WIDTH / 6, CUBE_SCALE * FIELD_HEIGHT * FIELD_WIDTH / 6, 1,  1);
  
  buildFX();
	buildTextures();
  setConstantBuffers();

  buildVertexLayouts();
	buildLights();
	buildRasterizerStates();
  buildBlendingStates();
  buildBuffers();

  mGame.init();
  mGame.newMatch();
  mGame.newRound(0);
  mGame.saveSettings(); // for test purposes

}


void CrazyTetrisApp::updateScene(float dt)
{
	D3DApp::updateScene(dt);

  currTime = mTimer.getGameTime();
  mGame.onTimer(currTime);
  

  
	// Update angles based on input to orbit camera around box.
	if(GetAsyncKeyState('A') & 0x8000)	mTheta -= 2.0f*dt;
	if(GetAsyncKeyState('D') & 0x8000)	mTheta += 2.0f*dt;
	if(GetAsyncKeyState('W') & 0x8000)	mPhi -= 2.0f*dt;
	if(GetAsyncKeyState('S') & 0x8000)	mPhi += 2.0f*dt; 

	// Restrict the angle mPhi.
	if( mPhi < 0.1f )	mPhi = 0.1f;
	if( mPhi > PI-0.1f)	mPhi = PI-0.1f;
	
  if(GetAsyncKeyState('R') & 0x8000)
  {
    mPhi = PI / 2;
    mTheta = 0;
  }
  
  if(GetAsyncKeyState(VK_F1) & 0x8000)	mGame.player[0].visualEffects.flippedScreen.enable(1.0f);
  if(GetAsyncKeyState(VK_F2) & 0x8000)	mGame.player[0].visualEffects.flippedScreen.disable();

  if(GetAsyncKeyState(VK_F3) & 0x8000)	mGame.player[0].visualEffects.rotatingField.enable(10.0f);
  if(GetAsyncKeyState(VK_F4) & 0x8000)	mGame.player[0].visualEffects.rotatingField.disable();
	
  if(GetAsyncKeyState(VK_F5) & 0x8000)	mGame.player[0].visualEffects.wave.enable(2.0f);
  if(GetAsyncKeyState(VK_F6) & 0x8000)	mGame.player[0].visualEffects.wave.disable();

  if(GetAsyncKeyState(VK_F7) & 0x8000)	mGame.player[0].visualEffects.semicubes.enable(1.f);
  if(GetAsyncKeyState(VK_F8) & 0x8000)	mGame.player[0].visualEffects.semicubes.disable();

  if(GetAsyncKeyState('1') & 0x8000)	 mGame.player[0].visualEffects.lantern.enable(2.0f);
  if(GetAsyncKeyState('2')& 0x8000)	   mGame.player[0].visualEffects.lantern.disable();

  if(GetAsyncKeyState(VK_NUMPAD6) & 0x8000)	Lights[1].pos.x += 2.f * dt;
  if(GetAsyncKeyState(VK_NUMPAD4) & 0x8000) Lights[1].pos.x -= 2.f * dt;

  if(GetAsyncKeyState(VK_NUMPAD8) & 0x8000)	Lights[1].pos.y += 2.f * dt;
  if(GetAsyncKeyState(VK_NUMPAD2) & 0x8000) Lights[1].pos.y -= 2.f* dt;

  if(GetAsyncKeyState(VK_NUMPAD7) & 0x8000)	Lights[1].pos.z += dt;
  if(GetAsyncKeyState(VK_NUMPAD1) & 0x8000) Lights[1].pos.z -= dt;

  if(GetAsyncKeyState(VK_NUMPAD9) & 0x8000)	Lights[1].range += dt;
  if(GetAsyncKeyState(VK_NUMPAD3) & 0x8000) Lights[1].range -= dt;

  // Convert Spherical to Cartesian coordinates: mPhi measured from +y
	// and mTheta measured counterclockwise from -z.
	eyePosW.x =  5.0f*sinf(mPhi)*sinf(mTheta);
	eyePosW.z = -5.0f*sinf(mPhi)*cosf(mTheta);
	eyePosW.y =  5.0f*cosf(mPhi);

	// Build the view matrix.
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&mView, &eyePosW, &target, &up);

}

void CrazyTetrisApp::drawScene()
{
	
  D3DXCOLOR diffuseColors[] =
  {
    RED, RED, RED,  GREEN,
    BLUE, BLUE, RED,  GREEN,
    YELLOW, BLUE, BLUE,  GREEN,
    YELLOW,  YELLOW, YELLOW,  GREEN
  };
  D3DApp::drawScene();
	// Restore default states, input layout and primitive topology 
	// because mFont->DrawText changes them.  Note that we can 
	// restore the default states by passing null.
	md3dDevice->OMSetDepthStencilState(0, 0);
	float blendFactors[] = {0.0f, 0.0f, 0.0f, 0.0f};

  //md3dDevice->RSSetState(rsSolidCullNone);
	  
	//Getting current time once
  currTime = mTimer.getGameTime();
  
  //Set constants
	fxTimeVar->SetFloat(currTime);
  mVP = mView * mProj;
	fxVPVar->SetMatrix((float*)&mVP);

  D3DXMATRIX temp, tempX, tempY;
  D3DXMatrixMultiply(&mGlobalRotation, 
                     D3DXMatrixRotationY(&tempY, 2 * PI * mGame.player[0].visualEffects.rotatingField.progress(currTime)),
                     D3DXMatrixRotationX(&tempX,     PI * mGame.player[0].visualEffects.flippedScreen.progress(currTime)));
  
  
  //Calculating lights
  Lights[1].pos.x = fieldToWorldX(mGame.player[0].visualEffects.lantern.positionX(currTime));
  Lights[1].pos.y = fieldToWorldY(mGame.player[0].visualEffects.lantern.positionY(currTime));
  //Lights[1].pos.x += sin(TWO_PI * mGame.player[0].visualEffects.wave.progress(currTime)) * sin(2 *  Lights[1].pos.y)  * 0.33;
  Lights[1].pos.z = -1.0f;
  Lights[1].dir = D3DXVECTOR4(0.0f, 0.0f,  1.0f, 0.0f);
  
  Lights[2].pos.x = fieldToWorldX(mGame.player[0].visualEffects.lantern.positionX(currTime));
  Lights[2].pos.y = fieldToWorldY(mGame.player[0].visualEffects.lantern.positionY(currTime));
  //Lights[2].pos.x += sin(TWO_PI * mGame.player[0].visualEffects.wave.progress(currTime)) * sin(2 *  Lights[2].pos.y)  * 0.33;
  Lights[2].pos.z =  1.0f;
  Lights[2].dir = D3DXVECTOR4(0.0f, 0.0f,  -1.0f, 0.0f);

  Lights[0].brightness = 1 - mGame.player[0].visualEffects.lantern.progress(currTime);
  Lights[1].brightness =     mGame.player[0].visualEffects.lantern.progress(currTime);
  Lights[2].brightness =     mGame.player[0].visualEffects.lantern.progress(currTime);

  D3DXVec4Transform(&Lights[1].pos, &Lights[1].pos, &mGlobalRotation);
  D3DXVec4Transform(&Lights[1].dir, &Lights[1].dir, &mGlobalRotation);
  
  D3DXVec4Transform(&Lights[2].pos, &Lights[2].pos, &mGlobalRotation);
  D3DXVec4Transform(&Lights[2].dir, &Lights[2].dir, &mGlobalRotation);

  fxLightVar->SetRawValue(&Lights, 0, MAX_LIGHTS * sizeof(Light) );
	fxEyePosVar->SetRawValue(&eyePosW,0,sizeof(D3DXVECTOR3));
   
  float semicubesProgress = mGame.player[0].visualEffects.semicubes.progress(currTime);

  fxWaveProgressVar->SetFloat(mGame.player[0].visualEffects.wave.progress(currTime));
  fxSemicubesProgressVar->SetFloat(semicubesProgress);
  
  fxGlobalRotationVar->SetMatrix((float*) &mGlobalRotation);
  fxWorldVar->SetMatrix((float*) &mWorld);


  //������ ������������ �������  
  md3dDevice->OMSetBlendState(0, blendFactors, 0xffffffff);       

  //������ ������
  fxWorldVar->SetMatrix((float*) D3DXMatrixTranslation(&tempX, 0.0f, 0.0f, 0.55f * WORLD_FIELD_WIDTH));
  md3dDevice->IASetInputLayout(texturedVertexLayout);
  techTextured->GetPassByIndex( 0 )->Apply(0);
  mWall.draw();
  

  md3dDevice->IASetInputLayout(cubesVertexLayout);
  
  if (semicubesProgress > MATH_EPS) techSemicubes->GetPassByIndex( 0 )->Apply(0);
                               else techCubes    ->GetPassByIndex( 0 )->Apply(0); 
  
  mBox.setVB_AndIB_AsCurrent(md3dDevice, cubeInstancesBuffer);
  cubeInstancesBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0, (void** ) &cubeInstances); 
  
  
  //Real picture
  
  for (int i = 0; i < mGame.player[0].nBlockImages; ++i)
  {
    cubeInstances[i].offset.x = fieldToWorldX(mGame.player[0].blockImage[i].positionX(currTime));
    cubeInstances[i].offset.y = fieldToWorldY(mGame.player[0].blockImage[i].positionY(currTime));
    cubeInstances[i].offset.z = 0;

    cubeInstances[i].diffuseColor  = mGame.player[0].blockImage[i].color; 
    cubeInstances[i].specularColor = mGame.player[0].blockImage[i].color * 0.5f + WHITE * 0.5f;
    cubeInstances[i].specularColor.a = 128.f;

  }
  cubeInstancesBuffer->Unmap();
  
  mBox.draw(mGame.player[0].nBlockImages);
  
  
  //Test picture
  /*
  for (int i =  0; i < FIELD_WIDTH; ++i)
  {
    //int maxJ = FIELD_HEIGHT;//std::rand() % (FIELD_HEIGHT);
    for (int j = 0; j < FIELD_HEIGHT; ++j)
    {
      cubeInstances[i* FIELD_HEIGHT + j].offset.x = (i - (FIELD_WIDTH - 1.0) / 2.) * CUBE_SCALE;
      cubeInstances[i* FIELD_HEIGHT + j].offset.y = (j - (FIELD_HEIGHT - 1.0) / 2.) * CUBE_SCALE;
      cubeInstances[i* FIELD_HEIGHT + j].offset.z = 0;

      cubeInstances[i* FIELD_HEIGHT + j].diffuseColor  = diffuseColors[(4 * i + j) % 15];
      cubeInstances[i* FIELD_HEIGHT + j].specularColor = diffuseColors[(4 * i + j) % 15] * 0.5f + WHITE * 0.5f;
      cubeInstances[i* FIELD_HEIGHT + j].specularColor.a = 128.f;
    }
   }
 
  cubeInstancesBuffer->Unmap();
 
 
  mBox.draw(MAX_BLOCKS);
  */
  

    
  md3dDevice->OMSetBlendState(transparentBS, blendFactors, 0xffffffff);    
  //������������� �� ����������� � ������ ������������ �������
      
  
	md3dDevice->RSSetState(0);
	// We specify DT_NOCLIP, so we do not care about width/height of the rect.
	RECT R = {5, 5, 0, 0};
	mFont->DrawText(0, mFrameStats.c_str(), -1, &R, DT_NOCLIP, WHITE);

	mSwapChain->Present(0, 0);
}

void CrazyTetrisApp::buildFX()
{
	DWORD shaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
 
	ID3D10Blob* compilationErrors = 0;
	HRESULT hr = 0;
	hr = D3DX10CreateEffectFromFile(L"effects.fx", 0, 0, 
		"fx_4_0", shaderFlags, 0, md3dDevice, 0, 0, &FX, &compilationErrors, 0);
	if(FAILED(hr))
	{
		if( compilationErrors )
		{
			MessageBoxA(0, (char*)compilationErrors->GetBufferPointer(), 0, 0);
			ReleaseCOM(compilationErrors);
		}
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX10CreateEffectFromFile", true);
	} 

	techCubes     = FX->GetTechniqueByName("techCubes");
  techSemicubes = FX->GetTechniqueByName("techSemicubes");
	techTextured  = FX->GetTechniqueByName("techTextured");
  
  //Constants
  fxCUBE_SCALE_INVERTED = FX->GetVariableByName("CUBE_SCALE_INVERTED")->AsScalar();
  
  //Matrix variables
  fxWorldVar = FX->GetVariableByName("gWorld")->AsMatrix();
	fxVPVar = FX->GetVariableByName("gVP")->AsMatrix();
  fxGlobalRotationVar = FX->GetVariableByName("gGlobalRotation")->AsMatrix();
	
	//Lights and camera position
  fxLightVar = FX->GetVariableByName("gLight");
	fxEyePosVar = FX->GetVariableByName("gEyePosW");

  //Colors
  fxColorDiffuseVar  = FX->GetVariableByName("gColorDiffuse");
  fxColorSpecularVar = FX->GetVariableByName("gColorSpecular");

  //Current time
  fxTimeVar = FX->GetVariableByName("gTime")->AsScalar();

  //Textures
  fxDiffuseMapVar = FX->GetVariableByName("gDiffuseMap")->AsShaderResource();

  //Visual effect's progress
  fxWaveProgressVar      = FX->GetVariableByName("gWaveProgress")->AsScalar();
  fxSemicubesProgressVar = FX->GetVariableByName("gSemicubesProgress")->AsScalar();
}

void CrazyTetrisApp::setConstantBuffers()
{
  fxCUBE_SCALE_INVERTED->SetFloat(2.0f/CUBE_SCALE);

  fxDiffuseMapVar->SetResource(texBackWallRV);

}

void CrazyTetrisApp::buildVertexLayouts()
{
	int numElements;
  D3D10_PASS_DESC passDesc;
  
  // Create the vertex input layout for cubes (instanced!)
	D3D10_INPUT_ELEMENT_DESC cubesVertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D10_INPUT_PER_VERTEX_DATA,   0},
		{"NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D10_INPUT_PER_VERTEX_DATA,   0},
    {"offset",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,  D3D10_INPUT_PER_INSTANCE_DATA, 1},
    {"diffuse",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 12, D3D10_INPUT_PER_INSTANCE_DATA, 1},
    {"specular", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 28, D3D10_INPUT_PER_INSTANCE_DATA, 1}
	};
  numElements = sizeof(cubesVertexDesc) / sizeof(D3D10_INPUT_ELEMENT_DESC);
	techCubes->GetPassByIndex(0)->GetDesc(&passDesc);
  HR(md3dDevice->CreateInputLayout(cubesVertexDesc, numElements, passDesc.pIAInputSignature,
	passDesc.IAInputSignatureSize, &cubesVertexLayout));

  D3D10_INPUT_ELEMENT_DESC texturedVertexDesc[] =
  {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0},
  };
  numElements = sizeof(texturedVertexDesc) / sizeof(D3D10_INPUT_ELEMENT_DESC);
  techTextured->GetPassByIndex(0)->GetDesc(&passDesc);
  HR(md3dDevice->CreateInputLayout(texturedVertexDesc, numElements, passDesc.pIAInputSignature,
	passDesc.IAInputSignatureSize, &texturedVertexLayout));

}
void CrazyTetrisApp::buildRasterizerStates()
{
	D3D10_RASTERIZER_DESC rsDesc;

  ZeroMemory(&rsDesc, sizeof(D3D10_RASTERIZER_DESC));
	rsDesc.FillMode = D3D10_FILL_SOLID;
	rsDesc.CullMode = D3D10_CULL_FRONT;
  rsDesc.MultisampleEnable = true;
  rsDesc.AntialiasedLineEnable = true;
  HR(md3dDevice->CreateRasterizerState(&rsDesc, &rsSolidCullFront));

  ZeroMemory(&rsDesc, sizeof(D3D10_RASTERIZER_DESC));
	rsDesc.FillMode = D3D10_FILL_SOLID;
	rsDesc.CullMode = D3D10_CULL_BACK;
  rsDesc.MultisampleEnable = true;
  rsDesc.AntialiasedLineEnable = true;
  HR(md3dDevice->CreateRasterizerState(&rsDesc, &rsSolidCullBack));

  ZeroMemory(&rsDesc, sizeof(D3D10_RASTERIZER_DESC));
	rsDesc.FillMode = D3D10_FILL_SOLID;
	rsDesc.CullMode = D3D10_CULL_NONE;
  rsDesc.MultisampleEnable = true;
  rsDesc.AntialiasedLineEnable = true;
  HR(md3dDevice->CreateRasterizerState(&rsDesc, &rsSolidCullNone));

  ZeroMemory(&rsDesc, sizeof(D3D10_RASTERIZER_DESC));
	rsDesc.FillMode = D3D10_FILL_SOLID;
	rsDesc.CullMode = D3D10_CULL_NONE;
  rsDesc.MultisampleEnable = false;  //����� ����� ���������
  rsDesc.AntialiasedLineEnable = true;
  HR(md3dDevice->CreateRasterizerState(&rsDesc, &rsForSemicubes));

}

void CrazyTetrisApp::buildLights()
{
  int i;
  for (i = 0; i < 3; i++)
  {
	  Lights[i].ambient   = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	  Lights[i].diffuse   = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.5f);
	  Lights[i].specular  = D3DXCOLOR(0.6f, 0.6f, 0.6f, 1.0f);
  }
  
  //Setting parallel light
  Lights[0].dir       = D3DXVECTOR4(0.0f, 0.0f, 1.0f, 0.0f);
  Lights[0].lightType = 1;
  Lights[0].brightness = 1.0f;
  //Lights[0].active = 0;  

  //Spots for lantern effect
  Lights[1].dir = D3DXVECTOR4(0.0f, 0.0f,  1.0f, 0.0f);
  Lights[1].pos = D3DXVECTOR4(0.0f, 0.0f, -1.0f, 1.0f);
  Lights[1].att.y = 1;
  Lights[1].spotPow = 4.f;
  Lights[1].range = 10;
  Lights[1].lightType = 3;
  Lights[1].brightness = 0.0f;

  Lights[2].dir = D3DXVECTOR4(0.0f, 0.0f, -1.0f, 0.0f);
  Lights[2].pos = D3DXVECTOR4(0.0f, 0.0f,  1.0f, 1.0f);
  Lights[2].att.y = 1;
  Lights[2].spotPow = 4.f;
  Lights[2].range = 10;
  Lights[2].lightType = 3;
  Lights[2].brightness = 0.0f;


  
}

void CrazyTetrisApp::buildBlendingStates()
{
  D3D10_BLEND_DESC blendDesc = {0};
  blendDesc.AlphaToCoverageEnable = false;
  blendDesc.BlendEnable[0] = true;
  blendDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
  blendDesc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
  blendDesc.BlendOp = D3D10_BLEND_OP_ADD;
  blendDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
  blendDesc.DestBlendAlpha = D3D10_BLEND_ZERO;
  blendDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
  blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

  HR(md3dDevice->CreateBlendState(&blendDesc, &transparentBS));
}


void CrazyTetrisApp::onResize()
{
	D3DApp::onResize();

	float aspect = (float)mClientWidth/mClientHeight;
	D3DXMatrixPerspectiveFovLH(&mProj, 0.25f*PI, aspect, 1.0f, 1000.0f);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif


	CrazyTetrisApp theApp(hInstance);
	
	theApp.initApp();

	return theApp.run();
}


void CrazyTetrisApp::buildBuffers()
{
  D3D10_BUFFER_DESC bufferDesc =
  {
    MAX_BLOCKS * sizeof(CubeInstance),
    D3D10_USAGE_DYNAMIC,
    D3D10_BIND_VERTEX_BUFFER,
    D3D10_CPU_ACCESS_WRITE,
    0
  };

  HR( md3dDevice->CreateBuffer(&bufferDesc, NULL, &cubeInstancesBuffer) )
 
}

void CrazyTetrisApp::buildTextures()
{
  D3DX10CreateShaderResourceViewFromFile(md3dDevice, L"Resources/Textures/Wall.dds", 0, 0, &texBackWallRV, 0);
}
float CrazyTetrisApp::fieldToWorldX(float fieldX)
{
  return (fieldX - (FIELD_WIDTH - 1.0f) / 2.0f) * CUBE_SCALE;
}

float CrazyTetrisApp::fieldToWorldY(float fieldY)
{
  return (fieldY - (FIELD_HEIGHT - 1.0f) / 2.0f) * CUBE_SCALE;
}

