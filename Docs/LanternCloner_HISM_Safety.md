# LanternCloner HISMロード安全性修正

対象: Unreal Engine 5.4.4 (35576357)、2026-09-09。

## 原因と発生タイミング

直接の停止原因は、非同期Tree Buildの結果に含まれるInstanceReorderTable件数と、反映先HISMのPerInstanceSMData件数の不一致。

保存済みクラッシュ `UECC-Windows-1BAE4C024E457D7B442A7D87EE0F5B80_0000` では、07:01:49.320 UTCにTest LevelのMAP LOAD開始、同じ543フレームの07:01:49.423にAssertionが発生している。

CrashContextのEngine DLL相対アドレス `0x13a14ee` と `0x13a1b7e` を、インストール済み5.4.4 DLLのexportsと照合した。前者は `ApplyBuildTree` (開始0x13a1490)、後者は `ApplyBuildTreeAsync` (開始0x13a19f0) に属する。従って非同期結果のゲームスレッドへの反映時のAssertionである。

旧ALanternClonerはOnConstructionから無条件に再構築（bGenerationEnabledがfalseならClear）し、タグ付きHISMへClearInstances、DestroyComponent、同じ固定名でNewObject、1件ずつAddInstanceを行っていた。HISMのbAutoRebuildTreeOnInstanceChangesは既定のtrueのままで、途中から非同期Buildが開始され得る。

5.4.4のHISMはロード時のOnPostLoadPerInstanceDataでもBuildTreeIfOutdatedを呼ぶ。ロード後のOnConstructionが保存済みComponentを破棄・再作成する設計が、このロード時Buildと競合する経路だった。固定名での再作成によるオブジェクト再初期化まで含めた個別のInstance数の遷移は、旧バイナリのクラッシュログに記録がないため未断定。Assertionの実行場所・非同期反映・旧更新経路・修正後のロード時コール順は確認済み。

## イベント調査

| イベント | 旧実装 | 修正後 |
| --- | --- | --- |
| OnConstruction | RebuildInstances または破棄を伴うClear | ログのみ。Instance更新なし |
| PostLoad | 独自overrideなし | Superと診断ログのみ |
| PostRegisterAllComponents | 独自overrideなし | Superと診断ログのみ |
| PostEditChangeProperty | 独自overrideなし。基底のConstruction再実行経由で生成し得る | Superとログ。自動生成なし |
| PostEditMove | 独自overrideなし。基底のConstruction再実行経由で生成し得る | Superとログ。自動生成なし |
| BeginPlay | 独自overrideなし | 独自overrideなし。生成を追加していない |
| Generate / Regenerate | それぞれRebuildInstancesを直接呼ぶ | 共通の明示更新処理を呼ぶ |

実EditorでのTest Levelロード時は `PostLoad → PostRegisterAllComponents → OnConstruction` を記録した。BlueprintのEventGraphは未接続の既定イベントであり、アセット内にGenerate / Regenerate / Clearの呼び出しノード参照は見つからなかった。Blueprintは変更していない。

## 更新方式

- HISMを継続使用。ISMへの置換・Engine本体改変なし。
- タグ付きComponentをMesh・Materialで再利用。Clear時にもDestroyComponentを呼ばない。
- 新しい組み合わせに限り、一意名のInstance Componentを作る。
- 全ComponentのAsync Build・Meshコンパイル・ロード状態を先に確認。処理中なら全更新を見送り、ログで再操作を案内する。部分的なClearや自動リトライ予約はしない。
- ゲームスレッド以外、CDO／テンプレート、Construction中、ロード中、同じActorの更新処理への再入を拒否する。
- 自動Tree Buildを停止してClearInstancesを実行。UE 5.4のClearInstancesはReorderTableを直接空にしないため、空データに対する同期 `BuildTreeIfOutdated(false, true)` を実行する。
- 配置をグループごとに収集し、AddInstancesで一括追加後、同期 `BuildTreeIfOutdated(false, true)` を実行する。その後、新規ComponentのみRegisterする。
- Full Generateは明示的なGenerate／Regenerate操作だけ。ロード、Details変更、移動からは生成しない。
- Previewボタンを追加。既定64灯、設定範囲1～256灯。全列・奥行き・高さにまたがる候補数を制限するので、大きな設定や低Densityでも全配置を走査しない。従来のPreviewVerticalCountはPreviewボタンだけに適用する。
- 生成統計を保存し、ロードだけで統計が0へ戻る問題も回避。
- IsGeneratedDataConsistentは読み取り専用診断。Async／Compilation／Tree状態とReorderTable・NumBuiltInstances・Instance数を検査する。

