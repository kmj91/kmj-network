#include "stdafx.h"
#include <windows.h>
#include "CSpriteDib.h"
#include "stdio.h"

////////////////////////////////////////////////////////////////////
// 생성자, 파괴자.
//
// Parameters: (int)최대 스프라이트 개수. (DWORD)투명칼라.
////////////////////////////////////////////////////////////////////
CSpriteDib::CSpriteDib(int iMaxSprite, DWORD dwColorKey)
{
	//-----------------------------------------------------------------
	// 최대 읽어올 개수 만큼 미리 할당 받는다.
	//-----------------------------------------------------------------
	m_iMaxSprite = iMaxSprite;
	m_dwColorKey = dwColorKey;
	m_stpSprite = new st_SPRITE[iMaxSprite];

	//기본 카메라 위치
	m_iCameraPosX = 0;
	m_iCameraPosY = 0;
}

CSpriteDib::~CSpriteDib()
{
	int iCount;
	//-----------------------------------------------------------------
	// 전체를 돌면서 모두 지우자.
	//-----------------------------------------------------------------
	for (iCount = 0; iCount < m_iMaxSprite; iCount++)
	{
		ReleaseSprite(iCount);
	}
	delete[] m_stpSprite;
}

///////////////////////////////////////////////////////
// LoadDibSprite. 
// BMP파일을 읽어서 하나의 프레임으로 저장한다.
//
// Parameters: (int)SpriteIndex. (char *)FileName. (int)CenterPointX. (int)CenterPointY.
// Return: (BOOL)TRUE, FALSE.
///////////////////////////////////////////////////////
BOOL CSpriteDib::LoadDibSprite(int iSpriteIndex, WCHAR *szFileName, int iCenterPointX,
	int iCenterPointY)
{
	//HANDLE		hFile;
	//DWORD		dwRead;
	int		iPitch;
	size_t		iImageSize;
	BITMAPFILEHEADER	stFileHeader;
	BITMAPINFOHEADER	stInfoHeader;
	
	FILE *fp;
	int err;
	int iCount;
	//-----------------------------------------------------------------
	// 비트맵 헤더를 열어 BMP 파일 확인.
	//-----------------------------------------------------------------
	err = _wfopen_s(&fp, szFileName, L"rb");
	if (err != 0) {
		return 0;
	}
	fread(&stFileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
	if (stFileHeader.bfType != 0x4d42) {
		return 0;
	}
	
	//-----------------------------------------------------------------
	// 한줄, 한줄의 피치값을 구한다.
	//-----------------------------------------------------------------
	fread(&stInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp);
	iPitch = (stInfoHeader.biWidth * (stInfoHeader.biBitCount / 8) + 3) & ~3;

	//-----------------------------------------------------------------
	// 스프라이트 구조체에 크기 저장.
	//-----------------------------------------------------------------
	m_stpSprite[iSpriteIndex].iWidth = stInfoHeader.biWidth;
	m_stpSprite[iSpriteIndex].iHeight = stInfoHeader.biHeight;
	m_stpSprite[iSpriteIndex].iPitch = iPitch;
	m_stpSprite[iSpriteIndex].iCenterPointX = iCenterPointX;
	m_stpSprite[iSpriteIndex].iCenterPointY = iCenterPointY;

	//-----------------------------------------------------------------
	// 이미지에 대한 전체 크기를 구하고, 메모리 할당.
	//-----------------------------------------------------------------
	iImageSize = iPitch * stInfoHeader.biHeight;
	m_stpSprite[iSpriteIndex].bypImage = new BYTE[iImageSize];

	//-----------------------------------------------------------------
	// 이미지 부분은 스프라이트 버퍼로 읽어온다.
	// DIB 는 뒤집어져 있으므로 이를 다시 뒤집자.
	// 임시 버퍼에 읽은 뒤에 뒤집으면서 복사한다.
	//-----------------------------------------------------------------
	BYTE *buffer = new BYTE[iImageSize];
	BYTE *tempBuffer;
	BYTE *tempSprite = m_stpSprite[iSpriteIndex].bypImage;
	fread(buffer, iImageSize, 1, fp);
	tempBuffer = buffer +(iPitch * (stInfoHeader.biHeight - 1));

	for (iCount = 0; iCount < stInfoHeader.biHeight; iCount++) {
		memcpy(tempSprite, tempBuffer, stInfoHeader.biWidth * 4);
		tempBuffer = tempBuffer - iPitch;
		tempSprite = tempSprite + iPitch;
	}

	delete[] buffer;
	fclose(fp);
	//CloseHandle(hFile);
	
	return FALSE;
}
///////////////////////////////////////////////////////
// ReleaseSprite. 
// 해당 스프라이트 해제.
//
// Parameters: (int)SpriteIndex.
// Return: (BOOL)TRUE, FALSE.
///////////////////////////////////////////////////////
void CSpriteDib::ReleaseSprite(int iSpriteIndex)
{
	//-----------------------------------------------------------------
	// 최대 할당된 스프라이트를 넘어서면 안되지롱
	//-----------------------------------------------------------------
	if (m_iMaxSprite <= iSpriteIndex)
		return;

	if (NULL != m_stpSprite[iSpriteIndex].bypImage)
	{
		//-----------------------------------------------------------------
		// 삭제 후 초기화.
		//-----------------------------------------------------------------
		delete[] m_stpSprite[iSpriteIndex].bypImage;
		//memset(&m_stpSprite[iSpriteIndex], 0, sizeof(st_SPRITE));
	}
}

///////////////////////////////////////////////////////
// DrawSprite. 
// 특정 메모리 위치에 스프라이트를 출력한다. (클리핑 처리)
// 체력바 처리
//
///////////////////////////////////////////////////////
void CSpriteDib::DrawSprite(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// 최대 스프라이트 개수를 초과 하거나, 로드되지 않는 스프라이트라면 무시
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// 지역변수로 필요정보 셋팅
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// 출력 중점으로 좌표 계산
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// 상단 에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	// 하단에 사이즈 계산. (하단 클리핑)
	// 왼쪽 출력 위치 계산. (좌측 클리핑)
	// 오른쪽 출력 위치 계산. (우측 클리핑)
	//-----------------------------------------------------------------

	//2018.02.08
	//카메라 기준으로 스프라이트 값 클리핑 처리
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//카메라 기준으로 iDrawX iDrawY 값 좌표 맞추기
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// 화면에 찍을 위치로 이동한다.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// 전체 크기를 돌면서 픽셀마다 투명색 처리를 하며 그림 출력.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*dwpDest = *dwpSprite;
			}
			
			//다음칸 이동
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// 다음줄로 이동, 화면과 스프라이트 모두..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}


void CSpriteDib::DrawImage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// 최대 스프라이트 개수를 초과 하거나, 로드되지 않는 스프라이트라면 무시
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;
		

	//-----------------------------------------------------------------
	// 지역변수로 필요정보 셋팅
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// 출력 중점으로 좌표 계산
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;
	
	//-----------------------------------------------------------------
	// 상단 에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	// 하단에 사이즈 계산. (하단 클리핑)
	// 왼쪽 출력 위치 계산. (좌측 클리핑)
	// 오른쪽 출력 위치 계산. (우측 클리핑)
	//-----------------------------------------------------------------
	
	//2018.02.08
	//카메라 기준으로 스프라이트 값 클리핑 처리
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//카메라 기준으로 iDrawX iDrawY 값 좌표 맞추기
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// 화면에 찍을 위치로 이동한다.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// 전체 크기를 돌면서 픽셀마다 그림 출력.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			*dwpDest = *dwpSprite;
			//다음칸 이동
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// 다음줄로 이동, 화면과 스프라이트 모두..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;
		
		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
	
	
}



