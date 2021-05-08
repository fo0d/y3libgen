/* ********************************************************* *
 * By Yuriy Y. Yermilov aka (binaryONE) cyclone.yyy@gmail.com
 *
 * website: code.computronium.io
 *
 * THIS SOFTWARE IS PROVIDED BY THE OWNER ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR
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
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hash.h"
#include "list.h"
#include "str.h"

/* **************************************************** *
String and character function library.

NOTE(s):
All functions that alter the original string
in anyway, REQUIRE that they have the string
passed to them be defined as an array as in char s[] = "blah";
OR the string needs to be malloc'd.
Because the above two methonds GUERANTEE that
the bytes are contiguous.

Most functions here require NULL '\000'
terminated strings to be passed to them.

The 'txt' interface is implemented with the
linked list library. However, it is up to the
user to make sure that the list and the lines
inside the data of the node are correctly numbered.
This is why, when a specific link needs to be found
and txt is formed in concatenated fashion or
the txt was preprocessed and its integerity is
in question, always use the list 'count' variable
which is maintained in a uniform fashion throughout
the amortized life of the linked list.
 * **************************************************** */

/*
 * returns the total number of bytes
 * in the text container from line pointed
 * to by txt parameters.
 */

int
y3_str_text_nbytes(struct y3_list* txt)
{
  int n_bytes;
  struct y3_str_line* li;

  for (n_bytes = 0; txt; txt = txt->next) {
    li = (struct y3_str_line*)txt->data;
    n_bytes += li->size;
  }
  return n_bytes;
}

/*
 * prints ouf the text starting from the line
 * pointed to by txt param.
 */

void
y3_str_show_text(struct y3_list* txt)
{
  while (txt) {
    y3_echo("line[%d]:%s", txt->id, y3_str_getline(txt, txt->id));
    txt = txt->next;
  }
}

/*
 * Sets flags for specific line.
 */

void
y3_str_setflags(struct y3_list* txt, int flags)
{
  struct y3_str_line* li;

  if (txt) {
    li = (struct y3_str_line*)txt->data;
    if (li)
      li->flags |= flags;
  }
}

/*
 * Gets flags for specific line.
 */

int
y3_str_getflags(struct y3_list* txt)
{
  struct y3_str_line* li;

  if (txt) {
    li = (struct y3_str_line*)txt->data;
    if (li)
      return li->flags;
  }
  return 0;
}

/*
 * Get address of the whole line structure pointer.
 */

struct y3_str_line*
y3_str_getline_p(struct y3_list* txt)
{
  if (txt)
    return (struct y3_str_line*)txt->data;
  return NULL;
}

/*
 * Creates a new line with given input and returns a pointer to it.
 */

struct y3_str_line*
y3_str_newline(const char* fname, char* txt, int id, int flags)
{
  struct y3_str_line* newline;
  if (!txt)
    return NULL;
  newline = malloc(sizeof(struct y3_str_line));
  if (newline) {
    newline->size = strlen(txt);
    newline->txt = y3_str_new(txt, newline->size);
    if (fname)
      newline->fname = y3_str_new((char*)fname, strlen(fname));
    newline->freeline = y3_str_freeline;
    newline->flags = flags;
    newline->id = id;
    newline->id_real = id;
  }
  return newline;
}

/*
 * Copies a line from one container to another.
 */

struct y3_list*
y3_str_copyline(struct y3_list* to, struct y3_list* from)
{
  struct y3_str_line *lt, *lf;

  if (from && from->data) {
    lf = (struct y3_str_line*)from->data;
    if (!lf)
      return 0;
    if (!to) {
      to =
        y3_list_create(y3_str_newline(lf->fname, lf->txt, lf->id, lf->flags),
                       (void*)lf->id,
                       (void*)-1,
                       from->links->M,
                       0);
      return to;
    } else {
      y3_str_freeline((struct y3_str_line*)to->data);
      lt = (struct y3_str_line*)to->data;
      if (lf)
        if (lt->size < lf->size)
          lt->size = lf->size;
      memmove(lt->txt, lf->txt, lt->size);
    }
  }
  return to;
}

/*
 * Frees all memory occupied by given test data.
 *
 * The data needs to be of type y3_str_line.
 */

void
y3_str_freeline(void* data)
{
  struct y3_str_line* li = (struct y3_str_line*)data;

  if (li) {
    if (li->txt) {
      free(li->txt);
      li->txt = NULL;
    }
    if (li->fname) {
      free(li->fname);
      li->fname = NULL;
    }
    free(li);
  }
}

/*
 * n_args = number of arguments should be "3"
 *
 * format for '...':
 * char *txt, size_t size, size_t id
 *
 * returns NULL if invalid number of args.
 * returns head of list if all good.
 *
 */

struct y3_list*
y3_str_saveline(int reset, const char* fname, int n_args, ...)
{
  static struct y3_list *list, *newlink;
  va_list params;
  char* txt;      // param 1
  size_t txt_len; // param 2
  size_t id;      // param 3
  int init = (list == NULL);

  if (n_args != 3)
    return NULL;
  else if (reset) {
    list = NULL;
    return NULL;
  }

  va_start(params, n_args);
  txt = va_arg(params, char*);
  txt_len = va_arg(params, size_t);
  id = va_arg(params, size_t);

  if (id == 0) {
    newlink = list = y3_list_create(y3_str_newline(fname, txt, 0, 0),
                                    0,
                                    (void*)-1,
                                    Y3_LIST_INITIAL_NODE_COUNT,
                                    init);
    if (!list)
      y3_error("Cannot create new line with text [%s]\n", txt);
  } else {
    newlink = y3_list_insert_last(
      newlink, y3_str_newline(fname, txt, id, 0), id, 0, init, (void*)id);
    if (!newlink)
      y3_error("Cannot create new line with text [%s]\n", txt);
  }
  va_end(params);
  return list;
}

char*
y3_str_getline(struct y3_list* txt, int id)
{
  struct y3_str_line* li;
  struct y3_list* node;
  y3_hashItem* item;

  // if(txt) {
  //    for( ; txt; txt = txt->next) {
  //        li = (struct y3_str_line *)txt->data;
  //        if(li->id == (unsigned)id)
  //            return li->txt;
  //    }
  //}

  if (txt) {
    item = y3_hash_search(txt->links, id);
    if (item != txt->links->NULL_item) {
      node = (struct y3_list*)item->value;
      li = (struct y3_str_line*)node->data;
      if (li)
        return li->txt;
    }
  }

  return NULL;
}

struct y3_list*
y3_str_getline_link(struct y3_list* txt, int id)
{
  struct y3_list* node;
  y3_hashItem* item;

  if (txt) {
    item = y3_hash_search(txt->links, id);
    if (item != txt->links->NULL_item) {
      node = (struct y3_list*)item->value;
      return node;
    }
  }

  return NULL;
}

char*
y3_str_getfname(struct y3_list* txt, int id)
{
  struct y3_str_line* li;

  if (txt) {
    for (; txt; txt = txt->next) {
      li = (struct y3_str_line*)txt->data;
      if (li->id == (unsigned)id)
        return li->fname;
    }
  }
  return NULL;
}

char*
y3_str_getcline(struct y3_list* txt)
{
  struct y3_str_line* li;

  if (txt) {
    li = (struct y3_str_line*)txt->data;
    if (li)
      return li->txt;
  }
  return NULL;
}

char*
y3_str_getcfname(struct y3_list* txt)
{
  struct y3_str_line* li;

  if (txt) {
    li = (struct y3_str_line*)txt->data;
    if (li)
      return li->fname;
  }
  return NULL;
}

struct y3_list*
y3_str_fgetline(struct y3_list* txt, int flags)
{
  for (; txt; txt = txt->next)
    if (y3_str_getflags(txt) & flags)
      return txt;

  return NULL;
}

