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
#include <stdio.h>
#include "list.h"
#include "hash.h"

y3_hashItem *
y3_list_hash_new(struct y3_list *node)
{
	y3_hashItem *item;

	item = (y3_hashItem *)malloc(sizeof(y3_hashItem));

	item->value = (struct y3_list *) node;

	return item;
}

int
y3_list_count(struct y3_list *list)
{
	register int i;

	for (i = 0; list; list = list->next)
		i++;
	return i;
}

void
y3_list_clear_links_custom_hs(struct y3_list *list, struct y3_hashtable *h)
{
	if (list) {
		y3_hash_release(list->links);

		list->links = h;
	}
}

struct y3_list *
y3_list_create_custom_hs(void *data, void *node_key, void *null_key, size_t table_size, int init, struct y3_hashtable *h)
{
	struct y3_list *node;
	if (!data || !(node = malloc(sizeof(struct y3_list))) || table_size <= 0)
		return NULL;

	node->data = data;
	node->next = NULL;
	node->prev = NULL;
	node->null_key = null_key;
	node->id = 0;
	node->flags = 0;

	if (init == Y3_LIST_CREATE_HASHTABLE_DO_NOT_INSERT)
		node->links = h;
	else if (init) {
		node->links = h;
		y3_hash_insert(node->links, node_key, y3_list_hash_new(node));
	}
	return node;
}

void
y3_list_clear_links(struct y3_list *list)
{
	if (list) {
		size_t table_size = list->links->M;
		void *null_key = list->null_key;

		y3_hash_release(list->links);

		list->links = y3_hash_new_table(
			table_size,
			null_key,
			y3_hash_int1,
			y3_hash_int1_less,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
			);
	}
}

struct y3_list *
y3_list_create(void *data, void *node_key, void *null_key, size_t table_size, int init)
{
	struct y3_list *node;
	if (!data || !(node = malloc(sizeof(struct y3_list))) || table_size <= 0)
		return NULL;

	node->data = data;
	node->next = NULL;
	node->prev = NULL;
	node->null_key = null_key;
	node->id = 0;
	node->flags = 0;

	if (init) {
		node->links = y3_hash_new_table(
			table_size,
			null_key,
			y3_hash_int1,
			y3_hash_int1_less,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
			);

		if (init != Y3_LIST_CREATE_HASHTABLE_DO_NOT_INSERT)
			y3_hash_insert(node->links, node_key, y3_list_hash_new(node));
	}
	return node;
}

/*
 * Wrapper function for the y3_list_insert_xxx() group of functions.
 *
 * where values:
 *               0 -> insert at beginning
 *               1 -> insert after
 *               2 -> insert at end (last) (DEFAULT action)
 *
 * The reason for it is it save writing if(!list) ... else ...
 *
 * The head of list is always returned, unlike the other functions
 * which return the newly inserted list, which has its own advantages.
 *
 * If the head of list is NULL, meaning the list is being created, then
 * the newly created node is returned, which is the head of list now.
 *
 * NOTE(s):
 *           if where != 0 or 1, it is taken as 2.
 */

struct y3_list *
y3_list_insert(struct y3_list *head_of_list, void *data, int id, int flags, int where, void *node_key)
{
	struct y3_list *newnode;
	int init = 0;

	if (!head_of_list)
		init++;

	if (where == 0)
		newnode = y3_list_insert_beginning(head_of_list, data, id, flags, init, node_key);
	else if (where == 1)
		newnode = y3_list_insert_after(head_of_list, data, id, flags, init, node_key);
	else
		newnode = y3_list_insert_last(head_of_list, data, id, flags, init, node_key);

	if (head_of_list == NULL)
		head_of_list = newnode;

	return head_of_list;
}

struct y3_list *
y3_list_insert_after(struct y3_list *node, void *data, int id, int flags, int init, void *node_key)
{
	struct y3_list *newnode;

	if (node == NULL)
		return y3_list_insert_beginning(node, data, id, flags, init, node_key);
	newnode = y3_list_create(data, node_key, node->null_key, node->links->M, init);
	newnode->next = node->next;
	if (node->next)
		node->next->prev = newnode;
	node->next = newnode;
	newnode->prev = node;
	newnode->id = id;
	newnode->flags = flags;
	//
	// We need to make sure the Hashtable isn't full.
	// Otherwise we need to expand it.
	//

	if (node->links->M == node->links->N) {
		y3_hash_expand(node->links);
	}

	newnode->links = node->links;

	if (y3_hash_search(node->links, node_key) != node->null_key) {
		//
		// implement and raise exception here...
		//
		printf("The key already exists in this list's hashtable!\n");
		return NULL;
	}

	y3_hash_insert(newnode->links, node_key, y3_list_hash_new(newnode));
	return newnode;
}

