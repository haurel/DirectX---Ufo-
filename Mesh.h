#pragma once

class CXMesh {
private:
protected:
	LPDIRECT3DDEVICE9 m_pDevice;

	LPD3DXMESH              g_pMesh = NULL;
	D3DMATERIAL9*           g_pMeshMaterials = NULL; 
	LPDIRECT3DTEXTURE9*     g_pMeshTextures = NULL; 
	DWORD                   g_dwNumMaterials = 0L;  
	D3DXMATRIX				worldMatrix;

	D3DXVECTOR3				m_Position;
	D3DXVECTOR3				m_LookAt;
	D3DXVECTOR3				m_Right;
	D3DXVECTOR3				m_Up;
	D3DXVECTOR3				m_fScale;
	D3DXVECTOR3				m_fTranslate;
	D3DXVECTOR3				m_RotAxis;

	float					m_fAngle;
	float					m_fRotAboutUp;
	float					m_fRotAboutRight;
	float					m_fRotAboutFacing;
	bool					m_UpdateRequired;

	HRESULT UpdateMeshMatrices();
public:
	LPD3DXMESH GetMesh();
	D3DXVECTOR3* GetPosition() { return &m_Position; }
	D3DXMATRIX* GetWorldMatrix() { return &worldMatrix; }
	HRESULT Update();
	CXMesh(LPDIRECT3DDEVICE9 pDevice, LPCSTR filename);
	
	void DrawMesh();
	void SetPosition(FLOAT X, FLOAT Y, FLOAT Z);
	void MoveForward(float fDistance);
	void MoveInDirection(float fDist, D3DXVECTOR3* Dir);
	void RotateDown(float fAngle);
	void RotateRight(float fAngle);
	void MoveRight(float fDist);
	void MoveUp(float fDist);
	void ScaleMesh(D3DXVECTOR3 value);
	void TranslateMesh(D3DXVECTOR3 valueTranslate);
	void RotationAxis(D3DXVECTOR3 rotAxis, float fAngle);
};

CXMesh::CXMesh(LPDIRECT3DDEVICE9 pDevice, LPCSTR filename)
{
	LPD3DXBUFFER pD3DXMtrlBuffer;

	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM,
								pDevice, NULL,
								&pD3DXMtrlBuffer, NULL, &g_dwNumMaterials,
								&g_pMesh)))
	{
		MessageBox(NULL, "Could not find ufo.x", "Meshes.exe", MB_OK);

	}

	D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
	g_pMeshMaterials = new D3DMATERIAL9[g_dwNumMaterials];
	g_pMeshTextures = new LPDIRECT3DTEXTURE9[g_dwNumMaterials];

	for (DWORD i = 0; i < g_dwNumMaterials; i++)
	{
		// Copy the material
		g_pMeshMaterials[i] = d3dxMaterials[i].MatD3D;

		// Set the ambient color for the material (D3DX does not do this)
		g_pMeshMaterials[i].Ambient = g_pMeshMaterials[i].Diffuse;

		g_pMeshTextures[i] = NULL;
		if (d3dxMaterials[i].pTextureFilename != NULL &&
			lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
		{
			// Create the texture
			if (FAILED(D3DXCreateTextureFromFile(pDevice,
				d3dxMaterials[i].pTextureFilename,
				&g_pMeshTextures[i])))
			{
				// If texture is not in current folder, try parent folder
				const TCHAR* strPrefix = TEXT("..\\");
				const int lenPrefix = lstrlen(strPrefix);
				TCHAR strTexture[MAX_PATH];
				lstrcpyn(strTexture, strPrefix, MAX_PATH);
				lstrcpyn(strTexture + lenPrefix, d3dxMaterials[i].pTextureFilename, MAX_PATH - lenPrefix);
				// If texture is not in current folder, try parent folder
				if (FAILED(D3DXCreateTextureFromFile(pDevice,
					strTexture,
					&g_pMeshTextures[i])))
				{
					MessageBox(NULL, "Could not find texture map", "Meshes.exe", MB_OK);
				}
			}
		}
	}

	pD3DXMtrlBuffer->Release();

	LPDIRECT3DVERTEXBUFFER9 VertexBuffer = NULL;
	D3DXVECTOR3* Vertices = NULL;
	D3DXVECTOR3 Center;
	FLOAT Radius;
	DWORD getFVFMesh = g_pMesh->GetFVF();
	DWORD FVFVertexSize = D3DXGetFVFVertexSize(g_pMesh->GetFVF());
	g_pMesh->GetVertexBuffer(&VertexBuffer);
	VertexBuffer->Lock(10, 10, (VOID**)&Vertices, D3DLOCK_DISCARD);
	D3DXComputeBoundingSphere(Vertices, g_pMesh->GetNumVertices(), FVFVertexSize, &Center, &Radius);

	VertexBuffer->Unlock();
	VertexBuffer->Release();


	m_Position = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_fScale = D3DXVECTOR3(0.0, 0.0, 0.0);
	m_LookAt = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	m_Right = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
	m_Up = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	m_RotAxis = D3DXVECTOR3(0.0, 0.0, 0.0);
	m_fAngle = 0;
	m_UpdateRequired = false;

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
	D3DXMatrixIdentity(&worldMatrix);

	m_pDevice = pDevice;
}

LPD3DXMESH CXMesh::GetMesh() {
	return g_pMesh;
}

