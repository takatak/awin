README-JP-UTF8.txt  日本語版README

awin に興味を持って頂きありがとうございます。


【ライセンス】

awin は GPL に従って配布されます。詳しくは、COPYING ファイルをご覧ください。
(ソースを参照した wmctrl が GPL だったので、GPL にしました)

【このプログラムで出来る事の概要】

1．ウィンドウタイトルに表示されている文字をキーワードにして、該当するウィンドウをアクティブ化する

2．該当するウィンドウが存在しない場合に実行するコマンドを指定できる
   これにより、該当するウィンドウが存在しない、つまりは希望するプログラムが起動していないので起動する
   という事ができるようになります

3．毎回ターミナルから実行するのは面倒ですから、任意のショートカットキーが押されたら実行するように
   指定しておくと便利です。そういう用途のためのプログラムです。
   lxde を使っている場合、
     ~/.config/openbox/lxde-rc.xml
   に登録して使うことができます。
   これにより今までプログラムの起動として使っていたキー・コンビネーションがプログラムの選択にも
   使えるようになります。
   KDE なら、カスタム・ショートカットに登録すれば良いでしょう。
   ランチャー・プログラムに登録するのも良い考えです。

【実行例】

1．ウィンドウの一覧を表示させる
      awin -l
       または
      awin
      ( -l は小文字 )

2．キーワード("easel")にマッチしたウィンドウタイトルをもつウィンドウの一覧を表示させる
      awin -L easel
      ( -L は大文字 )

   小文字の -l の後にキーワードを指定しても無視されます。
   キーワード指定して一覧を表示させる場合はこちらを使用してください。

3．キーワード("easel")にマッチしたウィンドウタイトルをもつウィンドウの一覧を表示させる
   指定したキーワードも表示させる
      awin -L easel -s
      ( -L は大文字 、 -s は小文字 )

4．キーワード("Iceweasel")の一覧を表示させ、最初のものをアクティブ化する
   ただし、別のワークスペースで表示されていたら、そのワークスペースに切り替える
      awin -L Iceweasel -a
      ( -L は大文字 、 -a は小文字 )

5．キーワード("Iceweasel"  webブラウザの一種)の一覧を表示させ、最初のものをアクティブ化する
   ただし、別のワークスペースで表示されていたら、そのワークスペースから今のワークスペースに移動させる
      awin -L Iceweasel -A
      ( -L は大文字 、 -A は大文字 )

6．キーワード("Iceweasel"  webブラウザの一種)の一覧を表示させ、最初のものをアクティブ化する
   もし実行されていなかったら実行する
      awin -L Iceweasel -a -c /usr/bin/iceweasel
       または
      awin -L Iceweasel -A -c /usr/bin/iceweasel
      ( -L は大文字 、 -c は小文字 、-a と -A の違いは4. / 5.を参照)
    注:安全を考えて、Ver2.1からは、-c オプションはフルパス指定に制限しました。
       これにより、実害は少ないと思いますが、起動させるコマンドとして /bin/sh の内部コマンドを指定することは出来なくなりました。
       また、指定されたコマンドが実行可能ファイルとして存在するか否かの簡易チェックを行うようにしました。
       簡易チェックの制約事項として、ディレクトリ名やコマンド名に空白文字が含まれていると、チェックが正常に行われない可能性があります。
       この簡易チェックを行わないようにするには、-c オプションの代わりに -C オプションを指定してください。
       -c オプションで指定された内容をしっかりパースする事ができれば良いのですが、私の技術力では難しいので、簡易版となりました。
       このチェエックは、実際にコマンドを実行する必要がある時のみ行われます。既に稼働しているウィンドウをアクティベートするだけの場合は
       チェックされません。でも、空白を含むコマンドで更にパラメータを渡そうとすると、上手く動作しません。

7. プロセスの一覧を表示させる
      awin -p
      ( -p は小文字 )

8. キーワード("easel")にマッチしたcmdlineを持つプロセスの一覧を表示させる
   見つかって、それと同じPIDを持つウィンドウがあればそれを、最初のものをアクティブ化する
   見つからなければ、iceweaselを起動する
      awin -P easel -a -c /usr/bin/iceweasel
       または
      awin -P easel -A -c /usr/bin/iceweasel
      ( -P は大文字 、 -c は小文字 、-a と -A の違いは4. / 5.を参照)