/*
 * fname    = file name to read text from.
 * tail     = last node in list, at which to start appending text read from
 * 'fname' error    = pointer to function that handles an error start_id = id
 * # from which to start counting upwards when updating the list nodes and
 * line ids. (this referres to the list which needs to be appended)
 *
 * NOTEs: if tail is NULL a new list is returned
 *        this is to insure that the list actually exists.
 *
 *        The null_key has to be uniform! meaning the same in the list being
 * appened to and from.
 *
 * RETURNS: pointer to tail of list.
 *
 */

struct y3_list*
y3_str_gettxt_append(struct y3_list* tail,
                     int start_id,
                     void (*error)(const char* fmt, ...),
                     const char* fname)
{
  struct y3_str_line *li, *new_li;
  int i, j;
  int n_lines;
  struct y3_list *txt, *newtxt;

  if (!fname)
    return tail;
  else if (!tail)
    return NULL;

  newtxt = txt = y3_str_gettxt(error, fname);

  if (!txt)
    return tail;

  n_lines = y3_list_count(txt);

  //
  // We re-insert the new text, to keep the hashtable current.
  //

  for (i = 0, j = start_id; i < n_lines; i++, j++) {
    txt = y3_list_link_id(txt, i);
    li = txt->data;
    new_li = y3_str_newline(li->fname, li->txt, li->id, li->flags);
    y3_list_insert_after(tail,
                         new_li,
                         tail->id + 1,
                         txt->flags,
                         Y3_LIST_CONTINUE,
                         (tail->id + 1));
    tail = tail->next;
    li->id_real = li->id;
  }

  //
  // Release the appended text.
  //

  y3_list_freeall(newtxt, y3_str_freeline);

  return tail;
}

/*
 * fname = file name to read text from.
 * r_size = number of bytes to read at a time from file.
 * proc = pointer to function that processes a line,
 *    this function get input as follows
 *    char *line, size_t length, size_t id
 * error = pointer to function that handles an error
 * eol = end of line string of characters. (not implemented)
 *
 * RETURNS: pointer to head of a list with all lines
 *      in it.
 */

struct y3_list*
y3_str_gettxt(void (*error)(const char* fmt, ...), const char* fname)
{
  char *lbuf = NULL, *rbuf = NULL;
  int fd, i, ch_num;
  size_t c, ln, n_bytes, r_size = 0x10000;
  struct y3_list* list = NULL;
  struct stat st;

  if (*fname == '\000') {
    error("y3_str_gettxt(): fname is null.");
    return NULL;
  } else if ((fd = open(fname, O_RDONLY)) < 0) {
    error("y3_str_gettxt(): Cannot open file [%s]\n", fname);
    return NULL;
  } else if (!stat(fname, &st)) {
#ifndef _DOSLIKE
    r_size = st.st_blksize;
#endif
  }

  n_bytes = c = ln = i = 0;
  lbuf = (char*)malloc(r_size);
  memset(lbuf, 0, r_size);
  rbuf = (char*)malloc(r_size);

  while ((ch_num = read(fd, rbuf, r_size)) > 0) {
    n_bytes += ch_num;

    while ((i < ch_num) && rbuf[i] != '\000') {
      c++;
      if (c > r_size) {
        r_size <<= 2;
        lbuf = (char*)realloc(lbuf, r_size);
        memset(lbuf, 0, r_size);
      }

      lbuf[c - 1] = rbuf[i];
      lbuf[c] = '\000';

      if (lbuf[c - 1] == '\n') { // this handles \r\n also
        list = y3_str_saveline(0, fname, 3, lbuf, c, ln);
        ln++;
        c = 0;
      }
      i++;
    }
    i = 0;
  }

  //
  // For some files, there may not be a new line after the
  // last line. However that line is still in the line buffer
  // and we have to save it.
  //

  if (!y3_str_nchar(lbuf, '\n') &&
      lbuf[0]) // this should be if(lbuf[c] != '\n')
    list = y3_str_saveline(0, fname, 3, lbuf, c, ln);

  free(lbuf);
  free(rbuf);
  y3_str_saveline(1, fname, 3, NULL, 0, -1);
  return list;
}

void
y3_echo(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
}

void
y3_error(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
  exit(911);
}

void
y3_debug(const char* fmt, ...)
{
#ifdef _Y3_LIB_DEBUG_ON
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);
#endif
}

void
y3_debug_log(int _switch, FILE* logfile, const char* fmt, ...)
{
  if (_switch) {
    va_list ap;
    va_start(ap, fmt);
    if (logfile) {
      vfprintf(logfile, fmt, ap);
      fflush(logfile);
    } else {
      vprintf(fmt, ap);
    }
    va_end(ap);
  }
}

void
y3_echo_log(int _switch, FILE* logfile, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if (logfile) {
    vfprintf(logfile, fmt, ap);
    fflush(logfile);
    if (_switch) {
      va_start(ap, fmt);
      vprintf(fmt, ap);
      va_end(ap);
    }
  } else {
    vprintf(fmt, ap);
  }
  va_end(ap);
}

/*
 * Allocate 's_size' bytes for 'new_buf' and copy 's'
 * into it.
 *
 * return 'new_buf'.
 */

char*
y3_str_new(char* s, size_t s_size)
{
  char* new_buf = NULL;

  if (s_size == 0 || !s)
    return s;

  new_buf = (char*)malloc(s_size + 1);
  memset(new_buf, 0, s_size + 1);
  memmove(new_buf, s, s_size);
  return new_buf;
}

/* ***************************************************** *
Count how many matching strings are in a
given string.

return 0 if error, or no match,
otherwise return match count.
 * ***************************************************** */
int
y3_str_nstr(char* s1, char* s2)
{
  char* s = s1;
  char* p = s2;
  unsigned mlen;
  int i, n;

  if (!s || !p)
    return 0;

  mlen = strlen(p);

  if (strlen(s) < mlen)
    return 0;

  i = n = 0;

_loop:
  i += y3_str_gotoc(&s[i], p[0], 0, '\000');
  if (y3_str_match(&s[i], p, mlen)) {
    n++;
    i++;
    goto _loop;
  }
  return n;
}

/*
 * This function checks whether 's' consists entirely
 * of the characters specified in the charset passed
 * by the user.
 *
 * RETURNS: -1 on error
 *           0 s contains characters not in charset
 *           1 s doesn't contain characters not in charset
 */

int
y3_str_incharset(char* s, char* charset)
{
  int i, s_len;

  if (!charset || charset[0] == '\000' || !s || s[0] == '\000')
    return -1;

  s_len = strlen(s);
  for (i = 0; i < s_len; i++)
    if (!y3_str_nchar(charset, s[i]))
      return 0;
  return 1; // success
}

/*
 * Same as y3_str_digital() except it ignores the specified characters.
 *
 * RETURNS: -1 if str == null or size <= 0
 *           0 if not digital
 *           1 if digital
 */

int
y3_str_digital2(char* str, int size, char* ignore)
{
  char* s;

  if (!str || size <= 0)
    return -1;

  s = (char*)malloc(size + 1);
  memmove(s, str, size);
  s[size] = '\000';

  if (ignore)
    size -= y3_str_xchars(s, ignore);

  if (y3_str_ndigit(s, size) == size) {
    free(s);
    return 1;
  }
  free(s);
  return 0;
}

int
y3_str_digital2_f(char* str, int size, char* ignore)
{
  char* s;

  if (!str || size <= 0)
    return -1;

  s = (char*)malloc(size + 1);
  memmove(s, str, size);
  s[size] = '\000';

  if (ignore)
    size -= y3_str_xchars(s, ignore);

  if (y3_str_ndigit_f(s, size) == size) {
    free(s);
    return 1;
  }
  free(s);
  return 0;
}

/*
 * Sam as y3_str_alphabetic() except it ignores the specified characters.
 *
 * RETURNS: -1 if str == null or size <= 0
 *           0 if not alphabetic
 *           1 if alphabetic
 */

