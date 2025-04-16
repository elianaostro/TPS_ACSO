#include "ej1.h"

// Inicializa una estructura de lista
string_proc_list* string_proc_list_create(void){
	string_proc_list* list = malloc(sizeof(string_proc_list));
	if (!list) return NULL;
	
	list->first = NULL;
	list->last  = NULL;
	return list;
}


// Inicializa un nodo con el tipo y el hash dado.
// El nodo tiene que apuntar al hash pasado por parámetro (no hay que copiarlo).
string_proc_node* string_proc_node_create(uint8_t type, char* hash){
	string_proc_node* node = malloc(sizeof(string_proc_node));
	if (!node) return NULL;
	
	node->next      = NULL;
	node->previous  = NULL;
	node->hash      = hash;
	node->type      = type;			
	return node;
}

// Agrega un nodo nuevo al final de la lista con el tipo y el hash dado.
// Recordar que no se debe copiar el hash, sino apuntar al mismo.
void string_proc_list_add_node(string_proc_list* list, uint8_t type, char* hash){
	string_proc_node* node = string_proc_node_create(type, hash);
	if (!node) return;
	
	if (list->first == NULL) {
		list->first = node;
	} else {
		list->last->next = node;
		node->previous   = list->last;
	}
	list->last = node;
}

// Genera un nuevo hash concatenando el pasado por parámetro con todos los hashes
// de los nodos de la lista cuyos tipos coinciden con el pasado por parámetro
char* string_proc_list_concat(string_proc_list* list, uint8_t type , char* hash){
	string_proc_node* current_node = list->first;
	char* result = malloc(1);
	result[0] = '\0';
	
	while(current_node != NULL){
		if (current_node->type == type) {
			char* new_result = str_concat(result, current_node->hash);
			free(result);
			result = new_result;
		}
		current_node = current_node->next;
	}
	
	char* final_result = str_concat(hash, result);
	free(result);
	return final_result;
}


/** AUX FUNCTIONS **/

void string_proc_list_destroy(string_proc_list* list){

	/* borro los nodos: */
	string_proc_node* current_node	= list->first;
	string_proc_node* next_node		= NULL;
	while(current_node != NULL){
		next_node = current_node->next;
		string_proc_node_destroy(current_node);
		current_node	= next_node;
	}
	/*borro la lista:*/
	list->first = NULL;
	list->last  = NULL;
	free(list);
}
void string_proc_node_destroy(string_proc_node* node){
	node->next      = NULL;
	node->previous	= NULL;
	node->hash		= NULL;
	node->type      = 0;			
	free(node);
}


char* str_concat(char* a, char* b) {
	int len1 = strlen(a);
    int len2 = strlen(b);
	int totalLength = len1 + len2;
    char *result = (char *)malloc(totalLength + 1); 
    strcpy(result, a);
    strcat(result, b);
    return result;  
}

void string_proc_list_print(string_proc_list* list, FILE* file){
        uint32_t length = 0;
        string_proc_node* current_node  = list->first;
        while(current_node != NULL){
                length++;
                current_node = current_node->next;
        }
        fprintf( file, "List length: %d\n", length );
		current_node    = list->first;
        while(current_node != NULL){
                fprintf(file, "\tnode hash: %s | type: %d\n", current_node->hash, current_node->type);
                current_node = current_node->next;
        }
}

