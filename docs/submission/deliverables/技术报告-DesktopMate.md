# 2026 首届 openvela AI 硬件开发者大赛 · 技术报告

## 1、信息表

| 项目 | 内容 |
|---|---|
| 作品名称 | DesktopMate 桌面伙伴 |
| 产品型号 | YNM-3000 |
| 队伍名称 | arc（专属仓 `contest2026_070_arc`） |
| 团队分工 | arclau（独立成队）：需求定义 / 硬件设计与焊接 / 用 AI 完成驱动与应用编码 / AI 语音链路 / 上板测试与文档 |
| 选题方向 | AI 硬件产品创新（同时含新硬件平台适配：R528 BSP + 无 datasheet 屏逆向驱动） |
| 硬件平台 | Allwinner R528S3-Gemini-S1 开发板（openvela 大赛赞助）+ 利旧 BOE 屏 |
| 队长 | arclau |
| 开源协议 | Apache 2.0 |

## 2、摘要

DesktopMate 用 openvela 大赛赞助的 R528 开发板与一块从旧平板拆下的 BOE 屏（1200×1920，横屏 1920×1200），在无 datasheet 条件下从安卓 DTB 逆向移植 MIPI 屏与 GT9271 触摸，运行 openvela（NuttX），做成一台常驻桌面的设备：LVGL 9.1 界面，集成电子宠物、日历/闹钟/对时、毫米波雷达健康提醒、豆包实时语音，以及音乐/文件/设置等 6 应用 6 子页。属平台适配与工程集成类项目，产品形态常见，差异主要在旧屏逆向与多能力集成。全程 AI Coding（AtomCode + 跨会话记忆体系），实际有效约 3 周交付约 68.7 万行工程代码、578 次固化，产出 6 个 Skill，用到图形（LVGL）、AI（ai_agent）、多媒体（XPlayer）三项 openvela 能力。

## 3、正文

### 3.1 绪论

**项目背景与问题定义**

本项目属平台适配与工程集成类：在赞助的 R528 开发板上逆向点亮一块没有资料的旧平板屏，再把 AI 语音、健康感知等能力集成成一台可常驻使用的桌面设备。产品形态并不新颖，主要价值在底层打通与集成完整度。项目面向办公桌久坐人群，处理三个具体问题：

- **旧屏再利用**：一块旧平板上拆下的无资料 BOE 屏，用在赞助开发板上，屏零成本（其余仅雷达/喇叭等少量外设）。
- **陪伴反馈弱**：桌面 OS 只有天气、音乐、文件等工具型页面，缺一个持续在场、会随外界事件（有人坐下、喝水达标、AI 对话）变化的角色；本项目用电子宠物承担这层反馈。
- **健康提醒方式生硬**：手机闹钟式提醒易被忽略/打断；改为基于人体存在检测的"在场才提醒、分级递进、不打扰"。

**技术难点**

- **无 datasheet 屏逆向**：用自研 `dts-to-vela-mipi` Skill，从 RK3399 三防平板电脑（1200×1920）的安卓 DTB 提取/翻译 init 序列、时序参数与引脚映射，移植为 R528（openvela/NuttX）MIPI 面板驱动并点亮。
- **横屏大屏驱动**：LVGL 矩阵旋转与 DIRECT 渲染不兼容（曾 Data Abort），最终在驱动层 `degree0` 实现 1920×1200 横屏。
- **GT9271 触摸驱动适配**（难点自 devlog 提取）：I2C 地址由复位时 INT 电平决定（0x5D/0x14），时序错则探测失败；LCD 上电会改写 PB2–PB5 的 pinmux 抢占触摸引脚；R528 TWI 不支持零长度写、combined transfer 成功判据须为 `ret==count-1`；芯片首次读取存在瞬态 NACK（需"失败→重置→重试"）；坐标解析涉及 mask、burst 读偏移、maxpoint、circbuf 采样尺寸一致性；横屏还需坐标变换，并需外接 I2C 上拉（2.2k）与规避 TWI1 引脚冲突。
- **单核 A7 上的大屏性能**：1920×1200 全屏刷新 + 144 帧精灵动画，需控制 RGB 缓冲与刷新策略。
- **音频链路**：短 WAV 尾音、麦克风关断 POP、切歌无声等多类问题的分层定位（App 侧 vs 驱动侧）。
- **弱网/断网可用性**：云端语音不可用时，本地陪伴与健康提醒必须继续工作。

**创新点**

- **硬件层**：旧平板屏逆向移植（MIPI 引脚定义 + 屏 init 序列 + 触摸配置全逆向），在赞助开发板上点亮大屏。
- **产品层**：电子宠物、健康提醒、桌面 OS 融合在同一块大屏上，把 AI 对话、健康状态、在座感知转成可见的宠物反应。
- **工程层**：开发流程为 AGENTS+DevLog 跨会话记忆体系，减少长周期开发中的上下文丢失。
- **起步方式**：项目不是从"写应用"开始，而是从**点亮一块没有资料的屏**开始——屏幕不亮则后续一切无从谈起；屏驱动由自研 Skill 从安卓 DTB 逐项翻译而来。

**研发历程（整体进度，2026-07-22 ~ 09-11）**

项目早期（7-22~7-28）在 `/data/openvela` 工作区完成"从屏到触摸"的底层打通，8 月起迁至 `/data/dm` 进入应用与产品化阶段。全周期实际有效约 3 周，578 次编译打包固化，全程以 AGENTS.md 主导 AI Coding（MiMo / AtomCode / OpenCode）。

| 阶段 | 时间 | 主要工作 | 关键产出 |
|---|---|---|---|
| ⓪ 起点·点屏 | 07-22~07-28 | 用 `dts-to-vela-mipi` Skill 把 **RK3399 三防平板电脑（1200×1920）的安卓 DTB** 提取/翻译为 R528（openvela）MIPI 面板驱动；注册 BOE 面板点亮；修 `Make.defs` 无条件编译 T070 导致的 `g_lcd0_config` 全局符号冲突（分辨率被锁 1024×600） | 屏幕点亮、分辨率 1200×1920 正确 |
| ① 触摸打通 | 07-24~07-28 | GT9271 I2C 触摸：地址选择时序、零长度写/返回值判据等驱动 bug、外部上拉电阻；坐标上报 | 触摸可用（LVGL 可交互） |
| ② UI 基座 | 08-03~08-09 | 从 LVGL demo 演进为浅色 Dock 桌面；UI 三阶段拆分重构（`deskmate_ui.c` → `ui/` 7 页）；音乐播放链路（MP3/切歌/尾音） | 桌面 OS 骨架 + 音乐 |
| ③ 能力接入 | 08-10~08-23 | WiFi/蓝牙对接；UART 调试工具；LD2410B 毫米波在座检测；天气后台；豆包实时语音；宠物初版 | 六大 Dock 应用 + 宠物 |
| ④ 系统化 | 08-24~08-31 | 健康提醒（喝水/久坐）、语音编排、Settings 子页体系、稳定性排查 | 健康 + 设置体系 |
| ⑤ 大屏体验 | 09-01~09-09 | 日历（农历/节气）+ 闹钟 + 日期时间对时；宠物 11 态×12 帧动画重做；iPad 风格子页；全量汉化 | 完整产品形态 |
| ⑥ 收尾发布 | 09-10~09-11 | 稳定性/死代码清理、打包防呆、提示音尾音根治、版本型号（YNM-3000 / V1.0.0）、日志合规、报告 | **ok-20260911-21 / V1.0.0-20260911** |

> 节点细节见随仓 `docs/devlog.md`（P1~P217）与 `docs/devlogs/`（含 7-24~7-28 早期日志）；**逐篇时间线见附录 A**。

**功能清单（最终交付）**

