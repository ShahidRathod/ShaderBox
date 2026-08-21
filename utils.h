#pragma once
#include <cstdlib>
#include "parser_errors.h"

constexpr int n_sz = 20;
#define RED "\x1b[31m"
#define RESET "\x1b[0m"


using  hashT = unsigned long int;

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

template <size_t sz, int N>
struct ConstexprStr {
    static const int size = (sz + 1) * N;
    char data[size] = { ' ' };
    hashT hashes[size] = { 1 };

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
        bool found = false;
        int i = 0;
        hashT str_h = hash(str);
        for (; i < N; i++) {
            if (hashes[i] == str_h) {
                found = true;
                break;
            }
        }

        indx = i;
        return found;
    }
};




constexpr int stage_count = 6;
constexpr ConstexprStr <n_sz, stage_count>
subtag_names{ "vertex,fragment,tess_control,tess_eval,geometry,compute," };

constexpr int cammnds = 3;
constexpr ConstexprStr<n_sz, cammnds>
cammnd_tags{ "copy,paste,scope," };

constexpr ConstexprStr <n_sz, stage_count + cammnds>
all_names{ "vertex,fragment,tess_control,tess_eval,geometry,compute,copy,paste,scope," };






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
        int len_temp = len;
        len++;
        return mem + len_temp;

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

enum class OpenClose : int { Open, Close };
struct TagInfo {
    TagType type;
    OpenClose opncls;
    bool is_tag;
};


template <typename T>
struct Linked {
    using NodeT = Node<T>;
    NodeT* root = nullptr;
    NodeT* end = nullptr;
    void add(NodeT* nxt_ptr) {
        if (!root)  root = nxt_ptr;
        else {
            NodeT* ptr = root;
            while (ptr->next != nullptr) ptr = ptr->next;
            ptr->next = nxt_ptr;
        }
        end = nxt_ptr;
    }
};

using HashLinkT = Linked<hashT>;
using HashNode = Node<hashT>;

struct Tag {

    static int counter;
    static MemPool<HashNode, 100> hash_pool;

    char buffer[n_sz] = { ' ' };
    char tag_name[n_sz] = "new_tag";
    int tags_inside = 0;
    int tag_no;
    int start_ln_no;
    int opn_tg_start;
    int cntnt_start;
    int cntnt_end;
    int cls_tg_end;
    int end_ln_no;
    bool is_writable = false;


    HashLinkT hash_lst;
    hashT tag_hash;
    Tag* next = nullptr;
    Tag* inside = nullptr;
    TagType type;
    bool is_init = false;

    Tag() {
        tag_no = counter++;
    }

    inline int tag_len() { return cntnt_start - opn_tg_start + cls_tg_end - cntnt_end; }
    inline int span() { return cls_tg_end - opn_tg_start; }

    int init_Tag(int depth) { // assumes the tag_name is already written
        if (depth > 1) is_writable = true;
        int type_indx;
        if (!all_names.has_at(tag_name, type_indx) && depth > 1) {
            RAISE_INVALID_TAG_NAME(tag_name);
        }
        is_init = true;
        return type_indx;
    }


    void commit_name() {
        strcpy(tag_name, buffer);
    }

    void commit_hash() {
        tag_hash = hash(buffer);
    }

    void check_end_same(int ln_no, int char_no) {

        bool is_same = hash(buffer) == hash(tag_name);
        if (!is_same) RAISE_CLOSE_TAG_MISMATCH();

    }

    void append_hash_lnk() {
        if (tag_no == 7) {
            int c = counter;
        }
        HashNode* ptr = hash_pool.request_mem();
        ptr->val = hash(buffer);
        hash_lst.add(ptr);
    }

    void addInside(Tag* nxt) { // add to next to current_scope

        if (!inside) inside = nxt;
        else {
            Tag* ptr = inside; // ptr by reference
            while (ptr->next) ptr = ptr->next;
            ptr->next = nxt;
        }

    }

};

int Tag::counter = 0;
MemPool<HashNode, 100> Tag::hash_pool;

template <int sz, int max_copy_depth>
struct TagTree {
    
    Tag safety_padding;
    Tag arr[sz];
    Tag root_obj;
    Tag* root = arr;
    Tag* curnt = root;

    Tag* stack[max_copy_depth];

    int stk_len = 0;


    static constexpr int  l_sz = sz * 10;

