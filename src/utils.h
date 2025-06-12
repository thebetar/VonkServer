#include <stdbool.h>

char *get_current_ip();

int start_server();

int handle_humidity(int humidity);
int handle_temperature(int temperature);
int tapo_toggle(char *type, bool turn_on);