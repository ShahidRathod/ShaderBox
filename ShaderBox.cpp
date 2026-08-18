#define _CRT_SECURE_NO_WARNINGS

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "utils.h"

using GLuint = unsigned int;
using GLenum = unsigned int;

enum {
    GL_VERTEX_SHADER = 0x8B31,
    GL_FRAGMENT_SHADER = 0x8B30,
    GL_TESS_CONTROL_SHADER = 0x8E88,
    GL_TESS_EVALUATION_SHADER = 0x8E87,
    GL_GEOMETRY_SHADER = 0x8DD9,
    GL_COMPUTE_SHADER = 0x91B9
};

int GLshader_to_index(GLenum enm)
{
    switch (enm) {
    case GL_VERTEX_SHADER:          return 0;
    case GL_FRAGMENT_SHADER:        return 1;
    case GL_TESS_CONTROL_SHADER:    return 2;
    case GL_TESS_EVALUATION_SHADER: return 3;
    case GL_GEOMETRY_SHADER:        return 4;
    case GL_COMPUTE_SHADER:         return 5;

    default: return -1;
    }
}

GLuint gl_compile_shader(GLenum type, const char* src) { return GLuint{}; }

using std::cerr;
using std::cout;

enum class Depth : int {
    file, entity, shader, camnd
};

enum class Space : int {
    NotAllowed, Allowed
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


constexpr bool contains(char target, const char* str)
{
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] == target)
            return true;
    }
    return false;
}




struct CompiledShaders {
    GLuint shaders[stage_count];
};

