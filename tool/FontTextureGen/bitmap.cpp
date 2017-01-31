#include "stdafx.h"

#include <fstream>
using std::fstream;

#include "bitmap.h"

bool bitmap::load(const string& sFilePath)
{
	fstream file;
	file.open(sFilePath.c_str(), fstream::in|fstream::binary);

	if(!file.is_open()) return false;

	uint8_t  c;
	uint16_t i;
	uint32_t n;
	int32_t  k;

	file.read(reinterpret_cast<char*>(&c), sizeof(c));
	if(c!='B') return false;

	file.read(reinterpret_cast<char*>(&c), sizeof(c));
	if(c!='M') return false;

	// ファイルサイズ
	file.read(reinterpret_cast<char*>(&n), sizeof(n));

	// 予約領域 2byte×2
	file.read(reinterpret_cast<char*>(&n), sizeof(n));

	// 画像データまでのオフセット
	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	
	/////////////////////
	// BMPINFOHEADER

	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	info_.biSize = n;

	file.read(reinterpret_cast<char*>(&k), sizeof(k));
	info_.biWidth = k;

	file.read(reinterpret_cast<char*>(&k), sizeof(k));
	info_.biHeight = k;

	file.read(reinterpret_cast<char*>(&i), sizeof(i));
	info_.biPlanes = i;

	file.read(reinterpret_cast<char*>(&i), sizeof(i));
	info_.biBitCount = i;

	// 24bitか32bitのみ対応
	if(!(info_.biBitCount==24 || info_.biBitCount==32))
		return false;

	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	info_.biCompression = n;

	// 無圧縮のみ対応
	if(info_.biCompression!=0)
		return false;

	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	info_.biSizeImage = n;

	file.read(reinterpret_cast<char*>(&k), sizeof(k));
	info_.biXPelsPerMeter = k;

	file.read(reinterpret_cast<char*>(&k), sizeof(k));
	info_.biYPelsPerMeter = k;

	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	info_.biClrUsed = n;

	file.read(reinterpret_cast<char*>(&n), sizeof(n));
	info_.biClrImportant = n;

	///////////////////////////
	// 画像データ

	// 画像サイズ分、確保
	create_data();

	// 画像を読み込む
	file.read(reinterpret_cast<char*>(data_), info_.biSizeImage);

	return true;
}

void bitmap::write(const string& sFilePath)
{
	fstream file;
	file.open(sFilePath.c_str(), fstream::out|fstream::binary);
	
	int8_t	 c;
	uint32_t n;

	c='B';
	file.write(reinterpret_cast<char*>(&c), sizeof(c));
	c='M';
	file.write(reinterpret_cast<char*>(&c), sizeof(c));

	// 全体のファイルサイズ
	n = 14+sizeof(info_)+info_.biSizeImage;
	file.write(reinterpret_cast<char*>(&n), sizeof(n));

	// 予約領域
	n = 0;
	file.write(reinterpret_cast<char*>(&n), sizeof(n));

	// ファイル先頭からデータまでのオフセット
	n = 14+sizeof(info_);
	file.write(reinterpret_cast<char*>(&n), sizeof(n));

	info_.biSize = sizeof(info_);

	// ヘッダー書き込み
	file.write(reinterpret_cast<char*>(&info_), sizeof(info_));

	// データ書き込み
	file.write(reinterpret_cast<char*>(data_), info_.biSizeImage);
}

void bitmap::set_info(int32_t nWidth, int32_t nHeight, uint32_t nBitCount)
{
	info_.biWidth	= nWidth;
	info_.biHeight	= nHeight;
	info_.biBitCount= nBitCount;
}

void bitmap::create_data()
{
	if(info_.biWidth>0
	&& info_.biHeight>0
	&& info_.biBitCount>0)
	{
		if(data_) delete[] data_;

		calc_line_byte();
		info_.biSizeImage = info_.biHeight * nLineByte_;
		data_= new BYTE[info_.biSizeImage]; 

		memset(data_, 0, info_.biSizeImage); // 0初期化
	}
}

void bitmap::calc_line_byte()
{
	nLineByte_ = info_.biWidth * info_.biBitCount / 8;
	nLineByte_ = (nLineByte_+3)&~3; // 4byteアライン
}

uint32_t bitmap::pixel(int32_t x, int32_t y)const
{
	uint32_t pos = pixel_pos(x,y);
	uint8_t b = data_[pos];
	uint8_t g = data_[pos+1];
	uint8_t r = data_[pos+2];
	
	return 0<<24|r<<16|g<<8|b;
}

void bitmap::set_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
	if(x>=info_.biWidth
	|| y>=info_.biHeight)
	{
		cout << "範囲外アクセスです " << x << " " << y << " " << endl;
		return;
	}

	uint32_t pos = pixel_pos(x,y);
	data_[pos]   = b;
	data_[pos+1] = g;
	data_[pos+2] = r;
}

uint32_t bitmap::pixel_pos(int32_t x, int32_t y)const
{
	return x*info_.biBitCount/8 + y*nLineByte_ ;
}
