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

#ifdef _LINUX
#define LK_UNLCK LOCK_UN
#define LK_NBLCK LOCK_NB
#define _locking lockf
#define _O_CREAT O_CREAT
#define _O_WRONLY O_WRONLY
#define _O_RDONLY O_RDONLY
#define _O_RDWR O_RDWR
#define _O_APPEN O_APPEN
#define _O_ASYNC O_ASYNC
#define _O_OSYNC O_OSYNC
#define _O_CLOEXEC O_CLOEXEC
#define _O_DIRECT O_DIRECT
#define _O_DIRECTORY O_DIRECTORY
#define _O_EXECL O_EXECL
#define _O_LARGEFILE O_LARGEFILE
#define _O_NONBLOCK O_NONBLOCK
#define _O_NDELAY O_NDELAY
#define _O_TRUNC O_TRUNC
#define _S_IWRITE S_IWRITE
#define _S_IREAD S_IREAD
#endif

#ifdef _DOSLIKE
#define open _open
#define setmode _setmode
#ifndef _DJGPP
#define lseek _lseek
#endif
#define chmod _chmod
#define close _close
#define read _read
#define write _write
#define fdopen _fdopen
#endif

#ifdef _DJGPP
#define _locking lock
#define _lockf lock
#define _lseek lseek
#define LOCK_SH 1 /* shared lock */
#define LOCK_EX 2 /* exclusive lock */
#define LOCK_NB 4 /* don't block when locking */
#define LOCK_UN 8 /* unlock */
#define LK_UNLCK LOCK_UN
#define LK_NBLCK LOCK_NB
#endif

#ifdef _SOLARIS
#define _locking lockf
#define LK_UNLCK F_ULOCK
#define LK_NBLCK F_LOCK
#define _O_CREAT O_CREAT
#define _O_WRONLY O_WRONLY
#define _O_RDONLY O_RDONLY
#define _O_RDWR O_RDWR
#define _O_APPEN O_APPEN
#define _O_ASYNC O_ASYNC
#define _O_OSYNC O_OSYNC
#define _O_CLOEXEC O_CLOEXEC
#define _O_DIRECT O_DIRECT
#define _O_DIRECTORY O_DIRECTORY
#define _O_EXECL O_EXECL
#define _O_LARGEFILE O_LARGEFILE
#define _O_NONBLOCK O_NONBLOCK
#define _O_NDELAY O_NDELAY
#define _O_TRUNC O_TRUNC
#endif

extern size_t Y3_IO_BLOCK_SIZE;
extern size_t y3_errno; // just like errno, but only set by y3_io lib
extern struct y3_io_stream* y3_stdin;
extern struct y3_io_stream* y3_stdout;
extern struct y3_io_stream* y3_stderr;
extern struct _y3_io_errors* y3_io_errors;
extern struct y3_io_stream_tab* ____y3_io_streams;

#define Y3_IO_READ_TO_EOF 0x0
#define Y3_IO_N_RESERVED_HANDLES 0x3 // stdin,stdout,stderr
#define Y3_IO_FORCE_RELEASE 110
#define Y3_IO_SET_FP -2
#define Y3_IO_EOF -1
#define Y3_IO_BOF 0

enum
{
  Y3_IO_HANDLE_ERROR = 0,
  Y3_IO_HANDLE_LOCKED_ERROR,
  Y3_IO_LIMIT_ERROR,
  Y3_IO_ERROR_STREAM_LOCKED,
  Y3_IO_SEEK_ERROR,
  Y3_IO_LOCK_ERROR,
  Y3_IO_UNLOCK_ERROR,
  Y3_IO_N_ERRORS // number of different errors for this interface
} Y3_IO_ERRORS;

struct _y3_io_errors
{
  char* s; /* error string */
  int id;  /* error id */
};

extern struct _y3_io_error_tab* y3_io_error_tab;

struct _y3_io_error_tab
{
  char* name; /* error string */
  int id;     /* error id */
};

struct y3_io_stream_tab
{
  struct y3_io_stream** streams;
  int count;
  int used;
};

void
y3_io_init_errors(void);
struct y3_io_stream_tab*
____y3_io_init(int handle_bound);
struct y3_io_stream*
y3_io_new(const char* fname,
          unsigned int o_flags,
          unsigned int p_flags,
          int fd);
int
y3_io_close(int force_release, int n_params, ...);
size_t
y3_io_read(struct y3_io_stream* stream, size_t nbytes);
size_t
y3_io_write(struct y3_io_stream* stream_to, const void* buf, size_t nbytes);
size_t
y3_io_copy(struct y3_io_stream* stream_to,
           struct y3_io_stream* stream_from,
           size_t nbytes);
struct y3_io_stream*
y3_io_get_stream(const char* name, int handle);
void
y3_set_io_block_size(size_t st_blksize);
int
y3_io_link(struct y3_io_stream* src, int n, ...);
void
y3_io_init_std(void);
void
y3_io_toggle_redirect(struct y3_io_stream* stream);
void
y3_io_link_delete(struct y3_io_stream* stream, int n_params, ...);
void
y3_io_toggle_rlock(struct y3_io_stream* stream);
void
y3_io_perror(struct y3_io_stream* stream);
void
y3_io_lock(struct y3_io_stream* stream, int fp, int nb);
void
y3_io_unlock(struct y3_io_stream* stream, int fp, int nb);
void
y3_io_seek(struct y3_io_stream* stream, int fp);

struct y3_io_stream
{
  int handle; // file descriptor
  int id;     // stream id
  char* name; // name of file (if any) associated with handle
  int nlen;   // length of name
  char* buf;  // read/write buffer
  int* links; // addresses of streams to which to write when writing to
  // this stream, if redirect flag is on.
  int nlinks;   // number of links in links table
  int redirect; // redirect flag
  int nrdr;     // number of streams this stream will redirect output to
  int rlock;    // redirection lock, set to avoid redirecting
  // to this stream even if it is in the links table
  // of another stream
  int fp;               // current file pointer position
  int io_lock;          // 1 if access lock is set, 0 otherwise
  int io_lock_fp;       // file pointer position at which the locking starts
  int io_lock_nb;       // number of bytes locked
  unsigned mode;        // file mode flags
  size_t bytes_read;    // total bytes read from this stream
  size_t bytes_written; // total bytes written from this stream
#ifndef _TI89
  struct stat st; // file status of handle
#endif
  char* error; // set to Y3_IO_*_ERROR or strerror(id);
};
