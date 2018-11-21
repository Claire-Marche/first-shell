struct node {
    int value;
    struct node *next;
};

struct list {
    struct node *first;
};

/*initializes a linked list
struct list *l a pointer on the list to be initialized
*/
void list_create(struct list *l);

/*initialises a linked list node
struct node *ne a pointer on the node to be initialized
int          n  the integer to be written in the note
*/
void node_create(struct node *ne, int n);

/*adds a node to a linked list
struct list *l a pointer on the list where the node is to be added
int          n the value the new node is to carry
*/
void list_add(struct list *l, int n);

/*frees all components of a linked list
struct list *l the list whose components are to be destroyed
*/
void list_destroy(struct list *l);
