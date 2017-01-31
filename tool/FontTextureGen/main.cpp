/*!
 *  フォントテクスチャ作成ツール
 *
 *　フォントファイルから、文字ごとのbmpを作成する。第一水準まで指定可能
 *  文字ごとのbmpから大きなbmpを作成する
 *
 *  ○ 指定したフォントのまとめフォントテクスチャを作る
 *		[フォントファイルパス] [-combine テクスチャサイズ]
 *
 *  ○ 指定したフォントの個別文字bmpを作る
 *		[フォントファイルパス]
 *
 *  ○ 指定したファイルに書かれてる文字のまとめフォントテクスチャを作る
 *		[-i 文字マップテキストへのパス] [-combine テクスチャサイズ]
 *
 *  ○ 指定したファイルに書かれてる文字の個別文字bmpを作る
 *		[-i 文字マップテキストへのパス]
 *
 *	○ 文字個別bmpをピクセルサイズの格子にして、まとめフォントテクスチャを作る
 *		[-letter ピクセルサイズ] [-combine テクスチャサイズ]
 *
 *   の5つの動作モードがある。
 *
 *	以下のオプションが任意で組み合わせることができる
 *  [-letter ピクセルサイズ] 生成する文字bmpのサイズ
 *  [-ascii]                 フォントファイル指定時に有効。アスキー文字までしか作らない。-jis1と一緒に指定すると-asciiが優先される
 *  [-jis1]                  フォントファイル指定時に有効。第一水準までしか作らない
 *  [-f 出力フォルダ]        ファイルの出力先。文字個別bmpまとめモードの時は文字個別bmpフォルダ(letter_tmp)が存在しているフォルダ
 *  [-mono]                  文字bmpを二値で出力する
 *  [-per]                   まとめテクスチャを作る時に文字をチャンネルごとに格納する。容量が4分の1になるがプログラム側の対応がいる
 */

#include "stdafx.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "charcodemap.h"
#include "bitmap.h"
#include "png.h"
#include "combine.h"

const string LETTER_FOLDER="letter_tmp/";

// 文字マップファイルを読んで、コード一覧にする
bool read_text_map_file(const string& sFilePath, set<uint16_t>& codetext);
// 文字bitmap生成する
bitmap* create_letter_bitmap(FT_Face face, uint16_t nLetterSjis);
// faceにあるletterをbmpにする。letterはsjisで指定すること
bool render_letter(FT_Face face, uint16_t nLetterSjis);

// まとめフォントテクスチャを直接作る
bool create_bitmap_combine(FT_Face face);
// 文字個別のbmpを出力する
bool create_bitmap_per_letter(FT_Face face);
// 指定されたフォルダにある文字bmpを画像に結合。文字列情報を書いたXMLが出力される
bool combine_letter_bmp();

//////////////////////////////////////
// arg
string			g_sFontPath;
uint32_t		g_nLetterPixelSize	= 32;
uint32_t		g_nLetterActSize	= g_nLetterPixelSize;
uint32_t		g_nCombinePixelSize	= 0;
bool			g_bAscii			= false;
bool			g_bJis1				= false;
FT_Render_Mode	g_nRenderMode		= FT_RENDER_MODE_NORMAL;
bool			g_bPerChannel		= false;
string			g_sFolder;
string			g_sInput;

string			g_sFontName;


