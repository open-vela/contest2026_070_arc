# DesktopMate 桌面伙伴（R528 + openvela）

> 2026 首届 openvela AI 硬件开发者大赛 ｜ 赛道：AI 硬件产品创新 ｜ 专属仓 `contest2026_070_arc`
> 队员：arclau（独立成队，软硬件全栈）｜ 协议：Apache 2.0 ｜ 截止 9.20
> 代码基线：**ok-20260911-21 / V1.0.0-20260911**（nsh md5 `e342f7c5` / 整包镜像 md5 `08009ea7`，48MB）

> 📷🎬 实拍照片与演示视频（≤5min）随大赛提交材料另行提供，不入本仓。

一句话：**一块零成本捡回来的平板主板，逆向点亮 MIPI 大屏，做成放在桌上天天用的 AI 伙伴**——陪伴（电子宠物）× 健康（在座提醒）× 好用（完整桌面 OS）。

## 一、作品简介（当前设备，以此为准）

一块"捡回来"的平板主板（Allwinner R528S3-Gemini-S1）+ 逆向点亮的 BOE 1200×1920 MIPI DSI 屏（横屏 1920×1200）+ GT9271 触摸 + UART 人体存在传感器 + 喇叭，跑 openvela，做成桌面 AI 伙伴。

**实际功能（上板可验，不画饼）**：
- **桌面 OS**：LVGL 浅色 Dock 桌面（状态栏/时钟/卡片/dock/锁屏），6 个 Dock 应用：音乐、日历、书籍、文件、AI 对话、设置（+锁屏），+ 6 个嵌套设置页：WiFi、蓝牙、日期时间、闹钟、宠物、UART 调试工具。
- **电子宠物**：12 种情绪状态、144 帧精灵动画，触摸/喂食/命名/成长/断电保持；独立宠物子页（状态/成长/喂食/清档重养）；行为与真实数据联动（坐下→好奇、喝水达标→庆祝、久坐→低落、AI 对话→开心，夜间 22–6 点休眠）。
- **日历/闹钟/时间**：日历含农历+二十四节气（太阳黄经 107 点全中）+纪念日增删；闹钟多闹钟有序插入+贪睡（音色继承）；日期时间独立子页（自动对时总闸+上次同步时间落盘兜底，无 RTC 恢复上次时间）。
- **健康提醒**：LD2410B 毫米波人体在座检测 → 喝水提醒（高温自动加密）+ 久坐 30/60/90min 三级提醒 + 每日统计；本地 23 个提示音直播（不断网可用）。
- **AI 对话**：豆包实时语音（纯按钮 PTT，无唤醒词）+ AI 子页 4 点按快问（无麦可测）+ 城市天气设置（无 GPS，手动设城市，语音/天气卡共用）。
- **语音编排**：Voice Director 按场景/时段/冷却选本地提示音播报（坐下欢迎、回来问候）。

**明确不做的（立项时想要，已裁剪，原因见二）**：
无语音唤醒词、无毒舌人格、无墨水屏/振动马达、无手机 App、无 MiMo 全程接入（云端用豆包/火山方舟）。

## 二、路线说明（立项 → 现设备）

立项源于"牛马健康助理"（86 盒 + 墨水屏 + 振动 + 手机 App + MiMo 全程，见源仓 `project_docs/homework/xuqiu.txt` 存档）。实际执行中按硬件现实收敛到 R528 大屏桌面形态：

| 立项 | 现状 | 原因（一句话） |
|---|---|---|
| 86 盒墨水屏 + 振动 | R528 + BOE 大屏，无振动 | 手头废弃平板主板零成本，大屏交互演示更直接 |
| MiMo 全程（flash/tts/pro） | 豆包实时语音 + 火山方舟 | 实时语音链路先调通豆包，MiMo provider 待后续迁移 |
| 唤醒词呼叫 | 纯按钮 PTT | 无 always-on 麦 + 无 KWS 算力，云端轮询已实测不可行；路线图为外挂 KWS 前端 |
| 毒舌人格/风格切换 | 中性关怀话术 | 单人窗口内保演示主链路，人格化后移 |
| 手机 App | 无（Settings 本地配置） | 单人 10 天窗口，只保端侧完整可验 |

