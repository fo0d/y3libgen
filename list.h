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

/*
* Dynamic hash-indexed doubly (bi-directional) linked list stuff...
*/

struct y3_list {
    void *data;
    struct y3_list *next;
    struct y3_list *prev;
    void *null_key;
    int id;
    int flags;
    struct y3_hashtable *links;   // hashtable with: keys = node_key(specified by user), value = node ptr
};

//
// This is the number of nodes by which the hashtable
// (that stores their addresses) will double.
//

#define Y3_LIST_INITIAL_NODE_COUNT 16384
#define Y3_LIST_NEW 1
#define Y3_LIST_CREATE_HASHTABLE_DO_NOT_INSERT 3
#define Y3_LIST_CONTINUE 0
#define Y3_LIST_LEAVE_HASH 1
#define Y3_LIST_DROP_HASH 0
#define Y3_LIST_DONT_HASH 7

void y3_list_clear_links_custom_hs(struct y3_list *list, struct y3_hashtable *h);
struct y3_list * y3_list_create_custom_hs(void *data, void *node_key, void *null_key, size_t table_size, int init, struct y3_hashtable *h);
struct y3_list *y3_list_create(void *data, void *node_key, void *null_key, size_t table_size, int init);
struct y3_list *y3_list_insert_after(struct y3_list *node, void *data, int id, int flags, int init, void *node_key);
struct y3_list *y3_list_insert_beginning(struct y3_list *list, void *data, int id, int flags, int init, void *node_key);
struct y3_list *y3_list_insert_last(struct y3_list *node, void *data, int id, int flags, int init, void *node_key);
struct y3_list *y3_list_insert(struct y3_list *head_of_list, void *data, int id, int flags, int where, void *node_key);
struct y3_list *y3_list_remove(struct y3_list *list, struct y3_list *node, void(*func)(void*), void *key, int leave_hash);
struct y3_list *y3_list_link_id(struct y3_list *list, void *id);
struct y3_list *y3_list_link_flags(struct y3_list *node, int flags);
int    y3_list_apply(struct y3_list *node, int(*func)(void*));
struct y3_list *y3_list_find(struct y3_list *node, struct y3_list *(*func)(void*,void*), void *data);
int    y3_list_count(struct y3_list *list);
void   y3_list_freeall(struct y3_list *txt, void(*func)(void*));
void   y3_list_copynode(struct y3_list *dst, struct y3_list *src, void(*func)(void*));

//
// Hashtable related stuff
//

void   y3_list_clear_links(struct y3_list *list);
int y3_list_hash_int(struct y3_hashtable *ht, void *_v);
int y3_list_hash_less(struct y3_hashtable *ht, void *a_value, void *b_value);
struct y3_hash_table_data *y3_list_hash_new(struct y3_list *node);