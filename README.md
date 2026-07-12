# Dirtynth

本文由 Codex 于 2026-07-12 根据 `L-Model-Readme-skill.md`、当前 `git` 历史、源码目录和旧 `README.md` 更新。

Dirtynth 是 L-Model 团队开发的 JUCE 波表合成器插件，当前主工程提供 VST3 和 Standalone 构建入口。主合成结构是 8 复音、双波表振荡器、双 mutant 波表变形、双滤波器和 12 个可分配调制槽；插件侧已经接入 MIDI note、CC、pitch bend、宿主状态保存、参数系统、文本 preset 拖出/拖入和 1024x480 单页编辑器。

当前音频链路从 `Source/PluginProcessor.cpp` 收集宿主 MIDI 事件并调用 `DirtynthSystem::ProcessBlock`，再由 `DirtynthVoice` 在每个 voice 内完成振荡器、异步 mutant 表更新、调制、滤波路由和输出累加。`EmbeddedTest` 是 Dirtylet / 板端方向的独立测试入口，复制了一套 DSP 源码并直接读 MIDI、输出音频；它和主插件共享大体结构，但当前并不完全同步主插件的全部 mutant 类型。

注意：当前工程仍处于实验阶段，音量、电平和滤波共振都可能产生较大的输出。实际试听时建议先降低宿主、声卡或外部监听音量，并进行限制保护。

## 根目录

`Dirtynth.jucer` 是 JUCE 工程描述文件，当前包含插件源码、UI 组件、DSP 文件、preset 组件和 Visual Studio 2022 导出设置。`Builds/VisualStudio2022` 保存由 JUCE 导出的 Visual Studio 工程，包含 `Dirtynth.sln`、SharedCode、VST3、Standalone 和 manifest helper 项目。

`Source` 是主插件源码目录，分为 JUCE 插件入口、UI 组件和 DSP 核心。`Presets` 保存插件侧文本 preset，文件名按生成时间命名，当前用于 `PresetComponent` 的拖入载入和拖出保存链路。`designer` 保存合成路由和包络/调制系统图，README 中的路由图片仍引用这里的 PNG。

`EmbeddedTest` 是不依赖 JUCE 插件壳的 Dirtylet 测试程序目录，包含自己的 `Source/dsp` 副本、`Presets`、`build.sh`、Visual Studio 测试工程、板端脚本以及若干已构建的测试二进制。`blog` 保存滤波器和 ADSR 相关文章，不参与主插件构建。`TODOlists.md` 记录当前任务状态，其中 `VirusGrain` 已标记完成，platform 抽象层仍处于进行中。

`README-Backup.md` 是旧 README 备份，`L-Model-Readme-skill.md` 是这次 README 更新使用的本地写作规则文件；两者当前都是工作区未跟踪文件，不属于主插件运行链路。

## 运行入口

`Source/PluginProcessor.cpp` 是 JUCE 处理器入口。`DirtynthAudioProcessor::processBlock` 将宿主传入的 MIDI buffer 转为 `DirtynthMidiEvent` 队列，把左右声道写指针交给 `DirtynthSystem::ProcessBlock`；`prepareToPlay` 只同步采样率，`getStateInformation` 和 `setStateInformation` 直接按 `DirtynthParams` 二进制块保存和恢复宿主 state。

`Source/PluginEditor.cpp` 是编辑器入口，构造 1024x480 的 `DirtynthUI` 和右上角 `PresetComponent`。`DirtynthUI` 负责把全局、osc、filter、routing、envelope 等控件绑定到 `DirtynthParams`，组合框中的 mutant 名称来自 `RegMutant::MutantNames`，因此主插件 UI 已经能选到 `HardSync`、`SelfPM`、`Kickizer`、`Disperser` 和 `VirusGrain`。

`EmbeddedTest/Source/main.cpp` 是桌面/板端直接测试入口。它读取 `EmbeddedTest/Presets`，通过 `MidiIO.h` 接收 MIDI，通过 `WaveIO.h` 输出音频，并调用本地副本 `DirtynthSystem::ProcessBlock`。这个入口当前用于 Dirtylet 硬件方向测试，不等同于插件主入口，也不代表 Dirtylet 最终发布形态。

## 核心模块

