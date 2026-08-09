#pragma once
#include <cstdlib>
#include "parser_errors.h"

constexpr int n_sz = 20;
#define RED "\x1b[31m"
#define RESET "\x1b[0m"


template <size_t sz, int N>
struct ConstexprStr {
    static const int size = (sz + 1) * N;
    char data[size] = { ' ' };
    hashT hashes[size] = { -1 };

    constexpr ConstexprStr(const char* str) {
        for (int i = 0; i < size; i++) data[i] = ' ';

        int indx = 0;
        int n = 0;
        int j = 0;
        char c = 0;

        for (int i = 0; i < size && n + j < size; i++) {
            c = str[i];
            if (c == '\0')
                break;
            else if (c == ',') {
                data[n + j] = '\0';
                n = j / sz + 1;
                j += sz;
                continue;
            }
            data[n + j] = c;
            n++;
        }

        for (int i = 0; i < N; i++) {
            hashes[i] = hash(operator[](i));
        }
    }

    constexpr const char* operator[](int i) const {
        return &data[(sz + 1) * i];
    }

    bool has_at(char* str, int& indx) const {
        int i = 0;
        hashT str_h = hash(str);
        for (; i < N; i++) {
            if (hashes[i] == str_h) return true;
        }

        indx = i;
        return false;
    }
};




constexpr int stage_count = 6;
constexpr ConstexprStr <n_sz, stage_count>
subtag_names{ "vertex,fragment,tess_control,tess_eval,geometry,compute," };

constexpr int cammnds = 3;
constexpr ConstexprStr<n_sz, cammnds>
cammnd_tags{ "copy,paste,scope" };

constexpr ConstexprStr <n_sz, stage_count + cammnds>
all_names{ "vertex,fragment,tess_control,tess_eval,geometry,compute,copy,paste,scope" };



using hashT = unsigned int;

constexpr hashT hash(const char* str, int n = 0) {
    hashT hash = 0;
    int k = 0;
    while (str[k] != '\0') {
        int i = n + k;
        hash += (hashT)(str[i]) * (2 << (i % 17));
        k++;
    }
    return hash;
}
bool hash_compare(char* str1, char* str2) { return hash(str1) == hash(str2); }
template <typename T, int sz>
struct MemPool {
    int len = 0;
    T mem[sz];

    T* request_mem() {
        if (len + 1 > sz) {
            std::cerr << "Mem pool memory overflow\n";
            exit(EXIT_FAILURE);
        }
        return mem + len;
        len++;
    }
};

template <typename T>
struct Node {
    T val;
    Node<T>* next;
};

enum class TagType : int {
    Vertex,
    Fragment,
    TessControl,
    TessEval,
    Geometry,
    Compute,
    Copy,
    Paste,
    Scope
};

template <typename T>
struct Linked {
    using NodeT = Node<T>;
    NodeT* root;
    NodeT* end = root;

    
    void add(NodeT* nxt_ptr) {
        end->next = nxt_ptr;
        end = end->next;
    }
};

using HashLinkT = Linked<hashT>;
using HashNode = Node<hashT>;

struct Tag {
    HashLinkT hash_lst;
    static MemPool<HashNode, 100> hash_pool;

    int start_ln_no;
    int opn_tg_strt;
    int cntnt_start;
    int cntnt_end;
    int cls_tg_end;
    int end_ln_no;
    bool is_writable = false;
    char name_buff[n_sz];
    char tag_name[n_sz];
    hashT tag_hash;
    Tag* next = nullptr;
    Tag* inside = nullptr;
    TagType type;

    Tag() {}

    int init_Tag(int depth) { // assumes the tag_name is already written

        if (depth > 1) is_writable = true;
        int type_indx;
        if (!all_names.has_at(name_buff, type_indx) && depth > 1) {
            RAISE_INVALID_TAG_NAME(name_buff);
        }
        return type_indx;
    }


    char* str_hash_lst() { return name_buff; }

    char* str() { return name_buff; }

    void commit_hash() {
        hash_lst.add(hash_pool.request_mem());
        hash_lst.end->val = hash(name_buff);
    }
    bool compare(const char* cstr) { return hash_compare(str(), (char*)cstr); }
};

