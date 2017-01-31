#pragma once

#include <png.h>
#include <zlib.h>

class png_file
{
public:
	png_file():pData_(nullptr){ clear_info(); }
	~png_file(){ delete[] pData_; png_image_free(&info_); }

	void clear_info();
	// 対応フォーマットはRGBかRGBAだけ、bAlphaをtrueにするとRGBA
	void set_info(uint32_t nWidth, uint32_t nHeight, bool bAlpha);
	bool create_data(uint8_t alpha=255);

	bool load(const string& sFilePath);
	bool write(const string& sFilePath);

	// 座標は左上から右下
	// RGBAで返って来る。フォーマットがRGBの時はAは255
	uint32_t pixel(uint32_t x, uint32_t y)const;
	void	 set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a=255);
	void	 set_pixel_channel(uint32_t x, uint32_t y, uint8_t c, uint32_t channel);

	uint32_t	width()const{ return info_.width; }
	uint32_t	height()const{ return info_.height; }

	png_image&	info(){ return info_; }
	uint8_t*	data(){ return pData_; }

private:
	uint32_t pixel_pos(uint32_t x, uint32_t y)const;

private:
	png_image info_;

	uint8_t*  pData_;
	uint32_t  nStride_;
};