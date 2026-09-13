/*
 * ui_books.c — 书架 + 阅读器子页【Phase 1 拆分】
 *
 * 2026-08-09 从 deskmate_ui.c 原样搬入：create_books_subpage（改名
 * ui_books_create）+ book_* 全部（书架/阅读器/进度/护眼持久化 3 文件）。
 * 纯 UI+轻 IO，无驱动依赖。逻辑一字不改。
 *
 * 红线（勿碰）：
 *   - book_reader_overlay 挂在 subpage_overlay 上，close_subpage 经
 *     book_reader_close() 清理（对象+清理一起搬，禁止拆散）。
 *   - 持久化文件 /data/book_progress.conf /data/book_theme.conf 格式
 *     与既有数据兼容，不得改动。
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include <lvgl/lvgl.h>

#include "deskmate_ui.h"

/* 前向声明（ui_books_create 使用 book_card_click_cb；book_open_reader 声明
 * 在 deskmate_ui.h，deskmate_ui.c files_open_file 也引用） */
static void book_card_click_cb(lv_event_t *e);
static void book_first_char(const char *s, char out[8]);   /* P0-1：UTF-8 首字符 */
static void book_toast_show(const char *msg);              /* P1-5：档位提示 toast */

/* ================================================================
 * BOOKS 子 APP（电子书阅读器）— books-reader-solution.md 重设计
 * 书架扫描 /mnt/sdcard/book（TF卡）+ /data/book（内置），流式分页阅读，
 * 避免大 txt 整本载入 LVGL 造成 OOM。
 * Apple 级高定 UI：毛玻璃卡片 + 柔和阴影 + 极简排版 + 微动画
 *
 * 常量 DM_MAX_BOOKS / DM_BOOK_DIR1/2/3 已上移到 deskmate_ui.h
 * （deskmate_ui.c 的 files_open_file 也引用，Phase 1 拆分后单一来源）。
 * ================================================================ */

/* 书架/阅读器共享状态（Phase 1 拆分：deskmate_ui.c files_open_file 引用，
 * deskmate_ui.h 有 extern 声明） */
char     dm_book_paths[DM_MAX_BOOKS][160];
char     dm_book_titles[DM_MAX_BOOKS][64];
uint32_t dm_book_count;

/* 阅读器状态（流式分页，只 fread 一屏） */
static lv_obj_t *book_reader_overlay;   /* 全屏阅读器覆盖层 */
static lv_obj_t *book_reader_text;      /* 正文 label */
static lv_obj_t *book_reader_title_lbl; /* 顶部书名 */
static lv_obj_t *book_reader_prog;      /* 底部百分比 */
static lv_obj_t *book_reader_page_lbl;  /* 底部页码 x / y */
static lv_obj_t *book_reader_slider;    /* 底部进度条（可拖动跳页） */
static lv_obj_t *book_reader_bar;       /* P0-2：底部控件条（护眼配色随档位） */
static lv_obj_t *book_reader_back_btn;  /* P0-2：顶部返回按钮（护眼配色） */
static lv_obj_t *book_reader_eye_btn;   /* P0-2：顶部护眼按钮（护眼配色） */
static lv_obj_t *book_reader_scroll;    /* P1-3：正文滚动容器（翻页后回顶） */
static lv_obj_t *book_toast;            /* P1-5：档位提示 toast */
static lv_timer_t *book_toast_timer;    /* toast 自动隐藏定时器 */
static int       book_cur_idx;          /* 当前阅读书籍索引 */
static long      book_offset;           /* 文件偏移（分页游标） */
static long      book_file_size;        /* 文件总字节 */
static int       g_eye_care_mode;       /* 护眼模式（0=白/1=米/2=黑） */
static int       g_book_slider_guard;   /* 防 set_value 触发 VALUE_CHANGED 递归 */
static uint32_t  book_slider_last_tick; /* P2-8：slider 拖动节流时间戳 */

#define BOOK_PROGRESS_FILE "/data/book_progress.conf"
#define BOOK_THEME_FILE    "/data/book_theme.conf"

#define BOOK_PAGE_CHARS 1400   /* 每屏大约读取字节数（UTF-8 边界截断） */

/* 扫描 /data/book + /sdcard/book + /mnt/sdcard/book 下 .txt 文件
 * 返回数量（去重）。优先顺序：内置 > TF卡短路径 > TF卡标准路径 */
static uint32_t book_scan_directory(void)
{
    DIR *dir;
    struct dirent *ent;
    uint32_t count = 0;
    char seen[DM_MAX_BOOKS][64] = {0};

    dm_book_count = 0;

    /* 扫描三个目录，合并去重（按文件名） */
    const char *dirs[3] = {DM_BOOK_DIR1, DM_BOOK_DIR2, DM_BOOK_DIR3};
    for (int d = 0; d < 3; d++) {
        dir = opendir(dirs[d]);
        if (dir == NULL)
            continue;

        while ((ent = readdir(dir)) != NULL && count < DM_MAX_BOOKS) {
            const char *dot = strrchr(ent->d_name, '.');
            if (dot == NULL || dot == ent->d_name)
                continue;
            if (strcasecmp(dot, ".txt") != 0)
                continue;

            /* 去重：同名文件只保留第一个（优先顺序见上） */
            int dup = 0;
            for (int i = 0; i < count; i++) {
                if (strcmp(seen[i], ent->d_name) == 0) {
                    dup = 1;
                    break;
                }
            }
            if (dup)
                continue;

            strncpy(seen[count], ent->d_name, sizeof(seen[0]) - 1);
            seen[count][sizeof(seen[0]) - 1] = '\0';
            snprintf(dm_book_paths[count], sizeof(dm_book_paths[count]),
                     "%s/%s", dirs[d], ent->d_name);
            snprintf(dm_book_titles[count], sizeof(dm_book_titles[count]),
                     "%.*s", (int)(dot - ent->d_name), ent->d_name);
            count++;
        }
        closedir(dir);
    }

    dm_book_count = count;
    return count;
}