### 研发历程（216 个 devlog 节点 / 79 篇日志 / 578 次编译打包固化）

> 每次编译打包成功打一个固化 tag（`ok-YYYYMMDD-N`），基本对应一次刷机上板验证。以下按血泪浓度排序：

| 战场 | 节点 | 反复次数 | 代表性血泪 |
|---|---|---|---|
| 🔊 声音/切歌 | 8-6~8-8 三天 **86 个 tag**，全项目最惨烈 | 三轮大战 | MP3 无声（44100→48000 重采样）→ 切歌无声 7 连（EBUSY 抢声卡→codec RDEN 清零写反+RAMP FSM 复位）→ 音频仲裁层根治 → 尾音两轮（驱动关断顺序是假凶，真凶是 App 侧漏 `sw_params`）→ 麦克风关断 POP |
| 🖥️ 屏幕触摸 | 8-3~8-6，38 tag + 两轮回马枪 | 逆向点亮 | BOE 屏无 datasheet，从 Android DTS 提 init 序列；mipi_config 全局符号互斥坑；后遇卡 LOGO（DSI 死等无超时）和 -7 屏闪（电源寄存器整写）两轮回马枪 |
| 🐱 电子宠物 | 9-6~9-9，48 tag | 三轮重做 | 黑猫事件（ar 归档污染：同名 C 追加 `_N.o` 永不替换，链接器取旧灰黑占位帧）→ 尺寸/走失/起名卡死 → 像素画→12 态×12 帧精灵链路全量重做 |
| 📡 UART | 8-26~8-27，32 tag | 方向错了重来 | 排障两天发现 P118 误判——以为的 UART 引脚其实是 TF 卡（PF2/PF4=SDC0）→ 换口 UART1 → 引脚又被 IO 扩展器覆盖 → 1.5M 档波特率极限失败收场 |
| 📶 WiFi 0x27 | 8-11 / 8-12 / 9-11 三次复发 | 同一症状三种根因 | 最后一次才实锤=res 分区没烧完整（固件校验 fail），由此催生打包三段断言防呆 |
| 🎨 UI 改版 | 贯穿全程 | 6 个大版本 | 模拟器免刷机工作流 → UI 架构三阶段拆分重构 → V0.0.1~V0.0.7 → iPad 风格子页 → 全量汉化 → 日历/闹钟返工（9-10 单日 44 tag，历史峰值） |
| 💥 崩溃挂死 | 全程散布 | 两轮集中扫雷 | `lv_color_t` 3B 越界 16KB（第一次 Data Abort）→ LVGL 矩阵旋转死机 → 栈溢出 → 两轮审计清零（一轮 14 项 + 一轮 6 项） |

## 三、技术亮点（嵌入式视角：难在哪，怎么证）

分层（一句话）：LVGL UI 层（`ui/` 7 页拆分）→ 服务层（`dm_net`/`dm_ai`/`dm_pet`/`dm_health`）→ 驱动层（MIPI DSI/GT9271/codec/LD2410B）。UI 只经服务层 API 碰驱动，驱动回调里禁碰 LVGL。

**四个硬骨头（均有根因定位，非调参碰运气）**：
1. **BOE 大屏逆向点亮**：无 datasheet，从 Android DTS 提取 init 序列/时序/引脚映射；`CONFIG_T070S140B_MIPI` 与 `CONFIG_BOE_1200X1920_MIPI` 互斥（`g_lcd0_config` 重定义），开机不加载即查此。
2. **开机偶发卡 LOGO**：NuttX 显示链路 `de_dsi.c:dsi_gen_wr()` 的 `while(inst_busy);` 无超时死等（面板 init 约 30 次调用，偶发不清零即 spin 住）；仿同文件 `dsi_dcs_wr`（50 次/5ms+强清）加界，根治。→ 公共仓 PR。
3. **健康提示音"多一声"尾音**：`tone_play_one` 只设 `hw_params` 漏 `sw_params`，短 WAV EOF 后 DMA 欠载重播上一段；补 `silence_size=boundary` 等与音乐/AI 路径一致的参数，上板验证消失。→ 私仓（App 侧）。
4. **切歌/连播无声**：codec `RDEN OFF` 清零写反 + RAMP FSM 未复位（OFF 清 RDEN + W1C 中断 + RAMP_SRST bit24）；`POWER_ANA_CTL@0x348` 只许 `update_bits()` 局部操作，禁整写（屏闪教训）。→ 公共仓 PR（仅麦克风关断 POP 一项，播放关断顺序已排除）。

