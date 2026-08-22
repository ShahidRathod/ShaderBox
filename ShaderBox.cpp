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


enum class CharOfTagsInt :int {
    slash = 0, colon = 1, underscore = 2, dollar = 3, endofchars = 4
};

enum class CharOfTagsChar : char {
    slash = '/', colon = ':', underscore = '_', dollar = '$'
};

struct CharPack {
    char a, b;
};

int tagsindex(char c) {
    int i = -1;
    switch (c)
    {
    case '/':
        i = 0;
        break;
    case ':':
        i = 1;
        break;
    case '_':
        i = 3;
        break;
    case '$':
        i = 4;
        break;
    }

    return i;
}

enum class Semantic {
    comesbefore, comesafter, notcoexist, norelation
};

constexpr int alowwed_no = 6;

enum class Dispatch : int { open, close, paste, not_tag };


// Syntax parse is currently only 
// respomnsible for ordered relation for validating if its a tag

template <int sz>
struct SyntaxParser
{
    Semantic arr[sz * sz] = { Semantic::norelation };
    const char* chars;
    int pos[sz];
    int count_allowed[alowwed_no] = { 1,-1,-1,1 };
    int  count[alowwed_no] = { 0 };

    int index(char a, char b) {
        return tagsindex(a) * sz, tagsindex(b);
    }

    void set(char a, char b, Semantic val) {
        arr[index(a, b)] = val;
    }

    bool eval_char(char c) {
        int i = tagsindex(c);
        if (i == -1) {
            return false;
        }
        if (!count[i]) pos[i]++;
        count[i]++;
        // no egar evaluation for the count_allowed;
    }

    bool eval_tag() {
        for (int i = 0; i < alowwed_no;i++) {
            if (count[i] > count_allowed[i]) {
                return false;
            }
            for (int j = 0; j < alowwed_no; j++) {
                Semantic sem = arr[i * n_sz + j];
                bool comes_before = pos[i] > pos[j];
                if (sem == Semantic::norelation) continue;
                else if (!comes_before && sem == Semantic::comesbefore) {
                    return false;
                }
                else if (comes_before && sem == Semantic::comesafter) {
                    return false;
                }
                else {
                    if (!count[i] && !count[j]) return false;
                }

            }
        }
    }
    
    int parse_offset() {
        return count[tagsindex(':')] + count[tagsindex('$')];
    }
    
    Dispatch dispatch_type() {
        if (count[tagsindex('/')])      return Dispatch::close;
        else if (count[tagsindex('$')]) return Dispatch::paste;
        else                            return Dispatch::open;

    }

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

    SyntaxParser<alowwed_no> tag_syntax;
    CircularBuff<delim_len> delim_tkn;
    int   active_shaders[stage_count] = { 0 };
    Tag* currnt_tag = tgtree.top();

    ShaderReader() {
    }

    void inscope(TagPos new_tag_pos) {
        
        Tag* old_scope = tgtree.top();
        Tag* new_tag = tgtree.make_tag();
        new_tag->open = new_tag_pos;
        old_scope->addInside(new_tag);
        tgtree.addStack(new_tag);
        currnt_tag = tgtree.top(); // should be same as new_tg
        bool v = currnt_tag == new_tag;
        depth++;
    }