`Source/dsp/DirtySystem.h` 是当前主 DSP 调度文件，包含 `RegMutant`、`RegFilter`、`RegEnvelope`、`MutantThreadPool`、`DirtynthVoice`、`DirtynthMidiEvent` 和 `DirtynthSystem`。`RegMutant` 当前注册 5 个波表变形器：`HardSync`、`SelfPM`、`Kickizer`、`Disperser`、`VirusGrain`；`MutantThreadPool` 在后台线程中根据波表 preset、波表位置、两级 mutant 类型和参数生成下一张积分波表，并把时间 `ts` 传给支持动态时间的 mutant。

`DirtynthVoice` 保存单个 voice 的两个 `WTOscillator`、两个可选滤波器、12 个包络/调制实例、voice 频率、力度、mutant 任务 ID 和当前参数副本。它在 `ProcessBlockAccumulating` 中按 envelope 间隔更新调制值，提交或接收 mutant 任务，处理 oscillator、PM / freq shifter、四种 filter topology、`outputAmp` 和 voice 音量累加。

`DirtynthSystem` 是插件级合成器对象，保存 8 个 `DirtynthVoice`、共享 `MutantThreadPool`、参数系统、voice 分配表和 MIDI note 状态。它负责 note on/off 的 voice 分配、控制器和 pitch bend 事件传递、参数更新以及最终把所有 active voice 累加到输出 buffer。

`Source/dsp/Wavetable.h` 是波表、FFT 和 mutant 变形实现文件。`TableMutant` 是 mutant 基类，提供 `Apply`、`SetMutantParams` 和 `SetTime`；`TableMutantSync` 做硬同步式读表重排，`TableMutantSelfPM` 做多级自相位调制，`TableMutantKickizer` 做 kick 形时间映射，`TableMutantDisperser` 做频域色散、harmonic shift 和 comb 塑形，`TableMutantVirusGrain` 做带时间参数的 Virus grain 风格波表处理，参数名对应 `Spread`、`Shift` 和 `Detune`。

`WTOscillator` 保存三份积分波表缓冲、当前播放相位、swap 状态和 IIR blep 对象。它通过后台准备好的积分波表做平滑切换，并在采样级输出当前波表结果；`WavetableGenerator` 当前生成 2048 点、64 层的波表表族，作为两个 oscillator 的源表。

`Source/dsp/DirtyParams.h` 定义 `DirtynthParams`、参数注册表、参数显示信息、手感映射和 preset 字符串读写入口。当前参数结构包含 master、octave、两个 oscillator、PM depth、两个 filter、topology、12 个 envelope/mod slot 和 2 个 effect 参数槽；effect 参数已经注册进参数表，但主音频处理链路还没有调用 `Source/dsp/effects` 中的效果器。

`Source/dsp/Filter.h` 保存滤波器基类和 7 个已注册滤波器：`SVFilter12dB`、`SVFilter24dB`、`Elliptic6order`、`CombFilter`、`CombFilter4Stage`、`RigidStringModal` 和 `PhaserFilter`。近期代码对 comb、四阶 comb 和硬弦模态的 delay / feedback / morph 更新做了更平滑的处理，主链路仍通过 `RegFilter` 在两个滤波器槽里动态选择类型。

`Source/dsp/Envelope.h` 保存 `Envelope` 基类、`ADSR` 和 `ModSource`。`ADSR` 负责 note gate 包络，attack 从当前值起步，release 使用一阶衰减；`ModSource` 负责 pitch、velocity、aftertouch、CC、CV 等控制源，并保留 curve、downbit、smooth、overshoot、HP 和 trajitter 等参数入口。

`Source/dsp/Serializer.*` 是文本序列化读写器，当前被 `DirtynthParamSystem` 和 `PresetComponent` 用于 `.txt` preset。`Source/dsp/Platform.h` 当前只提供 `Platform::BindToCPU`，主插件构造 `DirtynthSystem` 时绑定主线程，mutant 线程从 CPU1 开始绑定；Windows 分支目前保留空实现，Linux 分支使用 pthread affinity。

`Source/ui/DirtynthUI.h` 是主界面组件集合，包含 global、osc、filter、routing、envelope 等区域和参数绑定辅助函数。`Source/ui/PresetComponent.h` 是 preset 拖拽控件，负责把当前 `DirtynthParams` 序列化为文本文件，也负责读取拖入的文本 preset 并调用 `DirtynthSystem::SetParams`。

## 自上次 README 后的变化

上次 README 更新提交是 `2470a3a`，时间为 2026-06-24 00:08:17 +0800。之后到当前 `HEAD` 一共有 12 个提交，`git diff --stat 2470a3..HEAD` 显示 16 个文件变化，主要集中在 `Source/dsp/Wavetable.h`、`Source/dsp/DirtySystem.h`、`Source/dsp/DirtyParams.h`、`Source/dsp/Filter.h`、`Source/dsp/Platform.h`、preset 文件、工程文件和 `TODOlists.md`。

