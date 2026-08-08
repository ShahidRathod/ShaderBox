#define _CRT_SECURE_NO_WARNINGS

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "utils.h"
#include "parser_errors.h"


using std::cerr;
using std::cout;

enum class Depth : int {
    file, entity, shader, camnd
};

enum class Space : int {
    NotAllowed, Allowed
};

constexpr int stage_count = 6;
constexpr ConstexprStr <n_sz, stage_count>
subtag_names{ "vertex,fragment,tess_control,tess_eval,geometry,compute," };

constexpr int cammnds = 3;
constexpr ConstexprStr<n_sz, cammnds>
cammnd_tags{ "copy,paste,scope" };

constexpr ConstexprStr <n_sz, stage_count + cammnds>
all_names{ "vertex,fragment,tess_control,tess_eval,geometry,compute,copy,paste,scope" };
struct ShaderHandel {
    char  name[n_sz] = {};
    int   active_shaders[stage_count] = { 0 };
    char* operator[](const char* shdr);
};

static int check_subtag(const char* sbtg_name) {
    bool found = false;

    int indx = stage_count - 1;
    while (indx >= 0) {
        if (strcmp(sbtg_name, subtag_names[indx]) == 0) {
            found = true;
            break;
        }
        indx--;
    }

    if (!found) {
        RAISE_INVALID_SUBTAG_NAME(sbtg_name);
    }

    return indx;
}



