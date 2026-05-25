# 便宜的数字IIR椭圆滤波器设计

L-MODEL团队 赤狐

## 简介

众所周知，设计一个椭圆滤波器涉及各种复杂的数学运算，导致传统的iir椭圆滤波器设计方法难以应用于需要快速调制的场景。
这篇博客介绍一种邪修，通过全通滤波器代换的方式，扭曲频率轴，不改变幅度，实现低设计算力的椭圆滤波器设计。最后给出scipy的直接设计轮子与全通代换法的频响对比。

## 传递函数套传递函数的玩法

要理解一阶全通代换到底是在做什么，我们首先需要了解形如$H_1(H_2(z))$这样的系统，其内部究竟发生了什么，$H_2$是如何影响$H_1$的频响的。

首先，先看我们遇见的最普通的形式：$H(z)$。其中$z=\exp(jw)$。那么，不难看出，$H(z)$的频响，实际上就是$H(z)$关于$w$的“轨迹线”。

具体地说，在nyquist频率内，这条"轨迹线"是一个从$1$到$-1$，圆心为$0$半径为$1$的上半圆，其线性频响就是这条“轨迹线”经过系统传递函数得到的复数。

那么，假如$z'=z^2$呢？即$H(z^2)$究竟发生了什么。我们首先看$z'=z^2=\exp(j2w)$。原本在$[0,\mathrm{nyquist}]$的半圆，变成完整的圆。于是原本$[0,\mathrm{nyquist}]$的频响就相当于压缩到了$[0,\mathrm{nyquist}/2]$。对于实数系统来说，零极点是共轭的，下半圆的系统完全镜像。你可以看到$[\mathrm{nyquist}/2,\mathrm{nyquist}]$部分的频响实际上是$[0,\mathrm{nyquist}/2]$关于$\mathrm{nyquist}/2$的镜像。

## 全通代换的原理

对于一阶全通$\mathrm{APF}(z)=\frac{k+z}{1+kz}$，让AI稍做整理，我们可以发现，轨迹线依然是一个从$1$到$-1$，圆心为$0$半径为$1$的上半圆，只不过，$w$被拉伸了：
$$
w' = 2 \operatorname{atan}\left(\frac{1-k}{1+k}\cdot\tan\left(\frac{w}{2}\right)\right)
$$
可以发现，调节$k$可以使得$w$的上升变抖，或者变缓。有了这个操作，我们就可以提前或者推迟截止频率的到来了！

![](https://files.mdnice.com/user/176339/b4f9dbe2-d7e2-432c-9fd8-8d47e4a98c81.png)
图 1 不同$k$对$w'$的影响 https://www.desmos.com/calculator/ugm0nqc7pr

有了这个现象，接下来就顺理成章地去思考，比如说原本采样率为$\mathrm{sampleRate}=48000\mathrm{Hz}$，截止频率在$\mathrm{cutoffOrigin}=10000\mathrm{Hz}$的系统，我如何通过全通代换，让截止频率变到任意$\mathrm{cutoffTarget}$频率呢？

经过一番整理，得出一个$k$关于采样率$\mathrm{sampleRate}$，原始截止频率$\mathrm{cutoffOrigin}$和目标截止频率$\mathrm{cutoffTarget}$的表达式：
$$
k = \frac{\sin\left(\pi\frac{\mathrm{cutoffTarget}-\mathrm{cutoffOrigin}}{\mathrm{sampleRate}}\right)}{\sin\left(\pi\frac{\mathrm{cutoffTarget}+\mathrm{cutoffOrigin}}{\mathrm{sampleRate}}\right)};
$$
于是，我们终于可以实现对固定截止频率的椭圆滤波器进行截止频率的全通代换了。

## 化简全通代换后的系统

直接将$z=\frac{k+z}{1+kz}$代入iir传递函数$H$中进行手动化简，是一项非常痛苦的过程。

幸好sympy可以满足我们的需求。我们可以设定原始的iir系数符号$b_0b_1b_2\ldots a_1a_2\ldots$，设定“频率扭曲系数”符号$k$，然后通过sympy自动代入化简，就可以得到化简后的$b_0b_1b_2\ldots a_1a_2\ldots$关于$k$的表达式了。

注意到，$z=\frac{k+z}{1+kz}$这个变换是保阶的，也就是说，原本是二阶系统，出来的也是一个二阶系统。原本有5个系数$(b_0b_1b_2a_1a_2)$，出来依旧是5个系数$(b_0b_1b_2a_1a_2)$。

## 算法流程

1. 首先使用scipy设计一个采样率$48000\mathrm{Hz}$，截止频率为$10000\mathrm{Hz}$，通带$0.1\mathrm{dB}$，阻带$60\mathrm{dB}$的6阶IIR数字椭圆滤波器。使用其SOS输出，将原本一大块传递函数砍成3个biquad系数输出。
2. 用sympy生成新系数$b_0b_1b_2a_1a_2$关于旧系数$b_0b_1b_2a_1a_2$和$k$的表达式，整理成c语言封装成全通变换系数函数。
3. 封装$k$系数生成函数，通过设置采样率，原始截止频率，目标截止频率来得到系数$k$。
4. 对每个biquad系数应用一次全通变换系数，得到新的biquad系数。
5. 串联这几个biquad，跑新的应用过全通变换的biquad系数，就是任意截止频率的椭圆滤波器了。

## 效果对比
大家一定好奇这种设计方法与scipy直接进行椭圆滤波器设计的效果有多大差异，设计指标是否还能达到。

巧合的是，通过这种设计方法实现的便宜的椭圆滤波设计器，其频响和直接设计得到的频响居然完全重合！

![](https://files.mdnice.com/user/176339/13db7d64-13a8-4a47-be25-be3b31de0558.gif)
图 2 两种设计方法的对比。红线是直接设计法，蓝线是全通代换法

## 具体实现

大家可以去看L-MODEL倾情制作的Dirtynth合成器的滤波器源码，其中有具体实现。

其中的实现虽然不是基于biquad，而是svf。因为svf对于快速调制和低频场景下的稳定性很好，对于椭圆滤波器来说更建议使用。不过设计方法依然是相似的，只需加一层biquad转svf系数映射即可。我之后会写一篇文章详细说明这个方法以及特性。

https://github.com/hiirofox/Dirtynth/blob/master/Source/dsp/Filter.h
