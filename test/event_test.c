#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define EVENT_HEADER_TYPE "_t"
#define FAST_HEADER2 "header2"
#define FAST_HEADER3 "header3"
#define FAST_HEADER4 "header4"

#define FAST_BODY "jdowcdfdlfjdlfd"

#define EVENT_FAST_HEADERINDEX1 0
#define EVENT_FAST_HEADERINDEX2 1
#define EVENT_FAST_HEADERINDEX3 2
#define EVENT_FAST_HEADERINDEX4 3

struct agc_event_header {
    /*! the header name */
    char *name;
    /*! the header value */
    char *value;

    struct agc_event_header *next;
};

typedef struct agc_event_header agc_event_header_t;

struct agc_event {
    /*! the event id (descriptor) */
    int event_id;
        
    /*! the source of event, the same soure will be handled by same thread */
    uint32_t source_id;

    int debug_id;
    
    /*! the event headers */
    agc_event_header_t *headers; 
    
    agc_event_header_t *last_header;
    
     /*! the context of event */
    void *context;

    /*! the fast thing*/
    void *fast;
    
    /*! the body of the event */
    char *body;
 
};

typedef struct agc_event agc_event_t;

struct fast_event_node {
    agc_event_header_t **headers;
    int type;
    int header_numbers;
};

typedef struct fast_event_node fast_event_node_t;

const char *headers[4] = {EVENT_HEADER_TYPE, FAST_HEADER2, FAST_HEADER3, FAST_HEADER4};
const int valuelengths[4] = {8, 16, 32, 32};


static int agc_event_fast_set_strheader(agc_event_t *event, int index, const char *name, const char *value);
static int agc_event_fast_set_intheader(agc_event_t *event, int index, const char *name, int value);
static int agc_event_fast_set_uintheader(agc_event_t *event, int index, const char *name, uint32_t value);
static int agc_event_fast_set_body(agc_event_t *event, const char *body, int len);

static int agc_event_fast_get_type(agc_event_t *event, int *type);
agc_event_header_t *agc_event_get_header(agc_event_t *event, const char *header_name);

static inline void fast_add_header(agc_event_t *event, agc_event_header_t *header);
static inline agc_event_header_t *fast_get_header(agc_event_t *event, int index);
static int fast_event_create(agc_event_t **event, int fast_event_type, int event_id, const char **headers, 
                                const int *valuelengths, int headernumbers, int bodylength);

int main(int argc, char **argv)
{
	agc_event_t *new_event = NULL;
	agc_event_header_t *new_header = NULL;
	int i = 0;
	int e_type = 0;
	
	
	fast_event_create(&new_event, 1, 0, headers, valuelengths, 4, 64);
	
	agc_event_fast_set_strheader(new_event, EVENT_FAST_HEADERINDEX2, FAST_HEADER2, "strvalue");
	agc_event_fast_set_intheader(new_event, EVENT_FAST_HEADERINDEX3, FAST_HEADER3, 20);
	agc_event_fast_set_uintheader(new_event, EVENT_FAST_HEADERINDEX4, FAST_HEADER4, 200);
	
	agc_event_fast_set_body(new_event, FAST_BODY, (strlen(FAST_BODY) + 1));
	
	for (i = 0; i < 4; i++) {
		new_header = fast_get_header(new_event, i);
		printf("header: %d name: %s value: %s\n", i, new_header->name, new_header->value);
	}
	
	new_header = agc_event_get_header(new_event, FAST_HEADER2);
	if (new_header) {
		printf("old get header name: %s value: %s\n", new_header->name, new_header->value);
	}
	
	agc_event_fast_get_type(new_event, &e_type);
	printf("get header type: %d\n", e_type);
	
	printf("get body : %s\n", new_event->body);
	
}