这批更新把 `VirusGrain` 作为第 5 个 mutant 接入主插件：`DirtyParams.h` 将 `NumMutantTypes` 改为 5，`DirtySystem.h` 的 `RegMutant` 增加 `"VirusGrain"` 和 `TableMutantVirusGrain<TableWidth>`，`DirtyParams.h` 根据 mutant type 4 把三个参数显示为 `Spread`、`Shift`、`Detune`。`MutantThreadPool` 的任务结构新增时间戳 `ts`，后台处理时会先 `SetTime` 再 `SetMutantParams` 和 `Apply`，用于支持随时间变化的 mutant。

`Wavetable.h` 新增 `TableMutantVirusGrain`，内部保存时间、频域缓冲和两组临时 buffer，先对输入表做 FFT、按 spread/shift 拆出两组频带，再用时间相位和 detune 做双路 grain 式重组。`TableMutantDisperser` 的 comb 参数也被调整为更非线性的频域梳状塑形。

滤波部分近期修过 comb、四阶 comb 和硬弦模态的动态调制噪声。`CombFilter` 现在在 delay 更新中使用分段过渡，`CombFilter4Stage` 统一四级 comb 的 gain 和 morph 派生，`RigidStringModal` 在延迟读数、色散全通和 DC 处理上保留了更完整的状态重置。

`Platform.h` 新增平台绑定入口，`DirtynthSystem` 构造时调用 `Platform::BindToCPU(0)`，mutant 线程调用 `Platform::BindToCPU(threadId + 1)`。这部分与 `TODOlists.md` 中“重写 platform 抽象层”对应，当前已经有条件路径，但仍是进行中状态。

preset 目录新增 `Preset20260628_061210.txt`，并把几份旧时间戳 preset 重命名到 20260702。`Dirtynth.jucer` 和 Visual Studio SharedCode 工程同步纳入新的 DSP 文件状态，`README.md` 文件名也已经从早期的 `REAMDE.md` 拼写修正为当前名称。

## 合成路由

![路由模式](designer/路由模式.drawio.png)

## 调制路由

![新包络系统](designer/新包络系统.drawio.png)

原图描述的是更完整的包络/调制系统；当前代码已经有 12 个调制槽和多个调制源，但不是图中所有设想都已经完整落地。

## 构建

JUCE 插件工程使用 JUCE module 路径 `C:/JUCE/modules`。打开 `Builds/VisualStudio2022/Dirtynth.sln` 后，可以构建 `Dirtynth_VST3` 或 `Dirtynth_StandalonePlugin`。

嵌入式测试程序位于 `EmbeddedTest`，构建脚本为：

```bash
./build.sh debug
./build.sh release
./build.sh release-rk3506
./build.sh release-rv1106
./build.sh release-rk3566
./build.sh release-t153
```

其中交叉编译目标依赖本机对应 toolchain 和 sysroot 路径，脚本里已经写死了作者当前环境的路径。`EmbeddedTest/VSBuild/VSBuild.sln` 是 Windows 侧测试工程，`EmbeddedTest/dirtylet-*` 是当前仓库中保留的测试二进制产物。

## 相关笔记

- `blog/02.便宜的数字椭圆滤波器设计/02.便宜的数字椭圆滤波器设计.md`
- `blog/03.高效的ADSR包络算法设计/03final.md`

## 当前进度

当前主插件已经完成 JUCE synth plugin 入口、8 复音 voice 管理、双波表振荡器、后台 mutant 线程池、5 种 mutant、双滤波器、7 种滤波器类型、4 种滤波路由、12 个调制槽、ADSR / pitch / velocity / aftertouch / CC / CV 调制源、参数注册表、宿主状态保存、文本 preset 拖拽、单页 UI、Visual Studio 工程和基础 Dirtylet 测试入口。

当前仍处于占位或后续扩展状态的部分包括 effect 参数槽与 `Source/dsp/effects` 的正式接入、新包络系统重写、platform 抽象层整理、PCM / WAV 播放音头、相位整形 mutant，以及主插件 DSP 与 `EmbeddedTest/Source/dsp` 副本之间的同步。特别是 `VirusGrain` 已接入主插件 `Source/dsp`，但 `EmbeddedTest/Source/dsp` 当前仍只注册 4 个 mutant。