    void outscope(TagPos old_tag_pos) {
        currnt_tag->close = old_tag_pos;
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

        while (!contains(cursr, end_delim)) {
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


   
    void parse_tag_cls() {

        currnt_tag->cntnt_end = char_no - 1;
        currnt_tag->end_ln_no = ln_no;

        get_nxt(); // skippping '/'
        char buffer[n_sz];
        strcpy(buffer, currnt_tag->buffer);
        cpy_tag_str(currnt_tag->buffer, false, ">");
        currnt_tag->check_end_same(char_no, ln_no);
        strcpy(currnt_tag->buffer, buffer);

        currnt_tag->end_ln_no = ln_no;
        currnt_tag->cls_tg_end = char_no - 1;


    }

    void parse_tag_open() {
        currnt_tag->opn_tg_start = char_no - 1;
        int start_ln = ln_no;
        inscope(); // any tag created always gets us inscope and vice versa

        cpy_tag_str(currnt_tag->buffer, false, ">");
        currnt_tag->commit_hash();
        currnt_tag->commit_name();

        currnt_tag->cntnt_start = char_no - 1;
        char* first_str = currnt_tag->tag_name;

    }

    void parse_paste() {

        get_nxt(); // skipping '$'

        while (true) {
            char end = cpy_tag_str(currnt_tag->buffer, true, ":>");
            currnt_tag->append_hash_lnk();
            if (end == '>') break;
        }

        currnt_tag->cls_tg_end =
            currnt_tag->cntnt_start =
            currnt_tag->cntnt_end = char_no;

        HashLinkT hash_link = currnt_tag->hash_lst;
        Tag* tg_found = tgtree.find(hash_link);


        currnt_tag->addInside(tg_found);

        bool paste_in_itself =
            tgtree.find_in_branch(tg_found->inside, (hash_link.end), true);

        if (paste_in_itself) {

            RAISE_RECURSIVE_PASTING;
        }
        outscope();

    }


    void init_tag_syntax() {

        SyntaxParser<alowwed_no>pos_mat;
        pos_mat.chars = "/:_$ ";
        pos_mat.set('/', ':', Semantic::comesbefore);
        pos_mat.set('$', ':', Semantic::comesbefore);
        pos_mat.set('$', '_', Semantic::comesbefore);
        pos_mat.set('/', '$', Semantic::notcoexist);
        tag_syntax = pos_mat;

    }


    Dispatch check_tag_syntax(TagPos& tag_pos) {

        int  temp_char_no = char_no;
        bool temp_in_comnt = at_comnt;
        int  temp_ln_no = ln_no;

        bool is_tag = true;

        while (get_nxt() != '>') {
            is_tag = tag_syntax.eval_char(cursr);
            if (!is_tag) break;
        }

        is_tag = tag_syntax.eval_tag();
        Dispatch dis_type = tag_syntax.dispatch_type();
        
        if (is_tag) {
            tag_pos.start = temp_char_no - 1;
            tag_pos.end = char_no;
            tag_pos.ln_no = temp_ln_no;

            char_no = temp_char_no - 1 + tag_syntax.parse_offset();
            get_nxt();
            cursr;
            at_comnt = temp_in_comnt;
            ln_no = temp_ln_no;
        }


        return dis_type;

    }

    void loop() {
        while ((cursr != '<' && !at_comnt) && cursr != '\0')
            get_nxt();
    }

    void content_loop() {
        loop();
        if (cursr == '\0') return;

        TagPos tag_pos;
        Dispatch type = check_tag_syntax(tag_pos);

        get_nxt();
        skip_whitespc();

        switch (type)
        {
        case (Dispatch::open):
            
            inscope(tag_pos);
            parse_tag_open();
            break;

        case Dispatch::close:

            parse_tag_cls();
            outscope(tag_pos);
            break;

        case Dispatch::paste:
            inscope(tag_pos);
            parse_paste();

        }

        content_loop();
    }


    FILE* open_file(const char* file_name) {
        FILE* file = fopen(file_name, "r");
        if (!file) {
            RAISE_FILE_NOT_FOUND;
        }
        return file;
    }


    ShaderReader(const char* file_name) {

        file = open_file(file_name);
        fseek(file, 0, SEEK_END);
        file_sz = ftell(file) - 1;
        fseek(file, 0, SEEK_SET);
        if (file_sz >= b_sz) RAISE_INSUFFICIENT_SPACE;

        fread(buffer, sizeof(char), file_sz, file);
        fclose(file);
        strcpy(currnt_tag->tag_name, file_name);
        currnt_tag->append_hash_lnk();

        get_nxt();
        content_loop();

    }


    void compile_shader(const char* enitity, GLenum type) {
        int index = GLshader_to_index(type);

        HashNode enitiyNode, shaderNode;

        enitiyNode.val = hash(enitity);
        shaderNode.val = shaderNode.val = all_names.hashes[index];
        enitiyNode.next = &shaderNode;
        shaderNode.next = nullptr;

        Tag* found = tgtree.find_in_branch(tgtree.root, &enitiyNode);
        TagWriter writer(buffer, found);
        gl_compile_shader(type, writer.tag_content());

        delete[] buffer;

    }
};


int main() {

    ShaderReader<4000> reader("shaders.h");
    reader.tgtree.root;

    reader.compile_shader("surface", GL_VERTEX_SHADER);

    reader.tgtree.root;
    cout << "parsing complete\n";

}



// make a char interator object that will be shared with tag tree parser tag_parser 
// 