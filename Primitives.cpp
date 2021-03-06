//Geometrical primitives

#include <math.h>
#include "Primitives.h"
#include "Vertex.h"
#include "DirectXConstants.h"



float sgn(float t)
{
  if (t < -MATH_EPS) return -1.0f;
  if (t >  MATH_EPS) return  1.0f;
  return 0.0f;
}

Primitive::Primitive() : nVertices(0), nFaces(0), mVB(0), md3dDevice(0) {}

Primitive::~Primitive()
{
  if (mVB) ReleaseCOM(mVB);
}



Box::Box() : mIB(0) {}

Box::~Box()
{
  if (mIB) ReleaseCOM(mIB);
}


// Procedure generation of smoothed cube

void Box::init(ID3D10Device* device, float scale, float smoothnessRadius, int angleSteps)
{


  int i, j;
  float alpha, beta;
  float x, y, z;

  md3dDevice = device;

  nVertices = 8 * angleSteps * angleSteps;
  nFaces    = (2 * angleSteps - 1) * (8 * angleSteps + 4) ;

  UncoloredVertex* vertices = new UncoloredVertex[nVertices];

  for (i = 0; i < 2 * angleSteps; ++i)
    for (j = 0; j < 4 * angleSteps; ++j)
    {
      alpha = ((i - angleSteps + 0.5f) * (PI / 2.0f)) / angleSteps;
      beta  = ((j + 0.5f) * (PI / 2.0f)) / angleSteps;

      x = cosf(alpha) * cosf(beta);
      z = cosf(alpha) * sinf(beta);
      y = sinf(alpha);

      vertices[i * 4 * angleSteps + j].pos.x = scale * (smoothnessRadius * x + (1 - smoothnessRadius) * sgn(x));
      vertices[i * 4 * angleSteps + j].pos.y = scale * (smoothnessRadius * y + (1 - smoothnessRadius) * sgn(y));
      vertices[i * 4 * angleSteps + j].pos.z = scale * (smoothnessRadius * z + (1 - smoothnessRadius) * sgn(z));

      vertices[i * 4 * angleSteps + j].normal.x = x;
      vertices[i * 4 * angleSteps + j].normal.y = y;
      vertices[i * 4 * angleSteps + j].normal.z = z;
    }

  D3D10_BUFFER_DESC vbd;
  vbd.Usage = D3D10_USAGE_IMMUTABLE;
  vbd.ByteWidth = sizeof(UncoloredVertex) * nVertices;
  vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA vinitData;
  vinitData.pSysMem = vertices;
  HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));
  delete[] vertices;

  // Create the index buffer

  DWORD* indices = new DWORD[3 * nFaces];

  for (i = 0; i < 2 * angleSteps - 1; ++i)
    for (j = 0; j < 4 * angleSteps; ++j)
    {
      indices[6 * (i * 4 * angleSteps + j) + 0] = i * 4 * angleSteps + j;
      indices[6 * (i * 4 * angleSteps + j) + 1] = (i + 1) * 4 * angleSteps + j;
      indices[6 * (i * 4 * angleSteps + j) + 2] = (i + 1) * 4 * angleSteps + ((j + 1) % (4 * angleSteps));

      indices[6 * (i * 4 * angleSteps + j) + 3] = i * 4 * angleSteps + j;
      indices[6 * (i * 4 * angleSteps + j) + 4] = (i + 1) * 4 * angleSteps + ((j + 1) % (4 * angleSteps));
      indices[6 * (i * 4 * angleSteps + j) + 5] = i * 4 * angleSteps + ((j + 1) % (4 * angleSteps));

      int a = j + 1;
    }

  for (i = 0; i < 4 * angleSteps - 2; ++i)
  {
    indices[(2 * angleSteps - 1) * 24 * angleSteps + 3 * i + 0] = 0;
    indices[(2 * angleSteps - 1) * 24 * angleSteps + 3 * i + 1] = i + 1;
    indices[(2 * angleSteps - 1) * 24 * angleSteps + 3 * i + 2] = i + 2;
  }

  for (i = 0; i < 4 * angleSteps - 2; ++i)
  {
    indices[nFaces * 3 - 3 * i - 1] = nVertices - 1;
    indices[nFaces * 3 - 3 * i - 3] = nVertices - 2 - i;
    indices[nFaces * 3 - 3 * i - 2] = nVertices - 3 - i;
  }

  D3D10_BUFFER_DESC ibd;
  ibd.Usage = D3D10_USAGE_IMMUTABLE;
  ibd.ByteWidth = sizeof(DWORD) * nFaces * 3;
  ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
  ibd.CPUAccessFlags = 0;
  ibd.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA iinitData;
  iinitData.pSysMem = indices;
  HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));


  delete[] indices;
}

