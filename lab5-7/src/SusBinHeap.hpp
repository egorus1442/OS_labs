#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <cmath>

class SusBinHeap {
private:
    int current_free = 0;
    std::unordered_map<int, int> id2pos;
    std::vector<int> pos2id;

public:
    SusBinHeap() : current_free(0), id2pos(), pos2id() {}

    bool find(int id) {
        return id2pos.count(id);
    }

    void add_id(int id) {
        if (current_free >= pos2id.capacity()) {
            pos2id.reserve(2 * pos2id.capacity());
        }
        pos2id.push_back(id);
        id2pos[id] = current_free;
        ++current_free;
    }

    int get_parent_id(int id) {
        if (!find(id)) return -1; // ID не найден в куче

        int pos = id2pos[id];
        if (pos == 0) return -1; // Корень не имеет родителя

        return pos2id[(pos - 1) / 2];
    }

    int get_left_id(int id) {
        if (!find(id)) return -1; // ID не найден в куче

        int pos = id2pos[id];
        int left_pos = pos * 2 + 1;
        if (left_pos >= pos2id.size()) return -1; // Левый потомок не существует

        return pos2id[left_pos];
    }

    int get_right_id(int id) {
        if (!find(id)) return -1; // ID не найден в куче

        int pos = id2pos[id];
        int right_pos = pos * 2 + 2;
        if (right_pos >= pos2id.size()) return -1; // Правый потомок не существует
    
        return pos2id[right_pos];
    }

    bool is_left_child(int id) {
        int parent_id = get_parent_id(id);
        return get_left_id(parent_id) == id;
    }

    std::string get_path_to(int id) {
        if (!find(id) or id2pos[id] == 0) return "";

        // Создаём строку, в которой сохранится путь
        int len = std::log2(id2pos[id] + 1);
        std::string path(len, ' '); // Инициализируем строку пробелами

        // Проходимся вверх по куче и сохраняем путь
        int cur = id;
        int i = len - 1;
        while (cur != pos2id[0]) {
            int par = get_parent_id(cur);
            if (is_left_child(cur)) {
                path[i] = 'l';
            } else {
                path[i] = 'r';
            }
            --i;
            cur = par;
        }

        // Возвращаем путь
        return path;
    }
};
