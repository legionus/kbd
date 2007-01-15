typedef struct {
	char **table;
	int size;
} syms_entry;

extern syms_entry syms[];

struct syn {
	char *synonym;
	char *official_name;
};
extern struct syn synonyms[];

extern const int syms_size;
extern const int syn_size;

extern int set_charset(char *name);