9.ウィンドウの一覧を表示させる。ソート順は、PID>title>class それぞれ昇順とする。
      awin -l -o 1,a,4,a,3,a

10.xeyes を常に最前面に表示させる。起動していなければ右下に100x100で起動する。
      awin -w -L xeyes.XEyes -a -c "/usr/bin/xeyes -geometry 100x100-0-0" -S a,ABOVE

オプションについては、大文字/小文字は区別されます。make し make install した後で
  awin --help
を実行すると、簡単なヘルプが表示できます。
  man awin
を実行すると、より詳細な使い方が表示されます。

ウィンドウの一覧を表示させた場合は次が表示されます。

第1カラム  : PID      プロセス毎に違います。通常は数値が小さい方が以前に起動したプロセスです
第2カラム  : machine  プログラムを実行しているマシン(SSHのX11 forwardingを利用していたりするとリモートマシンのものになったりします)
第3カラム  : class    wm_class(res_name.res_class)おおよそプログラム毎に違います（詳しくは聞かないでください）
第4カラム  : title    ウィンドウのタイトルに表示されている文字列です

このツールを使う上で重要なのは第4カラムの title と第3カラムの class です。
キーワードがマッチするか否かをチェックするのは、基本第4カラムの文字列が対象です。
-w が指定された場合は、第3カラムが対象となります。プログラムの種類で選びたい場合はこちらのほうが便利かも知れません。
-m が指定された場合は、第2カラムが対象となります。(用途があるのか、はわかりませんが...)


このツールでアクティブ化させたいプログラムがある場合、そのプログラムを起動させた状態で、"awin -l" を実行し
どのようなキーワードで title または class をチェックすればいいかを考え、
それを指定して "awin -L xxxx" (xxxx  がキーワード)などでキーワードの指定の仕方に問題がないか確認。
その上で、"awin -L xxxx -a"などを実行してみてアクティブになる事を確認、その上で、該当プログラムを終了させて
"awin -L xxxx -a -c /hoge/yyyy" (yyyy が起動するコマンド)などで起動させる事の確認がとれれば、任意のショートカットキーに登録する。
こういう流れを想定しています。


なお、キーワードには簡単な正規表現が使用可能ですので、ある程度柔軟に記述できます。
正規表現について分からず知りたい方はインターネットで検索して調べてください。
私は限定的にしか知りませんが、それでも役に立ちます。
(gcc の特定の版では正規表現にはバグがある、との指摘もあるようです)

-f が -a/-A と共に指定された場合、-L または -P で指定されて表示されたリストのすべてに対してアクティベートを試みます。
-f が指定された場合、-g は無視されます。
-g/-G で座標に負の値が指定された場合、その要素は現状のままとなります。


【このプログラムを使うには】

ダウンロードしたこのツールの圧縮ファイルを解凍した先のディレクトリ内で、
 1) cd build
 2) cmake ../
 3) make
 4) sudo make install  または sudo make install/strip
する必要があります。
(このバージョンからCMakeが必要になりました)

make するには、前提ソフトウェアが導入されている必要があります。
debian jessie の場合、「sudo apt-get install cmake libx11-dev」が必要になるかも知れません。

【免責事項】

このツールを用いて、不利益・不都合等が発生したとしても、私も含めた誰も何の保障も補填も致しません。
全ては自己責任においてご使用ください。


【注意事項】

1. ツールのメッセジやソース中のコメント等は英語のようなものにしています。
   英語として誤っている部分はありますが、雰囲気で察してください。
   英語を解する日本人なら出来るはず?です。


2. 指定するキーワードに空白文字が含まれる場合には、キーワード全体をダブルクォーテーションで
   囲むことで私は対応しています。
   また記号が含まれる場合、正規表現の形式として解釈される可能性がありますので、注意が必要です。


3. 起動させるもの (-c で指定するもの)によっては次のようなメッセージが出る場合があります。

