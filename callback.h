// Simple C callback list
// JCE, 9-11-2020


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct callback_list;

// Add a callback to the list
// ** cbl = pointer to the pointer to the first element. Double pointer is needed to write the first element.
// void (*func)(void*) = function to call back. It takes one void pointer parameter, and returns nothing.
// void* par = void pointer parameter given to the callback function on execution.
void cb_add(callback_list **cbl, void (*func)(void*), void *par);

// Free the whole callback list from memory
void cb_free(callback_list **cbl);

// Call all callbacks in the list
void cb_call(callback_list *cbl);

#ifdef __cplusplus
}
#endif // __cplusplus

