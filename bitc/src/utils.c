// SPDX-License-Identifier: GPL-2.0+
/*******************************************************************************
 * This file is the part of the Bytelizer library
 *
 * (C) Copyright 2026 TheSnowfield
 * (C) Copyright 2026 [LLM] DeepSeek V4 Pro
 * (C) Copyright 2026 [LLM] MiMo V2 Omni
 *
 * Authors: TheSnowfield <17957399+TheSnowfield@users.noreply.github.com>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <bitc/utils.h>
#include <bitc/ast.h>
#include <bitc/const.h>

/* ═══════════════════════════════════════════════════════════════════
 *  base utilities
 * ═══════════════════════════════════════════════════════════════════ */

void str_toupper(const char* src, char* dst, int dsz) {
    if (!src || !dst || dsz <= 0) return;
    int i = 0;
    while (src[i] && i < dsz - 1) {
        dst[i] = (char)((src[i] >= 'a' && src[i] <= 'z') ? src[i] - 32 : src[i]);
        i++;
    }
    dst[i] = '\0';
}

void die(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

char* xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* d = (char*)malloc(n);
    if (!d) die("oom");
    memcpy(d, s, n);
    return d;
}

char* read_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    size_t sz = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 2);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, sz, f);
    fclose(f);
    buf[n]   = '\0';
    buf[n+1] = '\0';
    if (out_len) *out_len = n;
    return buf;
}

/* ═══════════════════════════════════════════════════════════════════
 *  type classification
 * ═══════════════════════════════════════════════════════════════════ */

const bit_type_info_t bit_primitive_types[BIT_PRIMITIVE_COUNT] = {
    {BIT_UINT8,  1}, {BIT_UINT16, 2}, {BIT_UINT32, 4}, {BIT_UINT64, 8},
    {BIT_INT8,   1}, {BIT_INT16,  2}, {BIT_INT32,  4}, {BIT_INT64,  8},
    {BIT_STRING, 0}, {BIT_BYTES,  0},
};

int bit_type_size(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < BIT_PRIMITIVE_COUNT; i++)
        if (strcmp(bit_primitive_types[i].name, name) == 0)
            return bit_primitive_types[i].size;
    return -1;
}

bool is_known_basetype(const char* bt) {
    return bit_type_size(bt) >= 0 || (bt && strcmp(bt, BIT_BYTES) == 0);
}

int field_base_size(node_t* f) {
    if (!f || !f->basetype) return 0;
    int sz = bit_type_size(f->basetype);
    return sz > 0 ? sz : 0;
}

int field_byte_size(node_t* f) {
    if (!f) return 0;
    if (strcmp(f->basetype, BIT_STRING) == 0)
        return (int)strlen(f->value ? f->value : "");
    int base = field_base_size(f);
    if (f->value && strchr(f->value, ' ')) {
        int cnt = 0;
        for (const char* p = f->value; *p; p++) if (*p == ' ') cnt++;
        return base * (cnt + 1);
    }
    return base;
}

bool field_is_be(node_t* f) {
    if (!f) return false;
    for (int i = 0; i < f->attr_count; i++)
        if (strstr(f->attrs[i], "be") && strstr(f->attrs[i], "with-uint"))
            return true;
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 *  input name resolution
 * ═══════════════════════════════════════════════════════════════════ */

bool is_input_ref(const char* name, char input_names[][64], int count) {
    if (!name) return false;
    for (int i = 0; i < count; i++)
        if (strcmp(input_names[i], name) == 0) return true;
    const char* arrow = strstr(name, "->");
    if (!arrow) arrow = strchr(name, '.');
    if (arrow) {
        int rlen = (int)(arrow - name);
        for (int i = 0; i < count; i++)
            if (strncmp(input_names[i], name, rlen) == 0 && input_names[i][rlen] == '\0')
                return true;
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 *  attribute search (AST walk)
 * ═══════════════════════════════════════════════════════════════════ */

static const char* find_attr_dfs(node_t* n, const char* key, int klen) {
    if (!n) return NULL;
    if (n->value && strncmp(n->value, key, klen) == 0 && n->value[klen] == ':')
        return n->value + klen + 1;
    for (node_t* ch = n->first_child; ch; ch = ch->next_sibling) {
        const char* r = find_attr_dfs(ch, key, klen);
        if (r) return r;
    }
    return NULL;
}

const char* find_attr_value(node_t* root, const char* key) {
    if (!root || !key) return NULL;
    return find_attr_dfs(root, key, (int)strlen(key));
}

/* ═══════════════════════════════════════════════════════════════════
 *  string formatting
 * ═══════════════════════════════════════════════════════════════════ */

void fmt_attr_value(const char* val, char* out, size_t outsz) {
    if (!val) { out[0] = '\0'; return; }
    size_t j = 0;
    for (const char* p = val; *p && j < outsz - 1; p++) {
        if (*p == '.')               out[j++] = '_';
        else if (*p >= 'A' && *p <= 'Z') out[j++] = (char)(*p + 32);
        else                         out[j++] = *p;
    }
    out[j] = '\0';
}

char* resolve_name_tmpl(node_t* root, const char* tmpl) {
    if (!tmpl) return xstrdup("");
    size_t cap = strlen(tmpl) + 128;
    char* out = (char*)malloc(cap);
    if (!out) return xstrdup(tmpl);
    size_t j = 0;
    const char* p = tmpl;
    while (*p && j < cap - 1) {
        if (strncmp(p, "$fmt{", 5) == 0) {
            p += 5;
            const char* end = strchr(p, '}');
            if (end) {
                char key[128];
                int kl = (int)(end - p);
                if (kl > 127) kl = 127;
                memcpy(key, p, kl); key[kl] = '\0';
                p = end + 1;
                const char* val = find_attr_value(root, key);
                if (val) {
                    char fmtd[256];
                    fmt_attr_value(val, fmtd, sizeof(fmtd));
                    size_t fl = strlen(fmtd);
                    if (j + fl >= cap) { cap = j + fl + 128; out = (char*)realloc(out, cap); }
                    memcpy(out + j, fmtd, fl); j += fl;
                }
            } else { out[j++] = *p++; }
        } else {
            out[j++] = *p++;
        }
    }
    out[j] = '\0';
    return out;
}