struct y3_list *
y3_list_insert_last(struct y3_list *node, void *data, int id, int flags, int init, void *node_key)
{
	struct y3_list *newnode;

	if (node == NULL)
		return y3_list_insert_beginning(node, data, id, flags, init, node_key);

	while (node->next)
		node = node->next;

	newnode = y3_list_create(data, node_key, node->null_key, node->links->M, init);
	newnode->next = node->next;
	node->next = newnode;
	newnode->prev = node;
	newnode->id = id;
	newnode->flags = flags;
	//
	// We need to make sure the Hashtable isn't full.
	// Otherwise we need to expand it.
	//
	if (node->links->M == node->links->N) {
		y3_hash_expand(node->links);
	}

	newnode->links = node->links;

	if (y3_hash_search(node->links, node_key) != node->null_key) {
		//
		// implement and raise exception here...
		//
		printf("The key already exists in this lists hashtable!\n");
		return NULL;
	}
	y3_hash_insert(newnode->links, node_key, y3_list_hash_new(newnode));
	return newnode;
}

struct y3_list *
y3_list_insert_beginning(struct y3_list *list, void *data, int id, int flags, int init, void *node_key)
{
	struct y3_list *newnode;

	newnode = y3_list_create(data, node_key, list->null_key, list->links->M, init);
	newnode->links = list->links;
	newnode->next = list;
	newnode->prev = NULL;
	list->prev = newnode;
	newnode->id = id;
	newnode->flags |= flags;

	//
	// We need to make sure the Hashtable isn't full.
	// Otherwise we need to expand it.
	//
	if (list->links->M == list->links->N) {
		y3_hash_expand(list->links);
	}

	newnode->links = list->links;

	if (y3_hash_search(list->links, node_key) != list->null_key) {
		//
		// implement and raise exception here...
		//
		printf("The key already exists in this lists hashtable!\n");
		return NULL;
	}

	y3_hash_insert(newnode->links, node_key, y3_list_hash_new(newnode));
	return newnode;
}

struct y3_list *
y3_list_remove(struct y3_list *list, struct y3_list *node, void(*func)(void*), void *key, int leave_hash)
{
	struct y3_list *head = list;
	struct y3_list *temp;

	/*
	 * This will remove head node...
	 */
	if (head == node) {
		temp = head;
		head = head->next;
		head->prev = NULL;
		if (!leave_hash)
			y3_hash_delete(temp->links, key);
		func(temp->data);
		free(temp);
		return head;
	}
	/*
	 * This will remove tail node...
	 */
	if (!node->next) {
		temp = node;
		node = node->prev;
		node->next = NULL;
		if (!leave_hash)
			y3_hash_delete(temp->links, key);
		func(temp->data);
		free(temp);
		return head;
	}

	/*
	 * This will remove node between head and tail...
	 */

	node->next->prev = node->prev;
	node->prev->next = node->next;
	if (!leave_hash)
		y3_hash_delete(node->links, key);
	func(node->data);
	free(node);
	return head;
}

struct y3_list *
y3_list_link_id(struct y3_list *list, void *id)
{
	y3_hashItem *item;

	item = y3_hash_search(list->links, id);

	if (item != list->null_key)
		return item->value;

	return NULL;
}

struct y3_list *
y3_list_link_flags(struct y3_list *node, int flags)
{
	while (node) {
		if (node->flags & flags)
			return node;
		node = node->next;
	}
	return NULL;
}

int
y3_list_apply(struct y3_list *node, int(*func)(void*))
{
	while (node) {
		if (func(node->data) != 0) // function failed
			return -1;
		node = node->next;
	}
	return 0;
}

/*
 * Copy src node to dst node using func() provided by user
 * to copy the data.
 */

void
y3_list_copynode(struct y3_list *dst, struct y3_list *src, void(*func)(void*))
{
}

/*
 * The data needs to be freed by the function
 * specified by the user.
 * 'func' is the function that does the freeing of
 * 'node->data' bits.
 */

void
y3_list_freeall(struct y3_list *txt, void(*func)(void*))
{
	struct y3_list *p, *tmp;

	//
	// Free the hashtable.
	//
	y3_hash_release(txt->links);

	//
	// Free the list.
	//
	for (p = txt; p; p = tmp) {
		tmp = p->next;
		func(p->data);
		p->data = NULL;
		free(p);
	}
}

struct y3_list *
y3_list_find(struct y3_list *node, struct y3_list *(*func)(void*, void*), void *data)
{
	return func(node, data);
}