/*********************************
	DrawSpriteRed
	빨간색 톤으로 이미지 출력
**********************************/
void CSpriteDib::DrawSpriteRed(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// 최대 스프라이트 개수를 초과 하거나, 로드되지 않는 스프라이트라면 무시
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// 지역변수로 필요정보 셋팅
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// 출력 중점으로 좌표 계산
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// 상단 에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	// 하단에 사이즈 계산. (하단 클리핑)
	// 왼쪽 출력 위치 계산. (좌측 클리핑)
	// 오른쪽 출력 위치 계산. (우측 클리핑)
	//-----------------------------------------------------------------

	//2018.02.08
	//카메라 기준으로 스프라이트 값 클리핑 처리
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//카메라 기준으로 iDrawX iDrawY 값 좌표 맞추기
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// 화면에 찍을 위치로 이동한다.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// 전체 크기를 돌면서 픽셀마다 투명색 처리를 하며 그림 출력.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*((BYTE *)dwpDest + 2) = *((BYTE *)dwpSprite + 2);
				*((BYTE *)dwpDest + 1) = *((BYTE *)dwpSprite + 1) / 2;
				*((BYTE *)dwpDest + 0) = *((BYTE *)dwpSprite + 0) / 2;
			}

			//다음칸 이동
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// 다음줄로 이동, 화면과 스프라이트 모두..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}




