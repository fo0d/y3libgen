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

// TODO:
//
// finish y3_io_link_delete(struct y3_io_stream *stream, int n_params, ...)
//
//

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "io.h"

/*
 * The stream IO sub-system.
 *
 * A stream is opened to a desired handle, which is determined
 * by the OS if specified by user as a file name, otherwise
 * it is determined by the user and is specified by the handle
 * id #.
 *
 * The error field in y3_io_stream data structure is set to
 * either one of the Y3_IO_* errors or the global 'errno' variable.
 *
 */

/*
 * Global variables.
 */

size_t y3_errno = 0;
size_t Y3_IO_BLOCK_SIZE = 0x10000; // 64KB
struct y3_io_stream *y3_stdin;
struct y3_io_stream *y3_stdout;
struct y3_io_stream *y3_stderr;
struct _y3_io_errors *y3_io_errors;
struct y3_io_stream_tab *____y3_io_streams;

/*
 * Initializes the IO subsystem.
 *
 * Sets all streams to NULL (unlocked).
 *
 * This is an internal function, not meant to be used
 * by user directly, however in that case nothing will
 * be broken unless physical members of the y3_io_stream_tab
 * are altered.
 *
 */

struct y3_io_stream_tab *
____y3_io_init(int handle_bound) {
    static struct y3_io_stream_tab stab;
    register int i;

    ____y3_io_streams = &stab;

    y3_io_init_errors();
    y3_io_init_std();

    if (handle_bound >= stab.count) {
        stab.streams = (struct y3_io_stream **)realloc(stab.streams, (handle_bound << 2) * sizeof(struct y3_io_stream *));
        i = stab.count;
        stab.count = handle_bound << 2;
        for (; i < stab.count; i++)
//            stab.streams[i] = NULL;
            stab.streams[i] = (struct y3_io_stream *)malloc(1 * sizeof(struct y3_io_stream));
    }
    return &stab;
}

/*
 * Initialize the 3 standard IO streams.
 */
void
y3_io_init_std(void) {
    static int init;
    size_t binary = 0;

#ifdef _DOSLIKE
    binary = O_BINARY;
#endif

    if (!init) {
        init++;
        y3_stdin = y3_io_new(NULL, O_RDONLY | binary, 0, 0);
        y3_stdin->name = "stdin";
        y3_stdout = y3_io_new(NULL, O_WRONLY | binary, 0, 1);
        y3_stdout->name = "stdout";
        y3_stderr = y3_io_new(NULL, O_RDWR | binary, 0, 2);
        y3_stderr->name = "stderr";
    }
}

void
y3_io_init_errors(void) {
    static struct _y3_io_errors io_err_tab[Y3_IO_N_ERRORS] = {
            {"Y3_IO_ERROR: Handle cannot be obtained for this stream.", Y3_IO_HANDLE_ERROR,},
            {"Y3_IO_ERROR: This handle is locked.",                     Y3_IO_HANDLE_LOCKED_ERROR,},
            {"Y3_IO_ERROR: Stream does not exists.",                    Y3_IO_LIMIT_ERROR,},
            {"Y3_IO_ERROR: This stream is in use.",                     Y3_IO_ERROR_STREAM_LOCKED,},
            {"Y3_IO_ERROR: Cannot seek.",                               Y3_IO_SEEK_ERROR,},
            {"Y3_IO_ERROR: Cannot lock specified bytes.",               Y3_IO_LOCK_ERROR,},
            {"Y3_IO_ERROR: Cannot unlock specified bytes.",             Y3_IO_UNLOCK_ERROR,},
    };

    y3_io_errors = &io_err_tab[0];
}

/*
 * return number of handles in use.
 */

int
y3_io_used_handles(void) {
    return ____y3_io_init(0)->used;
}

/*
 * Get the stream of the specified handle.
 */

struct y3_io_stream *
y3_io_get_stream(const char *name, int handle) {
    struct y3_io_stream_tab *stab;
    struct y3_io_stream *s;
    register int i, j;

    stab = ____y3_io_init(0);

    if (handle < 0) {
        //
        // find file name among existing steams in stab
        // on success use found streams handle to check if its
        // already in stab.
        //
        for (j = i = 0; i < stab->count && j < stab->used; i++) {
            s = stab->streams[i];
            if (s) {
                j++;
                if (strncmp(s->name, (char *) name, s->nlen - 1) == 0) {
                    handle = s->handle;
                    break;
                }
            }
        }
    }

    if (handle > 3)
        return stab->streams[handle];
    return NULL;
}

/*
 * Set the file pointer stream->fp to specified position.
 *
 * NOTE:
 *       Seeking is done from current stream->fp to
 *       stream->fp + fp if fp != 0 and != -1
 */