//////////////////////////////////////
// main
int main(int argc, char* argv[])
{
	bool bLetter=false;
	bool bLetterAct=false;
	bool bCombine=false;
	bool bFolder=false;
	bool bInput=false;

	////////////////////////
	// 引数解析
	for(int32_t i=1; i<argc; ++i)
	{
		string arg(argv[i]);
		if(arg=="-h")
		{
			cout << "Usage:" << endl;
			cout << "  [フォントファイルパス] [-letter 1文字当たりのピクセルサイズ] [-act 実際の生成サイズ] [-combine まとめるテクスチャサイズ] [-f 出力フォルダ] [-jis1] [-mono] [-per] [-i 文字マップテキスト(SJIS)]" << endl;
			return 1;
		}
		else if(arg=="-letter")
		{
			bLetter=true;
		}
		else if(arg=="-act")
		{
			bLetterAct=true;
		}
		else if(arg=="-combine")
		{
			bCombine=true;
		}
		else if(arg=="-f")
		{
			bFolder=true;
		}
		else if(arg=="-ascii")
		{
			g_bAscii = true;
		}
		else if(arg=="-jis1")
		{
			g_bJis1 = true;
		}
		else if(arg=="-mono")
		{
			g_nRenderMode = FT_RENDER_MODE_MONO;
		}
		else if(arg=="-per")
		{
			g_bPerChannel = true;
		}
		else if(arg=="-i")
		{
			bInput=true;
		}
		else
		{
			if(bFolder)
			{
				g_sFolder = arg;
				bFolder=false;
			}
			else if(bInput)
			{
				g_sInput = arg;
				bInput=false;
			}
			else if(bLetter)
			{
				g_nLetterPixelSize = lexical_cast<uint32_t>(arg);
				bLetter=false;

			}
			else if(bLetterAct)
			{
				g_nLetterActSize = lexical_cast<uint32_t>(arg);
				bLetterAct=false;
			}
			else if(bCombine)
			{
				g_nCombinePixelSize = lexical_cast<uint32_t>(arg);
				bCombine=false;
			}
			else if(g_sFontPath.empty())
			{
				g_sFontPath = arg;
				g_sFontName = fs::path(g_sFontPath).stem().string();
			}
		}
	}
	
	if(!g_sFontPath.empty())
	{ cout << "            フォントパス: " << g_sFontPath << endl; }

	cout << "     1文字ピクセルサイズ: " << g_nLetterPixelSize << endl;
	cout << "   実1文字ピクセルサイズ: " << g_nLetterActSize   << endl;

	if(g_nCombinePixelSize>0)
	{
		cout << "まとめるテクスチャサイズ: " << g_nCombinePixelSize <<  endl; 

		if(g_bPerChannel)
		{   cout << "        チャンネル別格納: true" << endl; }
	}
	
	if(!g_sFolder.empty())
	{
		if(g_sFolder.find_last_of("/\\")==string::npos) g_sFolder += "/";

		cout << "            出力フォルダ: " << g_sFolder << endl;
	}
		
	if(g_nRenderMode == FT_RENDER_MODE_MONO)
	{   cout << "                   REDER: MONO" << endl; }
	
	if(g_bAscii)
	{   cout << "                   ASCII: 1" << endl; }
	else if(g_bJis1)
	{   cout << "                     JIS: 1" << endl; }



	if(!g_sInput.empty())
	{   cout << "      文字マップテキスト: " << g_sInput << endl; }

	system("pause");

	// 出力するフォルダを作る
	if(!g_sFolder.empty())
	{
		boost::system::error_code err;
		if(!fs::is_directory(g_sFolder,err))
		{
			if(!fs::create_directory(g_sFolder,err))
				return false;
		}
	}

	// 実サイズの方が大きかったら、ピクセルサイズにする
	if(g_nLetterActSize>g_nLetterPixelSize) g_nLetterActSize=g_nLetterPixelSize;

	////////////////////////////
	// 実処理
	// フォント情報取得
	FT_Error	error;
	FT_Library	library;
	FT_Face		face;

	if(!g_sFontPath.empty())
	{
		error = FT_Init_FreeType(&library);
		if(error) return 1;

		error = FT_New_Face(library, g_sFontPath.c_str(), 0, &face);

		if(error == FT_Err_Unknown_File_Format)
			cerr << "読み込めないフォントファイルです。" << endl;
		if(error) return 1;
	
		error = FT_Set_Pixel_Sizes(face, 0, g_nLetterActSize);
		if(error) return 1;

		if(g_nCombinePixelSize>0 && g_nCombinePixelSize>g_nLetterPixelSize)
		{// 文字個別bmpを生成しつつ、1枚にまとめる
			create_bitmap_combine(face);
		}
		else
		{// 文字個別bmpを生成する
			create_bitmap_per_letter(face);
		}

		FT_Done_FreeType(library);
	}
	else if(g_nCombinePixelSize>0 && g_nCombinePixelSize>g_nLetterPixelSize)
	{// 文字bmpを結合して1枚にする
		combine_letter_bmp();
	}

	//system("pause");
	return 0;
}