int
y3_str_alphabetic2(char* str, int size, char* ignore)
{
  char* s;

  if (!str || size <= 0)
    return -1;

  s = (char*)malloc(size + 1);
  memmove(s, str, size);
  s[size] = '\000';

  if (ignore)
    size -= y3_str_xchars(s, ignore);

  if (y3_str_nalpha(s, size) == size) {
    free(s);
    return 1;
  }
  free(s);
  return 0;
}

/*
 * Check how many characters are remaining in the specified string.
 *
 * s    -> character string
 * ts   -> terminator string
 *
 * RETURNS:
 *          -1 if ts and or s are null.
 *
 *          number of characters from beginning of string to anything
 *          matching any of the ts characters.
 */

int
y3_str_tail_size(char* _s, char* _ts)
{
  char *s, *ts;
  int i = 0;

  if (!_ts || !_s)
    return -1;

  s = _s;
  ts = _ts;

  while (!y3_str_ischar(ts, s[i++])) {
  }
  return i;
}

/* **************************************************** *
Checks weather s contains consecutively chars
in cs.

This version check at most 'n' amount of characters.

returns the number of characters that were
consecutively checked,

or -1 if s or cs == NULL or if len of cs > s
 * **************************************************** */

int
y3_str_isconsecutive_n(char* s, char* cs, int n)
{
  int i, j;

  if (!s || !cs || n < 1)
    //        return -1;
    return 0;

  for (i = j = 0; s[i] != '\000' && cs[i] != '\000'; i++, j++) {
    if (s[i] != cs[i] || j >= n)
      break;
  }

  if (j != n)
    return 0;
  return 1;
}

/* ***************************************************** *
Check if c is between characters bc[0] and b[c1].
bc[0] & bc[1] can be equal.
It is assumed that bc[0] is before bc[1],
if not, then their roles are switched auto -
matically.

Logically, c cannot equal bc[0] and b[1].

pos is relative to c between bc[0] and bc[1]
if pos == -1 then last c between bc[0] & bc[1]
is returned.
if pos == 0 then first c between bc[0] & bc[1]
is returned.
if pos == (N > 0) then N c between bc[0] & bc[1]
is returned.

pos can be less than -1, but it is interpreted
as -1.. or less than 0.

stopc is character that is the terminator.

bc[0] == bc[1] == stopc is not allowed.
however, bc[0] != bc[1] == stopc is allowed.

The position of c between bc[0] & bc[1] is
returned, RELATIVE to c's position in the string!

Else, -1 is returned.
 * ***************************************************** */

int
y3_str_gotocb(char* s, char* bc, char c, int pos, int stopc)
{
  int i, j;

  if (strlen(bc) != 2)
    return -1;
  if (y3_str_nchar(bc, c))
    return -1;
  else if (bc[0] == bc[1]) {
    if (bc[1] == stopc)
      return -1;
    j = y3_str_nchar(s, bc[0]);
    if (j < 2)
      return -1;
    else {
      i = y3_str_gotoc(s, bc[0], 0, stopc);
      j = y3_str_gotoc(s, bc[0], -1, stopc);
    }
  } else {
    if (bc[1] == stopc) {
      bc[0] ^= bc[1];
      bc[1] ^= bc[0];
      bc[0] ^= bc[1];
    }
    i = y3_str_gotoc(s, bc[0], 0, stopc);
    j = y3_str_gotoc(s, bc[1], -1, stopc);
    if (i > j) {
      i ^= j;
      j ^= i;
      i ^= j;
    }
  }
  if (pos < -1)
    pos = -1;
  pos = y3_str_gotoc(s + i + 1, c, pos, stopc) + i + 1;
  if (pos >= j)
    return -1;
  return pos;
}

/* **************************************************** *
Only printable characters are expected to be
in the string parameter, which also needs to
be NULL terminated.
 * **************************************************** */

void
y3_str_collapsec(char* s, char c)
{
  int i;

_loop:
  i = y3_str_gotoc(s, c, 0, '\000');

  if (i >= 0) {
    if (s[i] == c) {
      s[i] = '\003';
      i++;
      if (s[i] == c)
        for (; s[i] == c; i++)
          s[i] = '\004';
      goto _loop;
    }
  }

  y3_str_xchars(s, "\004");
  y3_str_switchc(s, '\003', c);
}

char*
y3_str_revs(char* str)
{
  char* s = str;
  char* end;
  char c;

  if (*s) {
    end = s + strlen(s) - 1;
    while (s < end) {
      c = *s;
      *s = *end;
      *end = c;
      s++, end--;
    }
  }
  return (s);
}

/*
 * This function includes the terminating
 * NULL ('\000') character in the final
 * size of s[] array.
 *
 * n = number to convert to string
 * s[] = array where to store the string
 * *size = pointer to variable that stores the size of the string
 */

void
y3_str_ui64tos(uint64_t n, char s[], size_t slen, size_t* size)
{
  memset(s, 0, slen);
  y3_str_ui64toa_s(n, s, slen, 10);

  if (size != NULL)
    *(size) = strlen(s);
}

void
y3_str_i64tos(int64_t n, char s[], size_t slen, size_t* size)
{
  memset(s, 0, slen);
  y3_str_i64toa_s(n, s, slen, 10);

  if (size != NULL)
    *(size) = strlen(s);
}

int
y3_str_ui64toa_s(uint64_t value, char* str, size_t size, int radix)
{
  char buffer[65], *pos;
  int digit;

  if (!str || radix < 2 || radix > 36) {
    return -1;
  }

  pos = buffer + 64;
  *pos = '\0';

  do {
    digit = (int)(value % radix);
    value /= radix;

    if (digit < 10)
      *--pos = '0' + digit;
    else
      *--pos = 'a' + digit - 10;
  } while (value != 0);

  if (buffer - pos + 65 > size) {
    return -1;
  }

  memcpy(str, pos, buffer - pos + 65);
  return 0;
}

//
// returns
// 0  on OK
// -1 on ERROR
// -2 on ERROR with str buffer being too small
//

int
y3_str_i64toa_s(int64_t value, char* str, size_t size, int radix)
{
  int64_t val;
  unsigned int digit;
  int is_negative;
  char buffer[65], *pos;
  size_t len;

  if (str == NULL)
    return -1;
  //    if (size < 0)
  //        return -1;
  if (!(radix >= 2 && radix <= 36)) {
    str[0] = '\0';
    return -1;
  }

  if (value < 0 && radix == 10) {
    is_negative = 1;
    val = -value;
  } else {
    is_negative = 0;
    val = value;
  }

  pos = buffer + 64;
  *pos = '\0';

  do {
    digit = val % radix;
    val /= radix;

    if (digit < 10)
      *--pos = '0' + digit;
    else
      *--pos = 'a' + digit - 10;
  } while (val != 0);

  if (is_negative)
    *--pos = '-';

  len = buffer + 65 - pos;
  if (len > size) {
    size_t i;
    char* p = str;

    /* Copy the temporary buffer backwards up to the available number of
     * characters. Don't copy the negative sign if present. */

    if (is_negative) {
      p++;
      size--;
    }

    for (pos = buffer + 63, i = 0; i < size; i++)
      *p++ = *pos--;

    str[0] = '\0';
    return -2;
  }

  memcpy(str, pos, len);
  return 0;
}

/*********************************************************************
 * _atoi64 (NTDLL.@)
 *
 * Convert a string to a large integer.
 *
 * PARAMS
 * str [I] String to be converted
 *
 * RETURNS
 * Success: The integer value represented by str.
 * Failure: 0. Note that this cannot be distinguished from a successful
 * return, if the string contains "0".
 *
 * NOTES
 * - Accepts: {whitespace} [+|-] {digits}
 * - No check is made for value overflow, only the lower 64 bits are
 * assigned.
 * - If str is NULL it crashes, as the native function does.
 */

