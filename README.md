# DesktopMate 桌面伙伴（YNM-3000）— R528 + openvela

> 2026 首届 openvela AI 硬件开发者大赛 ｜ 赛道：**AI 硬件产品创新**（含新硬件平台适配）｜ 专属仓 `contest2026_070_arc`
> 队员：arclau（独立成队，软硬件全栈）｜ 协议：Apache-2.0（见 `LICENSE`）｜ 截止 9.20
> 代码基线：**ok-20260911-21 / V1.0.0-20260911**（`vela.bin`/nsh md5 `e342f7c5`；整包镜像 md5 `08009ea7`，48MB）

> 📷🎬 实拍照片与演示视频（≤5min）随大赛提交材料另行提供，不入本仓。

一句话：**一块利旧的平板主板 + 取自 RK3399 三防平板的 BOE 大屏**，逆向移植、移植点亮后跑 openvela，做成放在桌上天天用的 AI 伙伴——陪伴（电子宠物）× 健康（在座提醒）× 好用（完整桌面 OS）。

---

## 一、作品简介

一块**利旧的平板主板**（Allwinner R528S3-Gemini-S1）+ **取自 RK3399 三防平板的 BOE 1200×1920 MIPI DSI 屏**（横屏 1920×1200，由安卓 DTB 移植点亮）+ GT9271 触摸 + UART 人体存在传感器 + 喇叭，运行 openvela（NuttX），做成桌面 AI 伙伴。

**功能清单（上板可验）**

| 模块 | 功能 |
|---|---|
| 桌面 OS | 状态栏（时间/WiFi/蓝牙/电池）、hero 时钟、天气卡、健康卡、Dock（6 应用）、锁屏待机 |
| 电子宠物 | 12 情绪 × 12 帧（144 帧）动画；触摸/喂食/命名/成长/清档；与真实数据联动；断电持久化 |
| 日历 | 农历 + 二十四节气（太阳黄经逐点校验）+ 纪念日增删 |
| 闹钟 / 时间 | 多闹钟有序插入 + 贪睡 + 音色继承；自动对时总闸 + 无 RTC 落盘兜底 |
| 健康提醒 | LD2410B 毫米波在座检测；喝水 / 久坐分级提醒；每日统计；本地 23 个提示音 |
| AI 对话 | 豆包实时语音（纯按钮 PTT，无唤醒词）+ AI 子页点按快问 + 城市天气设置 |
| 音乐 | XPlayer 本地 MP3/WAV 播放 + 控件 + 提示音/音乐/AI 三路音频仲裁 |
| 其他 | 电子书、文件管理、WiFi/蓝牙设置、UART 调试工具、显示（夜览 / 自动亮度） |

**明确不做**：无语音唤醒词、无毒舌人格、无墨水屏/振动、无手机 App（立项设想已按硬件现实裁剪）。

---

## 二、研发历程（整体进度，2026-07-22 ~ 09-11）

项目不是从"写应用"开始，而是从**点亮一块没有资料的屏**开始——屏不亮则后续一切无从谈起；屏驱动由自研 Skill 从安卓 DTB 移植而来，是典型的"AI 原生 BSP"。全周期约 3 周，**578 次编译打包固化**，全程 AI Coding。

| 阶段 | 时间 | 主要工作 | 关键产出 |
|---|---|---|---|
| ⓪ 起点·点屏 | 07-22~07-28 | 用 `dts-to-vela-mipi` Skill 把 **RK3399 三防平板（1200×1920）的安卓 DTB** 提取/翻译为 R528 MIPI 面板驱动；注册 BOE 面板点亮；修 `Make.defs` 无条件编译 T070 致 `g_lcd0_config` 符号冲突（分辨率被锁 1024×600） | 屏幕点亮、分辨率正确 |
| ① 触摸打通 | 07-24~07-28 | GT9271 I2C 触摸：地址选择时序、零长度写/返回值判据等驱动 bug、外部上拉电阻；坐标上报 | 触摸可用（LVGL 可交互） |
| ② UI 基座 | 08-03~08-09 | 从 LVGL demo 演进为浅色 Dock 桌面；UI 三阶段拆分重构（`deskmate_ui.c` → `ui/` 7 页）；音乐播放链路 | 桌面 OS 骨架 + 音乐 |
| ③ 能力接入 | 08-10~08-23 | WiFi/蓝牙对接；UART 调试工具；LD2410B 在座检测；天气后台；豆包实时语音；宠物初版 | 六大 Dock 应用 + 宠物 |
| ④ 系统化 | 08-24~08-31 | 健康提醒（喝水/久坐）、语音编排、Settings 子页体系、稳定性扫雷 | 健康 + 设置体系 |
| ⑤ 大屏体验 | 09-01~09-09 | 日历（农历/节气）+ 闹钟 + 日期时间对时；宠物 12 态 × 12 帧重做；iPad 风格子页；全量汉化 | 完整产品形态 |
| ⑥ 收尾发布 | 09-10~09-11 | 稳定性/死代码清理、打包防呆、提示音尾音根治、版本型号、日志合规、报告 | **ok-20260911-21 / V1.0.0** |