/* 进度持久化：读取 */
static void book_progress_load(int idx)
{
    if (idx < 0 || idx >= (int)dm_book_count)
        return;

    FILE *f = fopen(BOOK_PROGRESS_FILE, "r");
    if (!f)
        return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char path[160];
        long offset;
        if (sscanf(line, "%159[^:]:%ld", path, &offset) == 2) {
            if (strcmp(path, dm_book_paths[idx]) == 0) {
                book_offset = offset;
                break;
            }
        }
    }
    fclose(f);
}

/* 进度持久化：保存 */
static void book_progress_save(int idx)
{
    if (idx < 0 || idx >= (int)dm_book_count)
        return;

    /* 读取现有内容，更新对应行 */
    char lines[DM_MAX_BOOKS][256];
    int line_count = 0;
    FILE *f = fopen(BOOK_PROGRESS_FILE, "r");
    if (f) {
        while (line_count < DM_MAX_BOOKS &&
               fgets(lines[line_count], sizeof(lines[0]), f))
            line_count++;
        fclose(f);
    }

    int found = 0;
    for (int i = 0; i < line_count; i++) {
        char path[160];
        if (sscanf(lines[i], "%159[^:]:", path) == 1 &&
            strcmp(path, dm_book_paths[idx]) == 0) {
            snprintf(lines[i], sizeof(lines[i]), "%s:%ld\n",
                     dm_book_paths[idx], book_offset);
            found = 1;
            break;
        }
    }
    if (!found && line_count < DM_MAX_BOOKS) {
        snprintf(lines[line_count], sizeof(lines[0]), "%s:%ld\n",
                 dm_book_paths[idx], book_offset);
        line_count++;
    }

    f = fopen(BOOK_PROGRESS_FILE, "w");
    if (f) {
        for (int i = 0; i < line_count; i++)
            fputs(lines[i], f);
        fclose(f);
    }
}

/* 护眼模式持久化：读取 */
static void book_theme_load(void)
{
    FILE *f = fopen(BOOK_THEME_FILE, "r");
    if (f) {
        fscanf(f, "%d", &g_eye_care_mode);
        if (g_eye_care_mode < 0 || g_eye_care_mode > 2)
            g_eye_care_mode = 0;
        fclose(f);
    } else {
        g_eye_care_mode = 0;
    }
}

/* 护眼模式持久化：保存 */
static void book_theme_save(void)
{
    FILE *f = fopen(BOOK_THEME_FILE, "w");
    if (f) {
        fprintf(f, "%d\n", g_eye_care_mode);
        fclose(f);
    }
}

#define BOOK_BG_NORMAL   0xFFFFFF
#define BOOK_BG_EYE      0xF9F6F0
#define BOOK_BG_DARK     0x1C1C1E
#define BOOK_TX_NORMAL   COL_TEXT
#define BOOK_TX_EYE      0x4A3F35
#define BOOK_TX_DARK     0xE0E0E0

static void book_apply_eye_style(void)
{
    uint32_t bg, tx, bar_bg, btn_bg;
    switch (g_eye_care_mode) {
    case 1:  bg = BOOK_BG_EYE;  tx = BOOK_TX_EYE;  bar_bg = 0xF3EFE6; btn_bg = 0xF3EFE6; break;
    case 2:  bg = BOOK_BG_DARK; tx = BOOK_TX_DARK; bar_bg = 0x2C2C2E; btn_bg = 0x2C2C2E; break;
    default: bg = BOOK_BG_NORMAL; tx = BOOK_TX_NORMAL; bar_bg = 0xFFFFFF; btn_bg = COL_BG; break;
    }
    if (book_reader_overlay) lv_obj_set_style_bg_color(book_reader_overlay, lv_color_hex(bg), 0);
    if (book_reader_text) lv_obj_set_style_text_color(book_reader_text, lv_color_hex(tx), 0);
    if (book_reader_title_lbl) lv_obj_set_style_text_color(book_reader_title_lbl, lv_color_hex(tx), 0);
    /* 页码与正文同色（暗色护眼模式下可读） */
    if (book_reader_page_lbl) lv_obj_set_style_text_color(book_reader_page_lbl, lv_color_hex(tx), 0);

    /* P0-2：底部控件条 + 顶部按钮 + 百分比 + slider 轨道全套随档位
     * （原实现只改背景/正文，黑档下 bar 白条 + 按钮浅色，视觉割裂） */
    if (book_reader_bar) {
        lv_obj_set_style_bg_color(book_reader_bar, lv_color_hex(bar_bg), 0);
        lv_obj_set_style_bg_opa(book_reader_bar,
            g_eye_care_mode == 2 ? LV_OPA_90 : LV_OPA_70, 0);
    }
    if (book_reader_back_btn) lv_obj_set_style_bg_color(book_reader_back_btn, lv_color_hex(btn_bg), 0);
    if (book_reader_eye_btn)  lv_obj_set_style_bg_color(book_reader_eye_btn,  lv_color_hex(btn_bg), 0);
    if (book_reader_prog) lv_obj_set_style_text_color(book_reader_prog, lv_color_hex(tx), 0);
    if (book_reader_slider) {
        /* 轨道：白档浅灰 / 黑档深灰；indicator 蓝 + knob 白保持高对比 */
        lv_obj_set_style_bg_color(book_reader_slider,
            lv_color_hex(g_eye_care_mode == 2 ? 0x3A3A3C : 0xE5E5EA), 0);
    }
}