int64_t
y3_str_atoi64(const char* str)
{
  int64_t RunningTotal = 0;
  int bMinus = 0;

  while (*str == ' ' || (*str >= '\011' && *str <= '\015')) {
    str++;
  } /* while */

  if (*str == '+') {
    str++;
  } else if (*str == '-') {
    bMinus = 1;
    str++;
  } /* if */

  while (*str >= '0' && *str <= '9') {
    RunningTotal = RunningTotal * 10 + *str - '0';
    str++;
  } /* while */

  return bMinus ? -RunningTotal : RunningTotal;
}

int64_t
y3_str_stoi64(const char* nptr, char** endptr, int base)
{
  int negative = 0;
  int64_t ret = 0;

  if (!(nptr != NULL))
    return 0;
  if (!(base == 0 || base >= 2))
    return 0;
  if (!(base <= 36))
    return 0;

  while (isspace(*nptr)) {
    nptr++;
  }

  if (*nptr == '-') {
    negative = 1;
    nptr++;
  } else if (*nptr == '+')
    nptr++;

  if ((base == 0 || base == 16) && *nptr == '0' &&
      tolower(*(nptr + 1)) == 'x') {
    base = 16;
    nptr += 2;
  }

  if (base == 0) {
    if (*nptr == '0')
      base = 8;
    else
      base = 10;
  }

  while (*nptr) {
    char cur = tolower(*nptr);
    int v;

    if (isdigit(cur)) {
      if (cur >= '0' + base)
        break;
      v = cur - '0';
    } else {
      if (cur < 'a' || cur >= 'a' + base - 10)
        break;
      v = cur - 'a' + 10;
    }

    if (negative)
      v = -v;

    nptr++;

    if (!negative &&
        (ret > I64_MAX_SIZE / base || ret * base > I64_MAX_SIZE - v)) {
      ret = I64_MAX_SIZE;
    } else if (negative && (ret < I64_MIN_SIZE / base ||
                            ret * base < I64_MIN_SIZE - v)) {
      ret = I64_MIN_SIZE;
    } else
      ret = ret * base + v;
  }

  if (endptr)
    *endptr = (char*)nptr;

  return ret;
}

/*
 * This functions includes the terminating
 * NULL ('\000') character in the final
 * size of s[] array.
 *
 * n = number to convert to string
 *
 * Returns NULL on error, otherwise returns string representation of 'n'.
 */

char*
y3_str_itos2(int n)
{
  int i = 0;
  int max_neg = 0, neg = 0;
  static char s[16];

  if (n < 0)
    neg = 1;
  if (n == -2147483647)
    max_neg = 1;
  if (neg) {
    if (max_neg)
      n += 10;
    n = -n;
  }

  do {
    if (max_neg && i == 1)
      ++n;
    s[i++] = n % 10 + '0';
    n /= 10;
  } while (n > 0);

  if (neg)
    s[i++] = '-';
  s[i++] = '\0';

  y3_str_revs(s);

  return s;
}

unsigned long long
y3_str_stoi(char* s)
{
  unsigned long long n, sign;

  while (*s == ' ' || *s == '\n' || *s == '\n') {
    s++;
  }

  sign = 1;
  if (*s == '+')
    s++;
  else if (*s == '-') {
    sign = -1;
    s++;
  }
  for (n = 0; *s >= '0' && *s <= '9'; s++)
    n = 10 * n + *s - '0';
  return (sign * n);
  //    _strtoui64(s, NULL, 16);
}

/* ****************************************************
Extract String:

Takes "from",
finds location of first quote,
finds location of last quote,
copies characters from and
including cs[0], to
and including cs[1]
into "to" buffer which has to be
made big enough before calling
the function.

The "to" buffer will be NULL terminated.

Returns:

>= 0    location of string in "from" if any,
-1    incomplete string or none at all.
 * **************************************************** */

int
y3_str_extstr(char* from, char* cs, char* to, unsigned int size)
{
  char* s = from;
  char* t = to;
  unsigned int h;
  int i, k, loc;

  if (!s || !cs || strlen(cs) != 2 || !y3_str_ischars(s, cs))
    return -1;

  loc = i = y3_str_gotoc(s, cs[0], 0, '\000');

  if (s[i] == cs[0]) {
    if ((k = y3_str_gotoc(s, cs[1], -1, '\000')) == -1)
      return 0;
    for (h = 0; i <= k && h < size; i++, h++)
      t[h] = *(s + i);
    if (h == size && t[h] != cs[1])
      return -1;
    t[h] = '\000';
    return loc;
  }
  return -1;
}

/* ****************************************************

"quotes" refer to characters in 'cs'

Extract String:

Takes "from",
finds location of first quote,
finds location of last quote,
copies characters from and
including cs[0], to
and including cs[1]
into "to" buffer which has to be
made big enough before calling
the function.

The "to" buffer will be NULL terminated.

Returns:

>= 0    location of string in "from" if any,
-1      incomplete string or none at all.

----------------------------------------------------
NOTE(s):

Same as y3_str_extstr();

except, instead of finding location of the first
quote it finds the location of the 'nth_quote' and
fill 'to' with the characters (with quotes) as a NULL
terminated string.

the 'inc_quotes' parameter is used to let the
user specify whether the quotes are to be included
in the extracted string.

'inc_quotes' 0 -> do not include quotes,
otherwise include quotes.

If nth_quote is > # of quotes then the last quote
is returned.

This is basically for caller that know what they're
doing, and the kind of input they are expecting.

If 'nth_quote' is < 0 or > # of total quotes in
'from', then 'nth_quote' is ignored.
----------------------------------------------------
 * **************************************************** */

int
y3_str_extstr_m(char* from,
                char* cs,
                char* to,
                unsigned int size,
                int nth_quote,
                int inc_quotes)
{
  char* s = from;
  char* t = to;
  unsigned int h;
  int i, k, loc;

  if (!s || !cs || strlen(cs) != 2 || !y3_str_ischars(s, cs))
    return -1;

  loc = i = y3_str_gotoc(s, cs[0], 0, '\000');

  if (nth_quote < 0 || nth_quote > y3_str_nchar(from, cs[1]))
    nth_quote = -1;

  if (s[i] == cs[0]) {
    if (!inc_quotes)
      i++; // skip the first quote
    if ((k = y3_str_gotoc(s, cs[1], nth_quote, '\000')) == -1)
      return 0;

    if (!inc_quotes)
      k--; // skip the last quote

    for (h = 0; i <= k && h < size; i++, h++)
      t[h] = *(s + i);
    if (h == size && t[h] != cs[1])
      return -1;
    t[h] = '\000';
    return loc;
  }
  return -1;
}

/*
 * This function returns an integer value representing
 * the length of the the longest word in a given string.
 *
 * Parameters:
 *
 * s         = string to search
 * seps      = string of characters, an encounter of which, would constitute
 * an end of a word. stopcs    = string of characters, an encounter of which
 * marks end of string and halts function execution.
 *
 * Returns:
 *           0 on error, >= 1 otherwise
 *
 * NOTE(s):
 *
 *           Maximum word length cannot be zero (doesn't make sense).
 *           The algorithm is O(n * C), where C = # of chars in seps.
 */

int
y3_str_get_maxwordlen(char* str, char* seps, char* stopcs)
{
  int m, l;
  char* s;

  if (!str || !seps)
    return 0;

  s = str;

  for (l = m = 0; !y3_str_ischar(stopcs, *s); s++) {
    if (y3_str_ischar(seps, *s)) {
      l = 0;
      continue;
    }
    if (++l > m)
      m = l;
  }

  return m;
}

/*
 * Check to see which extended ascii characters is no in 's' and sets 'c' to
 * it.
 *
 * return 1 if found, 0 if not.
 *
 * NOTE(s):
 *          character '\000' is ignored for obvious reasons.
 *          that is why, i starts with 1, not 0.
 */