////////////////////////////////////////////
// 各処理関数
////////////////////////////////////////////

// cが何バイト文字かを返す
uint32_t count_char_sjis(uint8_t c)
{
	if(c==0) return 0; // NULL
	if((c>=0x81 && c<=0x9f) || (c>=0xe0 && c<=0xfc)) return 2;
	// それ以外は1バイト
	return 1;
}

void assign_code(uint16_t& code, uint8_t byteUP, uint8_t byteDown)
{
	uint8_t* p = reinterpret_cast<uint8_t*>(&code);

	// リトルエンディアン
	p[0] = byteDown;
	p[1] = byteUP;
}

// 文字マップファイルを読み込む 
bool read_text_map_file(const string& sFilePath, set<uint16_t>& codetext)
{
	fstream file;
	file.open(sFilePath.c_str(), fstream::in);
	if(!file.is_open()) return false;

	uint16_t code;
	char     c,byte;

	while(!file.eof())
	{
		file.read(&c, sizeof(c));
		uint8_t i = c;

		switch(count_char_sjis(i))
		{
		case 1:
			// 制御コードはスルー
			if((i>=0 && i<=0x2f)||i==0x7f) continue;

			assign_code(code,0,c);
		break;

		case 2:
			file.read(&byte, sizeof(byte));
			assign_code(code,c,byte);
		break;

		default:
			continue;
		break;
		}

		codetext.emplace(code);
	}

	return true;
}

// 指定された文字のbitmap生成して返す
bitmap* create_letter_bitmap(FT_Face face, uint16_t nLetterSjis)
{
	FT_Error error;

	FT_UInt glyph_index = FT_Get_Char_Index(face, gCodeMap.to_unicode(nLetterSjis));

	error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP);
	if(error) return nullptr;

	error = FT_Render_Glyph(face->glyph, g_nRenderMode);
	//if(error) return 1;
	if(error) return nullptr;

	FT_Bitmap bit = face->glyph->bitmap;

	if(bit.width==0 || bit.rows==0) return nullptr;

	if(!( bit.pixel_mode == FT_PIXEL_MODE_GRAY
		||bit.pixel_mode == FT_PIXEL_MODE_MONO))
		return nullptr;

	//cout << std::dec << bit.width << " " << bit.rows << endl;

	/*if(g_nLetterPixelSize<bit.width)
		g_nLetterPixelSize=bit.width;

	if(g_nLetterPixelSize<bit.rows)
		g_nLetterPixelSize=bit.rows;*/

	bitmap* image = new bitmap();
	image->set_info(bit.width, bit.rows, 24);
	image->create_data();

	for(int32_t h=0; h<bit.rows; ++h)
	{
		uint32_t i = ((bit.rows-1)-h)*bit.pitch;
		for(int32_t w=0; w<bit.width; ++w)
		{
			uint8_t c = 0;
			if(bit.pixel_mode == FT_PIXEL_MODE_GRAY)
			{
				c = bit.buffer[i+w];
			}
			else
			{// FT_PIXEL_MODE_MONO
				uint8_t j = bit.buffer[i+w/8]; // 所属してるバイトを取得
				uint8_t b = 0x80>>(w%8); // 左からのビット位置
				if((j&b)>0) 
					c=255;
				else
					c=0;
			}

			image->set_pixel(w,h,c,c,c);
		}
	}

	return image;
}

string create_font_tex_id(uint32_t no)
{
	if(g_bPerChannel)
		return g_sFontName + "_" + lexical_cast<string>(g_nLetterPixelSize) + "_" + lexical_cast<string>(no) + "_per";
	else
		return g_sFontName + "_" + lexical_cast<string>(g_nLetterPixelSize) + "_" + lexical_cast<string>(no);
}

