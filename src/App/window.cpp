#include "../mana_common.h"

#include "window.h"

namespace mana{
namespace app{

window::window():bMenu_(false),nStyle_(STYLE_WINDOW),nExStyle_(0),bValid_(false),threadProc_(*this),hAccel_(NULL),bCloseMes_(false),bActive_(false),eState_(state::NONE){}

bool window::create(const string& sCaption,  bool bFullScreen, uint32_t nWidth, uint32_t nHeight,uint32_t nAffinity)
{
	// すでにウインドウが作成されている
	if(is_create())
	{
		logger::infoln("[window]すでにウインドウが作成されています。：" + sCaption_);
		return true;
	}

	set_state(state::INIT);

	sCaption_		= sCaption;
	nWidth_			= nWidth;
	nHeight_		= nHeight;
	bFullScreen_	= bFullScreen;

	return threadProc_.kick(nAffinity);
}

void window::wait_fin()
{
	if(!is_create()) return;
	threadProc_.join();
}

bool window::create_window()
{
	// ウインドウクラス設定
	win_info winfo;
	on_pre_window_create(winfo);

	winfo.wc_.hInstance		= ::GetModuleHandle(NULL);
	winfo.wc_.lpfnWndProc	= &window::WndProcShare;
	
	// ウインドウクラス登録
	if(::RegisterClassEx(&winfo.wc_)==0) return false;

	// ウインドウサイズや初期表示位置などを計算
	int32_t nRealWidth=0, nRealHeight=0;
	int32_t nPosX=0, nPosY=0;

	if(bFullScreen_)
	{// フルスクリーンの時は画面サイズと同じにする
		nWidth_  = nRealWidth	= ::GetSystemMetrics(SM_CXSCREEN);
		nHeight_ = nRealHeight	= ::GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{// ウインドウモードの時

		// 実際のウインドウサイズを取得
		if(!calc_real_window_size(nRealWidth, nRealHeight)) return false;

		// 中央合わせにするための初期表示位置計算
		nPosX = (::GetSystemMetrics(SM_CXSCREEN) - nRealWidth)/2;
		if(nPosX<0) nPosX=0;

		nPosY = (::GetSystemMetrics(SM_CYSCREEN) - nRealHeight)/2;
		if(nPosY<0) nPosY=0;
	}

	// ウインドウ作成
	hWnd_ = ::CreateWindowEx(nExStyle_, winfo.wc_.lpszClassName, sCaption_.c_str(), nStyle_, 
							 nPosX, nPosY, nRealWidth, nRealHeight,
							 NULL, NULL, winfo.wc_.hInstance, NULL);
	
	if(!hWnd_)
	{
		logger::fatalln("[window]ウインドウ作成ができませんでした。：" + to_str(::GetLastError()));
		return false;
	}

	::SetWindowLong(hWnd_, GWL_USERDATA, reinterpret_cast<LONG>(this));
	::ShowWindow(hWnd_, SW_SHOWNORMAL);

	return true;
}

void window::on_pre_window_create(win_info& winfo)
{
	winfo.wc_.style			= CS_HREDRAW | CS_VREDRAW;
	winfo.wc_.cbClsExtra	= 0;
	winfo.wc_.cbWndExtra	= 0;
	winfo.wc_.hIcon			= NULL;
	winfo.wc_.hIconSm	 	= NULL;
	winfo.wc_.hCursor		= ::LoadCursor(NULL, IDC_ARROW);
	winfo.wc_.hbrBackground	= (HBRUSH)::GetStockObject(BLACK_BRUSH); // 背景は黒
	winfo.wc_.lpszMenuName	= NULL;
	winfo.wc_.lpszClassName	= sCaption_.c_str(); // 他のアプリからこのウインドウを探す時などに使う

	if(bFullScreen_) 
		nStyle_ = STYLE_FULLSCREEN;
	else
		nStyle_ = STYLE_WINDOW;

	bMenu_ = winfo.wc_.lpszMenuName!=NULL;
}

void window::main()
{
	using namespace std::chrono;

	while(wnd_msg_peek())
	{
		std::this_thread::sleep_until(steady_clock::now()+milliseconds(100));
	}

	// ループを抜けたら終了処理開始だよね
	set_state(state::STOP);
}

bool window::wnd_msg_wait()
{
	MSG msg;
	LRESULT r = ::GetMessage(&msg, NULL, 0, 0);
	if(r!=-1) // -1はエラー
	{
		if(SUCCEEDED(::TranslateAccelerator(hWnd_, hAccel_, &msg)))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return is_valid();
}

bool window::wnd_msg_peek()
{
	MSG msg;
	while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE|PM_NOYIELD))
	{
		if(SUCCEEDED(::TranslateAccelerator(hWnd_, hAccel_, &msg)))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return is_valid();
}

LRESULT window::wnd_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_ACTIVATE:
		bActive_ = ((LOWORD(wParam)!=WA_INACTIVE) && (HIWORD(wParam)==0));
	break;

	case WM_CLOSE:
		bCloseMes_=true;
	break;

	case WM_DESTROY:
		::ShowWindow(hWnd_, SW_HIDE);
		::PostQuitMessage(0);
	break;

	default: return ::DefWindowProc(hWnd_, uMsg, wParam, lParam);
	}

	return 0;
}

uint32_t window::operator()()
{
	// Loggerスレッド初期化
	logger::initializer_thread log_init_thread(sCaption_);

	// ウインドウ作成
	if(!create_window())
	{
		end_window();
		return 1;
	}
	active_window();

	// ウインドウ動作
	main();

	// ウインドウ動作終了後処理
	set_state(state::STOP);

	// main動作抜けなので、終了確認なしで直接ウインドウを終了する
	::DestroyWindow(hWnd_);

	end_window();
	return 0;
}

////////////////////////////////////
// window操作
////////////////////////////////////

void window::show_window(bool bShow)
{
	if(bShow)
		::ShowWindow(hWnd_, SW_SHOW);
	else
		::ShowWindow(hWnd_, SW_HIDE);
}

bool window::calc_real_window_size(int32_t& nRealWidth, int32_t& nRealHeight)
{
	RECT clientSize;
	clientSize.left		= 0;
	clientSize.top		= 0;
	clientSize.right	= nWidth_;
	clientSize.bottom	= nHeight_;

	// 正確なウインドウサイズを計算
	if(::AdjustWindowRectEx(&clientSize, nStyle_, bMenu_, nExStyle_)==0) return false;

	nRealWidth	= clientSize.right  - clientSize.left;
	nRealHeight	= clientSize.bottom - clientSize.top;

	return true;
}

////////////////////////////////////
// static
////////////////////////////////////

LRESULT CALLBACK window::WndProcShare(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	window* pWnd = reinterpret_cast<window*>(::GetWindowLong(hWnd, GWL_USERDATA));
	if(pWnd)
		return pWnd->wnd_proc(hWnd, uMsg, wParam, lParam);
	else
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	
}

} // namespace app end
} // namespace mana end
