
!> PMlib Fortran インタフェイス一般の注意
!!
!! @note  PMlib Fortran インタフェイスでは引数を省略する事はできない。
!!			またFortran とC++のデータタイプの違いなどにより、
!!			引数仕様が異なるものがある
!! @note  Fortranコンパイラは character文字列fc に対してその長さ（文字数）
!!   		を示す引数を内部で自動的に追加する。
!! 			例えばfcがcharacter*(fc_size) fc と定義されている場合、
!!			call f_pm_api (fc)というインタフェイスに対して実際に生成される
!!			コードは右のCコードに相当する。 void f_pm_api_ 
!!			(char* fc, int fc_size)
!! @note  呼び出すFortranプログラムからこの追加引数fc_sizeを意識する必要はない。
!!
program f_fortran_api_general
end program



!> PMlib Fortran インタフェイスの初期化
!!
!! @param[in] integer nWatch 最初に確保する測定区間数。
!!
!! @note   測定区間数が不明、あるいは動的に増加する場合は
!!			nWatch の値を1 と指定してよい。指定した測定区間数では不足に
!!			なった時点でPMlib は必要な区間数を動的に増加させる。
!!
subroutine f_pm_initialize (nWatch)
end subroutine


!> PMlib Fortran 測定区間にプロパティを設定.
!!
!!   @param[in] character*(*) fc	測定区間を識別するラベル文字列。
!!   @param[in] integer f_type  測定対象タイプ(0:COMM:通信, 1:CALC:計算)
!!   @param[in] integer f_exclusive 排他測定フラグ(0:false, 1:true)
!!
!!   @note ラベルは測定区間を識別するために用いる。
!!
subroutine f_pm_setproperties (fc, f_type, f_exclusive)
end subroutine


!> PMlib Fortran 測定区間のスタート
!!
!!   @param[in] character*(*) fc	測定区間を識別するラベル文字列。
!!
!!   @note  fc引数について subroutine f_pm_setproperties のnoteを参照
!!
subroutine f_pm_start (fc)
end subroutine


!> PMlib Fortran 測定区間のストップ
!!
!!   @param[in] character*(*)	fc	測定区間を識別するラベル文字列。
!!   @param[in] real(kind=8)	fpt	 計算量。演算量(Flop)または通信量(Byte)
!!   @param[in] integer			tic  計算量に乗じる係数。測定区間を複数回
!!									実行する場合の繰り返し数と同じ意味合い。
!!
!!   @note  計算量または通信量をユーザが明示的に指定する場合は、
!!          そのボリュームは１区間１回あたりでfpt*ticとして算出される
!!   @note  出力レポートに表示される計算量はモードと引数の組み合わせで決める
!!	@verbatim
!!  計算量のモード
!!  (A) ユーザ申告モード
!!    - HWPC APIが利用できないシステムや環境変数HWPC_CHOOSERが指定
!!      されていないジョブでは自動的にユーザ申告モードで実行される。
!!    - ユーザ申告モードでは(1):setProperties() と(2):stop()への引数により
!!      出力内容が決定、HWPC詳細レポートは出力されない。
!!    - (1) ::setProperties(区間名, type, exclusive)の第2引数typeは
!!      測定量のタイプを指定する。計算(CALC)タイプか通信(COMM)タイプか
!!      の選択を行なう、ユーザ申告モードで有効な引数。
!!    - (2) ::stop (区間名, fPT, iC)の第2引数fPTは測定量。
!!      計算（浮動小数点演算）あるいは通信（MPI通信やメモリロードストア
!!      などデータ移動)の量を数値や式で与える。
!!
!!      setProperties()  stop()
!!      type引数         fP引数     基本・詳細レポート出力
!!      ---------------------------------------------------------
!!      CALC             指定あり   時間、fPT引数によるFlops
!!      COMM             指定あり   時間、fPT引数によるByte/s
!!      任意             指定なし   時間のみ
!!
!!  (B) HWPCによる自動算出モード
!!    - HWPC/PAPIが利用可能なプラットフォームで利用できる
!!    - 環境変数HWPC_CHOOSERの値によりユーザ申告値を用いるかPAPI情報を
!!      用いるかを切り替える。(FLOPS| BANDWIDTH| VECTOR| CACHE| CYCLE)
!!
!!  ユーザ申告モードかHWPC自動算出モードかは、内部的に下記表の組み合わせ
!!  で決定される。
!!
!!  環境変数     setProperties()の  stop()の
!!  HWPC_CHOOSER    type引数        fP引数       基本・詳細レポート出力      HWPCレポート出力
!!  ------------------------------------------------------------------------------------------
!!  NONE (無指定)   CALC            指定値       時間、fP引数によるFlops	 なし
!!  NONE (無指定)   COMM            指定値       時間、fP引数によるByte/s    なし
!!  FLOPS           無視            無視         時間、HWPC自動計測Flops     FLOPSに関連するHWPC統計情報
!!  VECTOR          無視            無視         時間、HWPC自動計測SIMD率    VECTORに関連するHWPC統計情報
!!  BANDWIDTH       無視            無視         時間、HWPC自動計測Byte/s    BANDWIDTHに関連するHWPC統計情報
!!  CACHE           無視            無視         時間、HWPC自動計測L1$,L2$   CACHEに関連するHWPC統計情報
!!	@endverbatim
!!
subroutine f_pm_stop (fc, fpt, tic)
end subroutine