void write_letter_xml(const vector<string>& vecFileList, const letter_info_map& mapLetterFile)
{
	using namespace boost::property_tree;
	using namespace boost::property_tree::xml_parser;
	ptree x;

	ptree& font = x.add("font","");
	font.put("<xmlattr>.id", g_sFontName + "_" + lexical_cast<string>(g_nLetterPixelSize));
	font.put("<xmlattr>.num", mapLetterFile.size());
	font.put("<xmlattr>.letter_size", g_nLetterPixelSize);
	font.put("<xmlattr>.antialias", g_nRenderMode==FT_RENDER_MODE_NORMAL);
	font.put("<xmlattr>.per", g_bPerChannel);

	// テクスチャ情報を書く
	uint32_t no=0;
	for(auto& it : vecFileList)
	{
		ptree& tex = font.add("texture","");
		tex.put("<xmlattr>.id",  create_font_tex_id(no));
		tex.put("<xmlattr>.src", it);
		++no;
	}

	// 文字情報を書く
	for(auto& it : mapLetterFile)
	{
		ptree& letter = font.add("letter","");
		// 文字コードは属性
		std::stringstream ss;
		ss << std::hex << std::setw(4) << std::setfill('0') << it.first;
		letter.put("<xmlattr>.code", ss.str());

		ptree& tex = letter.add("tex","");
		tex.put("<xmlattr>.id", create_font_tex_id(it.second.nNo));

		ptree& pos = letter.add("rect","");
		pos.put("<xmlattr>.left", it.second.nPosX);
		pos.put("<xmlattr>.top", it.second.nPosY);
		pos.put("<xmlattr>.right", it.second.nPosX + it.second.nWidth);
		pos.put("<xmlattr>.bottom", it.second.nPosY + it.second.nHeight);

		ptree& baseline = letter.add("base","");
		baseline.put("<xmlattr>.x", it.second.nBaseX);
		baseline.put("<xmlattr>.y", it.second.nBaseY);
		baseline.put("<xmlattr>.advance", it.second.nAdvance);

		ptree& channel = letter.add("channel","");
		channel.put("<xmlattr>.no", it.second.nChannel);
	}
	
	string sFontXML = g_sFolder + "font";
	if(!g_sFontName.empty()) sFontXML = g_sFolder + g_sFontName;
	if(g_bPerChannel) sFontXML += "_per";

	write_xml(sFontXML + "_" + lexical_cast<string>(g_nLetterPixelSize) + ".xml", x, std::locale(), xml_writer_make_settings(' ', 4, widen<string>("Shift_JIS")));
}

/////////////////////////////////////////
// まとめフォントテクスチャを直接作れる
void set_letter_info(letter_info& info, FT_GlyphSlot glyph)
{
	int32_t nWidth  = glyph->bitmap.width  - info.nPixelSize;
	int32_t nHeight = glyph->bitmap.rows - info.nPixelSize;

	info.nBaseX		= glyph->bitmap_left;
	if(nWidth>0) info.nBaseX-=nWidth;

	info.nBaseY		= glyph->bitmap_top;
	if(nHeight>0) info.nBaseY-=nHeight;

	info.nAdvance	= (glyph->metrics.horiAdvance/64);
}

bool create_bitmap_combine(FT_Face face)
{
	letter_info_map mapLetterFile; // 文字コードとファイル名のマップ

	combine combineImage;
	combineImage.init(g_sFolder+g_sFontName, g_nLetterPixelSize, g_nCombinePixelSize, g_bPerChannel);

	// フォントレンダリング
	if(g_sInput.empty())
	{// 文字マップテキストが無い時は、SJIS文字すべてを対象にする
		for(auto it : gCodeMap.sjis_to_unicode_map())
		{
			if(g_bAscii && it.first>0x7E)  break; //ASCIIまでならこれで収容
			if(g_bJis1 && it.first>0x9872) break; //第一水準までならこれで終了

			bitmap* pLetter = create_letter_bitmap(face, it.first);
			if(!pLetter) continue;

			letter_info info;
			info.nPixelSize = g_nLetterPixelSize;
			set_letter_info(info, face->glyph);

			combineImage.combine_image(it.first, info, *pLetter);

			delete pLetter;

			mapLetterFile.emplace(it.first, info);
		}
	}
	else
	{// 文字マップテキストを読み込む
		set<uint16_t> codetext;
		if(!read_text_map_file(g_sInput, codetext)) return false;

		for(auto it : codetext)
		{
			bitmap* pLetter = create_letter_bitmap(face, it);
			if(!pLetter) continue;

			letter_info info;
			set_letter_info(info, face->glyph);
			info.nPixelSize = g_nLetterPixelSize;

			combineImage.combine_image(it, info, *pLetter);

			delete pLetter;

			mapLetterFile.emplace(it, info);
		}
	}
	
	combineImage.fin();

	write_letter_xml(combineImage.filename_vector(), mapLetterFile);

	return true;
}