------<<ここから>>------
GLib-CRITICAL **: g_slice_set_config: assertion 'sys_page_size == 0' failed
Gtk-Message: (for origin information, set GTK_DEBUG): failed to retrieve property
`GtkRange::activate-slider' of type `gboolean' from rc file value "((GString*) 0x7f6e58e0fb40)" of type `GString'
Gtk-Message: (for origin information, set GTK_DEBUG): failed to retrieve property `GtkRange::activate-slider' of
type `gboolean' from rc file value "((GString*) 0x7f6e58e0fb40)" of type `GString'
------<<ここまで>>------

   このようなものは、このツールから出しているものではありません。
   このツールはこんなに長く詳しい(難しい)メッセージは出力しません。
   これはターミナルから直接起動しても出てくるようなメッセージなので、このツールに
   起因するものではないようです。
   起動するときに標準エラー出力を抑制すれば表示されなくなると思いますが、
   出ても実害はない（みたい）だし、全然表示されないとマズイと思い、特に抑制していません。

   このような状態が気になるコマンドがあれば、末尾に
      awin -a Iceweasel -c "/usr/bin/iceweasel 2>/dev/null"
   のようにエラー出力を抑制する事も検討してみてください。
   これは、"iceweasel"コマンドを実行する時に、標準エラー出力をリダイレクトして
   画面に表示させないようにしています。
    (/dev/null のパーミッションが適切な事が前提です)
   ダブルクォーテーションで呼び出すコマンドごと囲まないと、awin を実行する時のエラー出力を抑制する事になりますので、注意してください。


4. パフォーマンスについて
   パフォーマンスを考えてC言語で作成し、複雑な処理はしていないプログラムです。また、ある程度はパフォーマンスを考慮し記述しています。
   その為、余りに処理が遅いという事はないと思いますが、ご不満を感じさせる事があるかとも思います。その際はご容赦ください。
   気になるようであれば、Cソースを改編し make しなおしたり、CMakeLists.txt を改編し cmake　../ からやり直す、H/Wを変えるなどで対応を検討ください。
   awin を使わないというのも、作者としては悲しいですが、良い選択です。


5. 相性の悪いアプリがあるようです。
   Xアプリの中には PID をセットしないものがあり、それをアクティブ化する事については制限がつきます。
   そのようなアプリは -L を用いてのアクティブ化は大丈夫ですが、-P を用いてのアクティブ化は出来ません。
   Xでは、PID をセットする／しないは、アプリに委ねられているようです。自由ですね。
   そのため、全てのXアプリで PID が得られる訳ではありません。
   PID が得られなかった場合、このプログラムは 0 として処理を続行します。
   一方、-p/-P オプションで得られる PID は、OS が確実にセットしたものとなります。そのため、両者は不一致の状態に陥ります。
   -P オプションを指定した場合、該当の PID を持つウィンドウをアクティブ化しようとするために、見つからずにアクティブ化できません。

   そのようなアプリに何があるのかは分かりませんが、mlterm(ver.3.3.8) や xpdf(ver.3.03) はそういうアプリのようです。

   -l オプションで PID がゼロになってしまう場合、ターミナルから「xprop _NET_WM_PID」を実行し、該当する
   ウィンドウをクリックすることで、セットされているか否かが分かります。
      「_NET_WM_PID:  not found.」
   などとでるようであれば、セットされていません。セットされていれば、
      「_NET_WM_PID(CARDINAL) = 3151」
   のようにセットされているPID(この例ですと 3151 )が表示されます。

   またアプリがセットするということは、意図は分かりませんが、OS が認識しているのと異なる PID がセットされる可能性もあるかも知れません。
   そういう場合も、-P オプションからのアクティブ化は正しく動作しないと思われます。
   アクティブ化されない、あるいは意図しないウィンドウがアクティブ化される、などの状況になるでしょう。

   awin では、_NET_WM_PID がセットされていない場合に、別の方法で PID を取得する、という対処は難しいと考えています。
   保証されないから使わない!というのではなく、使えればラッキー!!!というスタンスです。

   該当するアプリが見つかった場合、メンテナンナーに連絡をとり、_NET_WM_PID をセットするよう要請するのが、もっとも望ましい
   対処方法と思います。
   では、私は mlterm / xpdf に関して何かアプローチをしたかと問われると、恥ずかしい限りです。

   PIDばかりではありませんでした。
   class もセットされない場合があるようです。この場合、-w オプションは使えません。Haroopad v0.13.1 はセットしていないようです。
   machine もセットされない場合があるようです。この場合、-m オプションは使えません。VirtualBox 5.1.12r112440 はセットしていないようです。
   title も同様に、セットしないアプリもあるはずです。


