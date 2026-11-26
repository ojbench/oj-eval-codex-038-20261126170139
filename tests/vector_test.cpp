#include "vector.hpp"
#include <algorithm>
#include <cassert>
#include <random>
#include <string>
#include <vector>

struct Item {
    static int live;
    int value;
    explicit Item(int v) : value(v) { ++live; }
    Item(const Item &o) : value(o.value) { ++live; }
    Item(Item &&o) noexcept : value(o.value) { ++live; o.value = -1; }
    Item &operator=(const Item &) = default;
    Item &operator=(Item &&) = default;
    ~Item() { --live; }
};
int Item::live = 0;

int main() {
    std::mt19937 rng(38);
    sjtu::vector<int> a;
    std::vector<int> b;
    for (int step = 0; step < 30000; ++step) {
        unsigned op = rng() % 5;
        int value = int(rng() % 1000);
        if (op == 0 || b.empty()) { a.push_back(value); b.push_back(value); }
        else if (op == 1) {
            size_t i = rng() % (b.size() + 1);
            a.insert(i, value); b.insert(b.begin() + i, value);
        } else if (op == 2) {
            size_t i = rng() % b.size();
            a.erase(i); b.erase(b.begin() + i);
        } else if (op == 3) { a.pop_back(); b.pop_back(); }
        else { std::sort(a.begin(), a.end()); std::sort(b.begin(), b.end()); }
        assert(a.size() == b.size());
        for (size_t i = 0; i < b.size(); ++i) assert(a[i] == b[i]);
    }
    {
        sjtu::vector<Item> v;
        for (int i = 0; i < 1024; ++i) v.push_back(Item(i));
        v.push_back(v[0]);
        assert(v.back().value == 0);
        v.insert(0, v[100]);
        assert(v.front().value == 100);
        auto copy = v;
        v.erase(0);
        v.clear();
        assert(copy.size() == 1026);
        copy = copy;
        auto moved = static_cast<sjtu::vector<Item> &&>(copy);
        assert(copy.empty());
    }
    assert(Item::live == 0);
    sjtu::vector<int> other;
    bool caught = false;
    try { (void)(a.begin() - other.begin()); } catch (const sjtu::invalid_iterator &) { caught = true; }
    assert(caught);
    sjtu::vector<int>::const_iterator ci = a.begin();
    assert(ci == a.begin());
}