template <int b_sz, int n> struct ShaderReader {

    bool at_comnt = false;
    bool in_tag = false;
    char cursr = 0;
    static constexpr int delim_len = 2 * n_sz;

    CircularBuff<delim_len> delim_tkn;
    char buffer[b_sz] = {};

    char* write_ptr = buffer;

    int ln_no = 1;
    int char_no = 0;

    ShaderHandel handel[n];
    int mem_left = b_sz;
    int sz = 0;
    bool skip_newln = true;

    ShaderHandel* curnt_element = handel;
    int curnt_indx = 0;
    FILE* file;

    int depth = 0;
    TagTree<20, 5> tgtree;

    Tag* currnt_tag = *tgtree.top;

    void inscope() {
        tgtree.addStack();
        currnt_tag = *tgtree.top;
        depth++;
    }

    void outscope() {
        tgtree.pop();
        currnt_tag = *tgtree.top;
        depth--;
    }
   
    hashT currnt_tag_hash() { currnt_tag->hash_lst.root->val; }

    char get_nxt() {
        int c = buffer[char_no];

        cursr = (char)c;

        if (delim_tkn.compare_str(R"(\\)", 2))
            at_comnt = true;

        if (cursr == '\n') {
            ln_no++;
            at_comnt = false;
        }

        if (c != ' ')
            delim_tkn.put_char(c);
        char_no++;
        return c;
    }

    bool skip_whitespc() {
        bool hit_newln = false;
        while ((cursr != EOF) && isspace(cursr)) {
            if (cursr == '\n') {
                hit_newln = true;
            }
            get_nxt();
        }
        return hit_newln;
    }


    bool is_nxt_token(char c ) {
        skip_whitespc();
        get_nxt();
        return cursr != c;
    }

    
    void cpy_tag_str(char* dst,bool expect_spc,char end_delim) {

        if (skip_whitespc()) {
            RAISE_NO_NEWLINE_INSIDE_TAGS;
        }

        // firt character in tag after the whitespace being skipped is first
        // character of name

        if (!isalpha(cursr)) {
            RAISE_SHADER_NAME_INVALID_START(cursr);
        }


        int t_name_indx = 0;

        char temp_name[n_sz] = {};

        while (cursr!=end_delim) {

            if (isspace(cursr)) {
                
                if (!expect_spc) 
                    break; // if space is not expected then break
                skip_whitespc();       
                if (cursr = end_delim) 
                    break; //skip white space then check for end_delim if space is 
                else {
                    RAISE_SHADER_NAME_HAS_WHITESPACE;
                    // if after skipping whitespc cursr != end_delim then error
                }
            }

            if (t_name_indx >= n_sz) {
                RAISE_SHADER_NAME_TOO_LONG(n_sz, dst);
            }

            if (isalnum(cursr) == 0 && cursr != '_') {
                RAISE_SHADER_NAME_INVALID_CHAR(cursr);
            }

            temp_name[t_name_indx++] = cursr;
            get_nxt();
        }

        get_nxt();
        strcpy(dst, temp_name);
        dst[t_name_indx + 1] = '\n';
    }

    

    void initiate_tag(bool expect_spc) {

        char c = (expect_spc)? '<' : ' ';
        cpy_tag_str(currnt_tag->tag_name, expect_spc,c);
        currnt_tag->tag_hash = hash(currnt_tag->tag_name);
        currnt_tag->cntnt_start = char_no;
        
    }

    void parse_scope_chain() {

        while (cursr != '>') {
            cpy_tag_str(currnt_tag->str_hash_lst(), true, ':');
            skip_whitespc();
            currnt_tag->commit_hash();
        }

        
    }

    bool parse_tag() {
        int tag_start = char_no - 1;
        int start_ln = ln_no;

        bool is_cls = !skip_whitespc() && cursr == '/';

        if (is_cls) {
            char cls_name[n_sz];
            
            cpy_tag_str (cls_name, false,'>');

            bool is_same = currnt_tag_hash() == hash(cls_name);
            if (!is_same) RAISE_CLOSE_TAG_MISMATCH();
            currnt_tag->cntnt_end = tag_start;
            currnt_tag->end_ln_no = start_ln;
            currnt_tag->cls_tg_end = char_no;
            outscope();
            return is_cls;
        }

        inscope();
        currnt_tag->opn_tg_strt = tag_start;
        bool in_cntnt = depth > 2;
        bool expect_spc = (is_cls ^ in_cntnt) && in_cntnt;

        initiate_tag(expect_spc);
        char* first_str = currnt_tag->tag_name;

        int indx;
        if (!all_names.has_at(first_str, indx) && depth > 1) {
            RAISE_INVALID_TAG_NAME(currnt_tag->str());
        }

        if (expect_spc) {

            TagType type;
            if (indx < (int)(TagType::Copy)) {
                RAISE_INVALID_TAG_NAME(currnt_tag->str());
            }

            type = (TagType)(indx);
            parse_scope_chain();

        }
        return is_cls;
    }

    void content_loop() {
        while ((cursr != '<' && !at_comnt) && cursr != EOF)
            get_nxt();
    }

    void read_element() {
        char* element_name = curnt_element->name;

        is_nxt_token_tag();
        initiate_tag(element_name);

        currnt_tag.start = char_no;

        char name_buff[n_sz] = {};

        for (int i = 0; i < stage_count; i++) {
            is_nxt_token_tag();
            bool is_cls = !skip_whitespc() && cursr == '/';
            if (is_cls)
                get_nxt();

            initiate_tag(name_buff);

            if (is_cls) {
                if (strcmp(name_buff, element_name) != 0) {
                    RAISE_ELEMENT_TAG_MISMATCH(element_name, name_buff, curnt_indx);
                }

                curnt_element->end = ln_no;
                break;
            }
            else {
                int type_indx = check_subtag(name_buff);

                curnt_element->shdr_line_no[2 * type_indx] = ln_no;

                if ((curnt_element->active_shaders[type_indx]++) > 1) {
                    RAISE_SHADER_ALREADY_DEFINED(
                        name_buff,
                        curnt_element->shdr_line_no[2 * type_indx],
                        curnt_element->shdr_line_no[2 * type_indx + 1]);
                }

                read_shader_content(type_indx, name_buff);
            }
        }
    }



    bool is_end_or_cmnd() {
        bool tag_end;
        if (cursr == '/') {
            tag_end = true;
        }
        else {
            char cmd_name[n_sz];
            initiate_tag(cmd_name, true);

            bool is_cpy = !strcmp(cmd_name, cammnd_tags[0]);
            bool is_pst = !strcmp(cmd_name, cammnd_tags[1]);

            if (is_cpy or is_pst) {
                char name[n_sz];
                initiate_tag(name);
                if (is_cpy) {
                    Tag cpy_range;
                    get_content_len(cpy_range.cntnt_start, cpy_range.cntnt_end, cpy_range.opn_tg_strt);
                }
            }
            tag_end = false;
        }

        return !tag_end; // why negetion: content_loop continues until tag is not end
    }

    int get_content_len(int& cntn_start, int& cntn_end, int& tag_start) {
        cntn_start = ftell(file) - 1;
        bool cls_found = false;
        bool has_newln;

        do {
            has_newln = false;
            content_loop();

            cntn_end = ftell(file) - 1;
            get_nxt(); // to get past the '<'
            has_newln = skip_whitespc();

        } while (is_end_or_cmnd());

        // if (has_newln) PRINT_EXIT("No newline character inside tags\n");
        if (has_newln)
            printf(RED "Warning newline character in tag "
                "will cause undefined behaviour.\n" RESET);

        tag_start = ftell(file);
        long len = cntn_end - cntn_start;

        get_nxt(); // to make the cursr past the '/' charater
        // because in the cpy_tag_name_at has skip_whitepsc
        //  and it will terminate immediately if the cursr is not a whitespc
        return len;
    }

    int write_from_at(int start, int len, char* write_dst) {
        fseek(file, start, SEEK_SET);
        size_t sz_read = fread(write_dst, sizeof(char), len, file);
        return sz_read;
    }

    template <bool reading_shader>
    void read_tag_content(int type_indx, char* opn_tg) {


        int len = get_content_len();

        if constexpr (reading_shader) {
            curnt_element->
                shdr_line_no[2 * type_indx + 1] = ln_no;
            // end line of the shader
        }

        char cls_shdr_tg[max_subtg_name_len];
        initiate_tag(cls_shdr_tg);

        long tg_end = ftell(file);

        long tg_len = tg_end - tg_strt;

        // int i  = check_subtag(cls_shdr_tg);

        if (strcmp(opn_tg, cls_shdr_tg) != 0) {
            RAISE_CLOSE_TAG_MISMATCH(cls_shdr_tg, opn_tg);
        }
    }

    void read_shader_content(int type_indx, char* opn_shdr_tg) {
        return read_tag_content<true>(type_indx, opn_shdr_tg);
    }

    FILE* open_file(const char* file_name) {
        FILE* file = fopen(file_name, "r");
        if (!file) {
            RAISE_FILE_NOT_FOUND;
        }
        return file;
    }

    ShaderReader() {}

    ShaderReader(const char* file_name) {

        file = open_file(file_name);
        fseek(file, 0, SEEK_END);
        long file_sz = ftell(file);

        if (file_sz >= b_sz) {
            RAISE_INSUFFICIENT_SPACE;
        }

        fread(buffer, sizeof(char), file_sz, file);
        fclose(file);

        for (int shader_indx = 0; shader_indx < n; shader_indx++) {
            skip_whitespc();
            if (char_no >= file_sz) {
                RAISE_MISSING_SHADERS(handel[shader_indx].name, shader_indx);
            }
            read_element();
            curnt_element++;
            curnt_indx++;

        }


    }

};


int main() {
 


}



// Currently: we write into the write_ptr the moment the content is approched
// What would be better: parse the content
// first and then write using range_tree_write
// benefit of parsing the whole file in this format is that paring technique would become
// consistent from top to bottom
//check if the get_content_len can accept the range object every time


/*
current approch: Writing the instant we encounter the content.
is_nxt_tkn_tag checking wheather the nxt char is '<' throwing
error if not. token name mismatch is not generalised.the sys call at every
fget at every step. we are egarly wtiting the buffers during constructor.
every tag that shares the same parent is next via linked list. when entering a tag we
add it on the stack , after the tag is parsed we pop it out.
this way the top of stack is the is the tag were in.
the content in outside the Shaders tag 'vertex','fragment' etc . will be ignored .
use enums for layers convention
while file is being parsed, the whole file can be buffered into the buff arr
after a given shader is requested


ALWAYS COUNT THE MONEY
*/


/*
The end fucntion must handel the continue and 
end contdition and the cpy_plain_name returns the same 
*/