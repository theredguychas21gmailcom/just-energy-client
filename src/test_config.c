#include <stdio.h>
#include "utils/utils.h"

int main(void) {
    json_error_t error;
    json_t *root = json_load_file("config.json", 0, &error);

    if (!root) {
        printf("Failed to load JSON: %s\n", error.text);
        return 1;
    }

    char query[128];
    if (json_to_query_string(root, query, sizeof(query)) == 0) {
        printf("Query string: %s\n", query);
    } else {
        printf("Parsing failed\n");
    }

    json_decref(root);
    return 0;
}
