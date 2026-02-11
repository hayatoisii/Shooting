#include <KamataEngine.h>
#include "GaneScene.h"
#include "TextureConverter.h"
#include <filesystem>

using namespace KamataEngine;
namespace fs = std::filesystem;

// Resourcesフォルダ内の全PNGをDDSに変換する関数
void ConvertAllTextures() {
	TextureConverter converter;

	// "Resources" フォルダの中にあるファイルを全部チェックする
	// ※もしフォルダ名が違う場合は "Resources" を書き換えてください
	std::string directoryPath = "Resources";

	// フォルダが存在するか確認
	if (!fs::exists(directoryPath)) {
		return;
	}

	// フォルダ内のファイルを1つずつ取り出すループ
	for (const auto& entry : fs::directory_iterator(directoryPath)) {
		// ファイルのパスを取得
		std::string filePath = entry.path().string();

		// 拡張子が ".png" のファイルだけを探す
		if (entry.path().extension() == ".png") {

			// 対応する .dds ファイルの名前を作る
			std::string ddsPath = filePath;
			ddsPath.replace(ddsPath.find(".png"), 4, ".dds");

			// ★工夫ポイント：
			// 「まだDDSがない」または「PNGの方が新しい（更新された）」場合だけ変換する
			// これをしないと、毎回全変換が走って起動が遅くなります
			if (!fs::exists(ddsPath) || fs::last_write_time(entry.path()) > fs::last_write_time(ddsPath)) {

				// 変換実行！
				converter.ConvertTextureWICToDDS(filePath);

				// ログ出力（Outputウィンドウで確認用）
				OutputDebugStringA(("Converted: " + filePath + "\n").c_str());
			}
		}
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	// ゲームウィンドウが出る前に、ここで全画像をチェックして変換してしまいます
	ConvertAllTextures();
	
	WinApp* win = nullptr;
	DirectXCommon* dxCommon = nullptr;
	// 汎用機能
	Input* input = nullptr;
	Audio* audio = nullptr;
	AxisIndicator* axisIndicator = nullptr;
	PrimitiveDrawer* primitiveDrawer = nullptr;
	GameScene* gameScene = nullptr;

	// ゲームウィンドウの作成
	win = WinApp::GetInstance();
	win->CreateGameWindow(L"StarShooting");

	// DirectX初期化処理
	dxCommon = DirectXCommon::GetInstance();
	dxCommon->Initialize(win);

#pragma region 汎用機能初期化
	// ImGuiの初期化
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();
	imguiManager->Initialize(win, dxCommon);

	// 入力の初期化
	input = Input::GetInstance();
	input->Initialize();

	// オーディオの初期化
	audio = Audio::GetInstance();
	audio->Initialize();

	// テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon->GetDevice());
	TextureManager::Load("white1x1.png");

	// スプライト静的初期化
	KamataEngine::Sprite::StaticInitialize(dxCommon->GetDevice(), WinApp::kWindowWidth, WinApp::kWindowHeight);

	// 3Dモデル静的初期化
	Model::StaticInitialize();

	// 軸方向表示初期化
	axisIndicator = AxisIndicator::GetInstance();
	axisIndicator->Initialize();

	primitiveDrawer = PrimitiveDrawer::GetInstance();
	primitiveDrawer->Initialize();
#pragma endregion

	// ゲームシーンの初期化
	gameScene = new GameScene();
	gameScene->Initialize();

	// メインループ
	while (true) {
		// メッセージ処理
		if (win->ProcessMessage()) {
			break;
		}

		// ImGui受付開始
		imguiManager->Begin();
		// 入力関連の毎フレーム処理
		input->Update();
		// ゲームシーンの毎フレーム処理
		gameScene->Update();
		// 軸表示の更新
		axisIndicator->Update();
		// ImGui受付終了
		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();
		// ゲームシーンの描画
		gameScene->Draw();
		// 軸表示の描画
		axisIndicator->Draw();
		// プリミティブ描画のリセット
		primitiveDrawer->Reset();
		// ImGui描画
		imguiManager->Draw();
		// 描画終了
		dxCommon->PostDraw();
	}
	delete gameScene;
	// 3Dモデル解放
	Model::StaticFinalize();
	audio->Finalize();
	// ImGui解放
	imguiManager->Finalize();

	// ゲームウィンドウの破棄
	win->TerminateGameWindow();

	return 0;
}