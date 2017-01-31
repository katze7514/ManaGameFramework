#pragma once

namespace mana{
namespace sound{

enum play_mode : uint32_t
{
	MODE_NONE,			//!< 無効値
	MODE_STOP,			//!< 停止
	MODE_PLAY,			//!< 通常再生
	MODE_PLAY_LOOP,		//!< ループ再生
	MODE_PAUSE,			//!< ポーズ

	// 再生状況取得の際に返ってくる
	MODE_REQ,
	MODE_PROC,
	MODE_ERR,
};

enum play_event : uint32_t
{
	EV_PLAY,	//!< 再生開始
	EV_STOP,	//!< 停止した
	EV_PAUSE,	//!< ポーズした
	EV_OVERRIDE,//!< イベントハンドラが上書きされた
	EV_END,
};

enum const_param
{
	STREAM_SECONDS			=	4,		//!< ストリームバッファの確保秒数
	DEFAULT_MAX_INFO_NUM	=	256,	//!< sound_playerで扱うサウンド最大数デフォルト値
	DEFAULT_MAX_BUFFER_NUM	=	32,		//!< サウンドバッファ保持最大数
	DEFAULT_MAX_REQUEST_NUM	=	16,		//!< サウンドリクエスト推定数
	DEFAULT_INTERVAL_MS		=	1000/30,//!< デフォルトインターバルms
};

///////////////////////////////
/*! @brief ボリューム
 *
 *  DSBVOLUMEを比率で扱えるようにするクラス
 *  演算はすべて比率で計算される
 */
class volume
{
public:
	volume():nDB_(0){}

	int32_t db()const{ return nDB_; }
	void	set_db(int32_t nDB){ if(nDB<=0) nDB_=nDB; }

	// 比率での取得。0～100
	int32_t per()const
	{
		if(nDB_>=-600)
		{
			return 50 + ((nDB_+600)*50)/600;
		}
		else if(nDB_>=-1200)
		{
			return 25 + ((nDB_+1200)*25)/600;
		}
		else if(nDB_>=-1800)
		{
			return 12 + ((nDB_+1800)*12)/600;
		}
		else if(nDB_>=-2400)
		{
			return 6 + ((nDB_+2400)*6)/600;
		}
		else if(nDB_>=-3000)
		{
			return 3 + ((nDB_+3000)*3)/600;
		}
		else if(nDB_>=-3600)
		{
			return 1 + ((nDB_+3600))/600;
		}
		else
		{
			return 0;
		}
	}

	// 比率での設定。0～100
	void set_per(int32_t nPer)
	{
		nPer = clamp(nPer, 0, 100);

		if(nPer>=50)
		{// 0 ～ -600
			nDB_ = -600 + 600*(nPer-50)/50;
		}
		else if(nPer>=25)
		{// -600 ～ -1200
			nDB_ = -1200 + 600*(nPer-25)/25;
		}
		else if(nPer>=12)
		{// -1200 ～ -1800
			nDB_ = -1800 + 600*(nPer-12)/12;
		}
		else if(nPer>=6)
		{// -1800 ～ -2400
			nDB_ = -2400 + 600*(nPer-6)/6;
		}
		else if(nPer>=3)
		{// -2400 ～ -3000
			nDB_ = -3000 + 600*(nPer-3)/3;
		}
		else if(nPer>=1)
		{// -3000～-3600
			nDB_ = -3600 + 600*(nPer-1);
		}
		else
		{// 
			nDB_ = DSBVOLUME_MIN;
		}
	}

private:
	int32_t nDB_;
};

inline const volume& operator+(const volume& lhs, const volume& rhs)
{
	volume tmp;
	tmp.set_per(lhs.per()+rhs.per());
	return std::move(tmp);
}

inline const volume& operator-(const volume& lhs, const volume& rhs)
{
	volume tmp;
	tmp.set_per(lhs.per()-rhs.per());
	return std::move(tmp);
}

inline const volume& operator*(const volume& lhs, const volume& rhs)
{
	volume tmp;
	tmp.set_per(lhs.per()*rhs.per()/100);
	return std::move(tmp);
}

} // namespace sound end
} // namespace mana end
