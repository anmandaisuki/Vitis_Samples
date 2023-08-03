# AXI GPIOのサンプル
普通のAXI GPIOのサンプルプログラム。

## 使い方
- init_GPIO()して、XGpio_DiscreteWrite()で出力/入力を操作すればよい。


## 関数の説明
```
XGpio_SetDataDirection(&gpio_inst, 1, 0);           // ポートが入力か出力かの設定。第２引数はポート。AXI GPIOの場合、最大2ポートで1か２になる。0,1じゃないので注意。 第３引数は入出力指定。0:出力, 1:入力
XGpio_DiscreteWrite(&gpio_inst, 1, 0x00000000);　   // ポートの操作。出力用。第２引数でポート、第３引数で書き込み。
XGpio_DiscreteRead(&gpio_inst, 1);                  // ポートの操作。入力読み込み用。
```

## 必要なハード要件(Vivado側の設定)
MicroblazeやZynqプロセッサなどのFPGA側のプロセッサでいくつか設定しておく必要がある。
* AXI GPIO (IPとしてAXI GPIOを配置し、プロセッサとAXIインターフェイスで接続する。)

## 注意事項
* XGpio_DiscreteWrite()は32bit毎の操作になるので、vivado側でポートの幅が1bitだったとしても、0x00000001と書くこと。(1bit目が1ならよいので、0xffffffffでもいい。)