> 节点细节：`docs/devlog.md`（216 节点）与 `docs/devlogs/`（102 篇，含 7-24~7-28 早期日志 + 逐篇时间线）。

---

## 三、技术亮点（难在哪，怎么证）

**四个硬骨头（均有根因定位，非调参碰运气）**
1. **BOE 大屏逆向移植**：无 datasheet，从 **RK3399 三防平板安卓 DTB** 提取 init 序列/时序/引脚映射；`CONFIG_T070S140B_MIPI` 与 `CONFIG_BOE_1200X1920_MIPI` 必须互斥（`g_lcd0_config` 重定义）；横屏走**驱动层旋转**（LVGL 矩阵旋转与 DIRECT 渲染不兼容，曾 Data Abort）。
2. **开机偶发卡 LOGO**：NuttX `de_dsi.c:dsi_gen_wr()` 的 `while(inst_busy);` 无超时死等；仿 `dsi_dcs_wr`（有界 50 次/5ms + 强清）加界根治。→ 公共仓 PR。
3. **健康提示音"多一声"尾音**：`tone_play_one` 只设 `hw_params` 漏 `sw_params(silence_size)`，短 WAV EOF 后 DMA 欠载重播上一段；补齐后上板验证消失。→ 私仓（App 侧）。
4. **切歌/连播无声**：codec `RDEN OFF` 清零写反 + RAMP FSM 未复位；`POWER_ANA_CTL@0x348` 只许 `update_bits()` 局部操作、**禁止整写**（-7 屏闪教训）。→ 公共仓 PR（仅麦克风关断 POP）。

**硬件设计与适配**：全新硬件平台适配（R528 BSP + 无 datasheet 屏逆向）。驱动/适配：MIPI DSI 面板（新增）、GT9271 触摸、DSI 链路加固、`sun8iw20-codec`（麦克风关断 POP）、LTR553 ALS（积分时间×增益校正）、SD-MMC 多块读修复、UART/LD2410B 换口与引脚冲突。
**选型教训**：早期评估 **6 英寸 2160×1080** 屏不可用——能点亮、纯色正常，但一进 LVGL UI 即扭曲畸变，确认为超 R528 显示链路上限；教训：选屏先确认 SoC 显示上限再投入。

**openvela 能力运用（图形 / AI / 多媒体三项）**：LVGL 9.1（图形）、`packages/ai_agent` + 自研 `dm_ai` WS（AI）、XPlayer + PCM（多媒体）。组件：`nuttx`、`apps/graphics/lvgl`、`packages/ai_agent`、`apps/audio` + XPlayer。
**对 openvela 的改进建议（实测，拟 PR）**：① `de_dsi` gen 写加超时；② LVGL 三处通用 bugfix（GE2D gating / 触摸物理分辨率 clamp / 缺字形占位）+ 文档明示 `lv_color_t` 按色彩格式字节数分配；③ 音频示例统一补 `sw_params`。

---

## 四、目录结构

