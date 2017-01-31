#pragma once

#include "bitmap.h"
#include "png_file.h"

struct letter_info
{
	string		sFilename;
	uint32_t	nPixelSize;
	uint32_t	nWidth,nHeight;
	int32_t		nBaseX,nBaseY;
	uint32_t	nAdvance; // 字送り量

	uint32_t	nPosX,nPosY; // まとめに書き込まれた位置
	uint32_t	nNo;		 // 書き込まれたまとめ番号
	uint32_t	nChannel;	 // 書き込まれたチャンネル。0:R 1:G 2:B 3:A
};

typedef std::map<uint16_t, letter_info> letter_info_map;

// bitmapを1枚の画像にまとめる
class combine
{
public:
	combine();
	~combine();

	// bPerChannelをtrueにするとチャンネル別にRGBAの巡に書き込まれる
	void init(const string& sFilePath, uint32_t nLetterPixelSize, uint32_t nCombineSize, bool bPerChannel=false);
	void fin();

	void combine_image(uint16_t code, letter_info& info, const bitmap& bmpLetter);

	// ファイル化したファイル名リスト
	const std::vector<string>& filename_vector()const{ return vecFileName_; }

private:
	void write_png();

private:
	png_file combineImage_;

	uint32_t nLetterPixelSize_;
	string	 sFilePath_;

	uint32_t nCombine_;
	int32_t  nCurX_;
	int32_t  nCurY_;

	bool	 bPerChannel_;
	uint32_t nChannel_;

	std::vector<string> vecFileName_;
};