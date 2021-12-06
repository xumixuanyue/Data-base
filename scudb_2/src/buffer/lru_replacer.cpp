/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace scudb {

    template <typename T> LRUReplacer<T>::LRUReplacer() : size_c(0) {
        head = new node();
        tail = head;
    }

    template <typename T> LRUReplacer<T>::~LRUReplacer() = default;

/*
 * Insert value into LRU
 */
    template <typename T> void LRUReplacer<T>::Insert(const T &value) {
        std::lock_guard<std::mutex> lock(mutex_w);
        
        auto it = tables.find(value);
        // 此时队列中没有该页面，将页面插入队尾（下次轮转从此往后开始）
        if (it == tables.end()) {
            tail->next = new node(value, tail);
            tail = tail->next;
            tables.emplace(value, tail);
            size_c++;
        }
        //页面已经在队列中，因为最近再次访问到，需将它放到队尾
        else {
            if (it->second != tail) {
                // 先从原位置移除
                node* pre = it->second->pre;
                node* cur = pre->next;
                pre->next = std::move(cur->next);
                pre->next->pre = pre;

                // 再放到尾部
                cur->pre = tail;
                tail->next = std::move(cur);
                tail = tail->next;
            }
        }
    }

/* If LRU is non-empty, pop the head member from LRU to argument "value", and
 * return true. If LRU is empty, return false
 */
    template <typename T> bool LRUReplacer<T>::Victim(T& value) {
        std::lock_guard<std::mutex> lock(mutex_w);
        //如果队列为空返回false
        if (size_c == 0) {
            return false;
        }
        //否则删除最近最久未被使用的页面
        value = head->next->data;
        head->next = head->next->next;
        if (head->next != nullptr) {
            head->next->pre = head;
        }

        tables.erase(value);
        if (--size_c == 0) {
            tail = head;
        }

        return true;
    }

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
    template <typename T> bool LRUReplacer<T>::Erase(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_w);
        // 删除队列中任意位置页面
        auto it = tables.find(value);
        if (it != tables.end()) {
            // 如果删除页面在中间则需要重新布置
            if (it->second != tail) {
                node* pre = it->second->pre;
                node* cur = pre->next;
                pre->next = std::move(cur->next);
                pre->next->pre = pre;
            }
            //否则直接在队尾删除
            else {
                tail = tail->pre;
                delete tail->next;
            }
            //如果队列删空则头尾指针指向一处
            tables.erase(value);
            if (--size_c == 0) {
                tail = head;
            }
            return true;
        }

        return false;
    }

    template <typename T> size_t LRUReplacer<T>::Size() {
        std::lock_guard<std::mutex> lock(mutex_w);
        return size_c;
    }

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace scudb
