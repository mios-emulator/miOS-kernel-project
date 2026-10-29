typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define NOOPT __attribute__((optimize("no-tree-loop-distribute-patterns")))

extern "C" {
NOOPT void *memset(void *d, int v, __SIZE_TYPE__ n) {
    u8 *p = (u8 *)d; while (n--) *p++ = (u8)v; return d;
}
NOOPT void *memcpy(void *d, const void *s, __SIZE_TYPE__ n) {
    u8 *p = (u8 *)d; const u8 *q = (const u8 *)s; while (n--) *p++ = *q++; return d;
}
NOOPT void *memmove(void *d, const void *s, __SIZE_TYPE__ n) {
    u8 *p = (u8 *)d; const u8 *q = (const u8 *)s;
    if (p < q) while (n--) *p++ = *q++;
    else { p += n; q += n; while (n--) *--p = *--q; }
    return d;
}
void __cxa_pure_virtual() { for (;;) __asm__ volatile ("hlt"); }
}

static u8 *heap_ptr = (u8 *)0x20000;
static u8 *const heap_end = (u8 *)0x70000;

void *operator new(__SIZE_TYPE__ n) {
    n = (n + 3) & ~3u;
    if (heap_ptr + n > heap_end) for (;;) __asm__ volatile ("hlt");
    void *p = heap_ptr; heap_ptr += n; return p;
}
void *operator new[](__SIZE_TYPE__ n) { return operator new(n); }
void operator delete(void *) noexcept {}
void operator delete(void *, __SIZE_TYPE__) noexcept {}
void operator delete[](void *) noexcept {}
void operator delete[](void *, __SIZE_TYPE__) noexcept {}

extern "C" { typedef void (*ctor_t)(); extern ctor_t __init_array_start[], __init_array_end[]; }
static void call_ctors() {
    for (ctor_t *c = __init_array_start; c < __init_array_end; c++) (*c)();
}

static inline u8 inb(u16 p) { u8 v; __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(p)); return v; }
static inline void outb(u16 p, u8 v) { __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(p)); }

template <typename T, int N>
struct Array {
    T data[N];
    int len = 0;
    void push(T v) { if (len < N) data[len++] = v; }
    T &operator[](int i) { return data[i]; }
};

static const int W = 80, H = 25;
static volatile u16 *const VGA = (volatile u16 *)0xB8000;
static int cx = 0, cy = 0;
static u8 attr = 0x07;

static void cursor_at(int pos) {
    outb(0x3D4, 14); outb(0x3D5, (u8)(pos >> 8));
    outb(0x3D4, 15); outb(0x3D5, (u8)(pos & 0xFF));
}
static void cursor() { cursor_at(cy * W + cx); }

static void scroll() {
    for (int i = 0; i < W * (H - 1); i++) VGA[i] = VGA[i + W];
    for (int i = W * (H - 1); i < W * H; i++) VGA[i] = (u16)((attr << 8) | ' ');
    cy = H - 1;
}

static void kputc(char c) {
    if (c == '\n') { cx = 0; cy++; }
    else if (c == '\b') {
        if (cx > 0) { cx--; VGA[cy * W + cx] = (u16)((attr << 8) | ' '); }
    } else {
        VGA[cy * W + cx] = (u16)((attr << 8) | (u8)c);
        if (++cx >= W) { cx = 0; cy++; }
    }
    if (cy >= H) scroll();
    cursor();
}
static void kprint(const char *s) { while (*s) kputc(*s++); }

static void clear() {
    for (int i = 0; i < W * H; i++) VGA[i] = (u16)((0x07 << 8) | ' ');
    cx = cy = 0;
    cursor();
}

enum { K_UP = 256, K_DOWN, K_LEFT, K_RIGHT };

static const char MAP[58] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b', '\t',
    'q','w','e','r','t','y','u','i','o','p','[',']', '\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
};
static const char MAPS[58] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b', '\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}', '\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
    'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
};

