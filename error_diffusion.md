誤差拡散メソッド
================

wakuflowプログラムでは色数を減らす変換を行う際にどのような方式で誤差拡散（ディザリング）を行うか指定できます。

現在のところ、対応しているのは次の通りです。

* `none` : 特にディザリングは行いません
* `floyd` : Floyd-Steinberg
* `sierra3` : Sierra (3 line)
* `sierra2` : Sierra (2 line)
* `sierra-lite` : Sierra Lite
* `atkinson` : Atkinson

変換サンプル
------------

| 指定          | サンプル                              |                                           |                                           |                                           |
|---------------|:-------------------------------------:|:-----------------------------------------:|:-----------------------------------------:|:-----------------------------------------:|
| (入力画像)    | ![](docimg/error/input.png)           | ![](docimg/error/input.png)               | ![](docimg/error/input.png)               | ![](docimg/error/input.png)               |
| `none`        | ![](docimg/error/bin_none.png)        | ![](docimg/error/websafe_none.png)        | ![](docimg/error/famicom_none.png)        | ![](docimg/error/gameboy_none.png)        |
| `floyd`       | ![](docimg/error/bin_floyd.png)       | ![](docimg/error/websafe_floyd.png)       | ![](docimg/error/famicom_floyd.png)       | ![](docimg/error/gameboy_floyd.png)       |
| `sierra3`     | ![](docimg/error/bin_sierra3.png)     | ![](docimg/error/websafe_sierra3.png)     | ![](docimg/error/famicom_sierra3.png)     | ![](docimg/error/gameboy_sierra3.png)     |
| `sierra2`     | ![](docimg/error/bin_sierra2.png)     | ![](docimg/error/websafe_sierra2.png)     | ![](docimg/error/famicom_sierra2.png)     | ![](docimg/error/gameboy_sierra2.png)     |
| `sierra-lite` | ![](docimg/error/bin_sierra-lite.png) | ![](docimg/error/websafe_sierra-lite.png) | ![](docimg/error/famicom_sierra-lite.png) | ![](docimg/error/gameboy_sierra-lite.png) |
| `atkinson`    | ![](docimg/error/bin_atkinson.png)    | ![](docimg/error/websafe_atkinson.png)    | ![](docimg/error/famicom_atkinson.png)    | ![](docimg/error/gameboy_atkinson.png)    |