void
y3_io_seek(struct y3_io_stream *stream, int fp) {
    int new_fp = -1;

    if (stream) {
        if (fp == Y3_IO_EOF) // seek to EOF
            new_fp = lseek(stream->handle, 0, SEEK_END);
        else if (fp == Y3_IO_BOF) // seek to BOF
            new_fp = lseek(stream->handle, 0, SEEK_SET);
        else if (fp == Y3_IO_SET_FP) // set stream->fp to current file pointer pos
            new_fp = lseek(stream->handle, 0, SEEK_CUR);
        else // seek to fp from stream->fp
            new_fp = lseek(stream->handle, stream->fp + fp, SEEK_SET);

        if (new_fp == -1)
            stream->error = y3_io_errors[Y3_IO_SEEK_ERROR].s;
        else
            stream->fp = new_fp;
    }
}

/*
 * Set the a lock on this stream so that no one else can have access to it.
 *
 * Locks nb (number of bytes) starting at position (fp) from start of file.
 *
 * The current file-pointer position prior to this call must(will) be preserved.
 *
 * Stream error is set to the appropriate error.
 */

void
y3_io_lock(struct y3_io_stream *stream, int fp, int nb) {
    int o_fp;

    if (stream) {
        o_fp = stream->fp;

        y3_io_seek(stream, fp);

        if (!stream->error) {
            if (_locking(stream->handle, LK_NBLCK, nb) == -1) {
                stream->error = y3_io_errors[Y3_IO_LOCK_ERROR].s;
            } else {
                fprintf(stderr, "stream [%d][%s] successfully locked [%d] bytes from position [%d]\n", stream->id,
                         stream->name, nb, fp);
                stream->io_lock = 1;
                stream->io_lock_fp = fp;
                stream->io_lock_nb = nb;
            }
        }
        stream->fp = o_fp;
    }
}

/*
 * Unlock this stream.
 *
 * Unlocks nb (number of bytes) starting at position (fp)
 *
 * Stream error is set to the appropriate error.
 */
void
y3_io_unlock(struct y3_io_stream *stream, int fp, int nb) {
    int o_fp;

    if (stream) {
        o_fp = stream->fp;

        y3_io_seek(stream, fp);

        if (!stream->error) {
            if (_locking(stream->handle, LK_UNLCK, nb) == -1) {
                stream->error = y3_io_errors[Y3_IO_LOCK_ERROR].s;
            } else {
                fprintf(stderr, "stream [%d][%s] successfully un-locked [%d] bytes from position [%d]\n", stream->id,
                         stream->name, nb, fp);
                if (stream->io_lock_nb - nb == 0) { // all bytes unlocked
                    stream->io_lock_nb = 0;
                    stream->io_lock_fp = -3;
                    stream->io_lock = 0;
                } else {
                    stream->io_lock_nb -= nb;
                    stream->io_lock = 0;
                    stream->io_lock_fp = -3;
                }
            }
        }
        stream->fp = o_fp;
    }
}

/*
 * In case of 'fd' being valid and 'fname' specified, fd
 * superseeds 'fname'.
 */

struct y3_io_stream *
y3_io_new(const char *fname, unsigned int o_flags, unsigned int p_flags, int fd) {
    struct y3_io_stream *stream;
    static int n_calls = 3, handle_bound = 1;
    struct y3_io_stream_tab *stab;

    stream = (struct y3_io_stream *)malloc(1 * sizeof(struct y3_io_stream));
    stream->error = NULL;
    stream->handle = fd;

    stab = ____y3_io_init(handle_bound);

    if (fd == 0 || fd == 1 || fd == 2)
        goto _std;

    if (!fname && (stream->handle < -1)) {
        stream->error = y3_io_errors[Y3_IO_HANDLE_ERROR].s;
        goto _error;
    }

    if (fname) {
        stream->nlen = strlen(fname) + 1;
        stream->name = (char *)malloc(stream->nlen);
        memmove(stream->name, fname, stream->nlen);
    } else {
        fname = NULL;
    }

    //
    // Check if handle is in the table, if not
    // hash the new handle, return that stream which
    // is associated with that handle.
    //

    if (!fname || y3_io_get_stream(fname, stream->handle) != NULL) {
        y3_errno = Y3_IO_ERROR_STREAM_LOCKED;
        goto _error;
#ifdef _DOSLIKE
        } else if ((stream->handle = _open(fname, o_flags, p_flags)) < 0) {
            stream->error = strerror(errno);
            goto _error;
        }
#else
    //} else if ((stream->handle = open(fname, o_flags)) < 0) {
    } else if ((stream->handle = open(fname, o_flags)) < 0) {
        stream->error = strerror(errno);
        goto _error;
    }
#endif

#ifdef _DOSLIKE
        else if (_fstat(stream->handle, &stream->st)) {
                stream->error = strerror(errno);
                goto _error;
            }
