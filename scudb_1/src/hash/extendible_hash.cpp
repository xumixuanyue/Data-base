#include <list>
#include <cassert>
#include <functional>
#include <bitset>
#include <iostream>

#include "hash/extendible_hash.h"
#include "page/page.h"

using namespace std;

namespace scudb {
/*
 * constructor
 * array_size: fixed array size for each bucket
 */
template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size) :bucket_size_(size)
{
  bucket_count_=0;
  pair_count_=0;
  depth=0;
  bucket_.emplace_back(new Bucket(0, 0));
  bucket_count_ =1;
}

/*
 * helper function to calculate the hashing address of input key
 */
template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) {
  //哈希模板定义一个函数对象，实现了散列函数。返回值为32位地址
  return hash<K>()(key);
}

/*
 * helper function to return global depth of hash table
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetGlobalDepth() const {
    lock_guard<mutex> lock(mutex_); //加锁
    return depth; //返回全局深度
}

/*
 * helper function to return local depth of one specific bucket
 * NOTE: you must implement this function in order to pass test
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {
    lock_guard<mutex> lock(mutex_); //加锁
    //在桶数组中找到Id对应的桶
    if(bucket_[bucket_id]) {
        return bucket_[bucket_id]->depth;
    }
    return -1;
}

/*
 * helper function to return current number of bucket in hash table
 */
template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const {
    lock_guard<mutex> lock(mutex_); //加锁
    return bucket_count_;
}

/*
 * lookup function to find value associate with input key
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {
  lock_guard<mutex> lock(mutex_); //加锁
  //根据全局桶的深度取i个有效位，得到桶的编号
  size_t position = HashKey(key) & ((1 << depth) - 1);
  //使用map的find函数，返回被查找元素的位置，没有则返回map.end().
  if(bucket_[position] && (bucket_[position]->items.find(key) != bucket_[position]->items.end())) {
          value = bucket_[position]->items[key];
          return true;
  }
  return false;
}

/*
 * delete <key,value> entry in hash table
 * Shrink & Combination is not required for this project
 */
template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {
    lock_guard<mutex> lock(mutex_);
    //同理，取i个有效位
    size_t position = HashKey(key) & ((1 << depth) - 1);
    size_t cnt = 0;

    if(bucket_[position]) {
        auto tmp_bucket = bucket_[position];
        cnt = tmp_bucket->items.erase(key);
        pair_count_ -= cnt;
    }
    return cnt != 0;
}

/*
 * insert <key,value> entry in hash table
 * Split & Redistribute bucket when there is overflow and if necessary increase
 * global depth
 */
template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
    lock_guard<mutex> lock(mutex_);
    size_t bucket_id = HashKey(key) & ((1 << depth) - 1);
    // 找到插入的位置，如果为空则新建一个桶
    if(bucket_[bucket_id] == nullptr) {
        bucket_[bucket_id] = std::make_shared<Bucket>(bucket_id, depth);
        ++bucket_count_;
    }
    auto bucket = bucket_[bucket_id];
    // 如果该位置有值，则覆盖
    if(bucket->items.find(key) != bucket->items.end()) {
        bucket->items[key] = value;
        return;
    }
    // 插入键值对
    bucket->items.insert({key, value});
    // 键值对数目增加
    pair_count_++;
    // 如果超出了设定的桶大小，则分裂桶，是否扩展桶数组取决于分裂前的桶的深度是小于还是等于全局深度
    // 默认取右i位
    if(bucket->items.size() > bucket_size_) {
        // 先记录旧的下标和深度
        auto old_index = bucket->id;
        auto old_depth = bucket->depth;
        shared_ptr<Bucket> new_bucket = split(bucket);

        // 若分裂后桶的局部深度大于全局深度，则要扩展桶数组
        if(bucket->depth > depth ) {
            auto size = bucket_.size();
            auto factor = (1 << (bucket->depth - depth));
            depth = bucket->depth;
            bucket_.resize(bucket_.size()*factor);
            // 修改和添加要插入的桶和新建的桶
            bucket_[bucket->id] = bucket;
            bucket_[new_bucket->id] = new_bucket;
            // 遍历桶数组
            for(size_t i = 0; i < size; ++i) {
                if(bucket_[i]) {
                    if(i < bucket_[i]->id){
                        bucket_[i].reset();
                    } else {
                        //令所有未分配桶的桶指针指向最近，地址最小的那个桶
                        auto step = 1 << bucket_[i]->depth;
                        for(size_t j = i + step; j < bucket_.size(); j += step) { 
                            bucket_[j] = bucket_[i];
                        }
                    }
                }
            }
        } else {
            for (size_t i = old_index; i < bucket_.size(); i += (1 << old_depth)) {
                bucket_[i].reset();
            }
            bucket_[bucket->id] = bucket;
            bucket_[new_bucket->id] = new_bucket;
            auto step = 1 << bucket->depth;
            for (size_t i = bucket->id + step; i < bucket_.size(); i += step) {
                bucket_[i] = bucket;
            }
            for (size_t i = new_bucket->id + step; i < bucket_.size(); i += step) {
                bucket_[i] = new_bucket;
            }
        }
    }
}


// 分裂新桶
template <typename K, typename V>
std::shared_ptr<typename ExtendibleHash<K, V>::Bucket>
ExtendibleHash<K, V>::split(std::shared_ptr<Bucket> &b) {
    // 先创建一个新桶
    auto res = make_shared<Bucket>(0, b->depth);//创建与b同深度的桶
    while(res->items.empty()) {
        // 深度加一
        b->depth++;
        res->depth++;
        // 用for循环实现两个桶的分配
        for(auto it = b->items.begin(); it != b->items.end();) {
            if (HashKey(it->first) & (1 << (b->depth - 1))) {
                res->items.insert(*it);
                res->id = HashKey(it->first) & ((1 << b->depth) - 1);
                it = b->items.erase(it);
            } else {
                ++it;
            }
        }
        //深度不够，还要进行循环
        if(b->items.empty()) {
            b->items.swap(res->items);
            b->id = res->id;
        }
    }
    bucket_count_++;
    return res;
}

template class ExtendibleHash<page_id_t, Page *>;
template class ExtendibleHash<Page *, list<Page *>::iterator>;
// test purpose
template class ExtendibleHash<int, string>;
template class ExtendibleHash<int, list<int>::iterator>;
template class ExtendibleHash<int, int>;
} // namespace scudb