int
y3_str_find_patsy_char(char* s, char* c)
{
  int i;

  if (s == NULL || c == NULL)
    return -1;

  for (i = 1; i < 256; i++) {
    if (!y3_str_nchar(s, (unsigned char)i)) { // found one
      (*c) = (char)i;
      return 1;
    }
  }
  return 0; // all 256 characters are in use
}

/*
 * Strips 'stag' from 'word'.
 *
 * returns 0 if cannot, 1 if stripped.
 */

int
y3_str_striptag(char* word, char* open_tag, char* close_tag)
{
  int wlen, close_taglen;
  char c[3] = {
    '\000',
    '\000',
    '\000',
  };

  if (word && open_tag && close_tag) {
    close_taglen = strlen(close_tag);
    wlen = strlen(word);
    if (!y3_str_isconsecutive(word, open_tag) ||
        !y3_str_find_patsy_char(word, &c[0]))
      return 0;
    word[0] = c[0];
    if (!y3_str_isconsecutive(&word[wlen - close_taglen], close_tag) ||
        !y3_str_find_patsy_char(word, &c[1]))
      return 0;
    word[wlen - 1] = c[1];
    y3_str_xchars(word, c);
  }

  return 1;
}

/*
 * Strips 'stag' from each word in 'words'.
 *
 * Returns number of words with no tags, which
 * includes the number of words that could not have
 * the tags stripped, -1 on error.
 *
 */

int
y3_str_striptagn(char** words, int nwords, char* open_tag, char* close_tag)
{
  int i, no_tag = 0;

  if (words && nwords && open_tag && close_tag) {
    for (i = 0; i < nwords; i++)
      if (!y3_str_striptag(words[i], open_tag, close_tag))
        no_tag++;
  } else
    return -1;
  return no_tag;
}

/*
 * Checks whether 'word' is tagged with given 'open_tag' and 'close_tag'
 * strings.
 *
 * Returns 1 if yes, 0 if not, -1 if error.
 */

int
y3_str_hastags(char* word, char* open_tag, char* close_tag)
{
  int wlen, close_taglen;
  if (word && open_tag && close_tag) {
    wlen = strlen(word);
    close_taglen = strlen(close_tag);
    if (y3_str_isconsecutive(word, open_tag) &&
        y3_str_isconsecutive(&word[wlen - close_taglen], close_tag))
      return 1;
  } else
    return -1;
  return 0;
}

/*
 * Checks whether 'word' is tagged with given 'open_tag' and 'close_tag'
 * strings.
 *
 * Returns 1 if yes, 0 if not, -1 if error.
 */

int
y3_str_hastags_wlen(char* word, char* open_tag, char* close_tag, int wlen)
{
  int close_taglen;
  if (wlen <= 0)
    return -1;
  if (word && open_tag && close_tag) {
    close_taglen = strlen(close_tag);
    if (wlen <= close_taglen)
      return -1;
    if (y3_str_isconsecutive(word, open_tag) &&
        y3_str_isconsecutive(&word[wlen - close_taglen], close_tag))
      return 1;
  } else
    return -1;
  return 0;
}

/* *********************************
Find the number of words
in a given ASCII string.

str needs to be NULL terminated.
 * ********************************* */

int
y3_str_nnwords(char* str, char* seps)
{
  char* s = str;
  char* seporator = seps;
  int nw;

  if (!s)
    return 0;

  for (nw = 0; *s; s++) {
    if (y3_str_ischar(seporator, *s))
      continue;
    nw++;
  }

  if (y3_str_ischar(seporator, *(s - 1)))
    return nw;
  return ++nw;
}

/* *******************************************
str = string to get bytes from
buf = string to put bytes into
skip_bytes =    bytes to skip over
and not put into buf.
stop_bytes =    bytes, any of which
are detected, terminate
execution.

buf needs to be big enough
buf is NULL terminated at end of
execution.
 ******************************************* */

unsigned long long
y3_str_getbytes(char* str,
                char* buf,
                char* skip_bytes,
                char* stop_bytes,
                unsigned long long nbytes)
{
  char* s = str;
  char* b = buf;
  unsigned long long i, j;

  for (j = i = 0; s[i] && i < nbytes; i++) {
    if (y3_str_ischar(skip_bytes, s[i]))
      i++;
    if (!y3_str_ischar(stop_bytes, s[i]))
      b[j++] = s[i];
  }

  b[j] = '\000'; // NULL terminate byte string

  return i;
}

int
y3_str_hasalpha(char* str, int size)
{
  char* s = str;
  int i;

  for (i = 0; isascii((unsigned char)s[i]) && i < size; i++)
    if (isalpha((unsigned char)s[i]))
      return 1;
  return 0;
}

int
y3_str_hasdigit(char* str, int size)
{
  char* s = str;
  int i;

  for (i = 0; isascii((unsigned char)s[i]) && i < size; i++)
    if (isdigit((unsigned char)s[i]))
      return 1;
  return 0;
}

/*
 * Returns number of characters that are digits
 */

int
y3_str_ndigit(char* str, int size)
{
  char* s = str;
  int i, n;

  for (n = i = 0; isascii((unsigned char)s[i]) && i < size; i++)
    if (isdigit((unsigned char)s[i]))
      n++;
  return n;
}

int
y3_str_ndigit_f(char* str, int size)
{
  char* s = str;
  int i, n;

  for (n = i = 0; isascii((unsigned char)s[i]) && i < size; i++)
    if (isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == '_' ||
        s[i] == 'e') {
      n++;
    } else if (i > 0 && s[i] == '-' && s[i - 1] == 'e') {
      n++;
    }
  return n;
}

/*
 * Returns number of characters that are alphabetic
 */

int
y3_str_nalpha(char* str, int size)
{
  char* s = str;
  int i, n;

  if (!str || size <= 0)
    return -1;

  for (n = i = 0; isascii(s[i]) && i < size; i++)
    if (isalpha(s[i]))
      n++;
  return n;
}

//
// This function handle ints
//

int
y3_str_digital(char* str, int size)
{
  if (!str || size <= 0)
    return -1;

  if (y3_str_ndigit(str, size) == size)
    return 1;
  return 0;
}

//
// This function handle decimals and ints
//

int
y3_str_digital_f(char* str, int size)
{
  if (!str || size <= 0)
    return -1;

  if (y3_str_ndigit_f(str, size) == size)
    return 1;
  return 0;
}

int
y3_str_alphabetic(char* str, int size)
{
  if (!str || size <= 0)
    return -1;

  if (y3_str_nalpha(str, size) == size)
    return 1;
  return 0;
}

/*
 * Summary:
 *
 * This functions checks the xfix string
 * for being the suffix or prefix of a
 * given string.
 *
 *
 * Paremeters:
 *
 * s      -> string to check
 * xfix   -> suffix or prefix string
 * stopcs -> characters which terminate the string
 * metac  -> specifier for using xfix as a prefix or suffix
 *           0 = prefix, > 0 = suffix
 *
 * Returns: 0 on error or not found, > 0 found
 *
 * NOTE(s):
 *
 * characters in stopcs must not appear in xfix.
 *
 */

int
y3_str_xfix(char* str, char* xfix, char* stopcs, int metac)
{
  char *s = str, *p = xfix;
  int n = 0;

  if (!p || !s)
    return 0;

  if (y3_str_nchars(p, stopcs))
    return 0;

  n = strlen(p);

  if (metac == 0) { // prefix
  } else {          // suffix
    //
    // Goto the end of the string.
    //
    for (; !y3_str_ischar(stopcs, *s) && *s; s++)
      ;
    //
    // Move back n number of chars
    // and check the string match.
    //
    s = s - n;
  }

  for (; *s++ == *p++ && n; n--)
    ; // keep looping

  if (!n)
    return 1;

  return 0;
}

int
y3_str_nchars(char* s, char* cs)
{
  int n = 0;

  if (!s || !cs)
    return n;

  while (*s) {
    n += y3_str_nchar(cs, *s++);
  }
  return n;
}