**工程质量（P180/P216 两轮扫雷，崩溃挂死类清零）**：
- UAF：子页关闭清空全部 UI 指针（含 playlist overlay/empty_lbl，8-08/9-11 两次血案）。
- 越界：文件删除确认框 snprintf、书籍进度、AI 127 帧 64bit 长度（`(int)` 截断变负→改 `size_t` 比较+超长截断收）。
- 挂死：AI 对端 close 时 `errno` 残留 EAGAIN 致死 fd 忙等（全路径 EOF 置 `ECONNRESET`）；UART/LD2410B 线程 poll 成功必验 `revents&POLLIN`。
- 数据：闹钟旧 4 字段档按行解析（fscanf 跨行偷数致整档错位）；宠物名 JSON 转义防丢档。
- 打包防呆：`pack` 末尾自动跑三段断言（res 路径级+字节/新鲜度、`nsh.fex==vela.bin`、镜像含完整块），失败即中断。

**公共仓 PR（另提，不在本仓）**：`sun8iw20-codec` 麦克风关断 POP、`de_dsi` 超时、`BOE_1200x1920` 面板、`ltr553` ALS 校正、LVGL 三 fix（GE2D gating/触摸物理分辨率 clamp/缺字形占位）→ `dev-ai-contest-2026`。

## 四、目录结构

| 路径 | 作用 | 编译树映射（contest2026_070_arc.xml） |
|---|---|---|
| `r528/luncher_dm/` | 桌面主应用（UI+宠物+AI+网络+天气+音乐，65M，已同步 ok-20260911-21 源码，构建产物已清） | `vendor/allwinnertech/apps/luncher_dm` |
| `r528/deskmate/` | 板级提交配置 defconfig | `.../r528s3-gemini-s1/configs/deskmate` |
| `r528/res_tones/` | 新增提示音 wav（23 个，4.6M） | `lichee/board/common/data/res/tones` |
| `r528/agent_secrets.h.example` | AI 凭证模板（真文件永不进仓） | 手动 cp 到 `packages/ai_agent/include/agent_secrets.h` |
| `skills/dts-to-vela-mipi/` | MIPI 屏逆向移植 Skill | 方法论，可复用 |
| `skills/deskmate-ui/` | LVGL 大屏 UI 设计 Skill | 方法论，可复用 |
| `skills/product-designer-ui/` | 产品设计 Skill | 方法论，可复用 |
| `skills/ai-devlog-system/` | **AI 长周期记忆体系 Skill**（AGENTS+DevLog 双文件机制蒸馏） | 方法论，可复用 |
| `skills/contest-log-collector/` | AI 日志归集 Skill（官方） | 日志采集用 |
| `docs/AGENTS.md` | **AI 会话交接协议实例**（3 周全程使用，评委导读头） | AI 开发过程证据 |
| `docs/devlog.md` + `docs/devlogs/`（102 篇） | **节点索引 + 全量开发日志**（216 节点，已脱敏） | AI 开发过程证据 |
| `docs/specs/`（16 个） | 语音/宠物/WiFi/书籍 设计规格（AI 参与设计的产物） | AI 开发过程证据 |
| `docs/methodology.md` | 排障方法论（10 条，AI 与人共同沉淀） | 方法论沉淀 |
| `docs/verify_checklist.md` | 上板验证清单（R/V 逐项勾选记录） | 验证纪律证据 |
| `logs/` | AI Coding 会话日志 | 按日志手册导出后提交 |