template <int b_sz> struct ShaderReader {

    bool at_comnt = false;
    bool in_tag = false;
    char cursr = 0;
    static constexpr int delim_len = 2 * n_sz;

    
    char buffer[b_sz] = {};

    char* write_ptr = buffer;

    int ln_no = 1;
    int char_no = 0;

    int mem_left = b_sz;
    int sz = 0;
    bool skip_newln = true;

    int curnt_indx = 0;
    FILE* file;
    long file_sz = 0;
    int depth = 0;
    TagTree<50, 10> tgtree;

    CircularBuff<delim_len> delim_tkn;
    int   active_shaders[stage_count] = { 0 };
    Tag* currnt_tag = tgtree.top();


    void inscope() {
        Tag* old_scope = tgtree.top();
        Tag* new_tag = tgtree.make_tag();

        old_scope->addInside(new_tag);
        tgtree.addStack(new_tag);
        currnt_tag = tgtree.top(); // should be same as new_tg
        bool v = currnt_tag == new_tag;
        depth++;
    }

    void outscope() {
        currnt_tag->commit_name();
        tgtree.pop();
        currnt_tag = tgtree.top();
        depth--;
    }

    hashT currnt_tag_hash() { return currnt_tag->tag_hash; }

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
    char seek(int c_no) {
        cursr = buffer[c_no];
        char_no = c_no + 1;
    }

    bool skip_whitespc() {
        bool hit_newln = false;
        while (isspace(cursr)) {
            if (cursr == '\n') {
                hit_newln = true;
            }
            get_nxt();
        }
        return hit_newln;
    }


    bool is_nxt_token(char c) {
        skip_whitespc();
        get_nxt();
        return cursr != c;
    }


    char cpy_tag_str(char* dst, bool expect_spc, const char* end_delim) {

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

        while (!contains(cursr ,end_delim)) {
            if (isspace(cursr)) {
                if (!expect_spc)
                    break; // if space is not expected then break
                skip_whitespc();
                if (contains(cursr, end_delim))
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
        char end_char = cursr;
        get_nxt();
        strcpy(dst, temp_name);
        dst[t_name_indx + 1] = '\n';
        return end_char;
    }



    void initiate_tag(bool expect_spc) {

        const char* c = (expect_spc) ? " " : ">";
        cpy_tag_str(currnt_tag->buffer, expect_spc, c);
        currnt_tag->commit_hash();
        currnt_tag->commit_name();
        currnt_tag->cntnt_start = char_no-1;

    }

    void parse_scope_tree() {

        while (true) {
            char end = cpy_tag_str(currnt_tag->buffer, true, ":>");
            currnt_tag->append_hash_lnk();
            if (end == '>') break;
        }


        HashLinkT currnt_hash_link = currnt_tag->hash_lst;
        Tag* tg_found = tgtree.find(currnt_hash_link);
        if (tgtree.find_in_branch(tg_found->inside, (currnt_hash_link.end))) {
            RAISE_RECURSIVE_PASTING;
        }
        currnt_tag->addInside(tg_found);
        outscope();
        content_loop();
        
    }

    void parse_end_tag(int tag_start) {

        char buffer[n_sz];
        strcpy(buffer,currnt_tag->buffer);
        cpy_tag_str(currnt_tag->buffer, false, ">");
        currnt_tag->check_end_same(char_no,ln_no);

        strcpy(currnt_tag->buffer,buffer);
        
        currnt_tag->cntnt_end = tag_start;
        currnt_tag->end_ln_no = ln_no;
        currnt_tag->cls_tg_end = char_no;
        
        outscope();
    }

    void name_override() {

        cpy_tag_str(currnt_tag->buffer, false, ">");
        currnt_tag->commit_hash();


    }
    
    bool parse_tag() {
        int tag_start = char_no - 1;
        int start_ln = ln_no;
        get_nxt();
        skip_whitespc();
        bool is_cls = cursr == '/';

        if (is_cls) {
            get_nxt();
            parse_end_tag(tag_start);
            return is_cls;
        }

        inscope(); // any tag created always gets us inscope and vice versa

        currnt_tag->opn_tg_strt = tag_start;
        bool in_cntnt = depth > 2;
        bool expect_spc = (is_cls ^ in_cntnt) && in_cntnt;

        initiate_tag(expect_spc);
        char* first_str = currnt_tag->tag_name;
        int indx = currnt_tag->init_Tag(depth);
        currnt_tag->type = (TagType)(indx);
        if (expect_spc) {
           
            if (indx < (int)(TagType::Copy)) {
                RAISE_INVALID_TAG_NAME(currnt_tag->tag_name);
            }
            TagType type = (TagType)indx; 
            if (type == TagType::Paste) parse_scope_tree();
            if (type == TagType::Copy) name_override();
        }
        return is_cls;
    }

    bool check_tag_syntax() {
        int  temp_char_no = char_no;
        bool temp_in_comnt = at_comnt;
        int  temp_ln_no = ln_no;

        char tg_special_chars[] = "/:";
        int  count[3] = {0};
        int pos[3] = {0};

        bool is_tag = true;
        while (get_nxt() != '>') {

            if (!isalnum(cursr) && cursr != ' ') {
                
                int i = 0;
                bool found = false;
                for (i = 0; i < 2 ; i++) {
                    if (tg_special_chars[i] == cursr) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    is_tag = false;
                    break;
                }
                if (count[i] == 0) pos[i]++;
                else {
                    if (i == 0) {
                        is_tag = false;
                        break;
                    }
                }
                count[i]++;
            }
        }

        if (pos[0] > pos[1] && count[1]>0) is_tag = false;
        if (is_tag) {
            char_no = temp_char_no - 1;
            get_nxt();
            cursr;
            at_comnt = temp_in_comnt;
            ln_no = temp_ln_no;
        }

        return is_tag;
    }
    
    void loop() {
        while ((cursr != '<' && !at_comnt) && cursr != '\0')
            get_nxt();
    }

    void content_loop() {
        loop();
        if (cursr == '\0') return;
        if (check_tag_syntax()) parse_tag();
        content_loop();
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
        file_sz = ftell(file)-1;
        fseek(file, 0, SEEK_SET);
        if ( file_sz >= b_sz) RAISE_INSUFFICIENT_SPACE;

        fread(buffer, sizeof(char), file_sz, file);
        
        fclose(file);

        strcpy(currnt_tag->tag_name,file_name);
        currnt_tag->append_hash_lnk();

        get_nxt();
        content_loop();
        
    }


     void compile_shader(const char* enitity,GLenum type) {
        int index = GLshader_to_index(type);

        HashNode enitiyNode,shaderNode;

        enitiyNode.val = hash(enitity);
        shaderNode.val = shaderNode.val = all_names.hashes[index];
        enitiyNode.next = &shaderNode;
        shaderNode.next = nullptr;

        Tag* found = tgtree.find_in_branch(tgtree.root, & enitiyNode);

        int cntn_len = tgtree.cntn_len_of_tag(found);
        char* shader = new char[cntn_len];

        TagWriter writer (buffer,shader);
        writer.tag_tree_write(found);

        gl_compile_shader(type,shader);
        delete[] buffer;

    }
};


int main() {

    ShaderReader<2000> reader ("shaders.h");

    reader.tgtree.root;

    reader.compile_shader("surface", GL_VERTEX_SHADER);

   // reader.tgtree.root;
    cout << "parsing complete\n";

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

5, 7, 12 , 16 17 ,18  23  

Aalo 50 rs

*/