template <int sz, int max_copy_depth>
struct TagTree {
    Tag arr[sz];
    Tag root_obj;
    Tag* root = arr;
    Tag* curnt = root;
    Tag** top;

    Tag stack[max_copy_depth];
    int stk_len = 0;

    Tag* level[sz];
    int level_offset = 0;
    int level_sz = 0;
    int nxt_level_sz = 0;

    TagTree() {}

    int len = 1;

    void update_top(int x) {
        *top = (arr + x);
        stk_len = x;
    }

    Tag* make_tag() { // gets memory 
        if (len >= sz) {
            std::cerr << "Max tag limit reached";
            exit(EXIT_FAILURE);
        }

        Tag* new_tg = arr + len;
        len++;
        return new_tg;
    }
    
    void addNext(Tag* nxt) { // add to next to current_scope

        Tag* ptr = *top;
        while (ptr != nullptr) ptr = ptr->next;
        ptr = nxt;

    }

    void pop() { update_top(stk_len - 1); }

    void addStack(Tag * ptr) { // just adds into stack whatever ptr is passed
        if (stk_len  >= max_copy_depth)
            std::cerr << RED "Max stack length reached\n" RESET;
        else {
            update_top(stk_len + 1);
            *top = ptr;
        }
    }

    Tag* find_hlpr(HashNode* hash) {

        level_offset += level_sz;
        level_sz = nxt_level_sz;
        nxt_level_sz = 0;

        for (int i = 0; i < level_sz; i++) {

            Tag* inside_ptr = level[i + level_offset];
            while (inside_ptr != nullptr) {

                level[level_offset + level_sz + nxt_level_sz] = inside_ptr->inside;
                nxt_level_sz++;

                if (hash->val == inside_ptr->tag_hash) {
                    if (hash->next == nullptr) return inside_ptr;
                    nxt_level_sz = 1;
                    return  find_hlpr(hash->next); 
                }

                inside_ptr = inside_ptr->next;
            }
        }
        return find_hlpr(hash);
    }

    Tag* find_in_branch (Tag * branch_root, HashNode* hashNode) {
        level[0] = branch_root;
        level_sz = 0;
        level_offset = 1;
        nxt_level_sz = 1;

        return find_hlpr(hashNode);
    }
    Tag* find(HashLinkT hash_link) {

        Tag* res = find_in_branch(root,hash_link.root);
       
        if (!res) {
            std::cerr << RED "Tag not found \n" RESET;
            std::exit(EXIT_FAILURE);
        }
        return res;
    }

};

struct TagWriter {
    FILE* file;

    char* dst;
    char* pen;

    TagWriter(FILE* fl, char* d) {
        file = fl;
        pen = dst = d;
    }

    inline size_t write_by_range(int start, int end) {
        fseek(file, start, SEEK_SET);
        size_t sz_read = fread(pen, sizeof(char), end - start, file);
        pen += sz_read;
        return sz_read;
    }

    void tag_tree_write(Tag* root) {
        if (!root->is_writable) return;

        int st = root->cntnt_start;
        int end;
        Tag* inside = root->inside;

        while (inside != nullptr) {
            end = inside->opn_tg_strt;
            write_by_range(st, end);
            tag_tree_write(inside);
            st = inside->cls_tg_end;
            inside = inside->next;
        }

        write_by_range(st, root->cntnt_end);
    }
};

template <int sz>
struct CircularBuff {
    int len = 0;
    char arr[sz + 1] = { ' ' };

    CircularBuff() { arr[sz] = '\0'; }
    int get_index(int x) { return x % sz; }

    void put_char(char c) {
        arr[get_index(len)] = c;
        arr[sz] = '\0';
        len++;
    }

    int compare_str(const char* str, int str_len) {
        bool is_equal = true;
        for (int i = 0; i < str_len; i++) {
            int indx = get_index(len - str_len + i);
            if (arr[indx] != str[i]) {
                is_equal = false;
                break;
            }
        }
        return is_equal;
    }
};
