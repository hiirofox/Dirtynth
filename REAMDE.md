# Dirtynth

本文由 Codex 于 2026-06-23 根据当前 `README.md`、`git` 历史和源码粗略整理进行更新。

注意：当前工程仍处于实验阶段，音量、电平和滤波共振都可能产生较大的输出。实际试听时建议先降低宿主、声卡或外部监听音量，并进行限制保护。

Dirtynth 是 L-Model团队 开发的波表合成器，核心是双波表振荡器加双滤波器的复音合成结构。它支持 8 复音，每个 voice 包含两路波表振荡器、波表 mutant 变形、两级可选滤波器、四种滤波路由方式，以及 12 个可分配调制槽。当前已经接入 ADSR、音高、力度、CC1、CC2 等调制源，参数系统、宿主状态保存和 preset 文本拖拽也已经实现。滤波器部分包含 SVF12dB(2阶SVF)、SVF24dB(4阶SVF)、Ellip6order(6阶椭圆滤波器)、Comb(梳状滤波器)、Comb4Stage(四阶梳状滤波器)、RSModal(硬弦模态)、Phaser(相位器) 等类型，整体声音方向偏实验性、数字感和可变形纹理。Dirtylet 则是 Dirtynth 的硬件版本方向，目前还在 EmbeddedTest 中做测试，不代表Dirtylet最终发布形态。

## 已经做到的部分

- JUCE synth plugin：接收 MIDI 输入，输出音频，提供 VST3 和 Standalone 工程。
- 8 复音 voice 管理：优先复用空闲 voice，再复用已经松键的 voice，最后循环分配。
- 双波表振荡器：2048 点表，内部生成多层积分表，并用后台 `MutantThreadPool` 生成变形后的波表。
- mutant 波表变形：`HardSync`、`SelfPM`、`Kickizer`、`Disperser`。音频线程只在新表准备好之后切换，避免在实时路径里做重计算。
- 双滤波器：`SVF12dB(2阶SVF)`、`SVF24dB(4阶SVF)`、`Ellip6order(6阶椭圆滤波器)`、`Comb(梳状滤波器)`、`Comb4Stage(四阶梳状滤波器)`、`RSModal(硬弦模态)`、`Phaser(相位器)`。
- 四种路由模式：并联、滤波器 1 进滤波器 2、串联、双路混合。
- 12 个调制槽：每个槽有两个 target 和两个 amount，target 指向参数系统中注册过的参数。
- 调制源：ADSR、音高、力度、CC1、CC2 等已经进入 voice 更新流程。
- ADSR：Attack 从当前值起步，Decay 到 sustain，Release 使用 RT60 一阶衰减；参数变化时会按当前段位置重新计算一次。
- 参数系统：参数描述、显示名、旋钮手感、调制范围和 target 列表由 `DirtynthParamSystem` 统一生成。
- Preset 拖拽：编辑器右上角新增 `PRESET` 区域，可以把当前参数拖出为 `.txt`，也可以把 `.txt` preset 拖回插件载入。
- UI：1024x480 单页界面，包含 global、osc、filter、routing、envelope 控制区。
- 状态保存：插件把 `DirtynthParams` 写入宿主 state，并在读取时恢复到引擎。

## 自上次 README 后的变化

上次修改 `README.md` 的提交是 `5a308f5`，时间为 2026-06-16 14:36:02 +0800。之后到当前 `HEAD` 一共有 7 个提交，`git diff --stat` 显示约 60 个文件变化，主要是新增 preset、嵌入式测试程序、参数序列化和少量 DSP 调整。生成本文前工作区没有未提交改动；生成本文后只新增 `REAMDE-next.md`。