6. ソート機能はC言語標準の qsort を利用しています。
   そのため、キーの大小評価として同じとなった場合、元の順序は必ずしも維持されません。
   例えば、同じウィンドウタイトルを持つ複数のプログラムを起動していて、それらの複数の
   ウィンドウタイトルがマッチするようなキーワードを指定してた上でソートさせた場合、
   必ずし期待した順になる、というわけではありません。
   複数のソート条件を指定して、希望するソート順になるよう工夫してみてください。
   マージソート等のアルゴリズムで実装すればベターなのでしょうが、そこまでの必要性を
   感じていないので未対応です。


7. コマンド起動時のウィンドウ位置を指定する機能(-G)の実装は簡易です。
   本来、起動時のウィンドウ表示位置を決めるのは、アプリケーション側の機能であるべきと考えます。
   しかし、サイズは指定できても、位置は指定できないアプリがあるようです。lxterminal (0.2.0)はそうでした(?多分)。
   アプリで指定できない場合は、おそらくは、ウィンドウ・マネージャが担うのだと思いますが、一律に制御される可能性があり、それでは困る場合もあるだろうと考え、
   起動時もウィンドウ位置を指定できるようにしました。しかし、起動させた後、すぐにウィンドウ位置を制御できるようになるわけではなく、
   少し待たなければならないようなのですが、これはPC環境や起動されるアプリによって異なり、一様ではありません。
   一定のマイクロ秒だけ待ってから制御を試みるようにしました。 
   待ち時間内にXウィンドウの情報として取得可能にならなかった場合、一定回数取得を繰り返します。
   取得可能にならなければ、制御を試みることもできないため awin によっては移動されません。
   この待つ時間は、-T オプションによる指定で変更可能です。安全を考えれば、長く待てばいいのですが、長すぎると待たされ使いづらいものになります。
   取得を繰り返す回数は -r オプション指定で変更可能です。-L や -c/-C　オプションの記述に間違いがない事が確信できた場合、この回数を増やしてやると
   待ち時間をそれほど長くしなくても、ウィンドウ位置が変更される確率は高くなります。
   ご自身の使用環境や対象アプリの起動所要時間等を考慮に入れて、使い勝手が良いような値を指定してください。
   固定の待ち時間(マイクロ秒)や繰り返す回数を変更したい場合は、awin.h 内を編集して make しなおす必要があります。
     固定の待ち時間  : WAIT_AFTER_EXEC_MICROSECOND ( 初期値 :  200000 )
     繰り返す回数    : MAX_RETRY_CNT_WINGET        ( 初期値 :  25     )


【作成経緯】

