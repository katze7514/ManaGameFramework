#pragma once

class bitmap
{
public:
	bitmap():data_(nullptr){ ::ZeroMemory(&info_, sizeof(info_)); info_.biPlanes = 1; }
	~bitmap(){ delete[] data_; }

	// sFilePathのBITMAPを読み込んでデータを展開する
	bool load(const string& sFilePath);

	// 現在のデータをsFilePathとして書き込む
	void write(const string& sFilePath);

	// 設定されてるinfoを元にbitmapデータ領域を確保する
	void create_data();

	BITMAPINFOHEADER& info(){ return info_; }
	BYTE*			  data(){ return data_; }
	uint32_t		  data_size()const{ return info_.biSizeImage; }
	uint32_t		  line_byte()const{ return nLineByte_; }

	void			  set_info(int32_t nWidth, int32_t nHeight, uint32_t nBitCount);

	// ピクセル操作。左下原点で右上に向かっていく座標系
	uint32_t		  pixel(int32_t x, int32_t y)const; // XRGBで返って来る
	void			  set_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b);

private:
	void	 calc_line_byte();
	uint32_t pixel_pos(int32_t x, int32_t y)const;

private:
	BITMAPINFOHEADER	info_;
	BYTE*				data_;
	uint32_t			nLineByte_; // 1ラインのバイト
};
