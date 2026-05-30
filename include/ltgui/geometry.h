#pragma once

namespace ltgui {

struct Point {
    int x = 0;
    int y = 0;

    Point() = default;
    Point(int x, int y) : x(x), y(y) {}

    Point operator+(const Point& o) const { return {x + o.x, y + o.y}; }
    Point operator-(const Point& o) const { return {x - o.x, y - o.y}; }
    Point& operator+=(const Point& o) { x += o.x; y += o.y; return *this; }
    Point& operator-=(const Point& o) { x -= o.x; y -= o.y; return *this; }
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

struct Size {
    int width = 0;
    int height = 0;

    Size() = default;
    Size(int w, int h) : width(w), height(h) {}

    bool isEmpty() const { return width <= 0 || height <= 0; }
    bool operator==(const Size& o) const { return width == o.width && height == o.height; }
    bool operator!=(const Size& o) const { return !(*this == o); }
    Size operator+(const Size& o) const { return {width + o.width, height + o.height}; }
    Size operator-(const Size& o) const { return {width - o.width, height - o.height}; }
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    Rect() = default;
    Rect(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    Rect(const Point& pos, const Size& size) : x(pos.x), y(pos.y), width(size.width), height(size.height) {}

    int left() const { return x; }
    int top() const { return y; }
    int right() const { return x + width; }
    int bottom() const { return y + height; }
    Point topLeft() const { return {x, y}; }
    Point bottomRight() const { return {x + width, y + height}; }
    Point center() const { return {x + width / 2, y + height / 2}; }
    Size size() const { return {width, height}; }

    bool contains(const Point& p) const {
        return p.x >= x && p.x < right() && p.y >= y && p.y < bottom();
    }

    bool contains(const Rect& r) const {
        return r.x >= x && r.y >= y && r.right() <= right() && r.bottom() <= bottom();
    }

    bool intersects(const Rect& o) const {
        return !(right() <= o.x || o.right() <= x || bottom() <= o.y || o.bottom() <= y);
    }

    Rect intersected(const Rect& o) const {
        int nx = x > o.x ? x : o.x;
        int ny = y > o.y ? y : o.y;
        int nr = right() < o.right() ? right() : o.right();
        int nb = bottom() < o.bottom() ? bottom() : o.bottom();
        if (nx < nr && ny < nb) return {nx, ny, nr - nx, nb - ny};
        return {};
    }

    Rect united(const Rect& o) const {
        int nl = x < o.x ? x : o.x;
        int nt = y < o.y ? y : o.y;
        int nr = right() > o.right() ? right() : o.right();
        int nb = bottom() > o.bottom() ? bottom() : o.bottom();
        return {nl, nt, nr - nl, nb - nt};
    }

    bool isEmpty() const { return width <= 0 || height <= 0; }
    bool operator==(const Rect& o) const {
        return x == o.x && y == o.y && width == o.width && height == o.height;
    }
    bool operator!=(const Rect& o) const { return !(*this == o); }

    void adjust(int dx1, int dy1, int dx2, int dy2) {
        x += dx1; y += dy1; width += dx2 - dx1; height += dy2 - dy1;
    }

    Rect adjusted(int dx1, int dy1, int dx2, int dy2) const {
        return {x + dx1, y + dy1, width + dx2 - dx1, height + dy2 - dy1};
    }

    Rect translated(int dx, int dy) const {
        return {x + dx, y + dy, width, height};
    }
};

} // namespace ltgui