void Box::draw(int nInstances, int instancesOffset)
{
  md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  md3dDevice->DrawIndexedInstanced(3 * nFaces, nInstances, 0, 0, instancesOffset);
}
void  Box::setVB_AndIB_AsCurrent(ID3D10Device* device, ID3D10Buffer* cubeInstancesBuffer)
{
  UINT stride[2] =
  {
    sizeof(UncoloredVertex),
    sizeof(CubeInstance)
  };

  UINT offset[2] = {0, 0};

  ID3D10Buffer* curVB[2];
  curVB[0] = mVB;
  curVB[1] = cubeInstancesBuffer;

  device->IASetVertexBuffers(0, 2, curVB, stride, offset);
  device->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
}


TexturedQuad::TexturedQuad() {}

TexturedQuad::~TexturedQuad() {}

void TexturedQuad::init(ID3D10Device* device, float worldWidth, float worldHeight, float texWidth, float texHeight)
{
  nVertices = 4;
  nFaces = 2;
  md3dDevice = device;
  TexturedVertex vertices[] =
  {

    {D3DXVECTOR3( worldWidth / 2.0f, -worldHeight / 2.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2(texWidth, texHeight)},
    {D3DXVECTOR3(-worldWidth / 2.0f, -worldHeight / 2.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2(0.0f, texHeight)},
    {D3DXVECTOR3( worldWidth / 2.0f,  worldHeight / 2.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2(texWidth, 0.0f)},
    {D3DXVECTOR3(-worldWidth / 2.0f,  worldHeight / 2.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), D3DXVECTOR2(0.0f, 0.0f)}
  };

  D3D10_BUFFER_DESC vbd;
  vbd.Usage = D3D10_USAGE_IMMUTABLE;
  vbd.ByteWidth = sizeof(TexturedVertex) * nVertices;
  vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA vinitData;
  vinitData.pSysMem = vertices;
  HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));
}

void TexturedQuad::draw()
{
  md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  UINT stride = sizeof(TexturedVertex);
  UINT offset = 0;

  md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
  md3dDevice->Draw(nFaces + 2, 0);
}

Glass::Glass() {}

Glass::~Glass()
{
  if (mIB) ReleaseCOM(mIB);
}

void Glass::init(ID3D10Device* device, float width, float height, float depth)
{
  nVertices = 12;
  nFaces = 6;
  md3dDevice = device;

  UncoloredVertex vertices[] =
  {
    //Left face
    {D3DXVECTOR3(-1.0f, +1.0f, -1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},
    {D3DXVECTOR3(-1.0f, +1.0f, +1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},
    {D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},
    {D3DXVECTOR3(-1.0f, -1.0f, +1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f)},
    //Bottom face
    {D3DXVECTOR3(-1.0f, -1.0f, -1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},
    {D3DXVECTOR3(-1.0f, -1.0f, +1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},
    {D3DXVECTOR3(+1.0f, -1.0f, -1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},
    {D3DXVECTOR3(+1.0f, -1.0f, +1.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f)},
    //Right face
    {D3DXVECTOR3(+1.0f, -1.0f, -1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},
    {D3DXVECTOR3(+1.0f, -1.0f, +1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},
    {D3DXVECTOR3(+1.0f, +1.0f, +1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)},
    {D3DXVECTOR3(+1.0f, +1.0f, -1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f)}
  };

  for (int i = 0; i < nVertices; ++i)
  {
    vertices[i].pos.x *= width  / 2.0f;
    vertices[i].pos.y *= height / 2.0f;
    vertices[i].pos.z *= depth  / 2.0f;
  }

  D3D10_BUFFER_DESC vbd;
  vbd.Usage = D3D10_USAGE_IMMUTABLE;
  vbd.ByteWidth = sizeof(UncoloredVertex) * nVertices;
  vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  vbd.CPUAccessFlags = 0;
  vbd.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA vinitData;
  vinitData.pSysMem = vertices;
  HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mVB));


  DWORD indices[] =
  {
    0, 1, 2,
    1, 3, 2,
    4, 5, 6,
    5, 7, 6,
    8, 9, 10,
    9, 11, 10
  };

  D3D10_BUFFER_DESC ibd;
  ibd.Usage = D3D10_USAGE_IMMUTABLE;
  ibd.ByteWidth = sizeof(DWORD) * nFaces * 3;
  ibd.BindFlags = D3D10_BIND_INDEX_BUFFER;
  ibd.CPUAccessFlags = 0;
  ibd.MiscFlags = 0;
  D3D10_SUBRESOURCE_DATA iinitData;
  iinitData.pSysMem = indices;
  HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mIB));
}


void Glass::draw()
{
  md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  UINT stride = sizeof(UncoloredVertex);
  UINT offset = 0;
  md3dDevice->IASetVertexBuffers(0, 1, &mVB, &stride, &offset);
  md3dDevice->IASetIndexBuffer(mIB, DXGI_FORMAT_R32_UINT, 0);
  md3dDevice->DrawIndexed(3 * nFaces, 0, 0);
}