char
y3_str_ischarstoC(char* str, char* cs, char stopc)
{
  char* s = str;
  int i = 0, j = 0;

  while (*(s + i) && *(s + i) != stopc) {
    while (*(cs + j)) {
      if (*(s + i) == *(cs + j))
        return *(cs + j);
      j++;
    }
    j = 0;
    i++;
  }

  return 0;
}

char
y3_str_ischarsto(char* s, char* cs, int stop)
{
  char* p = cs;
  int i = 0;

  while (*s && i++ < stop) {
    while (*cs) {
      if (*s == *cs)
        return *cs;
      cs++;
    }
    cs = p;
    s++;
  }

  return 0;
}

int
y3_str_asciival(char* str)
{
  char* s = str;
  int aux = 0;

  for (; *s; s++)
    aux += (int)(*s);
  return aux;
}

// updated

/* Assumes a NULL('\000') terminaled strings */

int
y3_str_xchars(char* s, char* cs)
{
  char* p = s;
  char* c = cs;
  int n = 0;
  int brk, len;
  int i;

  if (s == NULL || cs == NULL)
    return 0;

  len = strlen(p);
  brk = strlen(c);

  //
  // a little work-around
  // for C's context view.
  //
  {
    char* _new;

    if (!brk || !len)
      return 0;

    _new = (char*)malloc(len + 1);
    brk = i = 0;

    while (*s && i < len) {
      while (*cs) {
        if (*s == *cs) {
          brk = 1;
          n++;
          break;
        }
        cs++;
      }
      cs = c;
      brk > 0 ? (brk = 0), *s++ : (_new[i++] = *s++);
    }

    if (i) {
      _new[i] = '\000';
      memset(p, '\000', len);
      memmove(p, _new, i + 1);
    }
    free(_new);
  }

  return n;
}

int
y3_str_x2chars(char* s, char* cs, char* sc)
{
  char* p = s;
  char* c = cs;
  int n = 0;
  char* news;
  int brk, len;
  int i;

  if (s == NULL || cs == NULL)
    return 0;

  len = strlen(p);
  brk = strlen(c);

  if (!brk || !len)
    return 0;

  news = (char*)malloc(len + 1);
  brk = i = 0;

  while (*s && i < len) {
    if (y3_str_ischar(sc, *s)) {
      memmove(&news[i], s, len - i);
      i = len;
      break;
    }
    while (*cs) {
      if (*s == *cs) {
        brk = 1;
        n++;
        break;
      }
      cs++;
    }
    cs = c;
    brk > 0 ? (brk = 0), *s++ : (news[i++] = *s++);
  }

  if (i) {
    news[i] = '\000';
    memset(p, '\000', len);
    memmove(p, news, i + 1);
  }
  free(news);

  return n;
}

/* Assumes a NULL('\000') terminaled string */

unsigned char
y3_str_ischar(char* s, char c)
{
  char* p = s;

  if (!p)
    return 0;

  for (; *p; p++)
    if (*p == c)
      return 1;
  //
  // incase s contains a NULL to be matched.
  //
  if (*p == c)
    return 1;
  return 0;
}

unsigned char
y3_str_ischar_i(char* s, char c)
{
  char* p = s;

  if (!p)
    return 0;

  for (; *p; p++)
    if (*p == tolower(c) || *p == toupper(c))
      return 1;
  //
  // incase s contains a NULL to be matched.
  //
  if (*p == tolower(c) || *p == toupper(c))
    return 1;
  return 0;
}

/*      *******************************************
PARAMS:
str = string to process
c = character to find FIRST or LAST
metac = 0 FIRST, -1 LAST.
VARS:
i = position of metac in str
n = counter of metac's
t = total metac's in str
String gets parsed and if c is found
metac is checked. If c is not found
0 is returned, else i return;
i var is number of caracters from
start of passed string address
down to the FIRST or LAST c.
metac can also be a user specific value
that is greater than or equal to 1.
If so, then when c is found from left to
right n times and is equal to metac,
execution stops and i is returned.
The return of 0 and not -1 allows for some
more flexablity.
To avoid issues first make sure that the
caracter you want to go to exists by using
ischar()/ischars() function(s).
******************************************* */

int
y3_str_gotoc(char* str, char c, int metac, int stopc)
{
  char* s = str;
  int i;
  int n = 0;
  int t = y3_str_nchar(s, c); // total # of c in str

  if (metac <= -1) // keep user input in bounds
    metac = t;

  for (i = 0; *s != stopc; s++, i++) {
    if (*s == c) {
      n++;
      if (metac == 0) // FIRST
        return i;
      else if (n == t) // LAST
        return i;
      else if (n == metac) // USER SELECTED
        return i;
    }
  }

  if (!n)
    // return -1;
    return 0;
  return i;
}

int
y3_str_gotoc_i(char* str, char c, int metac, int stopc)
{
  char* s = str;
  int i;
  int n = 0;
  int t = y3_str_nchar_i(s, c); // total # of c in str

  if (metac <= -1) // keep user input in bounds
    metac = t;

  for (i = 0; *s != tolower(stopc) && *s != toupper(stopc); s++, i++) {
    if (*s == tolower(c) || *s == toupper(c)) {
      n++;
      if (metac == 0) // FIRST
        return i;
      else if (n == t) // LAST
        return i;
      else if (n == metac) // USER SELECTED
        return i;
    }
  }

  if (!n)
    // return -1;
    return 0;
  return i;
}

/* Assumes a NULL('\000') terminaled string */

int
y3_str_nchar(char* s, char c)
{
  int n = 0;

  while (*s) {
    if (*s++ == c)
      n++;
  }
  return n;
}

int
y3_str_nchar_i(char* s, char c)
{
  int n = 0;

  while (*s) {
    if (*s == tolower(c) || *s == toupper(c))
      n++;
    s++;
  }
  return n;
}

/* Assumes a NULL('\000') terminaled strings */

char
y3_str_ischars(char* str, char* cs)
{
  char* s = str;
  int i = 0, j = 0;

  while (*(s + i)) {
    while (*(cs + j)) {
      if (*(s + i) == *(cs + j))
        return *(cs + j);
      j++;
    }
    j = 0;
    i++;
  }

  return 0;
}

/* **************************************************** *
Checks weather s contains concecutively, chars
in cs.
returns the number of characters that were
concecutively checked,
or -1 if s or cs == NULL or if len of cs > s
* **************************************************** */

int
y3_str_isconsecutive(char* s, char* cs)
{
  int i;

  if (!s || !cs)
    //        return -1;
    return 0;

  for (i = 0; s[i] != '\000' && cs[i] != 0 && s[i] == cs[i]; i++)
    ;

  return i;
}

/* **************************************************** *
Checks weather s contains concecutively, chars
in cs.
returns the number of characters that were
concecutively checked,
or -1 if s or cs == NULL or if len of cs > s
* **************************************************** */

int
y3_str_isconsecutive_i(char* s, char* cs)
{
  int i;

  if (!s || !cs)
    //        return -1;
    return 0;

  for (i = 0;
       s[i] != '\000' && cs[i] != 0 &&
       (toupper(s[i]) == toupper(cs[i]) || tolower(s[i]) == tolower(cs[i]));
       i++)
    ;

  return i;
}

/* ********************************************** *
Same as above, only cs is a character
string an encounter of any chars of which,
in s, will terminate execution.
* ********************************************** */

