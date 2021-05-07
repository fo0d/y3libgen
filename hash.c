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

#include <stdlib.h> 
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "hash.h"
#include "primes.h"

/* *********** IMPLEMENTATION START ********* */

/* **************** DEFAULT COMPARABLES START ********************* */

int
y3_hash_int1_less(struct y3_hashtable *ht, void *a_value, void *b_value) {
  return ((int) a_value < (int) b_value);
}

size_t
y3_hash_int1(struct y3_hashtable *ht, void *_v) {
  return (ht->primes[(unsigned int) _v % ht->NPRIMES] * (unsigned int) _v) % ht->M;
}

size_t
y3_hash_int2(struct y3_hashtable *ht, void *_v) {
  if (_v != ht->NULL_item)
    assert(ht->M >= (int) _v);
  return (size_t)_v;
}

int
y3_hash_string_less(struct y3_hashtable *ht, void *a_value, void *b_value) {
  return strcmp((char *) a_value, (char *) b_value);
}

int
y3_hash_null(struct y3_hashtable *ht, size_t idx) {
  return ht->hash_getkey(&ht->st[idx]) == ht->NULL_item;
}

int
y3_hash_eq(struct y3_hashtable *ht, void *a_value, void *b_value) {
  return (!ht->hash_less(ht, a_value, b_value) && !ht->hash_less(ht, b_value, a_value));
}

/* **************** DEFAULT COMPARABLES END ********************* */

void
y3_hash_free_item(struct y3_hashtable *ht, y3_hashItem *item) {
  return;
}

void *
y3_hash_getkey(y3_hashItem *item) {
  return item->key;
}

void
y3_hash_setkey(y3_hashItem *item, void *key, void *value) {
  item->key = key;
  item->value = value;
}

void
y3_hash_set_dne(struct y3_hashtable *ht, int val) {
  ht->dne = val;
}

struct y3_hashtable *
y3_hash_new_table(size_t initial_size,
          void *NULL_item,
          size_t     (*hash_func)(struct y3_hashtable *ht, void *),
          int     (*hash_less)(struct y3_hashtable *ht, void *a, void *b),
          int     (*hash_null)(struct y3_hashtable *ht, size_t idx),
          int     (*hash_eq)(struct y3_hashtable *ht, void *a, void *b),
          void    (*hash_setkey)(y3_hashItem *item, void *key, void *value),
          void *(*hash_getkey)(y3_hashItem *item),
          void    (*hash_free_item)(struct y3_hashtable *ht, y3_hashItem *item)
) {
  struct y3_hashtable *ht;

  ht = (struct y3_hashtable *) malloc(sizeof(struct y3_hashtable));
  ht->M = 0;
  if (!ht->dne)
    ht->dne = 0;

  //
  // Set functions.
  //

  if (!hash_func)
    ht->hash_func = y3_hash_string5;
  else
    ht->hash_func = hash_func;

  if (!hash_less)
    ht->hash_less = y3_hash_string_less;
  else
    ht->hash_less = hash_less;

  if (!hash_null)
    ht->hash_null = y3_hash_null;
  else
    ht->hash_null = hash_null;

  if (!hash_eq)
    ht->hash_eq = y3_hash_eq;
  else
    ht->hash_eq = hash_eq;

  if (!hash_setkey)
    ht->hash_setkey = y3_hash_setkey;
  else
    ht->hash_setkey = hash_setkey;

  if (!hash_getkey)
    ht->hash_getkey = y3_hash_getkey;
  else
    ht->hash_getkey = hash_getkey;

  if (!hash_free_item)
    ht->hash_free_item = y3_hash_free_item;
  else
    ht->hash_free_item = hash_free_item;

  if (!NULL_item)
    ht->NULL_item = NULL;
  else
    ht->NULL_item = NULL_item;

  ht->primes = y3_init_primes();
  ht->NPRIMES = Y3_N_PRIME_NUMBERS;

  y3_hash_init(ht, initial_size, 0);

  return ht;
}

size_t
y3_hash_string1(struct y3_hashtable *ht, void *_v) {
  size_t h = 0, a = 127;
  char *v = (char *) _v;

  for (; *v != '\0'; v++)
    h = (a * h + *v) % ht->M;
  return h;
}

size_t
y3_hash_string2(struct y3_hashtable *ht, void *_v) {
  register size_t h;

  register size_t a; // prime 1
  register size_t b; // prime 2
  char *v = (char *) _v;

  a = (805306457);
  b = (1610612741);

  for (h = 0; *v != '\0'; v++, a = a * b % (ht->M - 1))
    h = (a * h + *v) % ht->M;

  return h;
}

size_t
y3_hash_string3(struct y3_hashtable *ht, void *_v) {
  register char *str = _v;
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash % ht->M;
}

size_t
y3_hash_string4(struct y3_hashtable *ht, void *_v) {
  register char *str = _v;
  unsigned long hash = 0;
  int c;

  while ((c = *str++))
    hash = c + (hash << 6) + (hash << 16) - hash;

  return hash % ht->M;
}

/*
* This is a horrible one.
*/