void CXMesh::DrawMesh() {
	for (DWORD i = 0; i < g_dwNumMaterials; i++)
	{
		// Set the material and texture for this subset
		m_pDevice->SetMaterial(&g_pMeshMaterials[i]);
		m_pDevice->SetTexture(0, g_pMeshTextures[i]);
		// Draw the mesh subset
		g_pMesh->DrawSubset(i);

	}
}

HRESULT CXMesh::UpdateMeshMatrices() {
	D3DXMATRIX matTotal;
	D3DXMATRIX matRotAboutUp, matRotAboutRight, matRotAboutFacing;
	D3DXMATRIX matScale, matTranslation, matRotation;

	D3DXMatrixScaling(&matScale, m_fScale.x, m_fScale.y, m_fScale.z);
	D3DXMatrixTranslation(&matTranslation, m_fTranslate.x, m_fTranslate.y, m_fTranslate.z);
	D3DXMatrixRotationX(&matRotation, m_fAngle);
	D3DXMatrixRotationAxis(&matRotation, &m_RotAxis, m_fAngle);

	D3DXMatrixRotationAxis(&matRotAboutRight, &m_Right, m_fRotAboutRight);
	D3DXMatrixRotationAxis(&matRotAboutUp, &m_Up, m_fRotAboutUp);
	D3DXMatrixRotationAxis(&matRotAboutFacing, &m_LookAt, m_fRotAboutFacing);
	

	D3DXMatrixMultiply(&matTotal, &matRotAboutUp, &matRotAboutRight);
	D3DXMatrixMultiply(&matTotal, &matRotAboutFacing, &matTotal);
	

	D3DXVec3TransformCoord(&m_Right, &m_Right, &matTotal);
	D3DXVec3TransformCoord(&m_Up, &m_Up, &matTotal);
	D3DXVec3Cross(&m_LookAt, &m_Right, &m_Up);

	if (fabs(D3DXVec3Dot(&m_Up, &m_Right)) > 0.01)
	{
		D3DXVec3Cross(&m_Up, &m_LookAt, &m_Right);
	}

	D3DXVec3Normalize(&m_Right, &m_Right);
	D3DXVec3Normalize(&m_Up, &m_Up);
	D3DXVec3Normalize(&m_LookAt, &m_LookAt);

	float fView41, fView42, fView43;
	fView41 = -D3DXVec3Dot(&m_Right, &m_Position);
	fView42 = -D3DXVec3Dot(&m_Up, &m_Position);
	fView43 = -D3DXVec3Dot(&m_LookAt, &m_Position);

	worldMatrix = D3DXMATRIX(m_Right.x, m_Up.x, m_LookAt.x, 0.0f,
		m_Right.y, m_Up.y, m_LookAt.y, 0.0f,
		m_Right.z, m_Up.z, m_LookAt.z, 0.0f,
		fView41, fView42, fView43, 1.0f);

	m_fRotAboutUp = m_fRotAboutRight = m_fRotAboutFacing = 0.0f;
	m_UpdateRequired = false;

	D3DXMatrixMultiply(&matTotal, &matScale, &matTotal);
	D3DXMatrixMultiply(&matTotal, &matRotation, &matTotal);
	D3DXMatrixMultiply(&worldMatrix, &matTranslation, &matTotal);
	return m_pDevice->SetTransform(D3DTS_WORLD, &worldMatrix);
}

HRESULT CXMesh::Update()
{
	if (m_pDevice)
	{
		if (m_UpdateRequired)
			return UpdateMeshMatrices();

		return m_pDevice->SetTransform(D3DTS_WORLD, &worldMatrix);
	}

	return E_FAIL;
}


void CXMesh::SetPosition(FLOAT X, FLOAT Y, FLOAT Z)
{
	m_Position = D3DXVECTOR3(X, Y, Z);
	m_UpdateRequired = true;
}

void CXMesh::MoveForward(float fDistance) {
	m_Position += fDistance * m_LookAt;
	m_UpdateRequired = true;
}

void CXMesh::MoveInDirection(float fDist, D3DXVECTOR3* Dir)
{
	D3DXVECTOR3 DirToMove(0, 0, 0);
	D3DXVec3Normalize(&DirToMove, Dir);
	m_Position += fDist * DirToMove;
	m_UpdateRequired = true;
}

void CXMesh::RotateDown(float fAngle) {
	m_fRotAboutRight += fAngle;
	m_UpdateRequired = true;
}
void CXMesh::RotateRight(float fAngle) {
	m_fRotAboutUp += fAngle;
	m_UpdateRequired = true;
}
void CXMesh::MoveRight(float fDist)
{
	m_Position += fDist * m_Right;
	m_UpdateRequired = true;
}

void CXMesh::MoveUp(float fDist)
{
	m_Position += fDist * m_Up;
	m_UpdateRequired = true;
}

void CXMesh::ScaleMesh(D3DXVECTOR3 valueScale) {
	m_fScale = valueScale;
	m_UpdateRequired = true;
}

void CXMesh::TranslateMesh(D3DXVECTOR3 valueTranslate) {
	m_fTranslate = valueTranslate;
	m_UpdateRequired = true;
}

void CXMesh::RotationAxis(D3DXVECTOR3 rotAxis,float fAngle) {
	m_RotAxis = rotAxis;
	m_fAngle = fAngle;
	m_UpdateRequired = true;
}