/* ********************************************************* *
 * By Yuriy Y. Yermilov aka (binaryONE) cyclone.yyy@gmail.com
 *
* website: code.computronium.io
*
* THIS SOFTWARE IS PROVIDED BY THE OWNER
 *``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE OWNER
 * PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ********************************************************** */
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define I64_MAX_SIZE (((int64_t)0x7fffffff << 32) | 0xffffffff)
#define I64_MIN_SIZE (-I64_MAX_SIZE - 1)

/*
 * Datatypes & Defines (macros).
 */

struct y3_str_line
{
  char* txt;
  char* fname;
  size_t size;
  size_t id;
  size_t id_real; // this is set to id however,
  // if a text is appended from another file
  // real_id will be set to the line number of
  // the appended text, starting with 0
  int flags;
  void (*freeline)(void*);
};

#define y3_str_line_dtp struct y3_str_line*
#define y3_str_dtp_size sizeof(struct y3_str_line)

/*
 * Prototypes.
 */

int64_t
y3_str_stoi64(const char* nptr, char** endptr, int base);
int
y3_str_i64toa_s(int64_t value, char* str, size_t size, int radix);
int
y3_str_ui64toa_s(uint64_t value, char* str, size_t size, int radix);
int
y3_str_ltoa_s(long value, char* str, size_t size, int radix);
int64_t
y3_str_atoi64(const char* str);

unsigned char
y3_str_match(char* str, char* strm, size_t size);
int
y3_str_hasalpha(char* s, int size);
int
y3_str_hasdigit(char* s, int size);
int
y3_str_digital(char* str, int size);
int
y3_str_alphabetic(char* str, int size);
int
y3_str_ndigit(char* str, int size);
int
y3_str_ndigit_f(char* str, int size);
int
y3_str_nalpha(char* str, int size);
int
y3_str_nchar(char* s, char c);
int
y3_str_nchars(char* s, char* _cs);
char
y3_str_ischars(char* s, char* _cs);
char
y3_str_ischarsto(char* s, char* _cs, int stop);
char
y3_str_ischarstoC(char* s, char* _cs, char stopc);
char
y3_str_ischarstoCS(char* s, char* _cs, char* stopcs);
unsigned char
y3_str_ischar(char* s, char c);
int
y3_str_xchars(char* s, char* _cs);
int
y3_str_x2chars(char* s, char* _cs, char* sc);
unsigned long long
y3_str_getbytes(char* str,
                char* buf,
                char* skipb,
                char* stopb,
                unsigned long long nbytes);
int
y3_str_gotoc(char* str, char c, int flag, int stopc);
int
y3_str_gotocs(char* str, char* _cs, int flag, int stopc);
int
y3_str_gotocb(char* s, char* bc, char c, int pos, int stopc);
char**
y3_str_maketags(char* open_str,
                char* close_str,
                int max_taglen,
                char* sep,
                int* tag_sets,
                int n_aux_sets);
int
y3_str_addtags(char** stags,
               char* open,
               char* close,
               int max,
               char* sep,
               int* nsets);
int
y3_str_getwords(char* str,
                char** wbuf,
                char* seps,
                int max_words,
                int max_wlen,
                char** st,
                int nst,
                char* stopcs);
void
y3_str_showtags(int _switch, FILE* logfile, char** stags, int tag_sets);
int
y3_str_nnwords(char* str, char* seporators);
int
y3_str_extstr(char* from, char* _cs, char* to, unsigned int size);
int
y3_str_extstr_m(char* from,
                char* _cs,
                char* to,
                unsigned int size,
                int nth_quote,
                int inc_quotes);
void
y3_str_switchc(char* str, char c, char n);
void
y3_str_ui64tos(uint64_t n, char s[], size_t, size_t* size);
void
y3_str_i64tos(int64_t n, char s[], size_t, size_t* size);
char*
y3_str_itos2(int n);
unsigned long long
y3_str_stoi(char* s);
char*
y3_str_revs(char* s);
void
y3_str_collapsec(char* s, char c);
int
y3_str_isconsecutive(char* s, char* _cs);
int
y3_str_isconsecutive_n(char* s, char* cs, int n);
int
y3_str_hasstr(char* str, char* sub_str, int sn);
int
y3_str_nstr(char* s1, char* s2);
int
y3_str_ltoa_helper(long value, char* str, size_t size, int radix);
int
y3_str_nchar_i(char* s, char c);
int
y3_str_chgstr(char* str,
              unsigned int s_sz,
              char* ps,
              unsigned int p_sz,
              unsigned int p,
              unsigned int chg_sz);
struct y3_list*
y3_str_gettxt(void (*error)(const char* fmt, ...), const char* fname);
struct y3_list*
y3_str_gettxt_append(struct y3_list* tail,
                     int start_id,
                     void (*error)(const char* fmt, ...),
                     const char* fname);
void
y3_error(const char* fmt, ...);
void
y3_echo(const char* fmt, ...);
void
y3_debug(const char* fmt, ...);
void
y3_echo_log(int _switch, FILE* logfile, const char* fmt, ...);
void
y3_debug_log(int _switch, FILE* logfile, const char* fmt, ...);
char*
y3_str_new(char* s, size_t s_size);
struct y3_list*
y3_str_saveline(int reset, const char* fname, int n_args, ...);
char*
y3_str_getline(struct y3_list* txt, int id);
struct y3_list*
y3_str_getline_link(struct y3_list* txt, int id);
struct y3_str_line*
y3_str_getline_p(struct y3_list* txt);
char*
y3_str_getfname(struct y3_list* txt, int id);
char*
y3_str_getcfname(struct y3_list* txt);
struct y3_list*
y3_str_fgetline(struct y3_list* txt, int flags);
char*
y3_str_getcline(struct y3_list* txt);
struct y3_list*
y3_str_copyline(struct y3_list* to, struct y3_list* from);
void
y3_str_setflags(struct y3_list* txt, int flags);
int
y3_str_getflags(struct y3_list* txt);
struct y3_str_line*
y3_str_newline(const char* fname, char* txt, int id, int flags);
void
y3_str_freeline(void* li);
void
y3_str_show_text(struct y3_list* txt);
int
y3_str_text_nbytes(struct y3_list* txt);
void
y3_str_freetags(char** stags, int tag_sets);
void
y3_str_freewords(char** words, int nwords);
int
y3_str_find_patsy_char(char* s, char* c);
int
y3_str_striptag(char* word, char* open_tag, char* close_tag);
int
y3_str_striptagn(char** words, int nwords, char* open_tag, char* close_tag);
int
y3_str_hastags(char* word, char* open_tag, char* close_tag);
int
y3_str_hastags_wlen(char* word, char* open_tag, char* close_tag, int wlen);
int
y3_str_incharset(char* s, char* charset);
int
y3_str_replace_dynamic(char* src,
                       char* str_old,
                       char* str_new,
                       char* ts,
                       int nrep);
char*
y3_str_replace(const char* str, const char* from, const char* to);
int
y3_str_digital2_f(char* str, int size, char* ignore);
int
y3_str_digital2(char* str, int size, char* ignore);
int
y3_str_digital_f(char* str, int size);
int
y3_str_alphabetic2(char* str, int size, char* ignore);
int
y3_str_tail_size(char* _s, char* _ts);
int
y3_str_get_maxwordlen(char* s, char* seps, char* stopcs);
int
y3_str_xfix(char* str, char* xfix, char* stopcs, int metac);
int
y3_str_asciival(char* str);

#define _FTOA_TOO_LARGE 0x00000001
#define _FTOA_TOO_SMALL 0x00000002

char*
y3_str_ftoa(float f, int* status);
