#pragma once
#include <cstdlib>

constexpr int n_sz = 20;
#define RED "\x1b[31m"
#define RESET "\x1b[0m"



using  hashT = unsigned int;

constexpr hashT hash(const char* str,int n =0) {
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

    void add(NodeT* nxt_ptr){
        end->next = nxt_ptr;
        end = end->next;
    }
};

using HashLinkT = Linked<hashT>;
using HashNode = Node<hashT>;


struct Tag {
   
    HashLinkT hash_lst;
    static MemPool<HashNode, 100> hash_pool;

    int      start_ln_no;
    int      opn_tg_strt;
    int      cntnt_start;
    int      cntnt_end;
    int      cls_tg_end;
    int      end_ln_no;
    bool     is_writable;
    char     name_buff[n_sz];
    char     tag_name[n_sz];
    hashT    tag_hash;
    Tag*     next = nullptr;
    Tag*     inside = nullptr;
    TagType  type;

    Tag() {}

    char* str_hash_lst() {
        return name_buff;
    }

    char* str() { return name_buff; }
    
    void commit_hash() {

        hash_lst.add(hash_pool.request_mem());
        hash_lst.end->val = hash(name_buff);

    }
    bool compare(const char* cstr) {
        return hash_compare(str(), (char*)cstr);
    }
};



template <int sz, int max_copy_depth>
struct TagTree {
    Tag    arr[sz];
    Tag    root_obj;
    Tag*   root = arr;
    Tag*   curnt = root;
    Tag**  top;
    Tag    stack[max_copy_depth];
    int    stk_len = 0;

    TagTree() {}

    int len = 1;

    void update_top(int x) {
        *top = (arr + x);
        stk_len = x;
    }

    void addNext() {

        if (len >= sz) {
            std::cerr << "Max tag limit reached";
            exit(EXIT_FAILURE);
        }

        Tag* ptr = *top;
        while (ptr != nullptr) ptr = ptr->next;
        
        ptr = arr + len;
        len++;

    }

    void pop() { update_top(stk_len - 1); }

    void addStack() {

        if (len >= sz) {
            std::cerr << "Max tag limit reached";
            exit(EXIT_FAILURE);
        }

        if (len + 1 > sz)
            std::cerr << RED "Max stack length reached\n" RESET;
        else {
            update_top(stk_len + 1);
            *top = arr+len;
            len++;
        }
    }

    Tag* find_hlpr(HashNode* hash, Tag* ptr,Tag* prnt ) {

        while (ptr != nullptr) {

            if (hash->val == ptr->tag_hash) {
                if (hash->next == nullptr) return ptr;
                find_hlpr(hash->next, ptr->inside,ptr);
                break;
            }

            ptr = ptr->next;
        }

        find_hlpr(hash,prnt->next->inside,prnt->next);/888888888888888888888888888888888*
    }

    Tag* find(HashLinkT hash_link) {
        Tag* res = find_hlpr(hash_link.root,root,root);
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


template <size_t sz, int N> struct ConstexprStr {
    static const int size = (sz + 1) * N;
    char data[size] = { ' ' };
    hashT hashes[size] = {-1};

    constexpr ConstexprStr(const char* str) {
        for (int i = 0; i < size; i++)
            data[i] = ' ';

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
            hashes[i] = hash(operator[] (i));
        }
    }

    constexpr const char* operator[](int i) const {
        return &data[(sz + 1) * i];
    }

    bool has_at(char* str, int& indx) const {
        int i = 0;
        hashT str_h = hash(str);
        for (; i < N;i++) {
            if (hashes[i] == str_h) return true;
        }

        indx = i;
        return false;
    }
};


template <int sz> struct CircularBuff {
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