## 変更対象

- `Source/amharon_lantan/Public/LanternCloner.h`
- `Source/amharon_lantan/Private/LanternCloner.cpp`
- `Content/LanternCloner/Maps/L_LanternCloner_Test.umap`（既存Actor・既存HISMを再利用して生成・保存）
- `Scripts/LanternCloner/verify_editor.py`（実Editorで実行する回帰検証）

Main Level、Camera／Sequence／MRQ、既存ランタンMesh／Material、BP_LanternCloner、ConfigのContent外設定は修正対象外。作業前のContent／ConfigのSHA-256一覧と比較して確認する。既存のgit変更はそのまま保持する。

## 検証方法

UnrealEditor.exe本体を新しいプロセスで起動。NullRHI／Commandletではなく、通常のEditorと描画系を動かし、Slateのtick間隔を空けてEditorのLevel操作APIとCallInEditorと同じUFunctionを実行する。UIボタンの座標クリックによる検証ではない。

`Saved/LanternClonerSafety/phase.txt` に `sequence` を設定して `py Q:/Scripts/LanternCloner/verify_editor.py` をExecCmdsで実行する。sequenceは開く・閉じる・再オープン・Clear・Generate・Regenerate・Details変更・Preview上限・元設定へ復帰・Test Level保存・Editor終了を行う。完全終了後、phaseを `restart` にして新しいEditorプロセスで再実行すると、保存配置と統計の復元、再開閉を検証して終了する。

テストログ、結果JSON、保存配置のハッシュ、作業前バックアップ、Asset差分は `Saved/LanternClonerSafety/` に格納する。スクリプトは本番Main Levelを保存しない。Preview検査の一時設定10×10×100は保存前に元の4×3×15へ戻す。

日本語パスでMSVCのPCH出力先が文字化けしたため、この作業では同じプロジェクトをsubstのQ:経由でビルドした。古いパスを保持するMakefileを使わないため `-NoUBTMakefiles` を指定した。ソースやAssetの別プロジェクトへのコピーは行っていない。

検証準備中に、Pythonから非公開Engineプロパティを読む検査をC++の読み取り専用診断へ変更した。また、Editorの保存中にSlate tickが再入したため検証スクリプト側にもbusyガードを追加し、保存先をTest Levelに限定したSaveMapへ変更した。これらは検査手順上の失敗であり、修正後のHISM Assertion再発ではない。

## 最終結果

最終検証はPASS。sequenceはPID 4480、保存後のrestartは別プロセスPID 19000で実行し、2026-09-09 16:42:26 JSTにEditorの正常終了を確認した。

要求された10手順（完全再起動、Test Levelロード、閉じる、再ロード、Clear、Generate、Regenerate、保存、完全再起動、再ロード）を実Editor API経由ですべて確認。154灯・462 Mesh Instance・22 HISMを維持し、再起動後も配置ハッシュと統計が一致した。Details変更時の自動生成なし、10×10×100設定でPreviewが64灯・192 Instanceに収まることもPASS。

`results_sequence.json` と `results_restart.json` の最終stepはともにPASS。Content／Configの作業前後ハッシュ差分はTest Levelのみ。Blueprint、Main、既存ランタンAssetは変更していない。

今回のTest Levelロード・生成の検証範囲ではStep3へ進める。Step3の作業自体は未実施。ユーザーの停止指示に従い、この時点で作業を終了した。ビルドに使ったQ:のsubst参照は残している（元プロジェクトへの別名で、ファイルコピーではない）。

## 参照

根拠はインストール済みUE 5.4.4の `HierarchicalInstancedStaticMesh.cpp`（ClearInstances、BuildTreeIfOutdated、ApplyBuildTreeAsync、OnPostLoadPerInstanceData）およびプロジェクト内クラッシュログ。

[Epic UE-68582](https://issues.unrealengine.com/issue/UE-68582) にも連続AddInstance時の自動Tree Build停止という過去の回避策が記載されているが、これは旧バージョンの別Assertionであり、今回の5.4.4の直接原因の証明としては扱っていない。