#else
    else if (fstat(stream->handle, &stream->st)) {
        stream->error = strerror(errno);
        goto _error;
    }
#endif

    if (stream->handle > handle_bound)
        stab = ____y3_io_init((handle_bound = stream->handle));

#ifdef _DOSLIKE
    if (_chmod(fname, p_flags) == -1)
        stream->error = strerror(errno);
#else
   if(fchmod(stream->handle, p_flags))
       stream->error = strerror(errno);
#endif

    y3_errno = errno;

    if (stream->error) {
        _error:
        y3_io_perror(stream);
        free(stream);
        return NULL;
    }

    _std:

    stab->streams[stream->handle] = stream;
    stab->used++;

    stream->buf = (char *)malloc(1 * sizeof(char));
    stream->id = n_calls++;
    stream->bytes_read = 0;
    stream->bytes_written = 0;
    stream->nlen = 0;
    stream->nlinks = 0;
    stream->rlock = 0;
    stream->nrdr = 0;
    stream->mode = o_flags;
    stream->fp = 0;
    stream->io_lock = 0;
    stream->io_lock_fp = 0;
    stream->io_lock_nb = 0;
    stream->links = (int *)malloc(1 * sizeof(int));
    return stream;
}

/*
 * Format for '...':
 *
 * n_params specifies how many parameters are sent.
 *
 * each parameter needs to be a pointer of 'struct y3_io_stream'
 *
 * The 'force_release' parameters ensures that even if
 * a stream couldn't be closed, the memory it occupied will be
 * released.
 *
 * The user will need to look through all the streams that
 * were not released (e.g. not NULL) and check that 'stream->error'
 * is set to the global 'errno' variable.
 *
 * Returns:  number of streams that couldn't be closed
 */

int
y3_io_close(int force_release, int n_params, ...) {
    struct y3_io_stream *stream;
    register int n_miss = 0;
    va_list params;

#ifdef _LINUX
    sync();
#else
    _flushall();
#endif

    va_start(params, n_params);
    while (n_params--) {
        stream = va_arg(params, struct y3_io_stream *);
        if (stream) {
            if (stream->io_lock)
                y3_io_unlock(stream, stream->io_lock_fp, stream->io_lock_nb);
            if (close(stream->handle) < 0) {
                stream->error = strerror(errno);
                n_miss++;
            }
            if (force_release) {
               	free(stream->name);
                free(stream->buf);
               	free(stream->links);
               	free(stream);
            }
        }
    }
    va_end(params);
    return n_miss;
}

void
y3_set_io_block_size(size_t st_blksize) {
    if (Y3_IO_BLOCK_SIZE < st_blksize) {
        Y3_IO_BLOCK_SIZE = st_blksize;
    }
}

/*
 * The optimum IO block size for read/write operations
 * is calculated on most systems, however BORLAND
 * doesn't support this.
 */

size_t
y3_io_read(struct y3_io_stream *stream, size_t nbytes) {
    int read_chunk, read_total = 0, to_read;
    static char *rbuf; // this chunk lives as long as the program lives

    rbuf = (char *)malloc(Y3_IO_BLOCK_SIZE * sizeof(char));

    if (stream->st.st_size == 0)
        return 0;

#ifdef _ST_BLKSIZE_OK
    y3_set_io_block_size(stream->st.st_blksize);
    y3_realloc(rbuf, Y3_IO_BLOCK_SIZE);
#endif

    if (nbytes == Y3_IO_READ_TO_EOF)
        to_read = stream->st.st_size;
    else
        to_read = nbytes;

    stream->buf = (char *)realloc(stream->buf, to_read);

    while ((read_chunk = read(stream->handle, rbuf, Y3_IO_BLOCK_SIZE)) > 0) {
        memmove(&stream->buf[read_total], rbuf, read_chunk);
        stream->bytes_read += read_chunk;
        read_total += read_chunk;
        if (read_total == to_read)
            break;
    }

    if (read_total != to_read)
        stream->error = strerror(errno);
    y3_io_seek(stream, Y3_IO_SET_FP); // set stream->fp to current file pointer position
    return read_total;
}

size_t
y3_io_copy(struct y3_io_stream *stream_to, struct y3_io_stream *stream_from, size_t nbytes) {
    register int i = 0, nrdr;
    struct y3_io_stream *stream;
    struct y3_io_stream_tab *stab = ____y3_io_streams;
    size_t bytes_written = 0;

    if (nbytes == 0 || stream_to == NULL || stream_from == NULL)
        return 0;

    stream = stream_to;

    nrdr = stream_to->nrdr;

    bytes_written += y3_io_write(stream, stream_from->buf, nbytes);

    if (stream->redirect && nrdr) {
        while (i++ < stream->nlinks) {
            stream = stab->streams[stream->links[i]];
            if (stream && !stream->rlock) {
                bytes_written += y3_io_write(stream, stream_from->buf, nbytes);
                nrdr--;
            }
        }
    }
    y3_io_seek(stream_to, Y3_IO_SET_FP);
    y3_io_seek(stream_from, Y3_IO_SET_FP);
    return bytes_written;
}

