#include "stdafx.h"

#include "png_file.h"

void png_file::clear_info()
{
	memset(&info_, 0, sizeof(info_));
	info_.version = PNG_IMAGE_VERSION;
}

void png_file::set_info(uint32_t nWidth, uint32_t nHeight, bool bAlpha)
{
	info_.width	 = nWidth;
	info_.height = nHeight;

	if(bAlpha)
		info_.format = PNG_FORMAT_RGBA;
	else
		info_.format = PNG_FORMAT_RGB;
}

bool png_file::create_data(uint8_t alpha)
{
	if(info_.width==0 || info_.height==0 || info_.format==0)
		return false;

	nStride_ = PNG_IMAGE_ROW_STRIDE(info_);
	uint32_t size = PNG_IMAGE_BUFFER_SIZE(info_,nStride_);
	pData_ =  new uint8_t[size];
	
	if(info_.format == PNG_FORMAT_RGBA)
	{
		for(uint32_t y=0; y<height(); ++y)
			for(uint32_t x=0; x<width(); ++x)
				set_pixel(x,y,0,0,0,alpha);

	}
	else
	{
		memset(pData_, 0, sizeof(uint8_t)*size);
	}

	return true;
}

bool png_file::load(const string& sFilePath)
{
	clear_info();
	png_image_begin_read_from_file(&info_, sFilePath.c_str());

	if(PNG_IMAGE_FAILED(info_))
	{
		cout << info_.message << endl;
		return false;
	}

	if(!create_data()) return false;

	png_image_finish_read(&info_, NULL, pData_, nStride_, NULL);

	if(PNG_IMAGE_FAILED(info_))
	{
		cout << info_.message << endl;
		return false;
	}

	return true;
}

bool png_file::write(const string& sFilePath)
{
	png_image_write_to_file(&info_, sFilePath.c_str(), 0, pData_, nStride_, NULL);

	if(PNG_IMAGE_FAILED(info_))
	{
		cout << info_.message << endl;
		return false;
	}

	return true;
}


uint32_t png_file::pixel(uint32_t x, uint32_t y)const
{
	if(!pData_) return 0;
	
	uint32_t i = pixel_pos(x,y);

	if(info_.format==PNG_FORMAT_RGB)
		return pData_[i]<<24|pData_[i+1]<<16|pData_[i+2]<<8|0xFF;
	else if(info_.format==PNG_FORMAT_RGBA)
		return pData_[i]<<24|pData_[i+1]<<16|pData_[i+2]<<8|pData_[i+3];
	else
		return 0;
}

void png_file::set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if(!pData_) return;

	if(x>=info_.width
	|| y>=info_.height)
	{
		cout << x << " " << y << " 範囲外アクセスです" << endl;
		return;
	}

	uint32_t i = pixel_pos(x,y);

	if(info_.format==PNG_FORMAT_RGB)
	{
		pData_[i]	= r;
		pData_[i+1]	= g;
		pData_[i+2]	= b;
	}
	else if(info_.format==PNG_FORMAT_RGBA)
	{
		pData_[i]	= r;
		pData_[i+1]	= g;
		pData_[i+2]	= b;
		pData_[i+3]	= a;
	}
}

void png_file::set_pixel_channel(uint32_t x, uint32_t y, uint8_t c, uint32_t channel)
{
	if(!pData_) return;

	if(x>=info_.width
	|| y>=info_.height)
	{
		cout << "範囲外アクセスです" << endl;
		return;
	}

	if(channel<3 || (channel>=3 && info_.format==PNG_FORMAT_RGBA))
	{
		uint32_t i = pixel_pos(x,y);
		pData_[i+channel]=c;
	}
}

uint32_t png_file::pixel_pos(uint32_t x, uint32_t y)const
{
	return x*PNG_IMAGE_PIXEL_SIZE(info_.format) + y*nStride_;
}