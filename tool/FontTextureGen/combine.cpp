#include "stdafx.h"

#include "combine.h"

combine::combine():nLetterPixelSize_(0),nCombine_(0),nCurX_(0),nCurY_(0),bPerChannel_(false),nChannel_(0){}

combine::~combine()
{
}

void combine::init(const string& sFilePath, uint32_t nLetterPixelSize, uint32_t nCombineSize, bool bPerChannel)
{
	sFilePath_ = sFilePath;
	nLetterPixelSize_ = nLetterPixelSize;

	bPerChannel_ = bPerChannel;

	combineImage_.set_info(nCombineSize,nCombineSize,true);
	combineImage_.create_data(0);

	nCombine_	= 0;
	nCurX_		= 0;
	nCurY_		= 0;
	nChannel_	= 0;
}

void combine::fin()
{
	write_png();	
}

void combine::combine_image(uint16_t code, letter_info& info, const bitmap& bmpLetter)
{
	int32_t nWidth  = bmpLetter.width()>nLetterPixelSize_  ? nLetterPixelSize_ : bmpLetter.width();
	int32_t nHeight = bmpLetter.height()>nLetterPixelSize_ ? nLetterPixelSize_ : bmpLetter.height();

	if(bmpLetter.width()>nLetterPixelSize_ || bmpLetter.height()>nLetterPixelSize_)
		cout << std::hex << code << std::dec << " " << "幅or高さがピクセルサイズを超えています。: " << nLetterPixelSize_ << " " << bmpLetter.width() << " " << bmpLetter.height() << endl;

	info.nWidth		= nWidth;
	info.nHeight	= nHeight;
	info.nNo		= nCombine_;
	info.nChannel	= nChannel_;

	info.nPosX		= nCurX_;
	info.nPosY		= nCurY_;

	for(int32_t y=0; y<nHeight; ++y)
	{
		for(int32_t x=0; x<nWidth; ++x)
		{
			uint32_t color = bmpLetter.pixel(x,y);
			uint8_t* pColor = reinterpret_cast<uint8_t*>(&color);

			uint32_t nX = nCurX_+x;
			uint32_t nY = nCurY_+nHeight-1-y;

			if(bPerChannel_)
				combineImage_.set_pixel_channel(nX, nY, pColor[0], nChannel_);
			else
				combineImage_.set_pixel(nX, nY, pColor[0],pColor[0],pColor[0],pColor[0]);
		}
	}

	// 次の格子へ
	nCurX_+=nLetterPixelSize_;
	if(combineImage_.width()-nCurX_<=nLetterPixelSize_)
	{
		nCurX_=0;
		nCurY_+=nLetterPixelSize_;

		if(combineImage_.height()-nCurY_<=nLetterPixelSize_)
		{// Yを超えたら2枚目へ
			nCurY_=0;

			if(bPerChannel_)
			{
				++nChannel_;
				if(nChannel_>=4)
				{
					nChannel_=0;
					write_png();
				}
			}
			else
			{
				write_png();
			}
		}
	}
}

void combine::write_png()
{
	string s = sFilePath_ + "_" + lexical_cast<string>(nLetterPixelSize_) + "_" + lexical_cast<string>(nCombine_++);

	if(bPerChannel_)
		s += "_per.png";
	else
		s += ".png";

	combineImage_.write(s);
	combineImage_.create_data(0); // バッファ初期化

	vecFileName_.emplace_back(s);
}