| 模块 | 功能 |
|---|---|
| 桌面 OS | 状态栏（时间/WiFi/蓝牙/电池）、hero 时钟、天气卡、健康卡、Dock（6 应用）、锁屏待机 |
| 电子宠物 | 11 情绪 × 12 帧（+走路 12 帧，共 144 帧）动画；触摸/喂食/命名/成长/清档；与真实数据联动；断电持久化 |
| 日历 | 农历 + 二十四节气（太阳黄经逐点校验）+ 纪念日增删 |
| 闹钟 / 时间 | 多闹钟有序插入 + 贪睡 + 音色继承；自动对时总闸 + 无 RTC 落盘兜底 |
| 健康提醒 | LD2410B 在座检测；喝水 / 久坐分级提醒；每日统计；本地 23 个提示音 |
| AI 对话 | 豆包实时语音（PTT）+ AI 子页点按快问 + 城市天气设置 |
| 音乐 | XPlayer 本地 MP3/WAV 播放 + 控件 + 提示音/音乐/AI 三路音频仲裁 |
| 其他 | 电子书、文件管理、WiFi/蓝牙设置、UART 调试工具、显示（夜览 / 自动亮度）、实体按键截图 |

### 3.2 系统方案设计

**系统总体架构**

```
┌─────────────────────────────────────────────────────────┐
│                  UI 层（LVGL 9.1 / XRGB8888）            │
│  Home（Dock：音乐 / 日历 / 书籍 / 文件 / AI / 设置）        │
│  状态栏/时钟/宠物/天气/健康卡/Dock/锁屏                   │
│  子页：WiFi / BT / 日期时间 / 宠物 / 闹钟 / UART 调试       │
├─────────────────────────────────────────────────────────┤
│              服务层（dm_net / dm_ai / dm_pet / dm_health）│
│  WiFi/蓝牙对接 │ AI WS Client │ 宠物状态机 │ 健康提醒仲裁 │
│  g_conn_gen 代数防竞态 │ 语音编排 │ pet_state 持久化      │
├─────────────────────────────────────────────────────────┤
│                     驱动层（NuttX）                       │
│  RTL8733BS WiFi │ GT9271 Touch │ MIPI DSI │ I2S Codec    │
│  BT Classic     │ LTR553 ALS   │ PWM 背光 │ UART(LD2410B)│
└─────────────────────────────────────────────────────────┘
```

**方案论证与选型**

- **开发板选型**：R528S3-Gemini-S1 大会赞助，屏幕取自 RK3399 三防平板的利旧屏，屏零成本；Cortex-A7 + 128MB 能承载 LVGL 大屏 + 音频，开机约 8s，契合桌面常开设备。
- **端/云职责划分**：端侧负责 UI 渲染、宠物动画、传感器采集、健康计时、本地提示音与音频播放；云端负责 LLM 推理与语音识别/合成（豆包实时语音）。
- **断网/弱网降级**：WiFi 断开时，宠物交互、健康提醒（本地提示音）、音乐（本地文件）、电子书/文件管理等全部可用；仅 AI 语音暂停并明确提示，不崩溃、不死锁。
- **服务端**：AI 链路为团队自建服务端（PC 侧 agent，本地 WS 端口），产品凭证不进代码仓。

**关键模块设计**

- **宠物系统**：11 种情绪状态机（idle/play/eat/sleep/happy/sad/hungry/curious/celebrate/touch/groom），每态 12 帧（+走路 12 帧，共 144 帧），持久化到 `/data/pet_state.json`；行为与真实数据联动（坐下→好奇、喝水达标→庆祝、久坐→低落、AI 对话→开心、夜间 22–6 休眠）。
- **日历/闹钟/时间**：农历 + 二十四节气（太阳黄经 107 点逐点校验全中）+ 纪念日增删；多闹钟有序插入 + 贪睡 + 音色继承；日期时间子页支持自动对时总闸，无 RTC 时以"上次同步时间"落盘兜底。
- **健康提醒**：LD2410B UART 在座检测 → 喝水（3 级递进）+ 久坐（30/60/90min 三级）+ 每日统计；每时段限播防打扰。
- **AI 语音**：豆包实时语音 WebSocket（RFC6455）+ PTT 按键说话 + 流式回复 + 本地显示/播报。
- **音频仲裁**：统一管理提示音/音乐/AI 三路播放，避免抢声卡；短 WAV 补齐 `sw_params(silence_size)` 消除尾音。

### 3.3 核心算法与技术原理

**AI 算法实现**

- **云端模型（产品主链路）**：豆包实时语音大模型，端到端完成 STT（语音转文字）→ LLM 推理 → TTS（文字转语音），经 WebSocket 长连接流式返回；端侧仅做 PCM 采集/播放。
- **MiMo 使用**：开发期通过 AtomCode 接入了 **MiMo-v2.5-Pro** 等模型参与编码（大赛鼓励方向）；产品侧预留多模型路由（`llm-router` 思路），当前语音主链路选用低时延的豆包实时方案。
- **接口与鉴权**：APP ID + Access Token 存于 `packages/ai_agent/include/agent_secrets.h`（已 `.gitignore`，随仓仅提供 `agent_secrets.h.example` 模板）；评委无 key 时，除 AI 对话外全部功能本地可用。
- **端侧处理**：无端侧大模型；端侧负责音频流、状态机、传感器融合与降级决策。

- **计算实现说明**：本项目无自研核心算法，所用计算均为公开/标准算法。唯一成规模的数值计算在日历：节气用低精度太阳黄经法（Schlyter 公式，零数据表），以北京日终点黄经首达目标日定交节，与紫金山天文台 107 个真值点（1998~2036）逐一比对全中；农历为 1900–2099 公有天文数据表 + 自写换算。天气走 open-meteo 公开 API（自写 HTTP/JSON 解析 + canvas 绘制），语音 STT/LLM/TTS 均在云端（豆包）。

**关键机制设计**

- **宠物状态机**：时间驱动转移——饥饿度随时间增长 → HUNGRY → 喂食 → EAT/HAPPY → 回 IDLE；同时接收真实事件（在座/喝水/久坐/AI 对话）作为转移激励。
- **健康提醒仲裁**：喝水与久坐独立计时、三级递进（如"喝口水吧"→"水还没喝呢"→"主人，水都凉啦"），每时段限播，避免打断。
- **WiFi 扫描状态机**：`g_conn_gen` 代数机制防止并发扫描竞态，`g_scan_busy` 标志防重入。
- **语音编排（Voice Director）**：按场景/时段/冷却选择本地提示音播报（坐下欢迎、回来问候），断网仍可用。

- **音频仲裁层（三路收口）**：提示音 / 音乐 / AI 语音共用一块声卡。前台音频（AI TTS 或本地提示音）播放前调 `dm_audio_fg_acquire()` 挂起音乐并释放声卡，播完由主线程 poll 消费完成信号自动恢复。设计受两条实测约束驱动——`XPlayerPause` 只 pause 不关 handle、仍占声卡，`XPlayerStop` 关 sink 后无法续播、恢复须整曲重播；另设 30s 挂起超时，避免无完成信号时音乐迟到复活。收口了提示音/音乐/AI 反复抢声卡的 EBUSY 问题。

**openvela 系统能力运用**

- 用到 **图形 / AI / 多媒体** 三项：

- **图形**：LVGL 9.1（XRGB8888）、Liquid Glass 卡片、自定义图标字体 `deskmate_icons`、驱动层横屏旋转。
- **AI**：`packages/ai_agent`（运行时 Skill 机制）+ 自研 `dm_ai` WebSocket 客户端，接入实时语音对话。
- **多媒体**：XPlayer 音频播放（MP3/WAV）+ PCM 音频流 + codec 配置。
- 涉及组件：`nuttx`（内核）、`apps/graphics/lvgl`（图形）、`packages/ai_agent`（AI）、`apps/audio` + XPlayer（多媒体）。