int
y3_str_gotocs_i(char* str, char* cs, int metac, int stopc)
{
  char* s = str;
  int i;
  char stopc_u = '\000', stopc_l = '\000';
  int n = 0;
  int t; // total # of c in str

  for (i = 0, t = 0; cs[i] != '\000'; i++)
    t += y3_str_nchar_i(s, cs[i]);

  if (metac <= -1) // keep user input in bounds
    metac = t;

  // this avoids issues with doing a toupper/tolower on a null...
  if (stopc == '\000')
    stopc_u = stopc_l = '\000';
  else {
    stopc_u = toupper(stopc);
    stopc_l = tolower(stopc);
  }

  for (i = 0; *s != stopc_u && *s != stopc_l; s++, i++) {
    if (y3_str_ischar_i(cs, *s)) {
      n++;
      if (metac == 0) // FIRST
        return i;
      else if (n == t) // LAST
        return i;
      else if (n == metac) // USER SELECTED
        return i;
    }
  }

  if (!n)
    return -1;
  return i;
}

/* ********************************************** *
Same as above, only cs is a character
string an encounter of any chars of which,
in s, will terminate execution.
* ********************************************** */

int
y3_str_gotocs(char* str, char* cs, int metac, int stopc)
{
  char* s = str;
  int i;
  int n = 0;
  int t; // total # of c in str

  for (i = 0, t = 0; cs[i] != '\000'; i++)
    t += y3_str_nchar(s, cs[i]);

  if (metac <= -1) // keep user input in bounds
    metac = t;

  for (i = 0; *s != stopc; s++, i++) {
    if (y3_str_ischar(cs, *s)) {
      n++;
      if (metac == 0) // FIRST
        return i;
      else if (n == t) // LAST
        return i;
      else if (n == metac) // USER SELECTED
        return i;
    }
  }

  if (!n)
    return -1;
  return i;
}

void
y3_str_switchc(char* str, char c, char n)
{
  char* s = str;
  int i;

  if (!y3_str_ischar(s, c))
    return;
  for (i = 0; s[i] != '\000'; i++)
    if (s[i] == c)
      s[i] = n;
  return;
}

/* ***************************************************** *
Checks for a substring inside string.
sn is the 'string number' parameter,
used to find a sn'th string in s.
returns:
-1    if not found or error in use.
> -1    if found (position of sub str).
* **************************************************** */

int
y3_str_hasstr(char* str, char* sub_str, int sn)
{
  char *s = str, *ss = sub_str;
  int i = 0;
  unsigned k, len;
  int n = 0;

  if (!s || !ss || sn <= 0)
    return -1;
  k = strlen(ss);
  len = strlen(s);
  if (len < k)
    return -1;

  i = y3_str_gotoc(s, ss[0], 0, '\000');

  if (!i && !y3_str_ischar(s, ss[0]))
    return -1;
_loop:
  if (y3_str_isconsecutive(&s[i], ss) == (signed)k)
    n++;
  if (n == sn)
    return i;
  if (s[i + 1] == '\000' || (unsigned)(i + 1) >= len)
    return -1;
  i += y3_str_gotoc(&s[i + 1], ss[0], 0, '\000') + 1;
  goto _loop;
}

int
y3_str_hasstr_i(char* str, char* sub_str, int sn)
{
  char *s = str, *ss = sub_str;
  int i = 0;
  unsigned k, len;
  int n = 0;

  if (!s || !ss || sn <= 0)
    return -1;
  k = strlen(ss);
  len = strlen(s);
  if (len < k)
    return -1;

  i = y3_str_gotoc_i(s, ss[0], 0, '\000');

  if (!i && !y3_str_ischar_i(s, ss[0]))
    return -1;
_loop:
  if (y3_str_isconsecutive_i(&s[i], ss) == (signed)k)
    n++;
  if (n == sn)
    return i;
  if (s[i + 1] == '\000' || (unsigned)(i + 1) >= len)
    return -1;
  i += y3_str_gotoc_i(&s[i + 1], ss[0], 0, '\000') + 1;
  goto _loop;
}

char**
y3_str_split(char* a_str, const char a_delim)
{
  char** result = 0;
  size_t count = 0;
  char* tmp = a_str;
  char* last_comma = 0;
  char delim[2];
  delim[0] = a_delim;
  delim[1] = 0;

  /* Count how many elements will be extracted. */
  while (*tmp) {
    if (a_delim == *tmp) {
      count++;
      last_comma = tmp;
    }
    tmp++;
  }

  /* Add space for trailing token. */
  count += last_comma < (a_str + strlen(a_str) - 1);

  /* Add space for terminating null string so caller
  knows where the list of returned strings ends. */
  count++;

  result = (char**)malloc(sizeof(char*) * count);

  if (result) {
    size_t idx = 0;
    char* token = strtok(a_str, delim);

    while (token) {
      assert(idx < count);
      *(result + idx++) = strdup(token);
      token = strtok(0, delim);
    }
    assert(idx == count - 1);
    *(result + idx) = 0;
  }

  return result;
}

char*
y3_str_ftoa(float f, int* status)
{
  typedef union
  {
    long L;
    float F;
  } LF_t;

  long mantissa, int_part, frac_part;
  short exp2;
  char* p;
  static char outbuf[15];
  LF_t x;

  *status = 0;
  if (f == 0.0) {
    outbuf[0] = '0';
    outbuf[1] = '.';
    outbuf[2] = '0';
    outbuf[3] = 0;
    return outbuf;
  }
  x.F = f;

  exp2 = (0xFF & (x.L >> 23)) - 127;
  //    exp2 = (unsigned char)(x.L >> 23) - 127;
  mantissa = (x.L & 0xFFFFFF) | 0x800000;
  frac_part = 0;
  int_part = 0;

  if (exp2 >= 31) {
    *status = _FTOA_TOO_LARGE;
    return 0;
  } else if (exp2 < -23) {
    *status = _FTOA_TOO_SMALL;
    return 0;
  } else if (exp2 >= 23)
    int_part = mantissa << (exp2 - 23);
  else if (exp2 >= 0) {
    int_part = mantissa >> (23 - exp2);
    frac_part = (mantissa << (exp2 + 1)) & 0xFFFFFF;
  } else /* if (exp2 < 0) */
    frac_part = (mantissa & 0xFFFFFF) >> -(exp2 + 1);

  p = outbuf;

  if (x.L < 0)
    *p++ = '-';

  if (int_part == 0)
    *p++ = '0';
  else {
    y3_str_ltoa_s(int_part, p, 14, 10);
    while (*p) {
      p++;
    }
  }
  *p++ = '.';

  if (frac_part == 0)
    *p++ = '0';
  else {
    char m, max;

    max = sizeof(outbuf) - (p - outbuf) - 1;
    if (max > 7)
      max = 7;
    /* print BCD */
    for (m = 0; m < max; m++) {
      /* frac_part *= 10;    */
      frac_part = (frac_part << 3) + (frac_part << 1);
      *p++ = (frac_part >> 24) + '0';
      frac_part &= 0xFFFFFF;
    }
    /* delete ending zeroes */
    for (--p; p[0] == '0' && p[-1] != '.'; --p)
      ;
    ++p;
  }
  *p = 0;

  return outbuf;
}

//
// returns
// 0  on OK
// -1 on ERROR
// -2 on ERROR with str buffer being too small
//

int
y3_str_ltoa_s(long value, char* str, size_t size, int radix)
{
  if (!(str != NULL))
    return -1;
  if (!(size > 0))
    return -1;
  if (!(radix >= 2 && radix <= 36)) {
    str[0] = '\0';
    return -2;
  }

  return y3_str_ltoa_helper(value, str, size, radix);
}

//
// returns
// 0  on OK
// -1 on ERROR
// -2 on ERROR with str buffer being too small
//
int
y3_str_ltoa_helper(long value, char* str, size_t size, int radix)
{
  unsigned long val;
  unsigned int digit;
  int is_negative;
  char buffer[33], *pos;
  size_t len;

  if (value < 0 && radix == 10) {
    is_negative = 1;
    val = -value;
  } else {
    is_negative = 0;
    val = value;
  }

  pos = buffer + 32;
  *pos = '\0';

  do {
    digit = val % radix;
    val /= radix;

    if (digit < 10)
      *--pos = '0' + digit;
    else
      *--pos = 'a' + digit - 10;
  } while (val != 0);

  if (is_negative)
    *--pos = '-';

  len = buffer + 33 - pos;
  if (len > size) {
    size_t i;
    char* p = str;

    /* Copy the temporary buffer backwards up to the available number of
     * characters. Don't copy the negative sign if present. */

    if (is_negative) {
      p++;
      size--;
    }

    for (pos = buffer + 31, i = 0; i < size; i++)
      *p++ = *pos--;

    str[0] = '\0';
    return -2;
  }

  memcpy(str, pos, len);
  return 0;
}