size_t
y3_io_write(struct y3_io_stream *stream_to, const void *dbuf, size_t nbytes) {
    register int i = 0, write_stat, nrdr;
    struct y3_io_stream *stream, *stream_next;
    struct y3_io_stream_tab *stab = ____y3_io_streams;
    size_t tail, to_write, bytes_written = 0, nbytes_orig = nbytes;
    const void *buf;

    if (nbytes == 0 || stream_to == NULL || dbuf == NULL)
        return 0;
    else
        to_write = nbytes;

    stream = stream_to;
    buf = dbuf;

    //
    // For some reason the O_BINARY or O_TEXT modes are not preserved.
    //
    // So we need to reset them before writing.
    //

#ifdef _ST_BLKSIZE_OK
    y3_set_io_block_size(stream->st.st_blksize);
#endif

    if (nbytes > Y3_IO_BLOCK_SIZE)
        nbytes = Y3_IO_BLOCK_SIZE;

    tail = to_write % 2;

    nrdr = stream_to->nrdr;

    _loop:
    while ((write_stat = write(stream->handle, buf, nbytes)) == (signed) nbytes) {
        stream->bytes_written += nbytes;
        bytes_written += nbytes;
        to_write -= nbytes;
        if ((signed) to_write - (signed) nbytes < (signed) tail)
            break;
    }

    if (tail && to_write) {
        if (write_stat != -1) {
            nbytes = tail;
            goto _loop;
        }
    }

    if (to_write)
        stream->error = strerror(errno);

    if (stream_to->redirect && nrdr) {
        while (i++ < stream_to->nlinks) {
//            stream_next = stab->streams[stream_to->links[i]];
            stream_next = stab->streams[stream_to->links[i]];
            if (stream_next && !stream_next->rlock) {
                stream = stream_next;
                to_write = nbytes_orig;
                nrdr--;
                goto _loop;
            }
        }
    }
    y3_io_seek(stream_to, Y3_IO_SET_FP);
    return bytes_written;
}

/*
 * toggles the redirect flag between on or off.
 *
 * this signals to y3_io_write() and y3_io_copy()
 * to output to the specified stream and also
 * to all the streams in it's 'links' table.
 */

void
y3_io_toggle_redirect(struct y3_io_stream *stream) {
    if (stream) if (stream->links && stream->nlinks) {
        if (stream->redirect)
            stream->redirect = 0;
        else
            stream->redirect = 1;
    }
}

/*
 * Links stream to other streams.
 *
 * Everything written to specified stream will also
 * be written to all streams in it's links.
 *
 * returns -1 on error, 0 on all good, > 0 on unable to link.
 */

int
y3_io_link(struct y3_io_stream *stream, int n_params, ...) {
    register int i, n_miss = 0;
    struct y3_io_stream *dst;
    va_list params;
    struct y3_io_stream_tab *stab;
    va_start(params, n_params);

    if (stream == NULL || n_params < 1)
        return -1;

    stab = ____y3_io_init(0);

    if (stab->count > stream->nlinks) {
        stream->links = (int *)realloc(stream->links, stab->count * sizeof(int *));
        memset(stream->links, 0, stab->count * sizeof(int *));
    }

    //stream->nlinks = stab->count;

    for (i = 0; i < n_params; i++) {
        dst = va_arg(params, struct y3_io_stream *);
        if (dst == NULL)
            n_miss++;
        else {
//            stream->links[dst->handle] = dst;
            stream->links[stream->nlinks] = dst->handle;
            stream->nlinks++;
            stream->nrdr++;
        }
    }
    va_end(params);

    return 0;
}

void
y3_io_link_delete(struct y3_io_stream *stream, int n_params, ...) {
}

void
y3_io_toggle_rlock(struct y3_io_stream *stream) {
    if (stream) {
        if (stream->rlock)
            stream->rlock = 0;
        else
            stream->rlock = 1;
    }
}

void
y3_io_perror(struct y3_io_stream *stream) {
    if (stream && stream->error) {
        //
        // Write the error to the standard error out stream
        //

        if (stream->name) {
            y3_io_write(y3_stderr, stream->name, strlen(stream->name));
            y3_io_write(y3_stderr, ": ", 2);
        }
        y3_io_write(y3_stderr, stream->error, strlen(stream->error));
        y3_io_write(y3_stderr, "\n", 1); // append a newline just in case
    }
}