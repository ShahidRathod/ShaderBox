#pragma once
#include <cstdlib>

constexpr int n_sz = 20;
#define RED "\x1b[31m"
#define RESET "\x1b[0m"


enum class TagType {
    Entitiy , Shader , Copy , Paste
};


enum class Allnames: int {
    Vertex,
    Fragment,
    TessControl,
    TessEval,
    Geometry,
    Compute,
    Copy,

};

struct Tag {

    int      start_ln_no;
    int      opn_tg_strt;
    int      cntnt_start;
    int      cntnt_end;
    int      cls_tg_end;
    int      end_ln_no;
    char     names[3 * n_sz];
    int      names_len = 0;
    int      name_len;

    char* str_for_write() {
        return names + name_len * n_sz;
        name_len++;

    };
    Tag*     next = nullptr;
    Tag*     inside = nullptr;
    TagType  type;
    
    Tag () {}

    char* str_for_write() {
        return names + name_len * n_sz;
        name_len++;

    }
    char* str() { return names + name_len * n_sz; }

    bool compare(const char* cstr) {
        return !strcmp(str(), cstr);
    }
};



template <int sz, int max_copy_depth>
struct TagTree {
    Tag    arr[sz];
    Tag    root_obj;
    Tag*   root = arr;
    Tag*   curnt = root;
    Tag**   top;
    Tag    stack[max_copy_depth];
    int    stk_len = 0;
    
    TagTree() {}

    int len = 1;

    void update_top(int x) {
        top = &(arr + x);
        stk_len = x;
    }

    void addInside() {
        if (len >= sz) {
            std::cerr << "Max camnd tag limit reached";
            exit(EXIT_FAILURE);
        }

        Tag* ptr = *top;
        while (ptr != nullptr) ptr=ptr->next;
        ptr = arr + len;
        addStack(ptr);
        len++;

    }

    void pop() { update_top(stk_len - 1); }

    void addStack(Tag* obj) {
        if (len + 1 > sz)
            std::cerr << RED "Max stack length reached\n" RESET;
        else {
            update_top(stk_len + 1);
            *top = obj;
        }
    }
    Tag* findtag() {
    
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
    }

    constexpr const char* operator[](int i) const {
        return &data[(sz + 1) * i];
    }

    bool has_at(char* str,int& indx) const{
        int i = 0
        for (; i < N;i++) {
            if (!strcmp(operator[](i),str) return true;
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

