
// tanuki-book-viewerDlg.cpp : 実装ファイル
//

#include "stdafx.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <limits>

#include "tanuki-book-viewer.h"
#include "tanuki-book-viewerDlg.h"
#include "afxdialogex.h"
#include "../../src/book.hpp"
#include "../../src/csa.hpp"
#include "../../src/search.hpp"
#include "../../src/usi.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
  CAboutDlg();

  // ダイアログ データ
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_ABOUTBOX };
#endif

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CtanukibookviewerDlg ダイアログ



CtanukibookviewerDlg::CtanukibookviewerDlg(CWnd* pParent /*=NULL*/)
  : CDialogEx(IDD_TANUKIBOOKVIEWER_DIALOG, pParent)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CtanukibookviewerDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_RICHEDIT22, m_richEditCtrlKifu);
  DDX_Control(pDX, IDC_RICHEDIT23, m_richEditCtrlBook);
}

BEGIN_MESSAGE_MAP(CtanukibookviewerDlg, CDialogEx)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_BN_CLICKED(IDOK, &CtanukibookviewerDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CtanukibookviewerDlg メッセージ ハンドラー

BOOL CtanukibookviewerDlg::OnInitDialog()
{
  CDialogEx::OnInitDialog();

  // "バージョン情報..." メニューをシステム メニューに追加します。

  // IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    BOOL bNameValid;
    CString strAboutMenu;
    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
  //  Framework は、この設定を自動的に行います。
  SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
  SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

  // TODO: 初期化をここに追加します。

  return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CtanukibookviewerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  }
  else
  {
    CDialogEx::OnSysCommand(nID, lParam);
  }
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CtanukibookviewerDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // 描画のデバイス コンテキスト

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // クライアントの四角形領域内の中央
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // アイコンの描画
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialogEx::OnPaint();
  }
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CtanukibookviewerDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}



void CtanukibookviewerDlg::OnBnClickedOk()
{
  CString csa;
  m_richEditCtrlKifu.GetWindowText(csa);

  std::string tempFilePath = "temp.csa";
  std::ofstream ofs(tempFilePath);
  if (!ofs.is_open()) {
    ::AfxMessageBox("一時ファイルの作成に失敗しました");
    return;
  }
  if (!(ofs << std::string(csa))) {
    ::AfxMessageBox("一時ファイルへの書き込みに失敗しました");
    return;
  }
  ofs.close();

  GameRecord gameRecord;
  if (!csa::readCsa(tempFilePath, gameRecord)) {
    ::AfxMessageBox("一時ファイルの読み込みに失敗しました");
    return;
  }

  Position position;
  setPosition(position, "startpos moves");
  std::list<StateInfo> stateInfos;
  for (const auto& move : gameRecord.moves) {
    stateInfos.push_back(StateInfo());
    position.doMove(move, stateInfos.back());
  }

  Book book;
  std::vector<std::pair<Move, int> > bookMoves = book.enumerateMoves(position, "../bin/book.bin");

  if (bookMoves.empty()) {
    m_richEditCtrlBook.SetWindowText("定跡データベースにヒットしませんでした");
    return;
  }

  int sumOfCount = 0;
  for (const auto& move : bookMoves) {
    sumOfCount += move.second;
  }
  std::string output;
  const std::vector<std::string> columnStrings = { "１", "２", "３", "４", "５", "６", "７", "８", "９", };
  const std::vector<std::string> rowStrings = { "一", "二", "三", "四", "五", "六", "七", "八", "九", };
  const std::vector<std::string> pieceTypeStrings = {
    "", "歩", "香", "桂", "銀", "角", "飛", "金", "玉",
    "と", "成香", "成桂", "成銀", "馬", "龍", };

  for (const auto& move : bookMoves) {
    char buffer[1024];
    int column = move.first.to() / 9;
    std::string columnString = columnStrings[column];
    int row = move.first.to() % 9;
    std::string rowString = rowStrings[row];
    PieceType pieceType = move.first.pieceTypeTo();
    std::string pieceTypeString = pieceTypeStrings[pieceType];
    sprintf_s(buffer, "%s%s%s %7d (%5.1f%%)\n",
      columnString.c_str(), rowString.c_str(), pieceTypeString.c_str(), move.second, 100.0 * move.second / sumOfCount);
    output += buffer;
  }
  m_richEditCtrlBook.SetWindowText(output.c_str());
}
