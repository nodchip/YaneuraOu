
// tanuki-book-viewer.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CtanukibookviewerApp:
// このクラスの実装については、tanuki-book-viewer.cpp を参照してください。
//

class CtanukibookviewerApp : public CWinApp
{
public:
	CtanukibookviewerApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CtanukibookviewerApp theApp;