对 openvela 的优化/改进（已实测；其中**驱动修复已提 PR** → [open-vela/vendor_allwinnertech#20](https://github.com/open-vela/vendor_allwinnertech/pull/20)，含 **BOE 面板 / de_dsi 超时 / ltr553 ALS / codec 麦克风 POP**；下列 2、3 为通用建议，暂未单独提）：

1. **`de_dsi.c` gen 写无超时死等**：`dsi_gen_wr()` 的 `while(inst_busy);` 无界自旋，面板 init 约 30 次调用中偶发不清零即卡 LOGO；仿同文件 `dsi_dcs_wr`（有界 50 次/5ms + 强清）补超时根治，建议对全部 gen 写路径统一加界。
2. **LVGL 三处通用 bugfix**：GE2D gating、触摸物理分辨率 clamp、缺字形占位；并建议文档明示 LVGL 9.1 + XRGB8888 下 `lv_color_t` 实际 3 字节，缓冲分配须按色彩格式字节数而非 `sizeof(lv_color_t)`（曾致 16KB 越界 Data Abort）。
3. **音频链路分层定位经验**：短 WAV 重播"尾音"是 App 侧 `tone_play_one` 漏 `sw_params(silence_size)` 导致 DMA 欠载重播上一段，与驱动关断顺序无关，建议示例代码统一补 `sw_params`；codec 麦克风关断 POP 才是驱动问题（先关 ADC→延时→再断 MICBIAS）。

**关键难点攻坚（按投入规模排序，均有根因定位）**

| # | 难点 | 投入规模 | 根因 → 解决（怎么证） |
|---|---|---|---|
| 1 | **音乐播放链路**：无声 → 切歌无声 → 卡顿 → 尾音 | 8-6~8-8，约 86 次固化（全项目最久） | ①无声=`writei` 帧数当字节返回 + `adecoder` 硬编码 16bit 覆盖 24bit；②MP3 无声=44100 未走已验证的 48000 重采样；③切歌无声=声卡 `ref_count!=0` 未释放 + `RDEN` 清零写反 + **close 不保证 Ramp FSM 复位**（close 清 bit30/28 + `RAMP_SRST`，ok-20260808-12）；④尾音=`tone_play_one` 漏 `sw_params(silence_size)` 致 DMA 欠载重播 → 建**音频仲裁层**收口 |
| 2 | **AI 实时语音链路** | 8-11~8-17，多日（P48~P113） | ai_agent 集成 HTTPS/TLS + LLM；MIC 录音（media 框架未编入→改 `snd_vela_pcm`、开 MIC1、16KB 栈）；**双向流式 TTS 按官方协议重写为 seed-tts-2.0**（X-Api-Key/事件帧/FinishSession）；ASR 响应帧/VAD/削波/WS 缓冲 128KB/TTS 403；尾音=PA 关断排在静音前（改 静音→5ms→关 PA→关 DAC，ok-20260901） |
| 3 | **BOE 大屏逆向移植 + 链路加固** | 8-3~8-6，38 tag | 无 datasheet → 从 **RK3399 三防平板安卓 DTB** 提取 init/时序/引脚；`g_lcd0_config` 互斥；横屏走**驱动层旋转**（LVGL 矩阵旋转与 DIRECT 不兼容，曾 Data Abort）；**卡 LOGO**=`de_dsi.c:dsi_gen_wr()` 无超时死等（加界根治，→公共仓 PR）；**-7 屏闪**=`POWER_ANA_CTL@0x348` 被整写（只许 `update_bits()`） |
| 4 | **GT9271 触摸驱动** | 7-24~7-28，六轮复盘 + 9 次烧录 | I2C 地址时序、零长度写、combined 返回值判据、瞬态 NACK、坐标 mask/burst/maxpoint/circbuf、LCD 抢占 pinmux、外部 2.2k 上拉、TWI1 冲突 |
| 5 | **WiFi `0x27` 三次复发** | 8-11 / 8-12 / 9-11 | 同一症状三种根因，最后实锤=**res 分区没烧完整**（固件校验 fail）→ 催生打包三段断言防呆 |
| 6 | **UART 工具与换口** | 8-26~8-27，32 tag | UART0 与 TF 共用 PF2/PF4；PD21/22 被 `drv_gpio` 覆盖；1.5M 波特率触 apb1 24M 极限失败；换 UART3 后因电平电路迁回 |
| 7 | **电子宠物精灵链路** | 9-6~9-9，48 tag | 11 态 × 12 帧；黑猫事件=**`ar` 归档同名 `_N.o` 不替换**取旧占位帧；G2D 缩放 bug → 预转 300×300；PIL 生成 + 分区扩容 |
| 8 | **LVGL 大屏 UI** | 全程，6 个大版本 | 字体 fallback 链（montserrat 缺字形）；`lv_color_t` 3B 越界 16KB（Data Abort）；flex 覆盖 `set_width`；矩阵旋转不兼容 |

### 3.4 系统实现

**软件/固件架构**

```
luncher_dm.c          ← 应用主入口（自启/开机切 luncher_dm）
├── deskmate_ui.c/.h  ← 共享构建器（卡片体系/子页框架/音乐播放核心）+ 全部宏
├── ui/
│   ├── ui_home.c     ← 主屏（状态栏+时钟+天气/健康卡+音乐组件+dock+锁屏）
│   ├── ui_wifi.c / ui_bt.c     ← WiFi / 蓝牙子页（iPad 风格）
│   ├── ui_music.c / ui_files.c ← 音乐 / 文件管理
│   ├── ui_books.c / ui_settings.c ← 电子书 / 设置
│   ├── ui_calendar.c / ui_alarm.c / ui_datetime.c ← 日历 / 闹钟 / 日期时间
│   ├── ui_pet.c / ui_uart_dbg.c ← 宠物子页 / UART 调试工具
├── dm_pet.c/h + pet_core/view ← 宠物状态机 + 持久化（core/view 三层门面）
├── dm_ai.c/h         ← AI 子页 WS client（豆包实时语音）
├── dm_health.c/h + dm_health_cfg ← 健康提醒（喝水/久坐/统计）+ 配置持久化
├── dm_alarm.c/h      ← 闹钟引擎（有序插入/贪睡/音色继承）
├── dm_city.c/h       ← 城市天气设置
├── dm_screenshot.c/h ← 实体按键截图
├── dm_net.c/h        ← WiFi/蓝牙驱动对接
├── dm_weather.c/h    ← 天气获取（后台 worker + DNS 重试）
├── dm_voice.c/h      ← 语音编排 / TTS 决策
├── dm_als.c/h        ← 光线感应（LTR553）
├── dm_ld2410b.c/h    ← 人体存在传感器（UART1）
└── dm_uart_dbg.c/h   ← UART 调试接收层
```

**数据流与关键流程**

```
AI 语音对话：
  按 PTT → 麦克风采集 PCM → WebSocket 发送豆包 → STT
        → LLM 推理 → 流式 TTS 音频 → 本地播放 + 文字显示

宠物成长：
  开机 → 读 /data/pet_state.json 恢复 → IDLE
       → 时间驱动饥饿度↑ → HUNGRY → 喂食 → EAT→HAPPY → 持久化

健康提醒：
  LD2410B 在座检测 → 计时（喝水/久坐）→ 阈值触发
        → 仲裁（时段限播/分级）→ 本地提示音 + 宠物动作

打包防呆：
  pack 末尾 → check_res 三段断言（res.fex 路径级+字节+新鲜度 /
             nsh.fex==vela.bin / 镜像含完整 res+nsh 块）→ 失败即中断
```

**硬件设计与适配**

**是否完成全新硬件平台适配/驱动开发：是。** 芯片/开发板：Allwinner R528S3-Gemini-S1 开发板（openvela 大赛赞助）+ 利旧 BOE 1200×1920 MIPI DSI 屏。拆U逆向屏幕引脚定义、嘉立创 EDA 画板、PCB 板焊接。

| 驱动/适配 | 类型 | 难点与解决方案 |
|---|---|---|
| BOE 1200×1920 面板 | 新增 MIPI DSI 面板驱动 | 无 datasheet → 从 RK3399 三防平板安卓 DTB 逆向 init 序列/时序/引脚映射；`CONFIG_BOE_1200X1920_MIPI` 与 `CONFIG_T070S140B_MIPI` 互斥编译（防 `g_lcd0_config` 重定义）；驱动层 `degree0` 实现横屏 |
| GT9271 触摸 | 触摸驱动适配 | 横屏坐标变换 + 多点触控；配合 LVGL 物理分辨率 clamp 修复 |
| DSI 链路 | 内核链路加固 | `de_dsi.c` gen 写补超时（卡 LOGO 根治） |
| sun8iw20 codec | 音频驱动修复 | 麦克风关断 POP：先关 ADC→延时→再断 MICBIAS |
| LTR553 ALS | 传感器校正 | 积分时间 × 增益校正因子 + 极暗环境估算 |
| SD-MMC 多块读 | 内核存储修复 | SD 单块读 I/O 瓶颈、多块读 16 必崩 → 恢复 `MMCSD_MULTIBLOCK_LIMIT=0`（见 8-7） |
| UART/LD2410B 换口 | 引脚冲突/换口 | UART0 与 TF 共用 PF2/PF4、PD21/22 被 `drv_gpio` 覆盖；换 UART3 验证后因电平电路迁回 UART1（见 8-27） |

- **外接模块**：LD2410B UART 人体存在传感器（PD21/PD22，mux4）+ 喇叭（I2S）+ GT9271 触摸（I2C）。
- **关键 BOM**：主板（openvela 大赛赞助）+ BOE 屏（利旧复用）+ GT9271（逆向复用）+ LD2410B（~¥15）+ 喇叭（~¥5）+ 屏幕转接板（~¥5）+ 嘉立创免费打样 FPC（¥0）。

**选型翻车教训：6 英寸 2160×1080 MIPI 屏不可用（早期评估）**

> 📷 实拍照片见提交版 Word/PDF（图 1：2160×1080 评估屏试验对照）。

在最终选定 BOE 1200×1920 之前，曾评估一块 **6 英寸 2160×1080 MIPI 屏**：打样了转接板、写好了面板驱动——屏能点亮、纯色色块显示正常，但**一进入 LVGL UI 画面即扭曲畸变**。排查数日后确认为 **R528 显示链路不支持该分辨率（超出 SoC 显示上限）**，与驱动、时序无关。

> **教训**：选屏应先在 SoC 规格上确认显示带宽/分辨率上限，再投入打样与驱动开发；否则即便点亮、纯色正常，GUI 层仍会因带宽/行缓冲不足而失败。

**应用/交互端设计**

- **UI**：LVGL 9.1 浅色 Dock 桌面，状态栏（时间/WiFi/蓝牙/电池）+ hero 时钟 + 天气卡 + 健康卡 + 宠物区 + 6 应用 Dock + 锁屏待机；子页统一 iPad 风格（状态卡/分组列表/开关），全量中文。
- **交互**：触摸为主；语音为纯按钮 PTT（无唤醒词，规避 always-on 麦与 KWS 算力问题）；锁屏无操作自动进入、触摸唤醒。
- **无手机 App**：所有配置（WiFi/蓝牙/城市/宠物名/闹钟）均在设备本地 Settings 完成，降低依赖、便于评委离线验证。

**各子 App：从 UI 到实现**

逐个说明每个子 App 的界面（UI）与落地实现（UI → 服务层 → 驱动）：

| 子 App | UI（页面/交互） | 实现（UI → 服务 → 驱动） |
|---|---|---|
| 主屏 Home | 状态栏（时间/天气/WiFi/蓝牙）+ hero 时钟 + 天气卡 + 健康卡（喝水/久坐双圈）+ 宠物区 + 6 应用 Dock + 锁屏待机 | 共享构建器 `deskmate_ui.c`；`create_status_bar_ex` 供各子页复用；天气 `dm_weather` 后台 worker；健康卡 `dm_health`；锁屏 standby overlay |
| WiFi | iPad 风格：状态卡 / 主开关 / 已连接 / SSID 分组 / 其他网络 / 已保存 / 手动密码 | `dm_net`：扫描后台线程 + `g_scan_busy` 防重入 + `g_conn_gen` 代数防竞态 → RTL8733BS 驱动 |
| 蓝牙 | 状态卡 / 主开关 / Discoverable / MY DEVICES / OTHER DEVICES | `dm_net` 蓝牙接口 → BT Classic/ZBlue；H4 open 110 已定位、挂起未解 |
| 音乐 | 子页播放器（歌单 + 进度拖动）+ HOME/锁屏播放控件 | `dm_sound` / XPlayer：parser 源码编译、三路音频仲裁、进度与曲目记忆 |
| 文件 | 真文件管理器：双根 / 面包屑 / 目录树 / 多选 / 复制剪切粘贴（后台进度环）/ 删除确认 / 新建 / 重命名 | NuttX VFS；无 `system()`（`mkdir(2)` 逐级建、`files_rmrf` 递归删，防 shell 注入）+ 系统目录黑名单；后台复制线程 + 定时器轮询进度 |
| 电子书 | 书架（继续阅读横幅 / 进度徽章）+ 阅读覆盖层 + 翻页 + 进度拖动 + 护眼三档 | `ui_books`：流式分页（每屏只 fread，UTF-8 边界截断）+ TXT 解析 + 进度/护眼持久化 |
| 设置 | 分组列表：WiFi / 蓝牙 / 日期时间 / 闹钟 / 宠物 / 显示 / 存储 / 关于 | `ui_settings` + `dm_health_cfg` 持久化；存储走 statfs 真值；关于区显示版本/型号 |
| 日历 | 月视图 + 农历 + 二十四节气 + 纪念日增删 + 翻月 | `ui_calendar`：太阳黄经节气（107 真值点校验）、农历公有表换算 |
| 闹钟 | 列表 + 新增/编辑 + 贪睡 | `dm_alarm` 引擎：有序插入 + 音色继承 |
| 日期时间 | hero 大时钟 + 自动/手动对时 + 滚轮 | `dm_weather` 对时链路 + 上次同步落盘兜底 |
| 宠物子页 | 状态 / 成长 / 喂食 / 清档 / 命名 | `dm_pet` 门面 + `pet_core`（引擎）/`pet_view`（视图）+ 精灵链路 |
| AI 对话 | AI 子页（PTT 圆钮 + 4 点按快问）+ 点按延录 | `dm_ai` WebSocket + `packages/ai_agent` → 豆包 ASR/TTS |
| UART 调试 | FLOATING 工具栏 + 日志窗 + 5 档波特率 + 3 配色 + 暂停 / HEX 地址 / 丢包统计 | `dm_uart_dbg` 8KB 环形缓冲 + 接收线程 + 共享键盘 `dm_kb_*`；uartdbg_test 注入自测 |

> 每个子 App 开发路径一致：先在共享构建器上搭 UI（卡片/子页/字体策略）→ 接服务层（`dm_*`）→ 对接驱动（WiFi/触摸/音频/UART），再上板验证并迭代。

**自定义 Skill**

开发期沉淀 **4 个**（均随仓提交，`skills/`）：

| Skill | 触发场景 | 核心能力 |
|---|---|---|
| `dts-to-vela-mipi` | 从 Linux DTS 移植 MIPI 屏到 Vela | 自动提取 init 序列/时序/引脚映射并生成驱动 |
| `deskmate-ui` | 1920×1200 浅色大屏 UI 设计 | LVGL 9.x Liquid Glass 卡片 + Dock 布局 + 字体策略 |
| `product-designer-ui` | 产品设计辅助 | 用户需求分析 → 交互方案 → 实现路径 |
| `ai-devlog-system` | 长周期项目 AI 会话记忆 | AGENTS.md + DevLog 节点索引双文件机制 |

**运行时 Skill（`/data/agent/skills/`，本赛道必填项）**：本项目将上述记忆体系进一步沉淀为 **2 个运行时 Skill**，定义为可投放至设备 `/data/agent/skills/` 的 Markdown（`skills/deskmate-agent/SKILL.md`、`skills/devlog/SKILL.md`），设备上的 openvela `ai_agent` 启动时扫描该目录、提取标题与描述注入系统提示词，按语义匹配触发：

| 运行时 Skill | 定义 | 触发场景 |
|---|---|---|
| `deskmate-agent` | 设备自我认知与陪伴/健康场景操作手册：设备事实表（R528/BOE 屏/LD2410B/音频能力）+ 陪伴与健康意图落地路径 + 断网降级规则 | 用户询问"你是什么/能做什么/几点/天气/我还坐着吗"，或表达陪伴/健康意图（喂宠物、提醒喝水、起来走走） |
| `devlog` | 开发日志节点索引与长周期记忆：三层记忆（AGENTS/devlog/子日志）读写规则、节点记法、超限归档 | 询问开发进度/P 号历史结论/某固化版本，或会话结束需要记录开发节点 |

> 落地方式：当前以 `.md` 形式随仓提供（评委可 `push` 到 `/data/agent/skills/` 复核）；未改动 `packages/ai_agent` 内置 C 宏，保持公共仓不受影响。

### 3.5 系统测试与结果分析

**测试环境**

- 硬件：R528S3-Gemini-S1 + BOE 1200×1920 MIPI DSI + GT9271 触摸 + LD2410B 传感器 + 喇叭。
- 固件基线（送审冻结基线，与 README 一致）：固化 tag `ok-20260911-21`（`vela.bin` / nsh.fex md5 `e342f7c5`，17,654,592 字节；整包镜像 md5 `08009ea7`，50,469,888 字节；res.fex md5 `82d2c99e`）。
- 编译：SDK 自带 GCC 13.4.0（`./build.sh <config>`）；烧录：整包 NAND 镜像。

**功能测试**

| 测试项 | 预期结果 | 实际结果 |
|---|---|---|
| 开机进入桌面 | 状态栏+时钟+卡片+dock | ✅ 通过 |
| WiFi 扫描/连接 | 列出 SSID 并连接成功 | ✅ 通过 |
| 蓝牙开关/扫描/配对 | 发现设备并配对 | ✅ 开关/扫描通过；配对受 H4 open 110 挂起限制（见 8-10） |
| 宠物触摸/喂食/命名/成长 | 状态切换 + 持久化 | ✅ 通过 |
| 日历（农历/节气/纪念日） | 正确显示并可增删 | ✅ 通过 |
| 多闹钟 + 贪睡 | 有序触发、贪睡生效 | ✅ 通过 |
| 日期时间对时 | 联网对时；无 RTC 时恢复上次时间 | ✅ 通过 |
| 喝水提醒 | 定时 + 语音播报 | ✅ 通过 |
| 久坐提醒 | 在座检测 + 3 级递进 | ✅ 通过 |
| AI 语音对话 | PTT → 流式回复 → 播报 | ✅ 通过（需填 key；无 key 时本地功能可用、AI 对话暂停并提示） |
| 音乐播放 | MP3/WAV + 进度条 | ✅ 通过 |
| 电子书阅读 | TXT 浏览 + 翻页 | ✅ 通过 |
| 文件管理 | 目录浏览 + 文件操作 | ✅ 通过 |
| 锁屏/待机/唤醒 | 无操作锁屏、触摸唤醒 | ✅ 通过 |
| 断电重启 | 状态持久化恢复 | ✅ 通过 |

> 注：受限项如实列出（蓝牙配对受 H4 挂起限制、AI 需 key），其余 13 项上板通过。

**性能测试**

| 指标 | 实测值 |
|---|---|
| 开机到桌面可用 | ~8s（估计，待精确复测） |
| WiFi 扫描（全信道） | ~3s（估计） |
| AI 语音首字节延迟 | 体感 ~1s 级（含网络，待精确复测） |
| LVGL 全屏刷新 | ~33ms/帧（30fps 设计目标） |
| 宠物动画帧率 | 精灵切帧约 2.8fps（360ms/帧，120ms 节拍 3 分频） |
| 整机功耗 | **2.8W（实测，桌面常亮运行态）** |
| 固件大小（vela.bin） | **16.84 MiB（17,654,592 字节，实测）** |
| 整包镜像 | **48.13 MiB（50,469,888 字节，含 res.fex，实测）** |
| 工程代码量 | **687,129 行**（业务逻辑 94,713 + 精灵数据 592,416，实测，与提交副本一致） |
| AI Coding Token | **≈4.21 亿**（输入 3.94 亿 + 输出 0.26 亿，缓存命中 3.89 亿，实测汇总） |

**可靠性与稳定性测试**

- **在座感知量化指标**（LD2410B，海凌科原厂说明书 V1.0，随仓 `docs/datasheets/`）：24GHz FMCW 毫米波，最远 6m、距离分辨率 0.75m、探测角 ±60°、平均电流 82mA@5V；静止人体（坐卧/微动）可识别，能量值 0~100 与 8 距离门独立可配。
- **误报/漏报兜底（应用层）**：目标能量低于灵敏度阈值判"无人"；2s 无帧过期机制防"离座误判仍在座"；1.5m 近距离门限过滤窗外/邻座干扰；雷达无帧时健康提醒静默（静默降级，不误播报）。
- **内存/越界**：修复 `lv_color_t` 3B 误用致 16KB 越界、文件删除框 `snprintf` 越界、书籍进度越界、AI 127 帧长度截断等，两轮专项排查（P180/P216）崩溃挂死类清零。
- **异常恢复**：WiFi 断连自动重连；AI WebSocket 断开自动清理、下次 PTT 重连；UART/LD2410B 线程 poll 成功必验 `revents&POLLIN`，对端 close 时 `errno` 残留致死 fd 忙等已修（全路径 EOF 置 `ECONNRESET`）。
- **打包防呆**：pack 末尾自动跑三段断言，任一失败即中断——根治"res 分区烧录不完整导致 WiFi 固件校验 fail（0x27）"事故。
- **连续运行**：目标 72 小时长稳（计划中，送审前补 24h+ 连续运行数据）。

### 3.6 AI-Native 开发说明

| 指标 | 数据 |
|---|---|
| AI Coding 代码占比 | 代码由 AI 生成（业务代码约 9.5 万行由 AI 生成初版、约 59.2 万行精灵 C 数组由 AI 工具链脚本生成，合计约 68.7 万行）；人工负责需求定义、硬件设计、上板验证与验收。 |
| 使用的 AI 工具 | **AtomCode（主力）** + OpenCode + Claude Code。⚠️ **AtomCode 不在大赛官方采集器支持列表内**（官方仅支持 claude-code / opencode / codex / kiro / mimocode / cursor），如实说明。 |
| 使用的模型 | AtomCode 内按任务切换：DeepSeek-V4-Flash / **MiMo-v2.5-Pro** / GLM 等（大赛 MiMo 已用于开发） |
| MCP 工具使用情况 | 未使用 MCP（全程用文件/shell/git 等内置工具 + 自建 Skill 弥补领域知识，如实说明） |
| Skills 使用与新增情况 | 使用：官方 openvela Skills（openvela-build / nuttx-driver-development / pcm-audio / memdump / kconfig-tweak 等，按需加载）；**新增沉淀 4 个开发期 Skill**（dts-to-vela-mipi / deskmate-ui / product-designer-ui / ai-devlog-system）+ **2 个运行时 Skill**（deskmate-agent / devlog），均随仓提交 |
| Token 使用总量 | **≈420,804,394**（输入 394,331,973 + 输出 26,472,421；缓存命中 389,133,056）——由 AtomCode 会话 `usage` 字段实测汇总 |

**过程数据**

- AI 会话：**149 个**会话记录随仓 `logs/`（AtomCode 93 + OpenCode 47 + Claude Code 9，覆盖 2026-08-01 ~ 2026-09-11）。
- 固化里程碑：**578 个 git tag**（nuttx/apps/luncher_dm，每次编译打包通过打一个，约对应一次上板验证）。
- 开发日志：**102 篇 devlog / P1–P217 节点索引（约 216 个有效节点）**，全程节点索引可溯（仓 `docs/devlogs/`）。

**AI 对开发效率的提升**

本节对照传统手工开发：本项目代码由 AI 生成，人工负责需求定义、硬件设计与焊接、上板验证与验收；若由一名有经验的嵌入式全栈工程师手工完成同等功能（无资料屏 BSP bring-up + 6 应用桌面 UI + 文件管理器/阅读器 + AI 语音 + 音频仲裁 + 健康/宠物系统 + 全链路调试），保守估计约 3~5 个月（工程经验估计，非实测），而本项目在有效约 3 周内完成。

- 分工模式：AI 生成 UI/业务初版、查 API、写构建与图像脚本；人工负责需求取舍、硬件设计与焊接、驱动对照 datasheet/寄存器手册核验、上板验证与回归。
- 交付与留痕：6 应用 + 宠物/日历/闹钟系统有效约 3 周完成（UI 子页 1~2 天/页）；每次编译打包自动打 tag、更新 devlog，578 次固化可回溯到版本。
- 调试与记忆：Data Abort / 越界 / 矩阵旋转 / 音频尾音等按"现象→多假设→最小验证"与 AI 联合定位；AGENTS+devlog 双文件让约 80 个会话上下文可回溯（已整理为 ai-devlog-system Skill）。

**遇到的问题与解决**

- AI 生成代码需人工 review（驱动对照 datasheet/寄存器手册核验）；144 帧精灵 ≈ 59 万行 C 数组超出单次生成能力，需分批生成 + 脚本；偶有 API 幻觉需查源码。
- 日志说明：前期使用 MiMo Coding 忘开官方插件记录 logs，结果 MiMo 额度用完就续不上了。所以转用主力开发工具 AtomCode，但 AtomCode 并不在大赛官方采集器支持列表内，为让评审看到完整 AI Coding 过程，用本仓脚本 `skills/contest-log-collector/scripts/backfill_atomcode.py` 把 AtomCode 原始会话（`~/.atomcode/sessions/`）归一化为官方事件 schema 后入仓，`tool` 字段如实标注为 `atomcode`，不冒充任何受支持工具。同期 OpenCode / Claude Code 属官方支持工具，由官方 `export-session.py --backfill` 导出（`tool` 标注 `opencode` / `claude-code`）。合计 149 会话（AtomCode 93 / OpenCode 47 / Claude Code 9），`seq` 单调、内容未经人工改写，可审计；官方校验器 `validate-log.py` 的 `tool` 枚举不含 `atomcode`，故 AtomCode 会话会报枚举错误——属"如实标注真实来源"与"官方枚举未覆盖该工具"之间的冲突，不是篡改。详见 `logs/README.md`。

### 3.7 总结与展望

**成果总结**

- 完成旧平板屏的逆向移植，做出一套 LVGL 桌面界面：6 个 Dock 应用 + 6 个嵌套子页 + 电子宠物 + 日历/闹钟/对时 + 健康提醒 + AI 语音（含点按快问/城市天气）。
- 实际有效约 3 周 AI Coding 交付约 68.7 万行工程代码（业务逻辑约 9.5 万 + 精灵数据 59.2 万），578 次固化，沉淀 4 个开发期 Skill + 2 个运行时 Skill。
- 状态持久化可靠（断电重启恢复）；健康感知有量化兜底策略；打包防呆杜绝 res 烧录事故。

**应用前景与商业价值**

- 目标受众：办公桌久坐人群（25~45 岁）、智能家居爱好者、AI 硬件开发者。
- 商业模式与规模化：以利旧屏压低硬件成本，靠宠物/健康提醒等常驻内容形成使用黏性；订阅等增值形式尚未验证，利旧屏来源不稳定、转接板需自行打样，量产可行性待验证。

**不足与未来工作：宠物单角色、健康提醒以时间规则为主、屏幕功耗偏高；后续分别扩展多角色、结合光感/温湿度自适应、探索低功耗面板或自动熄屏。**

## 4、作品照片与演示视频

实拍照片与演示视频见提交版 Word/PDF（报告内图 1~13）；本 MD 仅保留文字内容。

## 5、注意事项

- AI Coding 日志已入仓 `logs/`（149 个会话，2026-08-01 ~ 2026-09-11）；AtomCode 来源已由脚本归一化并如实标注 `tool`。
- 作品原创，遵循 Apache 2.0；公共仓驱动改动**已提 PR** → [open-vela/vendor_allwinnertech#20](https://github.com/open-vela/vendor_allwinnertech/pull/20)。
- openvela 系统能力落地：图形（LVGL）、AI（ai_agent + 语音对话）、多媒体（XPlayer/音频）。
- 语音交互为纯按钮 PTT，无唤醒词（已评估放弃 always-on 唤醒）。

## 附录 A：开发历程逐篇时间线（102 篇，2026-07-24 ~ 09-11）

> 文件名即日期；概要一句话，详情见仓 `docs/devlogs/` 源文件；与 3.1 阶段概览互为「总-分」。

| 日志文件 | 内容概要 |
|---|---|
| `7-24part1devlog.md` | 按 desktop-mate-ui 技能重构 LVGL 布局：Screen 分 Header/Content/Dock 三层，取消全部 set_pos 改 align/flex，新增共… |
| `7-24part2devlog.md` | GT9271 触摸适配：集成 BOE 屏、分辨率改 1200x1920、product_id 读 5 字节兼容 911/927、加 I2C 全寄存器诊断确认通信全通 |
| `7-25part1devlog.md` | GT9271 I2C 六轮调试复盘：经频率/引脚/CCU/GIC 排查后推翻前论，确认 twi_start() 成功（START 已发出），真故障是 TWI 产生 START 却不产生… |
| `7-25part2devlog.md` | GT9271 调试"专家思维模型"文档：给出四层排查法（静态分析→寄存器取证→差异分析→假设验证）与不跳步、一次改一变量等纪律，附第 5/6 轮教训。 |
| `7-25part3devlog.md` | R528/OpenVela SDK 能力手册：汇总 13 项调试能力（CCU 时钟/复位、MMIO 映射、GPIO pinmux、TWI STAT 状态码、GIC IRQ41、NSH … |
| `7-25part4devlog.md` | 给 Gemini 的 GT9271 靶向引导 Prompt：列出已排除假设、正确 GICD 地址、唯一未解问题（TWI 出 START 不出中断），按性价比给出三条方案。 |
| `7-25part5devlog.md` | GT9271 I2C 接力 Prompt：完整交代硬件/调用链/已试修复/排除假设，附第 6 轮烧录纪录表与注意事项 |
| `7-25part6devlog.md` | MiuMiu 施工日志：定位 GT9271 首次 I2C 读取必 NACK 属芯片行为，在 gt911_worker 加"失败→重置→重试"修复并验证 100% 恢复 |
| `7-25part7devlog.md` | MiuMiu 施工逻辑：按 9 次烧录记录，第 2 次 ENG_RES 捕获 msgs_idx=32（0x20 SLA+NACK）定位到芯片层，第 3 次"重置+重试"修复 |
| `7-25part8devlog.md` | MiuMiu 接力 Prompt：说明 GT9271 修复、LVGL 触摸 Demo、NXPlayer 开机播放的已完成项与验证清单，附开机流程、关键文件、已知问题与重编命令。 |
| `7-25part9devlog.md` | MiuMiu 调试思考方式：提出三层定位法（区分驱动/芯片、用诊断捕捉瞬态、接受芯片行为），强调诊断选在 sem_wait 之后才能抓到 msgs_idx=32，记录"上拉非万能、IN… |
| `7-25part10devlog.md` | MiuMiu 开发日志总览：按第 1~9 次烧录汇总 TWI 模式诊断、ENG_RES 突破、重置+重试修复、上拉/延迟实验、诊断清理、LVGL Demo、nxplayer 播放 |
| `7-26part1devlog.md` | LVGL Demo 触摸从完全无响应到坐标正常：7 个 Phase（设备路径、LCD 上电改写 pinmux、坐标 mask、burst 读偏移、maxpoint、circbuf 尺寸… |
| `7-26part2devlog.md` | geminifix：按钮点击失效根因是抬起时 touch_num==0 直接 goto 未发 TOUCH_UP 且缺 npoints/坐标 |
| `7-26part3devlog.md` | velafix-0726 音频/触摸/稳定性：音量统一约 70%，Music Demo 设备路径改 /dev/audio/pcm0p、MP3 改 WAV |
| `7-26part4devlog.md` | velafix-0726 第二阶段：TF 卡 cd_mode 2→3（CARD_ALWAYS_PRESENT）修复挂载 |
| `7-26part5devlog.md` | 7-26 施工逻辑：按 5 次烧录记录触摸修复路径（设备路径与自启、LCD 改写 pinmux 后恢复 PB2-PB5、坐标 mask 与 burst 读对齐、maxpoint/npo… |
| `7-26part6devlog.md` | 7-26 接力 Prompt：汇总 GT9271 通信与坐标解析 5 处修复（pinmux、mask、burst 读、maxpoint、npoints、缩放）、WiFi 日志静音与设备… |
| `7-26part7devlog.md` | 7-26 调试方法论：提出触摸四层定位模型（硬件 I2C/触摸驱动/NuttX 触摸子系统/LVGL），强调 circbuf 写入与读取的 SIZEOF_TOUCH_SAMPLE_S … |
| `7-26part8devlog.md` | VelaDevPower 思维框架 v1.0：从两篇调试文档提炼四层排查引擎、诊断优先清单与插入位置、最薄修复原则与"第一次能行陷阱"，汇成嵌入式底层调试手册。 |
| `7-27part1devlog.md` | TF 卡挂载问题全记录：SMHC0 在 rom_HAL_SDC_PowerOn 卡死，寄存器 dump 显示 SDC0 时钟门控与总线复位从未使能且 hal_writel 写该寄存器不… |
| `7-28devlog.md` | BOE 1200x1920 屏移植：修 Make.defs 对 GEMINI_S1 无条件编译 T070 配置致 g_lcd0_config 符号冲突（分辨率锁 1024x600） |
| `8-3devlog.md` | 移植 BOE 屏（面板/mipi_config）与 GT9271 触摸驱动，明确 mipi_config 二选一互斥、修 r528_boot.c PWM 宏缺 CONFIG_ 前缀、抽… |
| `8-4devlog.md` | luncher 由 luncher_mini 改名独立注册并适配 1920x1200 横屏 |
| `8-4part2devlog.md` | 把 Desktop Mate UI 从 lv_port_linux 的 main.c 搬入 luncher_dm（deskmate_ui.c 约 1877 行），修字体/尺寸溢出、板端… |
| `8-4part3devlog.md` | UI 视觉修整：状态栏/卡片/dock 按 8 英寸 1920x1200 标定 |
| `8-4part4devlog.md` | 写 Python 解析混淆 APK 的 resources.arsc，提取 Flyme 图标包 4066 个 512x512 PNG |
| `8-5devlog.md` | 统一子 APP 设计语言并重写 Music 播放器、重设计 Settings |
| `8-5part2devlog.md` | 按用户决策把全 APP 圆角统一为 RAD_CARD=40px（13 处）并删冗余 RAD_* 宏，抽 subpage_big_title 给 Games/Books/Files/AI… |
| `8-5part3devlog.md` | 切 luncher_dm 开机启动，发现 rcS.nsh 为 prebuilt 增量编译不刷新需 distclean |
| `8-5part4devlog.md` | 修上板与模拟器差异：字号对齐、删 dark 主题 |
| `8-5part8devlog.md` | 音乐播放器集成 XPlayer（mp3/flac/ogg/aac/wav）+真实进度+清单 UI |
| `8-6part1devlog.md` | XPlayer 收尾：启用 flac/pls parser 源码补齐三表缺口并打包固化 |
| `8-6part2devlog.md` | 无声根治：dm_sound_write 误把 writei 帧数当字节返回致数据错位重写，adecoder 第二处硬编码 16bit 覆盖 24bit |
| `8-6part3devlog.md` | MP3 无声根治：aplay 仅验证过 48000 而 44100 链路实际无声，adecoder 把 44100 纳入重采样到 48000 走已验证路径 |
| `8-6part4devlog.md` | 修切歌后无声：snd_pcm_open_substream 每次 open 强制 dapm_state=0 使 codec DAC 重新使能 |
| `8-6part5devlog.md` | 排查卡顿根治：否决重采样移 render 线程 |
| `8-6part6devlog.md` | 切歌无声获决定性证据：切歌后 aplay 再打开报 -16 EBUSY，定位 ksnd_pcm_open 的 ref_count!=0，证明声卡设备未真正释放 |
| `8-6part7devlog.md` | 三版诊断固件逐层排除 codec dapm/PA/DMA/搬运，证数据确实进 DAC |
| `8-7devlog.md` | 清 [dm] 日志洪水修复第一首死机 |
| `8-7part2devlog.md` | 面向外部 AI 的自包含排查报告：数字链路全面排除、aplay 播 WAV 有杂音证物理链路通，结论无声与歌曲强相关 |
| `8-7part3devlog.md` | 切歌无声决战：三次方向修正（歌曲级→XPlayerReset→声卡第二次 open），destroy&recreate 仍无声，mono 混音排除反相，buffer8192 修 XRU… |
| `8-8devlog.md` | 切歌无声软件侧最终排查与硬件验证指导：汇总已铁证排除项，规划任务 A（寄存器 dump/RAMP 复位/强制上下电）、任务 B 万用表 AC 测 LINEOUT 二分、任务 C 诊断收… |
| `8-8part1devlog.md` | A2/A3 证伪且 A3 致 BOOT0 重启回滚 |
| `8-8part2devlog.md` | 终极报告：bit30 为 R/W1C 硬件状态位写不进、模拟电源从未断电 |
| `8-8part3devlog.md` | 音乐控件系列修复：HOME/锁屏点播放补扫歌单并恢复历史曲目、修背光极相反、补 reset_idle_timer、三 UI 按钮按压统一白符号、歌单改紧凑+80% 玻璃 |
| `8-8part4devlog.md` | 修 WiFi SSID 列表空白（扫描未检查返回值致 g_scan_done 恒 0 卡"扫描中"）+Books 点不进（静态 hero 卡无点击事件、空书架无提示） |
| `8-9devlog.md` | 为 Books 加内置公版书与进度/护眼持久化、把 Files 从静态 mock 改为真文件管理器（双根/面包屑/多选/复制删除） |
| `8-9part2devlog.md` | 按 deskmate-ui 规范审核整改 Files/Books/Games：修面包屑点击、长按多选进即退、删除确认、UTF-8 首字符乱码、护眼三档配色、封面重设计，统一玻璃卡片与间… |
| `8-10devlog.md` | 回退被改坏的状态栏，补回子页/锁屏 overlay 的 FLOATING 修复，移植 iPad 风格 WiFi/蓝牙子页（flex 布局防重叠、扫描移后台线程加 g_scan_busy… |
| `8-10part2devlog.md` | 蓝牙 H4 open 110 全景：坐实 UART1 引脚改 PG6-9（mux2）、启用 CRTSCTS、复位 220→1000ms，sync_req 能发但芯片 3s 无 sync… |
| `8-10part3devlog.md` | 根治状态栏：子页用共享 create_status_bar_ex 并让 sbar 作 flex 首子元素钉顶、修 z-order、WiFi/蓝牙图标改开关语义 |
| `8-10part4devlog.md` | 天气链路闭环：WiFi 重连脚本改纯 NSH 语法，天气改惠州、去 NTP ping 前置、fetch 移后台线程并加点击刷新与 debug |
| `8-11part1devlog.md` | NTP 换国内源、加 HTTP Date 头兜底对时（mktime→timegm 修 -8h）与 hero 时钟点击对时，对时链路闭环、WiFi 上板可用 |
| `8-11part2devlog.md` | 集成官方 ai_agent 落地 AI 子页：修 JSON 注入/超时/回复截断/键盘，上板验证 HTTPS/TLS 与 LLM（deepseek-v4-flash-260425），自… |
| `8-11part3devlog.md` | MIC 录音链路修复：media 框架未编入改走 snd_vela_pcm，开 MIC1 通路，清 ANSI 颜色码与调试行，arecord 补 MIC 使能加大栈，rcS 改 ai_… |
| `8-11part4devlog.md` | 命令行实测 MIC 闭环：arecord 录放成功（-c 1 落盘 WAV 160000B 分毫不差、aplay 有声），确认 MICIN1+MBIAS→ADC1 链路与 MIC1 使… |
| `8-12devlog.md` | AI 语音链路大推进：UI 重构（去文本输入、Hero 改玻璃圆圈）、点按延录 4s、DNS IP 兜底、联网判断改 wlan0 IP、WS400 改 volc.bigasr、star… |
| `8-14devlog.md` | 打通双向流式 TTS：旧 V1 接口与账号资源不匹配，按官方 protocols_.py 重写为 seed-tts-2.0（X-Api-Key/事件帧/FinishSession），P… |
| `8-15part1devlog.md` | 语音全链路上板闭环 P73~P92：逐个修 TTS seq flags/WANT_READ 死循环、resp[1024] 截断、ASR batch 提前 break、VAD 参数、-1… |
| `8-15part2devlog.md` | 新会话交接入口：记录上一会话已闭环至 P92（代码 ok-20260815-31，AI 子页布局定稿），全链路均通、小智式交互与健康助理人设就绪 |
| `8-15part3devlog.md` | 健康助理第①层落地：设计定稿后新增 dm_health（接近传感判定在座/离开、喝水久坐双圈双提醒、配置文件 dm_health.cfg），优化开机串行、ai_agent 日志降噪，定… |
| `8-16part1devlog.md` | 健康助理迭代：prox 距离分档校准、离开即暂停倒数、欢迎语每时段限播、合成 12 个本地提示音走 dm_tone_play |
| `8-16part2devlog.md` | 修健康提醒两 bug：喝水/久坐同 tick 弹窗互覆盖致倒计时冻结改为弹窗排队，音乐占声卡时提示音静默改为停音乐语音优先 |
| `8-17part1devlog.md` | start_wifi.sh 提速：删 scan 后与多次 renew 的盲等 sleep，13s→约 5s |
| `8-23part1devlog.md` | 天气同步周期 4h 改 2h 并删 UI 时延 debug 标签（P114） |
| `8-23part2devlog.md` | review-4~review-9 审查修复：修中文数字解析越界读、诊断音乐起播独占声卡致 TTS 静默、表驱动重构、订阅/消费常驻修复锁屏命令失效、日志降噪、清死代码。 |
| `8-23part3devlog.md` | review-10 建音频仲裁层（acquire/release/poll，停音乐→播 TTS→重播恢复）治抢声卡 |
| `8-23part4devlog.md` | 补汉化蓝牙子页三段标题、luncher_dm.c 自绘窗口及 HOME 音乐"No track"（review-13） |
| `8-23part5devlog.md` | P117 用 LD2410B 毫米波雷达经 UART0 应用层直读替代 LTR553 做在座检测，dm_prox_get_cm 改读串口、dm_health 零改动 |
| `8-26part1devlog.md` | P118 上板 LD2410B 零字节排障五连固化：设备名改 /dev/uart0、加配置命令帧、补状态/字节计数日志 |
| `8-27part1devlog.md` | 新增 Settings UART 调试工具子 APP（环形缓冲/5 档波特率/3 配色/不锁屏） |
| `8-27part2devlog.md` | uartdbg_test not found 根因是缺 Make.defs 未被收录，补后须 distclean |
| `8-27part3devlog.md` | P120/121 实证 apb1 锁死 24M、dl=1 为过采样极限，移除 1500000 回归 115200 |
| `8-27part4devlog.md` | P125 锁屏显示 LD2410B 探测距离（有人 x.xm） |
| `8-28part1devlog.md` | P126 排障锁屏人体距离：%.1f 因 LV_USE_FLOAT 未开输出空改 %d.%d、%s 打印致死机、flex 缓存初始宽 52px 截断改显式宽 210 |
| `8-31part1devlog.md` | P129~P135 上板验证多数通过 |
| `9-1part1devlog.md` | 修 AI 语音尾音：根因 PA 关断排在静音前，驱动调序为静音→5ms→关 PA→关 DAC |
| `9-4part1devlog.md` | 补文件复制后台进度条 |
| `9-4part2devlog.md` | 亮度显示问题交接文档：梳理 P153~P158 排障史，确认数据链路全通、寄存器配置正确、有效位为 bit7，唯一未决点=LTR553 CH0 原始值恒 0，给出抓日志的三分支诊断流程… |
| `9-4part3devlog.md` | P159~P163 光感/暗光弹窗全链路闭环：驱动 lux 补积分时间×增益校正+ratio≥0.85 极暗估算 |
| `9-5part1devlog.md` | P164~P169 UART 调试工具：键盘不弹根因 FLOATING，改 BOTTOM_MID |
| `9-5part2devlog.md` | P170~P172：审查发现 WiFi 两处键盘未用 dm_kb_*、destroy 存在悬挂指针，加 is_valid 检查并迁移 |
| `9-5part3devlog.md` | P173~P174 V0.0.7 上板验证：多数通过，P141 尾音仍在，根因 dm_sound_destroy 无 fade-out/drain，补 30ms 静音+drain 修复 |
| `9-5part4devlog.md` | P175 Voice Director 听感修复：时段问候未引用、10min 冷却一刀切、候选含未合成空票，改为并入时段问候+back_cooldown_s=180+剔空票+hour<… |
| `9-5part5devlog.md` | P180 全量 BUG 修复：两轮审查筛 14 项并修复——高危 strcpy 栈溢出/缺 \0/system 命令注入改递归删除 |
| `9-6part2devlog.md` | P182~P187 电子宠物：口口/全透明/挤压根因纯拉丁字体、像素全 0、缺 FLOATING |
| `9-7part1devlog.md` | P188 模拟器移植：把板端 UI 源码复制到 /data/lv_port_linux 并为 7 个服务建 stub，更新 CMake、修编译错误 |
| `9-7part2devlog.md` | P189/P190 胖橘 96 帧：cat.png 切 96 张转 300×300 ARGB 规避 G2D 缩放 bug、扩至 8 帧 |
| `9-08part1devlog.md` | P197 宠物精灵收尾：重写 cut_sprite.py 自动 gutter 检测（12 行×8 列）消除切帧错位，删 PET_STATE_SLEEPY 统一 11 态、镜像翻转为负 … |
| `9-08part2devlog.md` | P198 修复开机概率卡 LOGO：定位 NuttX DSI 驱动 dsi_gen_wr() 无超时 spin，参照 dsi_dcs_wr() 加 50 次 delay+超时清 ins… |
| `9-08part3devlog.md` | 一次性修三问题：启动慢因 env.cfg 无长度参数整读 50MB 分区（两处加 2000000） |
| `9-09part1devlog.md` | 用 cat12.png 重切 128×128 精灵（144 张）并补到 16 帧，改 PET_FRAMES_PER_STATE=16、修 g_tick/2 动画周期 |
| `9-09part2devlog.md` | P201~P205 宠物极简重构：直连 cat_sprites 原生 12 帧并清构建污染，pet 拆成 core/view 三层门面（public API 零改），修命名 overl… |
| `9-09part3devlog.md` | P206 交作业启动：清理 SDK 1.9G+冗余素材、新建 deskmate 提交配置、clone 专属仓 contest2026_070_arc 并设计 linkfile 映射 |
| `9-10part1devlog.md` | P207 全应用审计修复包：快问改 4 点按、城市天气 A 方案、蓝牙/WiFi/音乐/UART/文件多 bug 修复、音量亮度落盘、去 Games 与报告/README 去虚 |
| `9-10part2devlog.md` | P208 验证收绿 11 项后新增日历与闹钟：ui_calendar 月视图+农历公有表+节日 |
| `9-10part3devlog.md` | P209：用户质疑 WiFi 配置没打进包，二进制抠出 usrdata.fex 内 wifi-home 配置自证包无问题，确认 lunch 2 与 vela.bin==nsh.fex |
| `9-10part4devlog.md` | P210 闹钟与日历返工：修 snooze 铃声栈变量未初始化、同分钟多闹钟只响首个、重复声明三真 bug |
| `9-10part5devlog.md` | P211 新建日期与时间独立子页（ui_datetime.c）：iOS 同款 hero 大时钟+自动/手动两区，总闸 dm_time_auto_enabled 真开关、上次同步记录，滚… |
| `9-11part1devlog.md` | P212~P216：911 bootlog WiFi 失败实为 res 分区未完整烧录（重打整包） |
| `9-11part2devlog.md` | P217 发布 V1.0.0：版本改 V1.0.0-20260911、型号 YNM-3000、删除假天气兜底 |
