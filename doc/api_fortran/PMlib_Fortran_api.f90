
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


!> PMlib Fortran 測定区間とそのプロパティを設定
!!
!!   @param[in] character*(*) fc	測定区間に与える名前のラベル文字列
!!   @param[in] integer f_type  測定対象タイプ(0:COMM:通信, 1:CALC:計算)
!!   @param[in] integer f_exclusive 排他測定フラグ(0:false, 1:true)
!!
!!   @note ラベル文字列は測定区間を識別するために用いる。
!!   各ラベル毎に対応した区間番号を内部で自動生成する
!!   @ 最初に確保した区間数init_nWatchが不足したら動的に追加する
    ///   第１引数は必須。第２引数は明示的な自己申告モードの場合に必須。
    ///   第３引数は省略可

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
!!   @note  引数はユーザ申告モードの場合にのみ利用される。 \n
!!   @note  測定区間の計算量は次のように算出される。 \n
!!          (A) ユーザ申告モードの場合は １区間１回あたりで flopPerTask*iterationCount \n
!!          (B) HWPCによる自動算出モードの場合は引数とは関係なくHWPC内部値を利用\n
!!   @note  HWPC APIが利用できないシステムや環境変数HWPC_CHOOSERが指定
!!          されていないジョブでは自動的にユーザ申告モードで実行される。\n
!!   @note  出力レポートに表示される計算量は測定のモード・引数の
!!          組み合わせで以下の規則により決定される。 \n
!!   @verbatim
!!
!!    (A) ユーザ申告モード
!!      - ユーザ申告モードでは(1)setProperties() と(2)stop()への引数により出力内容が決定される。
!!        (1) setProperties(区間名, type, exclusive)の第2引数typeが計算量のタイプを指定する。
!!        (2) stop (区間名, fPT, iC)の第2引数fPTは計算量（浮動小数点演算、データ移動)を指定する。
!!      - ユーザ申告モードで 計算量の引数が省略された場合は時間のみレポート出力する。
!!
!!    (B) HWPCによる自動算出モード
!!      - HWPC/PAPIが利用可能なプラットフォームで利用できる
!!      - 環境変数HWPC_CHOOSERの値により測定情報を選択する。(FLOPS| BANDWIDTH| VECTOR| CACHE| CYCLE)
!!
!!    ユーザ申告モードかHWPC自動算出モードかは、内部的に下記表の組み合わせで決定される。
!!
!!    環境変数     setProperties()  stop()
!!    HWPC_CHOOSER   type引数      fP引数      基本・詳細レポート出力      HWPC詳細レポート出力
!!    -----------------------------------------------------------------------------------------
!!    NONE (無指定)   CALC        指定値      時間、fP引数によるFlops     なし
!!    NONE (無指定)   COMM        指定値      時間、fP引数によるByte/s    なし
!!    FLOPS           無視         無視       時間、HWPC自動計測Flops     FLOPSに関連するHWPC統計情報
!!    VECTOR          無視         無視       時間、HWPC自動計測SIMD率    VECTORに関連するHWPC統計情報
!!    BANDWIDTH       無視         無視       時間、HWPC自動計測Byte/s    BANDWIDTHに関連するHWPC統計情報
!!    CACHE           無視         無視       時間、HWPC自動計測L1$,L2$   CACHEに関連するHWPC統計情報
!!    CYCLE           無視         無視       時間、HWPC自動計測cycle     CYCLEに関連するHWPC統計情報
!!
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


!> PMlib Fortran 全プロセスの測定結果、全スレッドの測定結果を集約
!! 
!!  @note  以下の処理を行う。
!!        各測定区間の全プロセスの測定結果情報をマスタープロセスに集約。
!!        測定結果の平均値・標準偏差などの基礎的な統計計算。
!!        経過時間でソートした測定区間のリストm_order[m_nWatch] を作成する。
!!        各測定区間のHWPCイベントの統計値を取得する。
!!        各プロセスの全スレッド測定結果をマスタースレッドに集約
!!  @note \n
!!      OpenMPの並列タイプが
!!      測定区間 start/stopの間でdo all/for all タイプの並列スレッドを発生するような
!!      一般的なOpenMPスレッド並列処理であれば、
!!      PMlib内部で必要な時にこのAPIが自動的に実行されるため、
!!      利用者がこのAPIを呼び出す必要はない。
!!  @note \n
!!      OpenMP parallel region 内の個別スレッドが測定区間をstart/stopする
!!      ようなPosix スレッド的な並列処理では、
!!      全スレッドの測定結果を集約するためにこのAPIを呼び出す必要がある。
!!      このようなPosix スレッド的な並列処理モードは現在はC++プログラムだけサポートしている。
!! 
subroutine f_pm_gather ()
end subroutine


!> PMlib Fortran   OpenMP parallel region内のマージ処理
!!
!!  @note \n
!!  OpenMPスレッド並列処理された測定区間のうち、 parallel regionの内側から
!!  区間を測定した場合、（測定区間の外側にparallel 構文がある場合）
!!  に限って呼び出しが必要な関数。
!!  parallel region内で呼び出された全測定区間のスレッド情報をマスタースレッドに集約する。
!!  parallel regionが全て測定区間の内側にある場合は呼び出し不要。
!!
subroutine f_pm_mergethreads ()
end subroutine



!> PMlib Fortran 測定結果の基本レポートを出力
!!
!!   @param[in] character*(*) fc	出力ファイル名(character文字列)
!!   @param[in] character*(*) fh 	ホスト名 (character文字列. ""の場合はrank 0 実行ホスト名を表示)
!!   @param[in] character*(*) fcmt 	任意のコメント (character文字列)
!!   @param[in] integer		psort	測定区間の表示順 (0:経過時間順に表示、1:登録順)
!!
!!   @note  基本レポートでは非排他測定区間の情報も出力する 
!!
subroutine f_pm_print (fc, fh, fcmt, psort)
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


!> PMlib Fortran 指定するプロセスのスレッド別詳細レポートを出力
!!
!!   @param[in] character*(*) fc	出力ファイル名(character文字列)
!!   @param[in] integer    rank_ID    指定するプロセスのランク番号
!!   @param[in] integer    psort 測定区間の表示順(0:経過時間順にソート、1:登録順で表示)
!!
subroutine f_pm_printthreads (fc, rank_ID, psort)
end subroutine


!> PMlib Fortran HWPC 記号の説明表示を出力
!!
!!   @param[in] character*(*) fc	出力ファイル名(character文字列)
!!
subroutine f_pm_printlegend (fc)
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