| 路径 | 作用 | 编译树映射（`contest2026_070_arc.xml`） |
|---|---|---|
| `r528/luncher_dm/` | 桌面主应用（UI+宠物+AI+网络+天气+音乐） | `vendor/allwinnertech/apps/luncher_dm` |
| `r528/deskmate/` | 板级提交配置 defconfig | `.../r528s3-gemini-s1/configs/deskmate` |
| `r528/res_tones/` | 新增提示音 wav（23 个） | `lichee/board/common/data/res/tones` |
| `r528/agent_secrets.h.example` | AI 凭证模板（真文件永不进仓） | 手动 cp 到 `packages/ai_agent/include/agent_secrets.h` |
| `logs/` | **AI Coding 会话日志**（149 会话，见 `logs/README.md`） | 作品提交要求 |
| `skills/` | Skill：`dts-to-vela-mipi` / `deskmate-ui` / `product-designer-ui` / `ai-devlog-system` / `deskmate-agent` / `devlog` / `contest-log-collector` | AI 开发证据 |
| `docs/devlog.md` + `docs/devlogs/`（102 篇） | 节点索引 + 全量开发日志 + 逐篇时间线 | AI 开发过程证据 |
| `docs/specs/`（16 个） | 语音/宠物/WiFi/书籍 设计规格 | AI 参与设计证据 |
| `docs/submission/deliverables/` | 技术报告 / 开发功能-分层 / 开发历程逐篇时间线 | 交付物 |
| `docs/AGENTS.md` · `docs/methodology.md` · `docs/verify_checklist.md` | 会话交接协议 / 排障方法论 / 上板验证清单 | 过程证据 |

---

## 五、运行方式（评委复现清单）

```bash
# 1. 拉工程（openvela 全量源码 + 本专属仓，linkfile 映射到编译树）
repo init -u https://github.com/open-vela/contest2026_070_arc \
  -b dev-ai-contest-2026 -m contest2026_070_arc.xml
repo sync -c -j8

# 2. 填 AI 凭证（豆包语音 + 火山方舟；无 key 时除 AI 对话外全部本地可用）
cp contest2026_070_arc/r528/agent_secrets.h.example \
   packages/ai_agent/include/agent_secrets.h

# 3. 编译（SDK 自带 GCC 13.4.0）
./build.sh vendor/allwinnertech/boards/r528/r528s3-gemini-s1/configs/deskmate -j$(nproc)

# 4. 打包烧录（整包 NAND 镜像，res 分区含 WiFi 固件+字体+提示音）
cd vendor/allwinnertech/lichee && source envsetup.sh && lunch_nuttx 2 && pack
# 产物：out/r528s3/gemini-s1_nand/rtos_nuttx_r528s3-gemini-s1_uart0_128Mnand.img
```

- **演示物料**：TF 卡放 `/sdcard/music`（1 mp3 + 1 wav）与 `/sdcard/book`（UTF-8 短 txt）；板子连 WiFi；雷达对人 ≤1.5m。
- **3 分钟冒烟**：① 开机进 Home（无卡 LOGO）；② 点猫有反应；③ AI 子页快问（无 key 可测本地链路）；④ 坐到传感器前播欢迎；⑤ 音乐播 mp3 无尾音、切歌不哑。
- **排障**：`RTL871X download_fw FAIL status=0x27` = res 分区没完整烧录，重烧整包；`亮度/光感恒 0` = 传感器未接。

---

## 六、AI Coding 使用说明（如实）

- **主力工具 AtomCode**（AtomGit 的 AI 编码助手）**不在大赛官方采集器支持列表内**（官方仅支持 claude-code / opencode / codex / kiro / mimocode / cursor），官方插件无法自动写入 `logs/`。
- 为让评审看到完整过程，本仓用**自研脚本**把 AtomCode 原始会话（`~/.atomcode/sessions/`）归一化为官方事件 schema，`tool` 字段**如实标注为 `atomcode`，不冒充**；同期 OpenCode / Claude Code 属支持工具，由官方 `export-session.py --backfill` 导出（共 **149 会话**：AtomCode 93 / OpenCode 47 / Claude Code 9）。详见 `logs/README.md`。
- 需求拆解/方案设计、UI/驱动/语音编码、疑难调试（Data Abort/越界/矩阵旋转/音频）均与 AI 协作，人工 review + 上板验证；**新增沉淀 4 个开发期 Skill + 2 个运行时 Skill**（`deskmate-agent`/`devlog`）。
- 过程数据：代码 **686,661 行**（业务逻辑 94,245 + 精灵数据 592,416）；**216 个 devlog 节点 / 102 篇**；**578 个固化 tag**。

---

## 七、提交与官方文档

- 提交截止 **9.20**；PR 自行 review 合入；首次贡献签 [CLA](https://openvela.com/#/community/cla)（PR 评论 `/check-cla`）。
- [大赛总览](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/contest_overview.md) ｜ [代码提交指南](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/code_submission_guide.md) ｜ [AI 日志手册](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/ai_coding_log_guide.md)
