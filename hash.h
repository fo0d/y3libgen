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

#include <stddef.h>
#include <stdlib.h>



/* *********** INTERFACE START ******* */

struct y3_hash_table_data {
    void *value;
    void *key;
};

typedef struct y3_hash_table_data y3_hashItem;

struct y3_hashtable {
    size_t       N;         // total inserted items
    size_t       M;         // total cells in table
    y3_hashItem *st;        // table
    void        *NULL_item; // marks unused cell

    int      *primes;       // link to prime values
    int      NPRIMES;       // size of primes array
    unsigned int collisions;
    int          dne;       // do not expand flag

    //
    // Links to user defined functions.
    //
    size_t        (* hash_func)(struct y3_hashtable *ht, void *);
    int           (* hash_less)(struct y3_hashtable *ht, void *a, void *b);
    int           (* hash_null)(struct y3_hashtable *ht, size_t idx);
    int           (* hash_eq)(struct y3_hashtable *ht, void *a, void *b);
    void          (* hash_setkey)(y3_hashItem *item, void *key, void *value);
    void         *(* hash_getkey)(y3_hashItem *item);
    void          (* hash_free_item)(struct y3_hashtable *ht, y3_hashItem *item);
};

void                 y3_hash_set_dne(struct y3_hashtable *ht, int val);
void                 y3_hash_expand(struct y3_hashtable *ht);
void                 y3_hash_insert(struct y3_hashtable *ht, void *key, y3_hashItem *item);
void                 y3_hash_debug_print_keys(struct y3_hashtable *ht);
y3_hashItem         *y3_hash_search(struct y3_hashtable *ht, void *key);
void                 y3_hash_delete(struct y3_hashtable *ht, void *key);
size_t               y3_hash_count(struct y3_hashtable *ht);
void                 y3_hash_init(struct y3_hashtable *ht, size_t size, int exp);
void                 y3_hash_release(struct y3_hashtable *ht);
int                  y3_hash_eq(struct y3_hashtable *ht, void *a, void *b);
int                  y3_hash_null(struct y3_hashtable *ht, size_t idx);

// General string hashing functions
int                  y3_hash_string_less(struct y3_hashtable *ht, void *a, void *b);
size_t               y3_hash_string1(struct y3_hashtable *ht, void *key); //
size_t               y3_hash_string2(struct y3_hashtable *ht, void *key); // (universal) default hash for strings
size_t               y3_hash_string3(struct y3_hashtable *ht, void *key); //
size_t               y3_hash_string4(struct y3_hashtable *ht, void *key); //
size_t                y3_hash_string5(struct y3_hashtable *ht, void *key); //

// General integer hashing functions
int                  y3_hash_int1_less(struct y3_hashtable *ht, void *a_value, void *b_value);
size_t               y3_hash_int1(struct y3_hashtable *ht, void *_key);
size_t               y3_hash_int2(struct y3_hashtable *ht, void *_key);

void                 y3_hash_setkey(y3_hashItem *item, void *key, void *value);
void                *y3_hash_getkey(y3_hashItem *item);

struct y3_hashtable *y3_hash_new_table(size_t initial_size,
    void         *NULL_item,
    size_t       (* hash_func)(struct y3_hashtable *ht, void *),
    int          (* hash_less)(struct y3_hashtable *ht, void *a, void *b),
    int          (* hash_null)(struct y3_hashtable *ht, size_t idx),
    int          (* hash_eq)(struct y3_hashtable *ht, void *a, void *b),
    void         (* hash_setkey)(y3_hashItem *item, void *key, void *value),
    void        *(* hash_getkey)(y3_hashItem *item),
    void         (* hash_free_item)(struct y3_hashtable *ht, y3_hashItem *item)
    );

void             y3_hash_free_item(struct y3_hashtable *ht, y3_hashItem *item);

//
// The stuff below is for Pual Hsieh's super-fast-hash.
//

//#include "pstdint.h" /* Replace with <stdint.h> if appropriate */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
    || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const unsigned short *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
    +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
/* *********** INTERFACE END ******* */