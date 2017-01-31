#include "stdafx.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

#include <boost/algorithm/clamp.hpp>
using boost::algorithm::clamp;

#include "bitmap.h"

// @param[in] inImage 入力画像
// @param[in] nMaxSearchRadius ディスタンス計算する時にデータを検索しに行く範囲を決める係数
// @param[in] outImage 出力先ディスタンスフィールド画像
void calc_distance_bmp(bitmap& inImage, float fMaxRadius, bitmap& outImage);

////////////////////////////
// main

// 元ファイル名 検索範囲係数 ディスタンスファイル名 ディスタンス横サイズ　ディスタンス縦サイズ
int main(int argc, char* argv[])
{
	string	sInFile;
	string	sOutFile;
	float   fRatio=2.0;
	int32_t	nWidth=0;
	int32_t	nHeight=0;

	if(argc!=6)
	{
		cout << "Usage:" << endl;
		cout << "\t元ファイル名 距離係数 ディスタンスファイル名 ディスタンス横サイズ ディスタンス縦サイズ" << endl;
		system("pause");
		return 1;
	}
	else
	{
		sInFile	 = argv[1];
		fRatio   = lexical_cast<float>(argv[2]);
		sOutFile = argv[3];
		nWidth	 = lexical_cast<int32_t>(argv[4]);
		nHeight	 = lexical_cast<int32_t>(argv[5]);

		cout << "arg:" << endl;
		cout << "\t" << sInFile.c_str() << " " << fRatio << " " << sOutFile.c_str() << " " << nWidth << " " << nHeight << endl;
	}

	bitmap inImage;
	if(!inImage.load(sInFile))
	{
		cout << sInFile.c_str() << "読み込み失敗しました。" << endl;
		system("pause");
		return 1;
	}

	// サイズの確認。ディスタンスファイルの方を大きくすることはできない
	if(inImage.info().biWidth < nWidth || inImage.info().biHeight < nHeight)
	{
		cout << "DistanceFieldの方を大きくすることはできません。" << endl;
		system("pause");
		return 1;
	}

	// ディスタンスファイルの準備
	bitmap outImage;
	outImage.set_info(nWidth, nHeight, 24);
	outImage.create_data();

	// ディスタンス計算＆プロット
	calc_distance_bmp(inImage, fRatio, outImage);

	// 書き出し
	outImage.write(sOutFile);
	
	system("pause");
	return 0;
}


//////////////////////////////////////////////
// distance計算系

// inImage内の(x,y)のディスタンスを計算する
float calc_distance_bmp_radius(bitmap& inImage, int32_t x, int32_t y, float fMaxRadius)
{
	uint32_t color = inImage.pixel(x,y);

	float minD  = fMaxRadius*fMaxRadius+1.0f/fMaxRadius;

	int32_t	sx,sy;
	int32_t	minX,maxX,minY,maxY;

	for(int32_t radius = 1; radius<fMaxRadius && radius*radius<minD; ++radius)
	{
		minX = clamp(x-radius, 0, inImage.info().biWidth-1);
		maxX = clamp(x+radius, 0, inImage.info().biWidth-1);
		minY = clamp(y-radius, 0, inImage.info().biHeight-1);
		maxY = clamp(y+radius, 0, inImage.info().biHeight-1);

		// ↑
		if(y+radius<inImage.info().biHeight)
		{
			for(sx = minX; sx<maxX; ++sx)
			{
				if(inImage.pixel(sx,maxY)!=color)
				{
					float d = static_cast<float>((sx-x)*(sx-x)+(maxY-y)*(maxY-y));
					if(d<minD) minD=d;
				}
			}
		}

		// ↓
		if(y-radius>0)
		{
			for(sx = minX; sx<maxX; ++sx)
			{
				if(inImage.pixel(sx,minY)!=color)
				{
					float d = static_cast<float>((sx-x)*(sx-x)+(minY-y)*(minY-y));
					if(d<minD) minD=d;
				}
			}
		}

		// →
		if(x+radius<inImage.info().biWidth)
		{
			for(sy = minY; sy<maxY; ++sy)
			{
				if(inImage.pixel(maxX,sy)!=color)
				{
					float d = static_cast<float>((maxX-x)*(maxX-x)+(sy-y)*(sy-y));
					if(d<minD) minD=d;
				}
			}
		}

		// ←
		if(x-radius>0)
		{
			for(sy = minY; sy<maxY; ++sy)
			{
				if(inImage.pixel(minX,sy)!=color)
				{
					float d = static_cast<float>((minX-x)*(minX-x)+(sy-y)*(sy-y));
					if(d<minD) minD=d;
				}
			}
		}
	}

	float df = sqrtf(minD);
	if(inImage.pixel(x,y)==0)
		df = -df;

	return df;
}

void calc_distance_bmp(bitmap& inImage, float fMaxRadiusRatio, bitmap& outImage)
{
	float fMaxRadius=1;

	if(inImage.info().biWidth > inImage.info().biHeight)
	{
		fMaxRadius = fMaxRadiusRatio*inImage.info().biWidth / outImage.info().biWidth;
	}
	else
	{
		fMaxRadius = fMaxRadiusRatio*inImage.info().biHeight / outImage.info().biHeight;
	}

	for(int32_t y=0; y<outImage.info().biHeight; ++y)
	{
		for(int32_t x=0; x<outImage.info().biWidth; ++x)
		{
			// ディスタンスフィールド上の位置
			uint32_t sx = x * (inImage.info().biWidth-1)  / (outImage.info().biWidth-1);
			uint32_t sy = y * (inImage.info().biHeight-1) / (outImage.info().biHeight-1);

			// 距離計算
			float d = calc_distance_bmp_radius(inImage,sx,sy,fMaxRadius);
			// 距離を比率にしてから、0～127.5にする
			d = 127.5f * d/fMaxRadius;
			d += 127.5f;
			uint32_t sd = clamp(static_cast<uint32_t>(d),0,255); // 端数繰り上げ

			// 計算結果を書き込む
			outImage.set_pixel(x,y,sd,sd,sd);
		}
	}
}