size_t
y3_hash_string5(struct y3_hashtable *ht, void *_v) {
  size_t hash, tmp, len;
  char *data = _v;
  size_t rem;

  if (data == ht->NULL_item)
    return 0;

  hash = len = strlen(data);

  rem = len & 3;
  len >>= 2;

  /* Main loop */
  for (; len > 0; len--) {
    hash += get16bits(data);
    tmp = (get16bits (data + 2) << 11) ^ hash;
    hash = (hash << 16) ^ tmp;
    //    data  += 2*sizeof (unsigned short);
    data += 4;
    hash += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
    case 3:
      hash += get16bits (data);
      hash ^= hash << 16;
      hash ^= data[sizeof(unsigned short)] << 18;
      hash += hash >> 11;
      break;
    case 2:
      hash += get16bits (data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
    default:
    case 1:
      hash += *data;
      hash ^= hash << 10;
      hash += hash >> 1;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return hash % ht->M;
}

void
y3_hash_release(struct y3_hashtable *ht) {
  register int i;

  for (i = 0; i < ht->M; i++)
    if (ht->st[i].key)
      ht->hash_free_item(ht, &ht->st[i]);
  free(ht);
}

void
y3_hash_init(struct y3_hashtable *ht, size_t size, int exp) {
  size_t i;
  size_t old_M;
  struct y3_hashtable *tmp_ht;

  if (ht->M == 0)
    ht->M = size;

  if (!exp)
    ht->N = 0;

  old_M = ht->M;

  ht->M <<= 1;

  if (!exp) {
    ht->st = malloc(ht->M * sizeof(y3_hashItem));
    for (i = 0; i < ht->M; i++)
      y3_hash_setkey(&ht->st[i], ht->NULL_item, ht->NULL_item);
  } else {
    ht->st = realloc(ht->st, ht->M * sizeof(y3_hashItem));
    tmp_ht = y3_hash_new_table(old_M,
                   ht->NULL_item,
                   ht->hash_func,
                   ht->hash_less,
                   ht->hash_null,
                   ht->hash_eq,
                   ht->hash_setkey,
                   ht->hash_getkey,
                   NULL
    );
    //
    // We need to rehash the hashtable if we're expanding it, 
				// and set the empty cells to NULL_item
    //
    if (ht->st) {
      for (i = 0; i < old_M; i++) {
        if (ht->st[i].key != ht->NULL_item) {
          y3_hash_insert(tmp_ht, ht->st[i].key, &ht->st[i]);
        } else {
          y3_hash_setkey(&tmp_ht->st[i], tmp_ht->NULL_item, tmp_ht->NULL_item);
        }
      }

      for (i = 0; i < ht->M; i++)
        y3_hash_setkey(&ht->st[i], ht->NULL_item, ht->NULL_item);
      ht->N = 0;

      for (i = 0; i < tmp_ht->M; i++) {
        if (tmp_ht->st[i].key != tmp_ht->NULL_item) {
          y3_hash_insert(ht, tmp_ht->st[i].key, &tmp_ht->st[i]);
        }
      }
      //      y3_hash_debug_print_keys(ht);
    }
    y3_hash_release(tmp_ht);
  }

  ht->collisions = 0;
}

size_t
y3_hash_count(struct y3_hashtable *ht) {
  return ht->N;
}

y3_hashItem *
y3_hash_search(struct y3_hashtable *ht, void *key) {
  size_t cell = ht->hash_func(ht, key);

  while (!ht->hash_null(ht, cell)) {
    //    printf("searching for key %d: cell %d ht->key %d\n", key, cell, ht->st[cell].key);
    if (ht->hash_eq(ht, key, ht->hash_getkey(&ht->st[cell])))
      return &ht->st[cell];
    else
      cell = (cell + 1) % ht->M;
  }

  return ht->NULL_item;
}

void
y3_hash_delete(struct y3_hashtable *ht, void *key) {
  register size_t j, i = ht->hash_func(ht, key);
  y3_hashItem *v;

  while (!ht->hash_null(ht, i)) {
    if (ht->hash_eq(ht, key, ht->hash_getkey(&ht->st[i])))
      break;
    else
      i = (i + 1) % ht->M;
  }

  if (ht->hash_null(ht, i))
    return;

  ht->hash_setkey(&ht->st[i], ht->NULL_item, ht->NULL_item);

  ht->N--;

  //  y3_hash_debug_print_keys(ht);

  for (j = i + 1; ; j = (j + 1) % ht->M, ht->N--) {
    if (ht->hash_null(ht, j))
      break;
    v = &ht->st[j];
    y3_hash_insert(ht, v->key, v);
    ht->hash_setkey(&ht->st[j], ht->NULL_item, ht->NULL_item);
  }
}

void
y3_hash_debug_print_keys(struct y3_hashtable *ht) {
  int x = 0;

  for (x = 0; x < ht->M; x++) {
//    printf("index [%d] has key %d\n", x, ht->st[x].key);
  }
}

void
y3_hash_insert(struct y3_hashtable *ht, void *key, y3_hashItem *item) {
  if (key != ht->NULL_item) {
    register size_t i = ht->hash_func(ht, key);

    if (!ht->hash_null(ht, i)) {
      //      printf("COLLISION\n");
      ht->collisions++;
    }

    if (ht->N++ > ht->M / 2)
      y3_hash_expand(ht);

    while (!ht->hash_null(ht, i))
      i = (i + 1) % ht->M;

    //    if (ht->N++ > ht->M/2 && !ht->dne)

    ht->hash_setkey(&ht->st[i], key, item->value);
  }
}

void
y3_hash_expand(struct y3_hashtable *ht) {
  y3_hash_init(ht, ht->M, 1);
}

/* *********** IMPLEMENTATION *** END ******* */