/*********************************
	DrawSpriteCompose
	합성 출력
**********************************/
void CSpriteDib::DrawSpriteCompose(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch)
{
	//-----------------------------------------------------------------
	// 최대 스프라이트 개수를 초과 하거나, 로드되지 않는 스프라이트라면 무시
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// 지역변수로 필요정보 셋팅
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// 출력 중점으로 좌표 계산
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// 상단 에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	// 하단에 사이즈 계산. (하단 클리핑)
	// 왼쪽 출력 위치 계산. (좌측 클리핑)
	// 오른쪽 출력 위치 계산. (우측 클리핑)
	//-----------------------------------------------------------------

	//2018.02.08
	//카메라 기준으로 스프라이트 값 클리핑 처리
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//카메라 기준으로 iDrawX iDrawY 값 좌표 맞추기
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// 화면에 찍을 위치로 이동한다.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// 전체 크기를 돌면서 픽셀마다 투명색 처리를 하며 그림 출력.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*((BYTE *)dwpDest + 2) = (*((BYTE *)dwpSprite + 2) / 2) + (*((BYTE *)dwpDest + 2) / 2);
				*((BYTE *)dwpDest + 1) = (*((BYTE *)dwpSprite + 1) / 2) + (*((BYTE *)dwpDest + 1) / 2);
				*((BYTE *)dwpDest + 0) = (*((BYTE *)dwpSprite + 0) / 2) + (*((BYTE *)dwpDest + 0) / 2);
			}

			//다음칸 이동
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// 다음줄로 이동, 화면과 스프라이트 모두..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}

// 체력바 용
void CSpriteDib::DrawPercentage(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch, int iPercentage) {
	//-----------------------------------------------------------------
	// 최대 스프라이트 개수를 초과 하거나, 로드되지 않는 스프라이트라면 무시
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// 지역변수로 필요정보 셋팅
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iCountY;
	int iCountX;
	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;
	BYTE *bypTempSprite;
	BYTE *bypTempDest;

	//-----------------------------------------------------------------
	// 출력 중점으로 좌표 계산
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// 상단 에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	// 하단에 사이즈 계산. (하단 클리핑)
	// 왼쪽 출력 위치 계산. (좌측 클리핑)
	// 오른쪽 출력 위치 계산. (우측 클리핑)
	//-----------------------------------------------------------------

	//2018.02.08
	//카메라 기준으로 스프라이트 값 클리핑 처리
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//카메라 기준으로 iDrawX iDrawY 값 좌표 맞추기
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	//-----------------------------------------------------------------
	// 화면에 찍을 위치로 이동한다.
	//-----------------------------------------------------------------
	dwpDest = (DWORD *)((BYTE *)(dwpDest + iDrawX) + (iDrawY * iDestPitch));
	bypTempSprite = (BYTE *)dwpSprite;
	bypTempDest = (BYTE *)dwpDest;

	//-----------------------------------------------------------------
	// 전체 크기를 돌면서 픽셀마다 투명색 처리를 하며 그림 출력.
	//-----------------------------------------------------------------
	for (iCountY = 0; iSpriteHeight > iCountY; iCountY++)
	{
		for (iCountX = 0; iSpriteWidth - ((100 - iPercentage) * ((double)iSpriteWidth / (double)100)) > iCountX; iCountX++)
		{
			if (m_dwColorKey != (*dwpSprite & 0x00ffffff)) {
				*dwpDest = *dwpSprite;
			}

			//다음칸 이동
			++dwpDest;
			++dwpSprite;
		}
		//-----------------------------------------------------------------
		// 다음줄로 이동, 화면과 스프라이트 모두..
		//-----------------------------------------------------------------
		bypTempDest = bypTempDest + iDestPitch;
		bypTempSprite = bypTempSprite + iSpritePitch;

		dwpDest = (DWORD *)bypTempDest;
		dwpSprite = (DWORD *)bypTempSprite;
	}
}