////////////////////////////////////////
// 文字をbmpファイルに書き出す
bool render_letter(FT_Face face, uint16_t nLetterSjis)
{
	bitmap* image = create_letter_bitmap(face,nLetterSjis);

	if(!image) return false;
	
	FT_Bitmap bit = face->glyph->bitmap;

	std::stringstream ss;
	ss  << std::hex << std::setw(4) << std::setfill('0') << nLetterSjis
		<< std::dec << "_" << g_nLetterPixelSize << "_" << bit.width << "_" << bit.rows
			<< "_" << face->glyph->bitmap_left << "_" << face->glyph->bitmap_top <<  ".bmp";

	image->write(g_sFolder + LETTER_FOLDER + ss.str()); 
	delete image;

	return true;
}

// 個別文字bmpを出力する
bool create_bitmap_per_letter(FT_Face face)
{
	// 文字bmpを出力するフォルダを作る
	string sLetterFolder = g_sFolder+LETTER_FOLDER;
	boost::system::error_code err;
	if(!fs::is_directory(sLetterFolder,err))
	{
		if(!fs::create_directory(sLetterFolder,err))
			return false;
	}

	// フォントレンダリング
	if(g_sInput.empty())
	{// 文字マップテキストが無い時は、SJIS文字すべてを対象にする
		for(auto it : gCodeMap.sjis_to_unicode_map())
		{
			if(g_bJis1 && it.first>0x9872) break; //第一水準までならこれで終了

			render_letter(face, it.first);
		}
	}
	else
	{// 文字マップテキストを読み込む
		set<uint16_t> codetext;
		if(!read_text_map_file(g_sInput, codetext)) return false;

		for(auto it : codetext)
			render_letter(face, it);
	}

	return true;
}

////////////////////////////////////////////
// 指定されたフォルダの文字bmpをまとめる
///////////////////////////////////////////
uint16_t parse_letter_info(const string& sFilename, letter_info& info)
{
	info.sFilename = sFilename;
	vector<string> token;
	boost::split(token, sFilename, boost::is_any_of("_."));

	char* endp;
	uint16_t code	= static_cast<uint16_t>(::strtoul(token[0].c_str(),&endp,16));
	info.nPixelSize = lexical_cast<uint32_t>(token[1]);
	info.nWidth		= lexical_cast<uint32_t>(token[2]);
	info.nHeight	= lexical_cast<uint32_t>(token[3]);
	info.nBaseX		= lexical_cast<int32_t>(token[4]);
	info.nBaseY		= lexical_cast<int32_t>(token[5]);

	return code;
}

// 指定されたフォルダにある文字別bmpを1枚にまとめる
bool combine_letter_bmp()
{
	string sLetterFolder = g_sFolder + LETTER_FOLDER;
	letter_info_map mapLetterFile; // 文字コードとファイル名のマップ

	// 文字bmpが展開されているファイル名一覧を取得する
	fs::directory_iterator dend;
	for(fs::directory_iterator it(sLetterFolder); it!=dend; ++it)
	{
		if(fs::is_directory(it->path())) continue;

		if(it->path().extension()!=".bmp") continue;

		string filename = it->path().filename().string();

		letter_info linfo;
		uint16_t	code  = parse_letter_info(filename,linfo);

		if(linfo.nPixelSize==g_nLetterPixelSize) 
			mapLetterFile.emplace(code, linfo);
	}

	if(mapLetterFile.empty()) return false;

	combine combineImage;
	combineImage.init(g_sFolder, g_nLetterPixelSize, g_nCombinePixelSize, true);

	bitmap bmpLetter;
	for(auto& it : mapLetterFile)
	{
		if(bmpLetter.load(g_sFolder + LETTER_FOLDER + it.second.sFilename))
			continue;

		combineImage.combine_image(it.first, it.second, bmpLetter);
	}

	write_letter_xml(combineImage.filename_vector(), mapLetterFile);

	return true;
}

namespace boost{
void throw_exception(const std::exception& e)
{
	cerr << e.what() << endl;
}
}
