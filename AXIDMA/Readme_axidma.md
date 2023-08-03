# AXI DMA　(SGなし)　のサンプルプログラム
普通のDMA動作(割り込みあり)のサンプルプログラム。
基本コピペして使えばおけ。

## 簡単な説明
1. DMA初期化
2. GIC初期化。このタイミングでDMAの割り込みが貼ったときの割り込み関数(dmacplt_isr)を登録する。
3. XAxiDma_SimpleTransfer()でDMA開始。XAxiDma_SimpleTransfer()の第3引数はDMAで転送するバイト数を指定する。今回バッファがint型(4byte)なので、4*BYTESIZEとした。

## 必要なハード要件(Vivado側の設定)
MicroblazeやZynqプロセッサなどのFPGA側のプロセッサでいくつか設定しておく必要がある。
* DMA (IPとしてAXI DMAを配置し、プロセッサとAXIインターフェイスで接続する。)
* GIC (Zynqの場合、interrupts > Fabric Interrupts > PL-PS Interrupt > IRQ_F2Pを有効化し、AXI_DMAのs2mm_interruptと接続する。) 

## 注意事項
* vivadoから作成する.xsaファイルから作成されるBSP(ドライバとか)の関数を使うので、.xsaファイルが適切でなかったり、Vitisのバージョンが変わって、`生成されるBSPが変更されると動かない。`
* axi dmaはXAxiDma_SimpleTransfer()で指定した以上のデータを受信すると、DMAIntErrをたてて適切に停止する。一回の転送ならそれで問題ないが、続けてXAxiDma_SimpleTransfer()ができなくなるため、ハード側で指定したバイト数以上のデータが転送されないようにtlast信号を発行するか、無理やりXAxiDma_Reset()で初期化する必要がある。[(p26参照)](https://docs.xilinx.com/v/u/ja-JP/pg021_axi_dma)
* simpleTransfer()では約8Mバイト長さが限界っぽい。data_lengthレジスタが23bitでそれで限界が決まっているっぽい。厳密には8388607バイトが限界？
* 16ワードアドレス毎じゃないとうまく書き込めない。0x01010cみたいに中途半端なアドレスだとだめ。0x0010000みたいにちょうど。
	- 要は16進数で1の位が0になってれば良い。


## 参考文献
* [AXI DMA(PG021)](https://docs.xilinx.com/v/u/ja-JP/pg021_axi_dma)