公共仓改动（不在本仓，另提 PR 到 `dev-ai-contest-2026`）：codec 麦克风关断 POP + DSI 超时 + BOE 面板 + ALS 校正 + LVGL 三处通用 bugfix（GE2D gating/触摸物理分辨率 clamp/缺字形占位）。

## 五、运行方式（评委复现清单）

```bash
# 1. 拉工程
repo init -u https://github.com/open-vela/contest2026_070_arc \
  -b dev-ai-contest-2026 -m contest2026_070_arc.xml
repo sync -c -j8

# 2. 填 AI 凭证（豆包语音 + 火山方舟）
cp contest2026_070_arc/r528/agent_secrets.h.example \
   packages/ai_agent/include/agent_secrets.h
# 用火山引擎控制台的 APP ID / Token / Ark key 填进去；
# 真文件已被 .gitignore 忽略，不会进 Git 历史。
# 评委无 key 时：除 AI 对话外全部功能本地可用（宠物/健康/音乐/文件/电子书/设置）。

# 3. 编译（SDK 自带 GCC 13.4.0，build.sh 自动注入）
./build.sh vendor/allwinnertech/boards/r528/r528s3-gemini-s1/configs/deskmate -j$(nproc)

# 4. 打包烧录（整包镜像，res 分区含 WiFi 固件+字体+提示音）
cd vendor/allwinnertech/lichee && source envsetup.sh && lunch_nuttx 2 && pack
# 产物：out/r528s3/gemini-s1_nand/rtos_nuttx_r528s3-gemini-s1_uart0_128Mnand.img
# 打包后必跑资源检查（见源仓 check_res.sh 思路：res.fex 路径级验证）
```

硬件：R528S3-Gemini-S1 + BOE 1200×1920 MIPI DSI（横屏 1920×1200）+ GT9271 触摸 + LD2410B 人体存在传感器（/dev/uart3）+ 喇叭。

**演示物料（缺了不影响开机，但对应功能测不了）**：
- TF 卡：1 个 mp3 + 1 个 wav 到 `/sdcard/music`，1 本 UTF-8 短 txt 到 `/sdcard/book`。
- 联网：板子连 WiFi（AI 对话/天气需要；宠物/健康/音乐/文件/电子书/设置全本地可用）。
- 雷达对人 ≤1.5m（在座检测/问候/提醒链路）。

**预期结果（3 分钟冒烟）**：
1. 开机进 Home：时钟/天气卡/宠物猫正常显示，无卡 LOGO。
2. 点猫：大小不变、有抚摸反应；长按只抚摸不弹条。
3. 进 AI 子页点快问（无 key 可测本地链路）；有 key 时按 PTT 说话，松开后喇叭播报。
4. 坐到传感器前：播坐下欢迎；离开再回：时段问候。
5. 进音乐播 mp3：立体声、无尾音；切歌不哑。

**排障一句**：`RTL871X download_fw FAIL status=0x27` = res 分区没完整烧录，重烧整包镜像；`亮度/光感恒 0` = 传感器积灰或未接，先查 `grep LTR553`。

## 六、AI Coding 使用说明

- 需求拆解/方案设计：与 AI 对话定 Dock 布局、子页信息架构、宠物成长数值。
- 编码：UI 子页、驱动对接、语音链路均由 AI 生成初版，人工 review 上板验证。
- 调试：Data Abort/越界/矩阵旋转等硬骨头按"现象→多假设→最小验证"与 AI 联合定位。
- 效率：6 应用 + 宠物/日历/闹钟系统约 3 周交付；沉淀 Skill 3 个，换屏/换 UI 风格可直接复用。
- 完整对话日志见 `logs/` 目录。

## 七、官方文档

- [大赛总览](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/contest_overview.md) ｜ [代码提交指南](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/code_submission_guide.md) ｜ [AI 日志手册](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/ai_coding_log_guide.md) ｜ [AI 硬件赛道导航](https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/ai_hardware/ai_hardware_guide_index.md)
- 提交截止 9.20；PR 自行合入；首次贡献签 [CLA](https://openvela.com/#/community/cla)（PR 评论 `/check-cla`）。
