# AXI DMA　(SGモード)　のサンプルプログラム
SG(Scatter Gather)モードでのAXI DMA。

## 簡単な説明
1. 普通にDMAの初期化。インスタンス化。
1. 普通に割り込みコントローラ(GIC)の初期化、インスタンス化。
1. 1で作成したDMAのインスタンスからBDリングのインスタンスを作成。XAxiDma_GetRing(XAxiDma*);
1. BDリングを作成。XAxiDma_BdRingCreate();③で作成したBDリングのインスタンス、BDリングを置くメモリアドレス、アライン方式、BDの数を指定する。メモリアドレスはvirutualとPhysicalがあるけど両方同じでおけ。アラインはXAXIDMA_BD_MINIMUM_ALIGNMENTとかでおけ。
1. 4で作成したBDリングをDMAのカレントディスクリプタレジスタにいつでも書き込める位置に配置。XAxiDma_BdRingAlloc()でBDリングのインスタンス、BDリング内のBDの数を指定。第３引数で、BDのポインタが返ってくる。
		　第３引数で現在のBDのポインタを取得し、以降でそれを利用して、BDの中身を書いていく。
1.BDリング内のBDの中身を書いていく。xaxidma_bd.hの関数を利用する。
	* XAxiDma_BdWrite()とデータシートのオフセットから書き込み。(word毎(32bit)の書き込み)			
	* あるいはXAxiDma_BdSetBufAddr()のようなマクロ利用。
	* 最低限、バッファアドレス（S2MM_Buffer_Address）、データ長さ(S2MM_CONTROL)、SOF/EOF(S2MM_CONTROL)を設定する。
	* BDリング内のBDが１つより多い場合は、XAxiDma_BdRingNext()とforループを利用して各bdの中身を埋めていく。
1. 割り込み関連の設定。XAxiDma_BdRingSetCoalesce（）//BDごとではなくBDリング単位での割り込みにする？
1. DMAのBDリングを順番にカレントディスクリプタレジスタに書き込むためにハードウェアに制御を渡す。XAxiDma_BdRingToHw()//BDリングインスタンスとBDの数、アドレスを渡す。
1. BDリングからの割り込みを有効化。XAxiDma_BdRingIntEnable().DMAの中のBDリングに関する割り込みの有効化。
1. XAxiDma_BdRingStart（）で開始。

## 実際に使用するとき > #defineを変更。#defineについての説明
AXI DMA SGモードを使用する際は、以下の#defineだけ環境に合わせて調整すれば基本的には動作する。
```
    //DMA
    #define DMA_BUF_ADDRESS 0x00ff0000   // DMAで格納するバッファの先頭アドレス。どのアドレスが空いているかはデバイスによって変わるので都度変更する必要あり。 
    #define DMA_BUF_LENGTH 500           // DMA_BUF_ADDRESSから何バイト分格納するか。

    //BDRING
    #define BDRING_NUM 1                // BD(BufferDiscriptor)リング内に何個のBDが存在するか。これによって、BDリングのサイズが決まる。
    #define BDRING_ADDRESS 0x00f10000   // BDリングの先頭アドレス。どのアドレスが空いているかはデバイスによって変わるので都度変更する必要あり。 

    //以下はBD書き込み用の#define。変更しなくてよい。オフセットやマスク。
        
        //offset
        #define BUFFER_ADDRESS_OFFSET 0x08
        #define CONTROL_OFFSET 0x18
        #define STATUS_OFFSET 0x1c

        //mask
        #define RXSOF_MASK 0x4000000    // BDリング内で先頭のBDであることを示すことを書き込むときに使用。
        #define RXEOF_MASK 0x2000000    //  BDリング内で最後のBDであることを示すことを書き込むときに使用。

``` 

## 必要なハード要件(Vivado側の設定)
MicroblazeやZynqプロセッサなどのFPGA側のプロセッサでいくつか設定しておく必要がある。
* DMA (IPとしてAXI DMAを配置し、プロセッサとAXIインターフェイスで接続する。)
* `AXI DMAはSGモードを有効にすること。(Vivado側で)`
* `tlast信号が発行されるようにすること。(vivado側で)`
* GIC (Zynqの場合、interrupts > Fabric Interrupts > PL-PS Interrupt > IRQ_F2Pを有効化し、AXI_DMAのs2mm_interruptと接続する。) 

## 注意事項
* vivadoから作成する.xsaファイルから作成されるBSP(ドライバとか)の関数を使うので、.xsaファイルが適切でなかったり、Vitisのバージョンが変わって、`生成されるBSPが変更されると動かない。`
* `SGモードの場合、XAxiDma_SimpleTransfer()は使えないので注意。`
* `tlast信号が無いとSGモードでのDMAは終了しない。ずっとDMA完了割り込みがかからなかった。`
    * `DMA_BUF_LENGTHが小さすぎて、例えばDMA_BUF_LENGTHが10byteでtlast信号が12byte目にくる場合もだめ。DMA_BUFにデータを格納している最中にtlast信号がアサートされるようにすること。`
    * `つまりtlast信号 < DMA_BUF_LENGTHになるようにする。`
* Xil_DCacheFlushRange()でキャッシュクリアしないと、データはメモリではなくキャッシュで止まっていることがある。
    - buf[100]でbuf[94]以降だけ、何故か値が正しくないみたいなことになる。

## より高度なことをしたいとき
今回はとりあえずSGモードを使うことを目的としたので、BDリング内のBDはすべて同じバッファサイズ、同じバッファアドレスをデータ格納先として指定するようになっている。そのため、異なるアドレスやバッファサイズを指定したいときはinit_BDRING()内を変更すると良い。
* バッファ先頭アドレスを変更したいときは以下を変更。
```
// Set buffer address on current BD
XAxiDma_BdWrite(cur_bd_ptr,BUFFER_ADDRESS_OFFSET,DMA_BUF_ADDRESS);
```
* バッファサイズを変更したいときは以下を変更。
```
// Set buffer length
control_register_write_data |= DMA_BUF_LENGTH;
```
* 以下のコメントアウトを解除すると、BDごとに割り込みが発生する？しないとBDリング単位での割り込みになる。EOFのBDが終了するときのみ割り込み発生。
```
    // // Setup interrupt coalescing so only 1 interrupt fires per BD ring transfer. Only activate when BD ring is more than 2
	// status = XAxiDma_BdRingSetCoalesce(bdring_inst, BDRING_NUM, 0);
    // if(status != XST_SUCCESS){
    //         return INIT_FAILURE;
    //     }
```

## 参考文献
* [AXI DMA(PG021)](https://docs.xilinx.com/v/u/ja-JP/pg021_axi_dma)