static int kgetc() {
    static bool shift = false, ext = false;
    for (;;) {
        while (!(inb(0x64) & 1)) {}
        u8 sc = inb(0x60);
        if (sc == 0xE0) { ext = true; continue; }
        if (sc == 0x2A || sc == 0x36) { shift = true;  continue; }
        if (sc == 0xAA || sc == 0xB6) { shift = false; continue; }
        if (sc & 0x80) { ext = false; continue; }
        if (ext) {
            ext = false;
            switch (sc) {
                case 0x48: return K_UP;
                case 0x50: return K_DOWN;
                case 0x4B: return K_LEFT;
                case 0x4D: return K_RIGHT;
            }
            continue;
        }
        if (sc < 58) {
            char c = shift ? MAPS[sc] : MAP[sc];
            if (c) return c;
        }
    }
}

static bool streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}
static bool startswith(const char *s, const char *p) {
    while (*p) if (*s++ != *p++) return false;
    return true;
}
static void itoa(int v, char *b) {
    char t[12]; int n = 0, i = 0;
    bool neg = v < 0;
    u32 u = neg ? (u32)(-v) : (u32)v;
    if (u == 0) t[n++] = '0';
    while (u) { t[n++] = (char)('0' + u % 10); u /= 10; }
    if (neg) b[i++] = '-';
    while (n) b[i++] = t[--n];
    b[i] = 0;
}

static void cell(int x, int y, u8 ch, u8 a) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    VGA[y * W + x] = (u16)((a << 8) | ch);
}
static int text(int x, int y, const char *s, u8 a) {
    int n = 0;
    for (; s[n]; n++) cell(x + n, y, (u8)s[n], a);
    return n;
}

static const u8 WIN_ATTR = 0x70;

class Window;
typedef void (*Action)(Window &);

struct Widget {
    int x, y;
    bool focusable;
    Widget(int x, int y, bool f) : x(x), y(y), focusable(f) {}
    virtual void draw(int ox, int oy, bool focused) = 0;
    virtual void activate(Window &) {}
};

struct Label : Widget {
    const char *s; const int *val;
    Label(int x, int y, const char *s, const int *val = nullptr)
        : Widget(x, y, false), s(s), val(val) {}
    void draw(int ox, int oy, bool) override {
        int n = text(ox + x, oy + y, s, WIN_ATTR);
        if (val) {
            char b[12]; itoa(*val, b);
            n += text(ox + x + n, oy + y, b, WIN_ATTR);
            text(ox + x + n, oy + y, "   ", WIN_ATTR);
        }
    }
};

struct Button : Widget {
    const char *s; Action cb;
    Button(int x, int y, const char *s, Action cb) : Widget(x, y, true), s(s), cb(cb) {}
    void draw(int ox, int oy, bool focused) override {
        u8 a = focused ? 0x4F : 0x30;
        int n = text(ox + x, oy + y, " ", a);
        n += text(ox + x + n, oy + y, s, a);
        text(ox + x + n, oy + y, " ", a);
    }
    void activate(Window &w) override { if (cb) cb(w); }
};

struct Checkbox : Widget {
    const char *s; bool checked = false;
    Checkbox(int x, int y, const char *s) : Widget(x, y, true), s(s) {}
    void draw(int ox, int oy, bool focused) override {
        u8 a = focused ? 0x4F : WIN_ATTR;
        text(ox + x, oy + y, checked ? "[x] " : "[ ] ", a);
        text(ox + x + 4, oy + y, s, a);
    }
    void activate(Window &) override { checked = !checked; }
};

class Window {
public:
    int x, y, w, h;
    const char *title;
    bool open = true;
    Array<Widget *, 12> items;
    int focus = -1;

    Window(int x, int y, int w, int h, const char *title)
        : x(x), y(y), w(w), h(h), title(title) {}

    void add(Widget *wd) {
        items.push(wd);
        if (focus < 0 && wd->focusable) focus = items.len - 1;
    }