// 테스트 용
void CSpriteDib::DrawTextID(int iSpriteIndex, int iDrawX, int iDrawY, BYTE *bypDest, int iDestWidth,
	int iDestHeight, int iDestPitch, HWND hWnd, UINT id) {
	//-----------------------------------------------------------------
	// 최대 스프라이트 개수를 초과 하거나, 로드되지 않는 스프라이트라면 무시
	//-----------------------------------------------------------------
	if (m_iMaxSprite < iSpriteIndex)
		return;

	if (m_stpSprite[iSpriteIndex].bypImage == NULL)
		return;

	//-----------------------------------------------------------------
	// 지역변수로 필요정보 셋팅
	//-----------------------------------------------------------------
	st_SPRITE *stpSprite = &m_stpSprite[iSpriteIndex];

	int iSpriteHeight = stpSprite->iHeight;
	int iSpriteWidth = stpSprite->iWidth;
	int iSpritePitch = stpSprite->iPitch;
	int iSpriteCenterX = stpSprite->iCenterPointX;
	int iSpriteCenterY = stpSprite->iCenterPointY;
	DWORD *dwpSprite = (DWORD *)stpSprite->bypImage;
	DWORD *dwpDest = (DWORD *)bypDest;

	//-----------------------------------------------------------------
	// 출력 중점으로 좌표 계산
	//-----------------------------------------------------------------
	iDrawX = iDrawX - iSpriteCenterX;
	iDrawY = iDrawY - iSpriteCenterY;

	//-----------------------------------------------------------------
	// 상단 에 대한 스프라이트 출력 위치 계산. (상단 클리핑)
	// 하단에 사이즈 계산. (하단 클리핑)
	// 왼쪽 출력 위치 계산. (좌측 클리핑)
	// 오른쪽 출력 위치 계산. (우측 클리핑)
	//-----------------------------------------------------------------

	//2018.02.08
	//카메라 기준으로 스프라이트 값 클리핑 처리
	int clippingY;
	int clippingX;

	if (iDrawY < m_iCameraPosY) {
		//2018.02.08
		clippingY = m_iCameraPosY - iDrawY;
		iSpriteHeight = iSpriteHeight - clippingY;
		if (iSpriteHeight > 0) {
			dwpSprite = (DWORD *)((BYTE *)dwpSprite + iSpritePitch * clippingY);
		}
		iDrawY = m_iCameraPosY;
	}

	if (iDrawY + iSpriteHeight >= m_iCameraPosY + iDestHeight) {
		iSpriteHeight = iSpriteHeight - ((iDrawY + iSpriteHeight) - (m_iCameraPosY + iDestHeight));
	}

	if (iDrawX < m_iCameraPosX) {
		//2018.02.08
		clippingX = m_iCameraPosX - iDrawX;
		iSpriteWidth = iSpriteWidth - clippingX;
		if (iSpriteWidth > 0) {
			dwpSprite = dwpSprite + clippingX;
		}
		iDrawX = m_iCameraPosX;
	}

	if (iDrawX + iSpriteWidth >= m_iCameraPosX + iDestWidth) {
		iSpriteWidth = iSpriteWidth - ((iDrawX + iSpriteWidth) - (m_iCameraPosX + iDestWidth));
	}


	if (iSpriteWidth <= 0 || iSpriteHeight <= 0) {
		return;
	}

	//2018.02.08
	//카메라 기준으로 iDrawX iDrawY 값 좌표 맞추기
	iDrawX = iDrawX - m_iCameraPosX;
	iDrawY = iDrawY - m_iCameraPosY;

	HDC hdc;
	RECT DCRect;

	GetClientRect(hWnd, &DCRect);
	hdc = GetDC(hWnd);

	WCHAR wchString[32];
	wsprintfW(wchString, L"%d", id);
	TextOut(hdc, iDrawX, iDrawY, wchString, wcslen(wchString));

	ReleaseDC(hWnd, hdc);
}



//카메라 위치
void CSpriteDib::SetCameraPosition(int iPosX, int iPosY) {
	m_iCameraPosX = iPosX;
	m_iCameraPosY = iPosY;
}