    Tag* level[l_sz];
    int level_offset = 0;
    int level_sz = 0;
    int nxt_level_sz = 0;
    int index = 0;

    TagTree() {
        stack[0] = arr;
    }

    int len = 1;

    Tag* top() { return stack[stk_len]; }

    Tag* end() { return arr + len-1;}

    Tag* make_tag() { // gets memory 
        if (len >= sz) {
            std::cerr << "Max tag limit reached";
            exit(EXIT_FAILURE);
        }

        Tag* new_tg = arr + len;
        len++;
        return new_tg;
    }

    inline void inc_tags_inside() {
        for (int i = 0; i < stk_len;i) {
            stack[i]->tags_inside++;
        }
    }
    void pop() { stk_len--; }

    void addStack(Tag* ptr) { // just adds into stack whatever ptr is passed

        if (stk_len >= max_copy_depth)
            std::cerr << RED "Max stack length reached\n" RESET;
        else {
            stk_len++;
            stack[stk_len] = ptr;
        }
        inc_tags_inside();
    }

    void add_nxt_layer(Tag* inside_ptr) {

        int write_index = level_offset + level_sz + nxt_level_sz;

        if (write_index >= l_sz) {
            std::cerr << RED "level overflow\n" RESET;
        }

        if (inside_ptr->inside) {
            level[level_offset + level_sz + nxt_level_sz] = inside_ptr->inside;
            nxt_level_sz++;
        }
    }

    Tag* nxt_tag_on_layer() {
        Tag* res = level[index + level_offset];
        index++;
        if (index > level_sz) {
            level_offset += level_sz;
            level_sz = nxt_level_sz;
            nxt_level_sz = 0;
            index = 0;
        }
        //std::cout <<"name: " << "\n";
        return res;
    }

    Tag* find_hlpr(HashNode* hash, bool check_resursive = false) {

        while (level_sz != 0) {
            Tag* inside_ptr = nxt_tag_on_layer();
            for (;
                inside_ptr;
                inside_ptr = inside_ptr->next) {

                if (inside_ptr->type == TagType::Paste && !check_resursive) 
                    continue;
                
                add_nxt_layer(inside_ptr);

                if (hash->val == inside_ptr->tag_hash) {
                    hash = hash->next;
                    if (hash == nullptr) return inside_ptr;

                    nxt_level_sz = 1;
                    break;
                }

            }
        }
        return nullptr;
    }

    Tag* find_in_branch(Tag* branch_root, HashNode* hashNode,bool check_resursive = false) {
        memset(level,0,sizeof(level));
        level[0] = branch_root;
        level_sz = 1;
        level_offset = 0;
        nxt_level_sz = 0;
        return find_hlpr(hashNode,check_resursive);
    }



    Tag* find(HashLinkT hash_link) {

        Tag* res = find_in_branch(root, hash_link.root);
        if (!res) {
            std::cerr << RED "Tag not found \n" RESET;
            std::exit(EXIT_FAILURE);
        }
        return res;

    }

};

struct WriteRange { int start, end; };


struct TagWriter {
    char* src;
    char* dst = nullptr;
    char* pen;
    Tag* root;
    char* tag_content() {return dst;}


    inline size_t write_by_range(int start, int end) {
        int len = end - start;
        memcpy(pen,src+start,len);
        pen += len;
        return len;
    }

    int cntn_len_of_tag(Tag* tag) {
        int cntn_len = tag->span();
        int i = 0;
        for (; i<tag->tags_inside;i++) {
            if (tag[i].type == TagType::Paste) {
                cntn_len += cntn_len_of_tag(tag[i].inside);
            }
            cntn_len -= tag[i].tag_len();
        }
        return cntn_len;
    }

    void tag_tree_write(Tag* root) {
        int end, start;
        int i = 0;
        for (;i<root->tags_inside;i++) {
            if (!root[i].is_writable) continue;
            if (root[i].type == TagType::Paste) {
                tag_tree_write(root[i].inside);
                continue;
            }
            start = root[i].cntnt_start;
            end = root[i].cntnt_end;
            if (root[i].tags_inside)
                end = root[i + 1].opn_tg_start;
            write_by_range(start, end);
        }
    }


    TagWriter(char* fl, Tag* rt ) {
        src = fl;
        root = rt;
        pen = dst = new char[cntn_len_of_tag(root)];
        tag_tree_write(root);
    }

    ~TagWriter() {
        delete[] dst;
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