    void draw() {
        for (int j = 1; j <= h; j++) cell(x + w, y + j, ' ', 0x00);
        for (int i = 1; i <= w; i++) cell(x + i, y + h, ' ', 0x00);
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++) {
                u8 ch = ' ';
                if (j == 0)          ch = i == 0 ? 0xC9 : i == w - 1 ? 0xBB : 0xCD;
                else if (j == h - 1) ch = i == 0 ? 0xC8 : i == w - 1 ? 0xBC : 0xCD;
                else if (i == 0 || i == w - 1) ch = 0xBA;
                cell(x + i, y + j, ch, WIN_ATTR);
            }
        text(x + 2, y, title, WIN_ATTR);
        for (int i = 0; i < items.len; i++)
            items[i]->draw(x + 1, y + 1, i == focus);
    }

    void move_focus(int dir) {
        if (focus < 0) return;
        for (int n = 0; n < items.len; n++) {
            focus = (focus + dir + items.len) % items.len;
            if (items[focus]->focusable) return;
        }
    }

    void run() {
        while (open) {
            draw();
            int k = kgetc();
            switch (k) {
                case 27: open = false; break;
                case '\t': case K_RIGHT: case K_DOWN: move_focus(1);  break;
                case K_LEFT: case K_UP:               move_focus(-1); break;
                case '\n': case ' ':
                    if (focus >= 0) items[focus]->activate(*this);
                    break;
            }
        }
    }
};

static void cmd_ui() {
    static int counter = 0;
    cursor_at(W * H);
    for (int i = 0; i < W * H; i++) VGA[i] = (u16)((0x19 << 8) | 0xB0);
    for (int i = 0; i < W; i++) cell(i, 0, ' ', 0x70);
    text(1, 0, "Mi OS v0.3", 0x70);

    Window win(18, 5, 44, 13, " Demo en C++ ");
    win.add(new Label(3, 2, "Hola desde C++!"));
    win.add(new Label(3, 4, "Contador: ", &counter));
    win.add(new Checkbox(3, 6, "Opcion de ejemplo"));
    win.add(new Button(3, 8, "Sumar",   [](Window &)   { counter++; }));
    win.add(new Button(13, 8, "Reset",  [](Window &)   { counter = 0; }));
    win.add(new Button(23, 8, "Salir",  [](Window &w)  { w.open = false; }));
    win.add(new Label(3, 10, "Tab: mover  Enter: usar  Esc: salir"));
    win.run();

    clear();
}

static void reboot() {
    while (inb(0x64) & 2) {}
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile ("hlt");
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void run(const char *c) {
    if (!*c) return;
    if (streq(c, "help")) {
        kprint("Comandos disponibles:\n");
        kprint("  help         - muestra esta ayuda\n");
        kprint("  about        - informacion del sistema\n");
        kprint("  clear        - limpia la pantalla\n");
        kprint("  echo <texto> - repite el texto\n");
        kprint("  color <0-f>  - cambia el color del texto\n");
        kprint("  ui           - demo de ventana con botones\n");
        kprint("  halt         - detiene el sistema\n");
        kprint("  reboot       - reinicia\n");
    } else if (streq(c, "about")) {
        kprint("Mi OS v0.3\n");
        kprint("Bootloader en assembly + kernel en C++ (32 bits).\n");
    } else if (streq(c, "clear")) {
        clear();
    } else if (startswith(c, "echo ")) {
        kprint(c + 5); kputc('\n');
    } else if (startswith(c, "color ") && hexval(c[6]) >= 0 && !c[7]) {
        attr = (u8)hexval(c[6]);
        if (attr == 0) attr = 0x07;
    } else if (streq(c, "ui")) {
        cmd_ui();
    } else if (streq(c, "halt")) {
        kprint("Sistema detenido.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    } else if (streq(c, "reboot")) {
        reboot();
    } else {
        kprint("Comando no encontrado. Proba con 'help'.\n");
    }
}

extern "C" void kernel_main() {
    call_ctors();
    char buf[64];
    int n;

    clear();
    attr = 0x0A; kprint("Mi OS v0.1\n");
    attr = 0x07; kprint("Escribi 'help' para ver los comandos.\n\n");

    for (;;) {
        u8 user = attr;
        attr = 0x0B; kprint("> "); attr = user;
        n = 0;
        for (;;) {
            int c = kgetc();
            if (c >= 256) continue;
            if (c == '\n') { kputc('\n'); break; }
            if (c == '\b') { if (n > 0) { n--; kputc('\b'); } continue; }
            if (c >= ' ' && n < 63) { buf[n++] = (char)c; kputc((char)c); }
        }
        buf[n] = 0;
        run(buf);
    }
}
