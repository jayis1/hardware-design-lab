/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"

/* Label every observed vertex with the lowest vertex in its component.
 * Sixteen bounded expansion passes suffice even for a Hamiltonian path.
 * No heap, recursion, coordinate inference or mechanical transition claims. */
static unsigned label_components(uint16_t settings, uint8_t labels[16])
{
    unsigned count = 0;
    for (unsigned word = 0; word < 16; ++word)
        labels[word] = 16;
    for (unsigned root = 0; root < 16; ++root) {
        if (!(settings & (1u << root)) || labels[root] != 16)
            continue;
        ++count;
        labels[root] = (uint8_t)root;
        for (unsigned pass = 0; pass < 16; ++pass) {
            bool changed = false;
            for (unsigned word = 0; word < 16; ++word) {
                if (labels[word] != root)
                    continue;
                for (unsigned bit = 0; bit < 4; ++bit) {
                    unsigned neighbor = word ^ (1u << bit);
                    if ((settings & (1u << neighbor)) && labels[neighbor] == 16) {
                        labels[neighbor] = (uint8_t)root;
                        changed = true;
                    }
                }
            }
            if (!changed)
                break;
        }
    }
    return count;
}

unsigned engine_components(const Engine *e)
{
    uint8_t labels[16];
    return label_components(e->settings, labels);
}

unsigned engine_bridges(const Engine *e)
{
    uint8_t labels[16];
    label_components(e->settings, labels);
    unsigned bridges = 0;
    for (unsigned word = 0; word < 16; ++word) {
        if (e->settings & (1u << word))
            continue;
        unsigned adjacent_components = 0;
        for (unsigned bit = 0; bit < 4; ++bit) {
            unsigned neighbor = word ^ (1u << bit);
            if (labels[neighbor] < 16)
                adjacent_components |= 1u << labels[neighbor];
        }
        /* A bridge candidate must join DISTINCT islands, not two vertices
         * already joined through other observed configurations. */
        if (engine_popcount(adjacent_components) >= 2)
            bridges |= 1u << word;
    }
    return bridges;
}

unsigned engine_path(const Engine *e, unsigned start, unsigned end, uint8_t path[16])
{
    if (!path || start > 15 || end > 15)
        return 0;
    if (!(e->settings & (1u << start)) || !(e->settings & (1u << end)))
        return 0;
    uint8_t queue[16], parent[16];
    for (unsigned i = 0; i < 16; ++i)
        parent[i] = 16;
    unsigned head = 0, tail = 1;
    queue[0] = (uint8_t)start;
    parent[start] = (uint8_t)start;
    while (head < tail && parent[end] == 16) {
        unsigned word = queue[head++];
        /* Numeric neighbor order gives a deterministic shortest witness. */
        for (unsigned neighbor = 0; neighbor < 16; ++neighbor) {
            if (!(e->settings & (1u << neighbor)) || parent[neighbor] != 16)
                continue;
            if (engine_popcount(word ^ neighbor) != 1)
                continue;
            parent[neighbor] = (uint8_t)word;
            queue[tail++] = (uint8_t)neighbor;
        }
    }
    if (parent[end] == 16)
        return 0;
    unsigned length = 0;
    for (unsigned word = end;; word = parent[word]) {
        path[length++] = (uint8_t)word;
        if (word == start)
            break;
    }
    for (unsigned i = 0; i < length / 2; ++i) {
        uint8_t swap = path[i];
        path[i] = path[length - 1 - i];
        path[length - 1 - i] = swap;
    }
    return length;
}
