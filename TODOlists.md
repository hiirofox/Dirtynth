Dn=Done. Ip=InProgress.

01.0. 做相位整形mutant: (phasemix minp2linp phase2mag)。

01.5.Dn. 仿制virus ti的一个波表效果器，命名为VirusGrain。

01.6. 给mutant加上自己的状态，而不是单纯输入时间参数。

01.7. 制作重采样mutant。可调采样率和采样位宽，jitter。
01.8. 制作formant shift mutant，可以线性，指数缩放映射。还有个参数用于调基频偏移。
01.9. 制作spectral gate mutant。

01.95. 尝试mutant实现fm。可能不一定合适。到时候换成延迟线实现fm吧。

02.0.Dn. 修好comb,4阶comb,硬弦模态滤波器的调制咔哒声问题。

03.0. 串上效果器。

04.0. 重写新包络系统。

05.0.Ip. 重写platform抽象层，与延迟，cpu，线程调度有关。

06.0. 加一个pcm读取wav播放音头，可插入filter1,filter2，也可直接跟随voice播放。
音头带有一些基础参数，pitch，formant shift，keytrack，pogo（fl采样器这么叫的），decay，click等。

07.0. 重新实现一整套系统，包括包络系统那些，在DirtynthAlpha2。