static int fast_event_create(agc_event_t **event, int fast_event_type, int event_id, const char **headers, 
                                const int *valuelengths, int headernumbers, int bodylength)
{
	agc_event_t *new_event = NULL;
	int payload_size = 0;
	int i = 0;
	char *buff = NULL;
	fast_event_node_t *fast = NULL;
	agc_event_header_t *new_header = NULL;

	new_event= malloc(sizeof(agc_event_t));
	memset(new_event, 0, sizeof(agc_event_t));
	assert(new_event);
	new_event->event_id = event_id;
	
	printf("event \n");

	/* payload
	*  fast_event_node_t
	*  agc_event_header_t* * headernumbers
	*  agc_event_header_t * headernumbers
	*  name->value name->value
	*  body
	*/
	payload_size += sizeof(fast_event_node_t);
	payload_size += headernumbers * sizeof(agc_event_header_t *);
	payload_size += headernumbers * sizeof(agc_event_header_t);
	
	for (i = 0; i < headernumbers; i++) {
		payload_size += (strlen(headers[i]) + 1);
		payload_size += valuelengths[i];
	}

	payload_size += bodylength;

	buff = malloc(payload_size);
	assert(buff);
	memset(buff, 0, payload_size);
	
	printf("fillter \n");
	fast = (fast_event_node_t *)buff;
	buff += sizeof(fast_event_node_t);

	//fill fast
	new_event->fast = fast;
	fast->type = fast_event_type;
	fast->header_numbers = headernumbers;
	fast->headers = (agc_event_header_t **)buff;
	buff += headernumbers * sizeof(agc_event_header_t *);

	//fill headers
	printf("fill headers \n");
	for (i = 0; i < headernumbers; i++) {
		fast->headers[i] = (agc_event_header_t *)buff;
		fast_add_header(new_event, fast->headers[i]);
		buff += sizeof(agc_event_header_t);
	}

	printf("fill values \n");
	for (i = 0; i < headernumbers; i++) {
		new_header = fast->headers[i];
		new_header->name = buff;
		buff += (strlen(headers[i]) + 1);
		new_header->value = buff;
		buff += valuelengths[i];
	}

	printf("fill body \n");
	if (bodylength > 0) {
		new_event->body = buff;
	}

	if (headernumbers > 0) {
		new_header = fast->headers[EVENT_FAST_HEADERINDEX1];
		strcpy(new_header->name, EVENT_HEADER_TYPE);
		sprintf(new_header->value, "%d", fast_event_type);
	}

	*event = new_event;
	
	return 0;
}

static inline void fast_add_header(agc_event_t *event, agc_event_header_t *header)
{
	if (event->last_header) {
		event->last_header->next = header;
	} else {
		event->headers = header;
	}

	event->last_header = header;
}

static inline agc_event_header_t *fast_get_header(agc_event_t *event, int index)
{
	fast_event_node_t *fast;
	agc_event_header_t *header;
	
	if (!event || !event->fast) {
		return NULL;
	}

	fast = event->fast;
	if (index >= fast->header_numbers) {
		printf("invalid index %d\n", index);
		return NULL;
	}

	header = fast->headers[index];

	if (!header) {
		printf("no header index %d\n", index);
		return NULL;
	}

	return header;
}

static int agc_event_fast_set_strheader(agc_event_t *event, int index, const char *name, const char *value)
{
	agc_event_header_t *header = NULL;
	
	header = fast_get_header(event, index);
	if (!header) {
		return 1;
	}
	
	strcpy(header->name, name);
	strcpy(header->value, value);
	return 0;
}

static int agc_event_fast_set_intheader(agc_event_t *event, int index, const char *name, int value)
{
	agc_event_header_t *header = NULL;

	header = fast_get_header(event, index);

	if (!header) {
		return 1;
	}
	
	strcpy(header->name, name);
	sprintf(header->value, "%d", value);
	return 0;
}

static int agc_event_fast_set_uintheader(agc_event_t *event, int index, const char *name, uint32_t value)
{
	agc_event_header_t *header = NULL;

	header = fast_get_header(event, index);

	if (!header) {
		return 1;
	}
	
	strcpy(header->name, name);
	sprintf(header->value, "%u", value);
	return 0;
}

agc_event_header_t *agc_event_get_header(agc_event_t *event, const char *header_name)
{
	agc_event_header_t *hp = NULL;
	
	if (!header_name)
		return NULL;
    
	for (hp = event->headers; hp; hp = hp->next) {
		printf("search headername %s\n", hp->name);
		if (!strcasecmp(hp->name, header_name)) {
			return hp;
		}
	}	
	
	return NULL;
}

static int agc_event_fast_get_type(agc_event_t *event, int *type)
{
	agc_event_header_t *header = NULL;

	header = fast_get_header(event, EVENT_FAST_HEADERINDEX1);
	if (!header) {
		return 1;
	}

	*type = atoi(header->value);
	return 0;
}

static int agc_event_fast_set_body(agc_event_t *event, const char *body, int len)
{
	if (!event || !event->body) {
		return 1;
	}

	memcpy(event->body, body, len);
	return 0;
}