/*
 * src     -> string to be modified (inplace)
 * str_old -> string to be replaced in src
 * str_new -> string to replace str_old with in src
 * ts      -> string of terminators for str_old
 * nrep    -> number of replacements to make (maximum)
 *
 * NOTE(s):
 *       1) If str_new is NULL then str_old will just be removed.
 *       2) If ts exists, then str_old will only be replaced if
 *          its instance is postfixed with any of characters from ts.
 *       3) strings need to be NULL terminated.
 *       4) nrep is ignored if it is <= 0
 *
 * RETURNS: number of strings replaced.
 */

int
y3_str_replace_dynamic(char* _src,
                       char* str_old,
                       char* str_new,
                       char* ts,
                       int nrep)
{
  int n, old_str_pos, new_len = 0, old_len, sn = 1, src_len;
  char* src;

  if (_src && str_old) {
    if (str_new && *str_new != '\000')
      new_len = strlen(str_new);
    //
    // Make sure that str_old and str_new are not identical.
    //
    if (y3_str_match(str_new, str_old, new_len))
      return 0;
    old_len = strlen(str_old);
    src = _src;
    src_len = strlen(src);
    for (n = 0; (old_str_pos = y3_str_hasstr(src, str_old, sn)) > -1; n++)
      if (old_str_pos > -1) {
        if (ts && !y3_str_ischar(ts, src[old_str_pos + old_len])) {
          sn++;
          continue;
        }
        if (nrep > 0 && n >= nrep)
          break;
        if (*str_new != '\000')
          (void)y3_str_chgstr(
            src, (src_len + 1), str_new, new_len, old_str_pos, old_len);
        else
          memmove(&src[old_str_pos],
                  &src[old_str_pos + old_len],
                  src_len - old_len);
        //                    memmove(&src[old_str_pos], &src[old_str_pos +
        //                    old_len], strlen(src) - old_len);
        //
        // In case the new string contains an old string in it, we have to
        // update the start starting addr of the source string.
        // Otherwise we will wind up in an infinite loop.
        //
        src = src + old_str_pos + new_len;
        src_len += (new_len - old_len);
      } else
        break;
    return n;
  }
  return -1;
}

/* ******************************************** *
 * check if "str" is equal to "strm".
 *
 * strm needs to be NULL terminated.
 *
 * ******************************************** */

unsigned char
y3_str_match(char* str, char* strm, size_t str_len)
{
  char* s = str;
  char* p = strm;
  int n = str_len;

  if (!s || !p || n <= 0)
    return 0;

  while (*s && *p && (*s++ == *p++)) {
    n--;
  }

  if (n == 0) {
    if (*(s - 1) != *(p - 1) || *p) {
      return 0;
    }
    return 1;
  }

  return 0;
}

/* ***************************************************** *
 * change string "str" to "ps" in "s".
 *
 * str      -> main string
 * s_sz     -> size of str
 * ps       -> replacement string
 * p_sz     -> size of ps
 * p        -> position in s to start write ps
 * sp_sz    -> size of string 'being replaced' at (str + p)
 *
 * example of usage:
 *
 * to replace "88" in "s", with "XX"
 *
 * RETURNS: 0 if error 1 otherwise
 *
 *  int i = 1;
 *  char s[] = "88 13 98 39 400 88 900 88 abc 88 xyz88 88";
 *  _loop:
 *    p = str_hasstr(s, "88", i);
 *
 *   if(p == -1)
 *       goto _done;
 *   i += str_chgstr(s, slen, "XX", 2, p, 2);
 *   goto _loop;
 * ***************************************************** */

int
y3_str_chgstr(char* str,
              unsigned int s_sz,
              char* ps,
              unsigned int p_sz,
              unsigned int p,
              unsigned int sp_sz)
{
  char* s;
  char* buf;

  if (!s_sz || !p_sz || str == NULL || ps == NULL)
    return 0;

  s = str + p;

  if (p_sz + p >= s_sz) { // check if no tail.
    // remember to add a '\n' + '\000'
    int end_sz = s_sz - (p + sp_sz);
    char* end;

    end = (char*)malloc(end_sz);
    memmove(end, &str[p + sp_sz], end_sz);
    memmove(str + p, ps, p_sz);
    memmove(str + p + p_sz, "\n\000", 2);
    //
    // Attach the end, if any
    //
    if (end_sz)
      memmove(&str[p_sz + p], end, end_sz);
    free(end);
    return 1;
  }

  buf = (char*)malloc(s_sz);
  memset(buf, '\000', s_sz);
  memmove(buf, s + sp_sz, s_sz - p); /* save the tail */
  memmove(s, ps, p_sz);              /* insert the string */
  memmove(s + p_sz, buf, s_sz - p);  /* attach the tail */

  free(buf);
  return 1;
}

char*
y3_str_replace(const char* str, const char* from, const char* to)
{
  /* Adjust each of the below values to suit your needs. */

  /* Increment positions cache size initially by this number. */
  size_t cache_sz_inc = 16;
  /* Thereafter, each time capacity needs to be increased,
   * multiply the increment by this factor. */
  const size_t cache_sz_inc_factor = 3;
  /* But never increment capacity by more than this number. */
  const size_t cache_sz_inc_max = 1048576;

  char *pret, *ret = NULL;
  const char *pstr2, *pstr = str;
  size_t i, count = 0;
  ptrdiff_t* pos_cache = NULL;
  size_t cache_sz = 0;
  size_t cpylen, orglen, retlen, tolen, fromlen = strlen(from);

  /* Find all matches and cache their positions. */
  while ((pstr2 = strstr(pstr, from)) != NULL) {
    count++;

    /* Increase the cache size when necessary. */
    if (cache_sz < count) {
      cache_sz += cache_sz_inc;
      pos_cache =
        (ptrdiff_t*)realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
      if (pos_cache == NULL) {
        goto end_repl_str;
      }
      cache_sz_inc *= cache_sz_inc_factor;
      if (cache_sz_inc > cache_sz_inc_max) {
        cache_sz_inc = cache_sz_inc_max;
      }
    }

    pos_cache[count - 1] = pstr2 - str;
    pstr = pstr2 + fromlen;
  }

  orglen = pstr - str + strlen(pstr);

  /* Allocate memory for the post-replacement string. */
  if (count > 0) {
    tolen = strlen(to);
    retlen = orglen + (tolen - fromlen) * count;
  } else
    retlen = orglen;
  ret = (char*)malloc(retlen + 1);
  if (ret == NULL) {
    goto end_repl_str;
  }

  if (count == 0) {
    /* If no matches, then just duplicate the string. */
    strcpy(ret, str);
  } else {
    /* Otherwise, duplicate the string whilst performing
     * the replacements using the position cache. */
    pret = ret;
    memcpy(pret, str, pos_cache[0]);
    pret += pos_cache[0];
    for (i = 0; i < count; i++) {
      memcpy(pret, to, tolen);
      pret += tolen;
      pstr = str + pos_cache[i] + fromlen;
      cpylen = (i == count - 1 ? orglen : pos_cache[i + 1]) - pos_cache[i] -
               fromlen;
      memcpy(pret, pstr, cpylen);
      pret += cpylen;
    }
    ret[retlen] = '\0';
  }

end_repl_str:
  /* Free the cache and return the post-replacement string,
   * which will be NULL in the event of an error. */
  free(pos_cache);
  return ret;
}