- `preset控件`：新增 `Source/ui/PresetComponent.h`、`Source/dsp/Serializer.*`，并把 preset 控件挂到 `PluginEditor`。现在可以把当前参数保存为文本 preset，也可以从文本 preset 恢复参数。
- `好玩的preset` 和后续 preset：新增 `Presets/Preset*.txt`，保存了若干当前参数快照，供插件和嵌入式测试程序读取。
- 调制源小改：`Envelope.h` 中的 `ModSource` 增加/整理了 curve、downbit、smooth、HP、overshoot、trajitter 等参数入口，当前主要仍是控制值传递和范围保护，部分更复杂处理还处于预留或实验状态。
- Dirtylet / 嵌入式测试：新增 `EmbeddedTest`。这里是 Dirtylet 硬件版本的测试入口，里面复制了一套 DSP 源码，`main.cpp` 负责读取 MIDI、调用 `DirtynthSystem::ProcessBlock`、输出音频，并可用方向键切换 `EmbeddedTest/Presets` 中的 preset。
- MIDI/音频 IO：`EmbeddedTest/Source/MidiIO.h` 同时支持 Windows WinMM 和 Linux ALSA；`WaveIO.h` 同时支持 Windows waveOut 和 Linux ALSA PCM。
- 嵌入式构建：`EmbeddedTest/build.sh` 增加 native debug/release，以及 RK3506、RV1106、RK3566、T153 等交叉编译目标。
- 板端脚本：`EmbeddedTest/BoardScript` 增加 IRQ 迁移和 CPU0 占用观察脚本，服务于低延迟测试和线程调度观察。
- 波表变形调整：`Source/dsp/Wavetable.h` 把 `TableWidth` 提到 namespace 级别，mutant 实现共享该常量；`Disperser` 的 comb 部分改成更非线性的频域梳状塑形，听感目标更偏“好玩”的色散/梳状纹理。
- 工程文件同步：`Dirtynth.jucer` 和 Visual Studio 工程文件更新，用来纳入新增的 preset UI 与序列化源码。

## 合成路由

![路由模式](designer/路由模式.drawio.png)

## 调制路由

![新包络系统](designer/新包络系统.drawio.png)

原图描述的是更完整的包络/调制系统；当前代码已经有 12 个调制槽和多个调制源，但不是图中所有设想都已经完整落地。

## 代码入口

- `Source/PluginProcessor.*`：JUCE 宿主桥接、MIDI 收集、状态保存。
- `Source/PluginEditor.*`：插件编辑器入口，负责放置主 UI 和 preset 拖拽控件。
- `Source/ui/DirtynthUI.h`：插件界面和参数绑定。
- `Source/ui/PresetComponent.h`：preset 文本文件的拖出保存和拖入载入。
- `Source/dsp/DirtySystem.h`：复音、调制、路由、DSP 调度。
- `Source/dsp/DirtyParams.h`：参数结构、参数注册表、参数显示信息和 preset 字符串读写入口。
- `Source/dsp/Serializer.*`：简单文本序列化读写器，当前用于 preset 文件。
- `Source/dsp/Wavetable.h`：波表振荡器、表生成、mutant 处理。
- `Source/dsp/Filter.h`：滤波器注册表和具体滤波器实现。
- `Source/dsp/Envelope.h`：ADSR 和调制源。
- `EmbeddedTest/Source/main.cpp`：不依赖 JUCE 插件壳的嵌入式/桌面直接测试入口。
- `EmbeddedTest/Source/MidiIO.h`：Windows/ALSA MIDI 输入输出。
- `EmbeddedTest/Source/WaveIO.h`：Windows/ALSA 音频输出。

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

其中交叉编译目标依赖本机对应 toolchain 和 sysroot 路径，脚本里已经写死了作者当前环境的路径。

## 相关笔记

- `blog/02.便宜的数字椭圆滤波器设计/02.便宜的数字椭圆滤波器设计.md`
- `blog/03.高效的ADSR包络算法设计/03final.md`

## 仍需注意

- `Source/dsp/effects` 里已有 delay、chorus、reverb 等效果代码，但当前主合成路径仍以 oscillator、filter、routing、modulation 为核心，效果链不应理解为已经完整接入。
- `EmbeddedTest` 是 Dirtylet 硬件版本的测试入口，不等同于正式产品封装或最终发布形态；其中复制了一套 DSP 源码，后续如果继续发展，需要留意和 `Source/dsp` 的同步关系。
- 当前 README 草稿按源码粗略归纳，不展开过多内部算法细节。
- 作者的话：没钱买token，所以codex写的文章一般般，能读懂就行了。其他更加一言难尽。