m-yamashita さんの書かれた Quickey.py (http://qiita.com/m-yamashita/items/8c9b22079fb375653eb2) を見て
便利そうだと思いました。

Quickey.py を使っても良かったのですが、Autokey は使わないつもりだったのです。非力な環境で使っていますので。
（Autokey に依存していないのは知っています）
直接 Quickey.py を使うには、python を指定して使用しなければならないというのも面倒で...
python 経由で起動しなかった時の挙動も焦るものでしたし...
いや、python が分からないというのも大きな理由です（魅力的な言語だとは思い、少し勉強してはみました）。

で、シェルスクリプトで使いそうな機能のみ実装してみようと思ったのですが、give up!

そこでシェルスクリプトでやろうとした事をC言語のプログラムとして作成してみたのがこのツールの最初です。
挫折の産物ですが、何とか使えるレベルにはなっていると思います。
python と Quickey.py と wmctrl(複数回?) を呼び出すより、このツールだけ呼び出す方が、処理負荷も I/O 負荷も軽いのではないかと思います。
[Ver 2.0 になってから wmctrl を呼び出すこともなくなりました(主要な機能のみを見様見真似で内包)ので、より軽く動作すると思います。]


【プログラム・ソースの簡単な説明】

以下のソース・ファイルで構成されています。

1. awin.c   main 関数を含むプログラム本体  プロセス用関数もここに記述されています。
   awin.h   awin.c のヘッダー・ファイル
2. mywin.c  X Window 関連用関数
   mywin.h  mywin.c のヘッダー・ファイル
3. mymem.c  メモリー管理用関数
   mymem.h  mymem.c のヘッダー・ファイル

次にそれぞれの.cファイル毎に簡単に説明します。改編される場合の一助になれば幸いです。

1.awin.c

   文字列配列を宣言するときの添字として、次の値が#defineされています。
      BUF_LEN 1024
   検索するとして指定した文字列の格納用、実行するコマンドの格納用、正規表現のコンパイルがエラーになった時の詳細取得用で使っています。

   グローバル変数として次が宣言されています。
      MySortConfig SortConfig[SORT_MAX];
   qsort で指定する比較関数の中で(厳密にはさらにそれから呼ばれる関数の中で)使用される変数で、
   複数のフィールドを指定したソート条件を記憶させるためのものです。

   int IsNumber()
       文字列が数値のみかどうかをチェックします。数値のみであれば桁数を返します。数値以外が含まれれば0を返します。

   int MyCompPList()
       qsort に指定する比較関数です。プロセス情報のソート用です。
       次のMyCompPListA() を呼び出しているだけです。

   int MyCompPListA()
       比較関数の実体です。
       PID と cmdline の比較を実装しています。

   GetProcessList()
       /proc ディレクトリ内を調べて、PID と cmdline を取得します。
       (項目は ps コマンドで表示されるものの PID と CMD に相当します）
       cmdline 内がnull文字で区切られている場合、空白文字に置き換えます。
       google-chrome 等を起動していると、全体でとても長いものになります。
       リスト表示すると折り返しで見辛くなるケースもあるかと思います。
       画面の幅に合わせて途中までで出力をやめるオプションも考えましたが、真面目にアルファベット以外の文字列の事を
       考慮しないとならないようなので、諦めました。
       このプログラムはそこまでするような種類のものではないと思っています。

   print_list_pid()
       プロセスリストを表示します。

   MyExit()
       X サーバへの接続が済んでいればそれをクローズし、指定された戻り値を指定して exit します。

   Is_Executable()
      実行可能ファイルか否かをチェックします。

   Is_Directory()
      ディレクトリが存在するか否かをチェックします。

   ParseDir()
      -c / -C オプションで指定された内容をディレクトリ部とファイル名部、パラメータ部に分割します。

   MyExec()
       アプリの起動が確認できなかった場合に、-c /-C で指定されたコマンドを起動する処理です。
       -g オプションが指定されている場合、このプロセスの優先度を保存した上で最低にして、所定のマイクロ秒数待ち、優先度を戻します。

   main()
       main 関数です。
       オプション解析、正規表現のコンパイル等を行って、各処理を呼び出しています。

   注: awin.h 内で特に注意を要するのは、次の部分です。

       #define PDATA_MAX 11616
         これは、プロセス情報を格納する配列の個数に使用されています。
         あまりに大きいと、その分メモリを消費しますが、あまりに小さいと全てのプロセスの情報を格納できません。
         最初配列を動的に確保していたのですが、細切れのメモリー確保/開放が多いと割と簡単にエラーになりました。
         細切れのメモリー確保を回避する一貫として、とりあえず固定配列にしています。
         私の使っていたLinuxの設定の上限値がこの値でした。
         ulimit -u を実行すれば、上限値がわかります。
         通常のデスクトップ用途なら十分大きい値だと思います。半分以下でも大丈夫なのではないかと思います。

       #define WAIT_AFTER_EXEC_MICROSECOND  200000
         コマンド起動後に、起動されたコマンドが処理を開始するのを待つマイクロセコンド秒です。
         -T オプションで変更可能です。

       #define MAX_RETRY_CNT_WINGET         25
         次の mywin.c の MyGetWinList() に渡す、繰り返しの最大回数です。繰り返しの間隔は、上記のコマンド起動後に
         待つマイクロ秒数を流用しています(リトライの理由がコマンド起動後のリスト取得の確実化の為)。
         -r オプションで変更可能です。


2.mywin.c
   wmctrl のソース・コードから、最低限必要そうな部分を持ってきて、私が少しでも分かるように手直ししたものが中心です。
   これまで X11 アプリを作ったこともない者が行った作業です。不都合に遭遇しては回避策を探すというのを繰り返しているので、
   色々不十分な点があるかと思います。まずい点などありましたら、ご教授頂ければ嬉しいです。
       digit() のロジックはネットで見つけました。最初はif〜then〜elseのネストで対応しようと思っていました。どっちが負荷が軽いのでしょうか。

   int digit()
        数値の桁数を返します。

   void MySetPriority()
        このプロセスのプライオリティを保存して変更、復元します。

   int MyChengeState()
        ウィンドウの状態を変更します。

   int window_move()
        ウィンドウ位置を移動します。
        コマンドを起動させた後、XMoveWindow() を呼び出しても位置が移動してくれない事があり、試行錯誤しています。
        ウィンドウマネージャ以外が、アプリ側のウィンドウが制御可能になるまで待つ方法がわからないのが、試行錯誤の理由です。
        この症状が起きた場合、-T オプションで待つ時間を増やしてみると良いようです。
        また、見た目的には十分に制御可能になっているハズで、XMoveWindow()を呼び出した後で位置を取得してみると移動させた値に
        なっているにもかかわらず、目視としては位置移動していない、という不可解な事象にも遭遇しました。
        こちらの事象が起きているか否か、をawinで把握する方法もわかりません。再現性も低く、対策は不明です。

   void activate_window ()
        ウィンドウをアクティブ化します。

   bool fGetWinFramePos()
        ウィンドウのフレームの左上座標を取得します。

   void move_and_activate_window ()
        現在デスクトップに移動させた後にウィンドウをアクティブ化します。

   int  MyGetWinList()
        次の getWinList() のラッパー関数です。
        リスト取得が0件だった場合、処理を繰り返します。

   int  getWinList()
        ウィンドウ情報のリストを得ます。

   int  MyCompWin()
       qsort に指定する比較関数です。ウィンドウ情報のソート用です。
       後述のMyCompWinA() を呼び出しているだけです。

   void DisplayWinList()
       ウィンドウ情報をリスト表示します。

   unsigned char *get_prop()
       ウィンドウのプロパティを取得します。良く理解出来ていません。

   Window *get_client_list()
       クライアント・リストを取得します。
       get_prop()を呼び出しています。

   char *get_property ()
       文字列型のプロパティを取得します。
       get_prop()を呼び出しています。

   int client_msg()
       クライアント・ウィンドウにイベントを送ります。良く理解していません。

   int window_to_all_desktop()
       ウィンドウのデスクトップを全てのデスクトップに変更します。
       KDEではSTICKYをセットしたのではダメだったので追加しました。

   int window_to_desktop ()
       ウィンドウのデスクトップを変更します。

   unsigned long get_pid()
       ウィンドウのプロパティからPIDを取得します。
       get_prop()を呼び出しています。
       見慣れたPIDにするのに、ちょっとした計算が行われていますが、これも理解していません。
       計算式はネットで漁って見つけました。

   char *get_machine()
       ウィンドウのプロパティからmachineを取得します。

   char *get_wmclass()
       ウィンドウのプロパティからclassを取得します。

   char *get_title()
       ウィンドウのプロパティからtitleを取得します。
       get_property() を呼び出しています。

   int MyCompWinA()
        比較関数の実体です。
        PID、machine、class、title の比較を実装しています。

   int MyCheckDuplicate()
       リスト出力する際に呼ばれ PID、machine、class、title が同じか否かを判定します。

   注: mywin.h 内で特に注意を要するのは、次の部分だと思います。
        #define WAIT_MICROSECOND_ACTIVATE_AFTER_MOVE    1     /* unit : microsecond  200000 is 0.2 sec. */
       これは、デスクトップを移動させてからウィンドウをアクティブにする迄の間の待ち時間です。
       短すぎると、不具合が起きる可能性があるようです。長くすると、安全ですが待たされます。
       wmctrl では、この部分は100000 ( 0.1 秒)でしたが、私の環境では十分ではない事があるようでした。
       短すぎた場合の不具合ですが、私が経験したのは、LXPanel( class が panel.lxpanel となっているもの)が画面からなくなりました。
       単純に 200000 ( 0.2 秒)にすると、その症状はなくなりましたが、操作していて引っかかる感じがありました。
       そこで、アクティブ化する直前に、このプログラムの優先順位を最低になるようにしました。
       システムや他のプログラムを優先して処理できるようにすれば、デスクトップの移動が早く終わるのではと考えたからです。
       (アクティブ化した後は、このプログラムは終了するだけなので、優先順位は戻していません)
       その上で、この値が 0 以上であれば、その時間待ちます。

       どういう理屈で不具合になるのか、私には分かりませんので、他にどういう不具合が起こりえるかも分かりません。
       デスクトップの移動をさせると何かおかしいが他は正常だ、という場合には少し長くして様子をみてください。
       (-t オプションはこの調整をコンパイルせずに行うために追加しました）
       私の環境では、今は 0 でも大抵は大丈夫なようですが、たまにウィンドウ描写が遅れる時があるので、1 にしてあります。
       優先順位が最低であれば、usleepを呼び出す事が重要な意味を持つのだと思います。
       なお、ご存知でしょうが、この値を指定しても厳密にその時間だけ待つ訳ではありません。
       システムの動作状況、システムタイマーがサポートする単位などによって、設定した値よりも少し待たされると思います。

        当時の私の環境: 
                  CPU         : Intel Core2Duo T710 1.80 GHz 2コア
                  Memory      : 3GB
                  Graphic     : Intel GM965
                  OS          : Debian linux (3.x pae)
                  Destribution: Kona Linux 3.0 Light
                  Language    : ja_JP.UTF-8
                  (仮想環境ではありません。ショボいですが、古いながらもSSDにしてるので、そこそこ使えます)


3.mymem.c
   1.の注で記した、細切れのメモリー確保を回避するための機能を記述したソースです。
   あまり賢い機能は実装していません。
   awin自身、起動されたら程なく終了し長居する機能ではないため、メモリ解放機能は実体がありません。
   プログラムが終了するときに、OSが効率的に開放してくれる事を期待しています。
    (他の部分でもあまり積極的にメモリ開放していません...)
   また一旦割り当てたメモリのサイズ変更の機能も提供していません。処理が複雑・大規模になりますし、
   割り当てられたメモリへのアクセス方法にも制約がでるためです。
   awin用に程よい機能にしている積りです。

   void *MyAssignMem()
        メモリー割り当て用関数です。
        後述の MyManageMem() を呼び出します。

   void MyChangeUnitMem()
        メモリーを実際に確保する単位を変更します。
        後述の MyManageMem() を呼び出します。
        確保する単位の初期値は、
            #define MYALLOC_DEFAULT_ALLOC_UNIT  65536
        で指定されています。

   void MyFreeMem()
        割り当てられたメモリー開放用関数です。
        後述の MyManageMem() を呼び出します。
        処理の実体は今はありません。とりあえず用意しただけです。

   int MyManageMem()
        処理の本体です。
        メモリーを次で指定された個数まで、確保し初期化します。
             #define MYALLOC_MAX_POOL            10
        確保したメモリーをMyAssignMem()を経由して部分的にアサインします。
        確保する単位は、MYALLOC_DEFAULT_ALLOC_UNIT または、MyChangeUnitMem()で指定された値のバイト数です。
        但し、要求されたバイト数がそれより大きい場合は、それを満たす大きさのものを確保します。
        この動きを考慮して、MYALLOC_MAX_POOL の値と、MYALLOC_DEFAULT_ALLOC_UNITの値あるいは、MyChangeUnitMem()で指定する値を
        調整する必要があります。
        通常は、"Err:Exceed allocation pool"のメッセージが表示されたら、値を増やす方向で調整を考える、でOKだと思います。
        メモリ消費量を最低限にしたいという場合は、減らす方向で調整してみてください。