!> PMlib Fortran 測定区間のリセット
!!
!!   @param[in] character*(*) fc	測定区間を識別するラベル文字列。
!!
!!   @note  fc引数について subroutine f_pm_setproperties のnoteを参照
!!
subroutine f_pm_reset (fc)
end subroutine


!> PMlib Fortran  全測定区間のリセット
!!
!!
subroutine f_pm_resetall ()
end subroutine


!> PMlib Fortran 測定結果の基本レポートを出力
!!
!!   @param[in] character*(*) fc	出力ファイル名(character文字列)
!!   @param[in] integer		psort	測定区間の表示順 (0/1)
!!							(0:経過時間順にソート後表示、1:登録順で表示)
!!
!!   @note  基本統計レポートでは非排他測定区間の情報も出力する 
!!
subroutine f_pm_print (fc, psort)
end subroutine


!> PMlib Fortran MPIランク別詳細レポートを出力
!!
!!   @param[in] character*(*) fc	出力ファイル名(character文字列)
!!   @param[in] integer		legend  HWPC 記号説明の表示 (0:なし、1:表示する)
!!   @param[in] integer		psort	測定区間の表示順 (0/1)
!!                       (0:経過時間順にソート後表示、1:登録順で表示)
!!
!!   @note  詳細レポートでは排他測定区間の情報のみを出力する 
!!   @note  環境変数HWPC_CHOOSERの指定があれば、HWPC詳細レポートも出力する 
!!
subroutine f_pm_printdetail (fc, legend, psort)
end subroutine


!> PMlib Fortran プロセスグループ単位でのMPIランク別詳細レポートを出力
!!
!!   @param[in] character*(*)	fc	出力ファイル名(character文字列)
!!   @param[in] integer		p_group  MPI_Group型groupのgroup handle
!!   @param[in] integer		p_comm   MPI_Comm型groupに対応するcommunicator
!!   @param[in] integer配列	pp_ranks(*)	 groupを構成するrank番号配列
!!   @param[in] integer		group    プロセスグループ番号
!!   @param[in] integer		legend   HWPC 記号説明の表示 (0:なし、1:表示する)
!!   @param[in] integer		psort	測定区間の表示順 (0/1)
!!							(0:経過時間順にソート後表示、1:登録順で表示)
!!
!!   @note  HWPCを測定した計集結果があればそれも出力する
!!   @note  MPI_Group, MPI_Comm型は呼び出すFortranでは integer 型である
!!
subroutine f_pm_printgroup (fc, p_group, p_comm, pp_ranks, group, legend, psort)
end subroutine


!> PMlib Fortran MPI_Comm_splitで作成するグループ毎にMPIランク詳細レポートを出力
!!
!!   @param[in] character*(*)	fc	出力ファイル名(character文字列)
!!   @param[in] integer		new_comm   MPI communicator
!!   @param[in] integer		icolor    MPI_Comm_split()のカラー変数
!!   @param[in] integer		key    MPI_Comm_split()のkey変数
!!   @param[in] integer		legend   HWPC 記号説明の表示 (0:なし、1:表示する)
!!   @param[in] integer		psort	測定区間の表示順 (0/1)
!!							(0:経過時間順にソート後表示、1:登録順で表示)
!!
!!   @note  HWPCを測定した計集結果があればそれも出力する
!!   @note  MPI_Group, MPI_Comm型は呼び出すFortranでは integer 型である
!!
subroutine f_pm_printcomm (fc, new_comm, icolor, key, legend, psort)
end subroutine


!> PMlib Fortran 測定途中経過レポートを出力（排他測定区間を対象とする）
!!
!!   @param[in] character*(*) fc	出力ファイル名(character文字列)
!!   @param[in] character*(*) comments	任意のコメント(character文字列)
!!   @param[in] integer		psort	測定区間の表示順 (0/1)
!!							(0:経過時間順にソート後表示、1:登録順で表示)
!!
subroutine f_pm_printprogress (fc, comments, psort)
end subroutine


!> PMlib Fortran ポスト処理用traceファイルの出力
!!
!!   @note  プログラム実行中一回のみポスト処理用traceファイルを出力できる
!!      現在サポートしているフォーマットは OTF(Open Trace Format) v1.1
!!
subroutine f_pm_posttrace ()
end subroutine


!> PMlib Fortran
!! 全プロセスの全測定結果情報をマスタープロセスに集約
!!
!!   @note  旧バージョンとの互換維持用。
!!			利用者は通常このAPIを呼び出す必要はない。
!!
subroutine f_pm_gather ()
end subroutine