static void book_eye_toggle_cb(lv_event_t *e)
{
    (void)e;
    g_eye_care_mode = (g_eye_care_mode + 1) % 3;
    book_apply_eye_style();
    book_theme_save();
    /* P1-5：toast 提示当前档位（原实现切换无任何反馈） */
    book_toast_show(g_eye_care_mode == 0 ? "护眼：白" :
                    g_eye_care_mode == 1 ? "护眼：米" : "护眼：黑");
}

/* P1-5：档位提示 toast（底部居中深色胶囊，2s 自动隐藏） */
static void book_toast_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (book_toast) {
        lv_obj_del(book_toast);
        book_toast = NULL;
    }
    book_toast_timer = NULL;
}

static void book_toast_show(const char *msg)
{
    lv_obj_t *p = book_reader_overlay ? book_reader_overlay : subpage_overlay;
    if (!p) return;
    if (book_toast_timer) { lv_timer_del(book_toast_timer); book_toast_timer = NULL; }
    if (book_toast) { lv_obj_del(book_toast); book_toast = NULL; }

    book_toast = lv_obj_create(p);
    lv_obj_set_size(book_toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(book_toast, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_bg_opa(book_toast, LV_OPA_80, 0);
    lv_obj_set_style_radius(book_toast, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(book_toast, 0, 0);
    lv_obj_set_style_pad_all(book_toast, DM(6), 0);
    lv_obj_set_style_pad_left(book_toast, DM(10), 0);
    lv_obj_set_style_pad_right(book_toast, DM(10), 0);
    lv_obj_clear_flag(book_toast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(book_toast, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(book_toast, LV_ALIGN_BOTTOM_MID, 0, -DM(40));
    lv_obj_move_foreground(book_toast);

    lv_obj_t *l = lv_label_create(book_toast);
    lv_label_set_text(l, msg);
    lv_obj_set_style_text_font(l, FONT_BODY, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(l);

    book_toast_timer = lv_timer_create(book_toast_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(book_toast_timer, 1);
}

    /* 继续阅读卡：扫描 book_progress.conf 找第一本 offset>0 的书（书架横幅） */
static int book_last_reading_idx(void)
{
    FILE *f = fopen(BOOK_PROGRESS_FILE, "r");
    if (!f) return -1;

    char line[256];
    int found = -1;
    while (fgets(line, sizeof(line), f)) {
        char path[160];
        long offset;
        if (sscanf(line, "%159[^:]:%ld", path, &offset) == 2 && offset > 0) {
            for (uint32_t i = 0; i < dm_book_count; i++) {
                if (strcmp(path, dm_book_paths[i]) == 0) {
                    found = (int)i;
                    break;
                }
            }
            if (found >= 0) break;
        }
    }
    fclose(f);
    return found;
}

/* P0-1：取书名首字符（完整 UTF-8 序列，中文 3 字节不乱码）。
 * 原实现 `char fc[2] = {s[0], '\0'}` 只取首字节，中文必显示乱码。 */
static void book_first_char(const char *s, char out[8])
{
    if (!s || !*s) { strcpy(out, "?"); return; }
    unsigned char c = (unsigned char)s[0];
    int n = 1;
    if ((c & 0xE0) == 0xC0 && s[1]) n = 2;                    /* 2 字节 */
    else if ((c & 0xF0) == 0xE0 && s[1] && s[2]) n = 3;       /* 3 字节（中文） */
    else if ((c & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) n = 4; /* 4 字节 */
    memcpy(out, s, (size_t)n);
    out[n] = '\0';
}

/* 某本书的阅读百分比（书架进度徽章用，0 表示未读） */
static int book_progress_pct(int idx)
{
    if (idx < 0 || idx >= (int)dm_book_count) return 0;

    long offset = 0;
    FILE *f = fopen(BOOK_PROGRESS_FILE, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            char path[160];
            long off;
            if (sscanf(line, "%159[^:]:%ld", path, &off) == 2 &&
                strcmp(path, dm_book_paths[idx]) == 0) {
                offset = off;
                break;
            }
        }
        fclose(f);
    }
    if (offset <= 0) return 0;

    struct stat st;
    if (stat(dm_book_paths[idx], &st) != 0 || st.st_size <= 0) return 0;
    int pct = (int)(offset * 100 / st.st_size);
    return pct > 100 ? 100 : pct;
}

void ui_books_create(lv_obj_t *parent)
{
    lv_coord_t sw = lv_disp_get_hor_res(lv_disp_get_default());

    /* Column layout */
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, CARD_GAP, 0);

    subpage_big_title(parent, "书籍");

    /* 扫描目录 */
    book_scan_directory();

    /* 书封配色 */
    static const uint32_t book_colors[] = {
        0xD35400, 0x8E44AD, 0x2980B9, 0x16A085,
        0xC0392B, 0x2C3E50, 0x7D3C98, 0x148F77,
    };

    /* ── 继续阅读横幅（Android 阅读器惯例）：有进度时置顶，一键续读 ── */
    int resume_idx = book_last_reading_idx();
    if (resume_idx >= 0 && resume_idx < (int)dm_book_count) {
        lv_obj_t *resume = lv_obj_create(parent);
        lv_obj_set_size(resume, lv_pct(100), DM(64));
        lv_obj_set_style_bg_color(resume, lv_color_hex(COL_CARD), 0);
        lv_obj_set_style_bg_opa(resume, LV_OPA_90, 0);
        lv_obj_set_style_radius(resume, RAD_CARD, 0);
        lv_obj_set_style_shadow_width(resume, 12, 0);
        lv_obj_set_style_shadow_color(resume, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(resume, LV_OPA_20, 0);
        lv_obj_set_style_border_width(resume, 0, 0);
        lv_obj_set_style_pad_left(resume, DM(8), 0);
        lv_obj_set_style_pad_right(resume, DM(8), 0);
        lv_obj_set_flex_flow(resume, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(resume, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(resume, DM(10), 0);
        lv_obj_clear_flag(resume, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(resume, book_card_click_cb, LV_EVENT_CLICKED,
                            NULL);
        lv_obj_set_user_data(resume, (void *)(uintptr_t)resume_idx);

        /* 左侧小封面（渐变 + 首字母；2026-08-09 对齐新书封：彩色→浅灰白） */
        lv_obj_t *thumb = lv_obj_create(resume);
        lv_obj_set_size(thumb, DM(36), DM(48));
        lv_obj_set_style_radius(thumb, DM(4), 0);
        lv_obj_set_style_bg_color(thumb, lv_color_hex(book_colors[resume_idx % 8]), 0);
        lv_obj_set_style_bg_grad_color(thumb, lv_color_hex(0xF2F2F7), 0);
        lv_obj_set_style_bg_grad_dir(thumb, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_opa(thumb, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(thumb, 0, 0);
        lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(thumb, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *tf = lv_label_create(thumb);
        char tfc[8];
        book_first_char(dm_book_titles[resume_idx], tfc);   /* P0-1：完整 UTF-8 首字符 */
        lv_label_set_text(tf, tfc);
        lv_obj_set_style_text_font(tf, FONT_TITLE, 0);
        lv_obj_set_style_text_color(tf, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(tf);

        /* 中间：继续阅读 + 书名 + 进度 */
        lv_obj_t *tcol = make_clean_cont(resume);
        lv_obj_set_size(tcol, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(tcol, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(tcol, DM(2), 0);
        lv_obj_set_flex_grow(tcol, 1);
        lv_obj_clear_flag(tcol, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *hint = lv_label_create(tcol);
        lv_label_set_text(hint, "继续阅读");
        lv_obj_set_style_text_font(hint, FONT_CAPTION, 0);
        lv_obj_set_style_text_color(hint, lv_color_hex(COL_SEC), 0);

        lv_obj_t *rt = lv_label_create(tcol);
        lv_label_set_long_mode(rt, LV_LABEL_LONG_DOT);
        lv_obj_set_width(rt, sw - 2 * EDGE_PAD - DM(160));
        lv_label_set_text(rt, dm_book_titles[resume_idx]);
        lv_obj_set_style_text_font(rt, FONT_BODY, 0);
        lv_obj_set_style_text_color(rt, lv_color_hex(COL_TEXT), 0);

        /* 右侧继续阅读圆钮（复用音乐播放器按钮样式与点击回调；
         * P2-6 由 LV_SYMBOL_PLAY 改为 LV_SYMBOL_NEXT——阅读前进语义） */
        music_round_btn(resume, DM(40), LV_SYMBOL_NEXT, book_card_click_cb,
                        (void *)(uintptr_t)resume_idx);
    }

    /* ── Bookshelf grid ──
     * 6 列书封卡片，A4 比例（1:1.414），书封外观
     * 2026-08-09 改 4 列 → 6 列：原卡 396×560 过大，6 列 248×350 紧凑统一 */
    int cols = 6;
    lv_coord_t card_w = (sw - 2 * EDGE_PAD - (cols - 1) * CARD_GAP) / cols;
    lv_coord_t card_h = (lv_coord_t)(card_w * 1.414f);  /* A4 1:1.414 书封 */

    lv_obj_t *shelf_cont = make_clean_cont(parent);
    lv_obj_set_width(shelf_cont, lv_pct(100));
    lv_obj_set_height(shelf_cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(shelf_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(shelf_cont, CARD_GAP, 0);
    lv_obj_set_style_pad_row(shelf_cont, CARD_GAP, 0);
    lv_obj_set_flex_align(shelf_cont, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    /* 书封配色（定义见函数开头，供继续阅读横幅与书封共用） */
    uint32_t n = dm_book_count > DM_MAX_BOOKS ? DM_MAX_BOOKS : dm_book_count;

    for (uint32_t i = 0; i < n; i++) {
        /* 书封卡片 — Liquid Glass 白玻璃卡 + 顶部柔和渐变书封区 + 深色书名
         * （2026-08-09 封面重设计：弃深色大色块/书脊，对齐全 UI 浅色玻璃语言） */
        lv_obj_t *bcard = lv_obj_create(shelf_cont);
        lv_obj_set_size(bcard, card_w, card_h);
        lv_obj_set_style_bg_color(bcard, lv_color_hex(COL_CARD), 0);
        lv_obj_set_style_bg_opa(bcard, LV_OPA_70, 0);          /* 白玻璃 */
        lv_obj_set_style_border_width(bcard, 1, 0);
        lv_obj_set_style_border_color(bcard, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_opa(bcard, LV_OPA_50, 0);
        lv_obj_set_style_radius(bcard, RAD_CARD, 0);            /* 统一 40px 圆角 */
        lv_obj_set_style_shadow_width(bcard, 8, 0);
        lv_obj_set_style_shadow_color(bcard, lv_color_hex(0x000000), 0);
        lv_obj_set_style_shadow_opa(bcard, LV_OPA_10, 0);
        lv_obj_set_style_clip_corner(bcard, true, 0);           /* 子元素随卡圆角裁剪 */
        lv_obj_set_style_pad_all(bcard, 0, 0);
        lv_obj_set_scrollbar_mode(bcard, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(bcard, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(bcard, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_flex_flow(bcard, LV_FLEX_FLOW_COLUMN);
        /* 按压反馈：轻微放大 + 阴影加深 */
        lv_obj_set_style_transform_width(bcard, 4, LV_STATE_PRESSED);
        lv_obj_set_style_transform_height(bcard, 4, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(bcard, 20, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(bcard, LV_OPA_30, LV_STATE_PRESSED);
        lv_obj_set_user_data(bcard, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(bcard, book_card_click_cb, LV_EVENT_CLICKED, NULL);

        /* 书封区（上部 62%）：彩色 → 浅灰白 柔和渐变 + 白色首字符 */
        lv_obj_t *cover = lv_obj_create(bcard);
        lv_obj_set_size(cover, lv_pct(100), lv_pct(62));
        lv_obj_set_style_bg_color(cover, lv_color_hex(book_colors[i % 8]), 0);
        lv_obj_set_style_bg_grad_color(cover, lv_color_hex(0xF2F2F7), 0);
        lv_obj_set_style_bg_grad_dir(cover, LV_GRAD_DIR_VER, 0);
        lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cover, 0, 0);
        lv_obj_clear_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(cover, LV_OBJ_FLAG_CLICKABLE);

        /* 首字符（白色 FONT_TITLE，书封区中央） */
        lv_obj_t *mi = lv_label_create(cover);
        char fc[8];
        book_first_char(dm_book_titles[i], fc);             /* P0-1：完整 UTF-8 首字符 */
        lv_label_set_text(mi, fc);
        lv_obj_set_style_text_font(mi, FONT_TITLE, 0);
        lv_obj_set_style_text_color(mi, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_opa(mi, LV_OPA_90, 0);
        lv_obj_align(mi, LV_ALIGN_CENTER, 0, -DM(4));

        /* 信息区（下部 38%）：书名（深色）+ .txt 标签 */
        lv_obj_t *info = lv_obj_create(bcard);
        lv_obj_set_size(info, lv_pct(100), lv_pct(38));
        lv_obj_set_style_bg_opa(info, LV_OPA_0, 0);
        lv_obj_set_style_border_width(info, 0, 0);
        lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(info, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(info, DM(8), 0);
        lv_obj_set_style_pad_row(info, DM(4), 0);
        lv_obj_clear_flag(info, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);

        /* 书名（深色，居中，超长省略） */
        lv_obj_t *bt = lv_label_create(info);
        lv_label_set_long_mode(bt, LV_LABEL_LONG_DOT);
        lv_obj_set_width(bt, card_w - DM(24));
        lv_label_set_text(bt, dm_book_titles[i]);
        lv_obj_set_style_text_font(bt, FONT_BODY, 0);
        lv_obj_set_style_text_color(bt, lv_color_hex(COL_TEXT), 0);

        lv_obj_t *tag = lv_label_create(info);
        lv_label_set_text(tag, ".txt");
        lv_obj_set_style_text_font(tag, FONT_CAPTION, 0);
        lv_obj_set_style_text_color(tag, lv_color_hex(COL_SEC), 0);

        /* 右下角进度徽章（读过才显示，Android 阅读器惯例） */
        int bpct = book_progress_pct((int)i);
        if (bpct > 0) {
            lv_obj_t *pbadge = lv_obj_create(bcard);
            lv_obj_set_size(pbadge, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_align(pbadge, LV_ALIGN_BOTTOM_RIGHT, -DM(4), -DM(4));
            lv_obj_set_style_bg_color(pbadge, lv_color_hex(COL_BLUE), 0);
            lv_obj_set_style_bg_opa(pbadge, LV_OPA_COVER, 0);
            lv_obj_set_style_radius(pbadge, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_border_width(pbadge, 0, 0);
            lv_obj_set_style_shadow_width(pbadge, 6, 0);
            lv_obj_set_style_shadow_color(pbadge, lv_color_hex(COL_BLUE), 0);
            lv_obj_set_style_shadow_opa(pbadge, LV_OPA_40, 0);
            lv_obj_set_style_pad_left(pbadge, DM(4), 0);
            lv_obj_set_style_pad_right(pbadge, DM(4), 0);
            lv_obj_set_style_pad_top(pbadge, DM(1), 0);
            lv_obj_set_style_pad_bottom(pbadge, DM(1), 0);
            lv_obj_clear_flag(pbadge, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_t *pl = lv_label_create(pbadge);
            lv_label_set_text_fmt(pl, "%d%%", bpct);
            lv_obj_set_style_text_font(pl, FONT_CAPTION, 0);
            lv_obj_set_style_text_color(pl, lv_color_hex(0xFFFFFF), 0);
            lv_obj_center(pl);
        }
    }

    /* 书架为空 */
    if (dm_book_count == 0) {
        lv_obj_t *empty = lv_label_create(parent);
        lv_label_set_text(empty, "把 .txt 书放到 /data/book 或 /sdcard/book 后重新进入");
        lv_obj_set_style_text_font(empty, FONT_BODY, 0);
        lv_obj_set_style_text_color(empty, lv_color_hex(COL_SEC), 0);
    }
}

/* ================================================================
 * BOOK READER — 全屏阅读器
 * ================================================================ */

/* 书架卡片点击 → 打开阅读器 */
static void book_card_click_cb(lv_event_t *e)
{
    lv_obj_t *card = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_obj_get_user_data(card);
    if (idx >= 0 && idx < (int)dm_book_count) {
        book_open_reader(idx);
    }
}

/* 阅读器关闭（close_subpage 清理入口，deskmate_ui.h 声明） */
void book_reader_close(void)
{
    if (book_reader_overlay) {
        book_progress_save(book_cur_idx);
        /* P1-5：阅读器销毁前先清 toast 定时器/对象，防野指针 */
        if (book_toast_timer) { lv_timer_del(book_toast_timer); book_toast_timer = NULL; }
        book_toast = NULL;
        lv_obj_del(book_reader_overlay);
        book_reader_overlay = NULL;
        book_reader_text = NULL;
        book_reader_title_lbl = NULL;
        book_reader_prog = NULL;
        book_reader_page_lbl = NULL;
        book_reader_slider = NULL;
        book_reader_bar = NULL;        /* P0-2 */
        book_reader_back_btn = NULL;   /* P0-2 */
        book_reader_eye_btn = NULL;    /* P0-2 */
        book_reader_scroll = NULL;     /* P1-3 */
    }
}

static void book_reader_back_cb(lv_event_t *e)
{
    (void)e;
    book_reader_close();
}

/* UTF-8 边界安全截断 */
static int book_utf8_trim(char *buf, int len)
{
    if (len <= 0) return 0;
    int n = len;
    while (n > 0) {
        unsigned char c = (unsigned char)buf[n - 1];
        if ((c & 0x80) == 0) break;
        if ((c & 0xC0) == 0xC0) break;
        n--;
    }
    buf[n] = '\0';
    return n;
}

/* 分页引擎 */
static void book_page_render(void)
{
    FILE *f;
    static char buf[BOOK_PAGE_CHARS + 8];
    size_t rd;
    int n, pct;

    if (book_reader_text == NULL) return;

    f = fopen(dm_book_paths[book_cur_idx], "rb");
    if (f == NULL) {
        lv_label_set_text(book_reader_text, "（无法打开文件）");
        return;
    }

    fseek(f, book_offset, SEEK_SET);
    rd = fread(buf, 1, BOOK_PAGE_CHARS, f);
    fclose(f);

    if (rd == 0) {
        lv_label_set_text(book_reader_text, "—— 全文完 ——");
        if (book_reader_prog) lv_label_set_text(book_reader_prog, "100%");
        if (book_reader_page_lbl) {
            long total = (book_file_size + BOOK_PAGE_CHARS - 1) / BOOK_PAGE_CHARS;
            lv_label_set_text_fmt(book_reader_page_lbl, "%ld / %ld", total, total);
        }
        g_book_slider_guard = 1;
        if (book_reader_slider) lv_slider_set_value(book_reader_slider, 100, LV_ANIM_OFF);
        g_book_slider_guard = 0;
        /* P1-3：新页回顶（防上一页滚动位置残留） */
        if (book_reader_scroll) lv_obj_scroll_to_y(book_reader_scroll, 0, LV_ANIM_OFF);
        return;
    }

    n = book_utf8_trim(buf, (int)rd);
    lv_label_set_text(book_reader_text, buf);

    pct = (book_file_size > 0) ? (int)(book_offset * 100 / book_file_size) : 0;
    if (book_reader_prog) lv_label_set_text_fmt(book_reader_prog, "%d%%", pct);

    /* 页码：当前页 = offset/CHARS + 1，总页向上取整 */
    if (book_reader_page_lbl) {
        long total = (book_file_size + BOOK_PAGE_CHARS - 1) / BOOK_PAGE_CHARS;
        long cur = book_offset / BOOK_PAGE_CHARS + 1;
        if (cur > total) cur = total;
        lv_label_set_text_fmt(book_reader_page_lbl, "%ld / %ld", cur, total);
    }

    /* P1-3：翻页/跳页后滚动归零，新页从顶部开始看 */
    if (book_reader_scroll) lv_obj_scroll_to_y(book_reader_scroll, 0, LV_ANIM_OFF);

    /* 进度条同步（guard 防 VALUE_CHANGED 递归触发 book_slider_cb） */
    g_book_slider_guard = 1;
    if (book_reader_slider) lv_slider_set_value(book_reader_slider, pct, LV_ANIM_OFF);
    g_book_slider_guard = 0;
}

static void book_page_prev_cb(lv_event_t *e)
{
    (void)e;
    if (book_offset <= 0) return;
    long target = book_offset - BOOK_PAGE_CHARS;
    book_offset = target < 0 ? 0 : target;
    book_page_render();
}

static void book_page_next_cb(lv_event_t *e)
{
    (void)e;
    if (book_offset >= book_file_size) return;
    book_offset += BOOK_PAGE_CHARS;
    book_page_render();
}

/* 底部进度条拖动 → 跳页（2026-08-09 新增）。值 0-100 映射到文件偏移，
 * 对齐到整页（BOOK_PAGE_CHARS 倍数），保证与 prev/next 的页边界一致。
 * P2-8：拖动高频触发 VALUE_CHANGED，200ms 内节流只记偏移不重渲染，
 * 松手 RELEASED 强制渲染最终位置（book_slider_release_cb） */
static void book_slider_cb(lv_event_t *e)
{
    (void)e;
    if (g_book_slider_guard) return;
    if (book_file_size <= 0 || book_reader_slider == NULL) return;
    int v = lv_slider_get_value(book_reader_slider);
    long target = (long)v * book_file_size / 100;
    /* 对齐到页边界：target / CHARS 向下取整，避免半页错位 */
    book_offset = (target / BOOK_PAGE_CHARS) * BOOK_PAGE_CHARS;
    if (book_offset > book_file_size - 1) book_offset = book_file_size - 1;
    if (book_offset < 0) book_offset = 0;
    if (lv_tick_elaps(book_slider_last_tick) < 200) return;   /* P2-8 节流 */
    book_slider_last_tick = lv_tick_get();
    book_page_render();
}

/* P2-8：松手强制渲染（节流跳过的最后一次必须落盘到屏幕） */
static void book_slider_release_cb(lv_event_t *e)
{
    (void)e;
    if (g_book_slider_guard) return;
    if (book_file_size <= 0 || book_reader_slider == NULL) return;
    int v = lv_slider_get_value(book_reader_slider);
    long target = (long)v * book_file_size / 100;
    book_offset = (target / BOOK_PAGE_CHARS) * BOOK_PAGE_CHARS;
    if (book_offset > book_file_size - 1) book_offset = book_file_size - 1;
    if (book_offset < 0) book_offset = 0;
    book_page_render();
}

/* 打开阅读器（Phase 1 拆分：deskmate_ui.c files_open_file 调用，非 static） */
void book_open_reader(int idx)
{
    FILE *f;
    lv_coord_t sw, sh;

    if (idx < 0 || idx >= (int)dm_book_count) return;
    if (book_reader_overlay) book_reader_close();

    book_theme_load();
    book_progress_load(idx);

    f = fopen(dm_book_paths[idx], "rb");
    if (f == NULL) {
        book_file_size = 0;
    } else {
        fseek(f, 0, SEEK_END);
        book_file_size = ftell(f);
        fclose(f);
    }

    if (book_offset < 0) book_offset = 0;
    if (book_offset > book_file_size) book_offset = 0;

    book_cur_idx = idx;

    sw = lv_disp_get_hor_res(lv_disp_get_default());
    sh = lv_disp_get_ver_res(lv_disp_get_default());

    uint32_t init_bg = (g_eye_care_mode == 1) ? BOOK_BG_EYE :
                       (g_eye_care_mode == 2) ? BOOK_BG_DARK : BOOK_BG_NORMAL;
    lv_obj_t *reader_parent = subpage_overlay ? subpage_overlay : lv_scr_act();
    book_reader_overlay = lv_obj_create(reader_parent);
    lv_obj_set_size(book_reader_overlay, sw, sh);
    lv_obj_set_style_bg_color(book_reader_overlay, lv_color_hex(init_bg), 0);
    lv_obj_set_style_bg_opa(book_reader_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(book_reader_overlay, 0, 0);
    lv_obj_set_style_pad_all(book_reader_overlay, 0, 0);
    lv_obj_clear_flag(book_reader_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(book_reader_overlay);

    /* 顶部：返回按钮（玻璃圆形）*/
    lv_obj_t *back_btn = lv_btn_create(book_reader_overlay);
    lv_obj_set_size(back_btn, DM(40), DM(40));
    lv_obj_set_pos(back_btn, DM(16), DM(8));
    assert(DM(16) + DM(40) <= sw && DM(8) + DM(40) <= sh);   /* 铁律2：边界断言 */
    lv_obj_set_style_radius(back_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_70, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_add_event_cb(back_btn, book_reader_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_icon = lv_label_create(back_btn);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_font(back_icon, FONT_ICON, 0);
    lv_obj_set_style_text_color(back_icon, lv_color_hex(COL_BLUE), 0);
    lv_obj_center(back_icon);
    book_reader_back_btn = back_btn;   /* P0-2：护眼配色随档位 */

    /* 顶部：书名（居中）*/
    book_reader_title_lbl = lv_label_create(book_reader_overlay);
    lv_label_set_long_mode(book_reader_title_lbl, LV_LABEL_LONG_DOT);
    lv_label_set_text(book_reader_title_lbl, dm_book_titles[idx]);
    lv_obj_set_width(book_reader_title_lbl, sw - DM(140));
    lv_obj_align(book_reader_title_lbl, LV_ALIGN_TOP_MID, 0, DM(16));
    lv_obj_set_style_text_font(book_reader_title_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(book_reader_title_lbl, lv_color_hex(COL_TEXT), 0);

    /* 顶部：护眼模式切换按钮 */
    lv_obj_t *eye_btn = lv_btn_create(book_reader_overlay);
    lv_obj_set_size(eye_btn, DM(40), DM(40));
    lv_obj_set_pos(eye_btn, sw - DM(56), DM(8));
    assert(sw - DM(56) >= 0 && sw - DM(56) + DM(40) <= sw && DM(8) + DM(40) <= sh);   /* 铁律2 */
    lv_obj_set_style_radius(eye_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(eye_btn, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(eye_btn, LV_OPA_70, 0);
    lv_obj_set_style_shadow_width(eye_btn, 0, 0);
    lv_obj_set_style_border_width(eye_btn, 0, 0);
    lv_obj_add_event_cb(eye_btn, book_eye_toggle_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *eye_icon = lv_label_create(eye_btn);
    lv_label_set_text(eye_icon, LV_SYMBOL_EYE_OPEN);
    lv_obj_set_style_text_font(eye_icon, FONT_ICON, 0);
    lv_obj_set_style_text_color(eye_icon, lv_color_hex(COL_ORANGE), 0);
    lv_obj_center(eye_icon);
    book_reader_eye_btn = eye_btn;     /* P0-2：护眼配色随档位 */

    /* 内容区：可滚动文字（底部预留控件条空间） */
    lv_obj_t *scroll_cont = lv_obj_create(book_reader_overlay);
    lv_obj_set_size(scroll_cont, sw - 2 * DM(32), sh - DM(190));
    lv_obj_set_pos(scroll_cont, DM(32), DM(56));
    assert(DM(32) + (sw - 2 * DM(32)) <= sw && DM(56) + (sh - DM(190)) <= sh);   /* 铁律2 */
    lv_obj_set_style_bg_opa(scroll_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(scroll_cont, 0, 0);
    lv_obj_set_scrollbar_mode(scroll_cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(scroll_cont, LV_DIR_VER);
    book_reader_scroll = scroll_cont;  /* P1-3：翻页后回顶 */

    book_reader_text = lv_label_create(scroll_cont);
    lv_obj_set_width(book_reader_text, sw - 2 * DM(32) - DM(8));
    lv_label_set_long_mode(book_reader_text, LV_LABEL_LONG_WRAP);
    lv_label_set_text(book_reader_text, "加载中…");
    lv_obj_set_style_text_font(book_reader_text, FONT_BODY, 0);
    lv_obj_set_style_text_color(book_reader_text, lv_color_hex(COL_TEXT), 0);

    /* ── 底部控件条：上一页 | 页码 | 进度条 | 百分比 | 下一页 ──
     * 风格对齐音乐播放器：白色玻璃圆钮 + 蓝色进度条。
     * 2026-08-09：原翻页回调从未挂载按钮（阅读器只能滚一屏 1400 字符
     * 无法翻页），此处挂上 book_page_prev/next_cb 并新增可拖动跳页 slider。 */
    lv_obj_t *bar = lv_obj_create(book_reader_overlay);
    lv_obj_set_size(bar, sw - 2 * DM(32), LV_SIZE_CONTENT);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -DM(20));
    lv_obj_set_style_bg_color(bar, lv_color_hex(COL_CARD), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_70, 0);
    lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_shadow_width(bar, 8, 0);
    lv_obj_set_style_shadow_opa(bar, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(bar, DM(6), 0);
    lv_obj_set_style_pad_column(bar, DM(8), 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    book_reader_bar = bar;             /* P0-2：护眼配色随档位 */

    lv_obj_t *prev_btn = music_round_btn(bar, DM(40), LV_SYMBOL_PREV,
                                         book_page_prev_cb, NULL);
    (void)prev_btn;

    book_reader_page_lbl = lv_label_create(bar);
    lv_label_set_text(book_reader_page_lbl, "1 / 1");
    lv_obj_set_style_text_font(book_reader_page_lbl, FONT_BODY, 0);
    lv_obj_set_style_text_color(book_reader_page_lbl, lv_color_hex(COL_TEXT), 0);

    book_reader_slider = lv_slider_create(bar);
    lv_obj_set_flex_grow(book_reader_slider, 1);
    lv_obj_set_height(book_reader_slider, DM(6));
    lv_obj_set_style_bg_color(book_reader_slider, lv_color_hex(COL_BLUE),
                              LV_PART_INDICATOR);
    lv_obj_set_style_radius(book_reader_slider, DM(3), LV_PART_INDICATOR);
    lv_obj_set_style_radius(book_reader_slider, DM(3), LV_PART_KNOB);
    lv_obj_set_style_bg_color(book_reader_slider, lv_color_hex(0xFFFFFF),
                              LV_PART_KNOB);
    lv_obj_set_style_bg_opa(book_reader_slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(book_reader_slider, DM(3), LV_PART_KNOB);
    lv_slider_set_range(book_reader_slider, 0, 100);
    lv_obj_add_event_cb(book_reader_slider, book_slider_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(book_reader_slider, book_slider_release_cb,
                        LV_EVENT_RELEASED, NULL);   /* P2-8：松手强制渲染 */

    book_reader_prog = lv_label_create(bar);
    lv_label_set_text(book_reader_prog, "0%");
    lv_obj_set_style_text_font(book_reader_prog, FONT_BODY, 0);
    lv_obj_set_style_text_color(book_reader_prog, lv_color_hex(COL_SEC), 0);

    lv_obj_t *next_btn = music_round_btn(bar, DM(40), LV_SYMBOL_NEXT,
                                         book_page_next_cb, NULL);
    (void)next_btn;

    book_apply_eye_style();
    book_page_render();
}
