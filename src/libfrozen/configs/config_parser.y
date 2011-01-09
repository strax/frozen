%{
#include <libfrozen.h>
#include <configs/config.h>
#include <configs/config_parser.tab.h>	

void yyerror (hash_t **, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE config__scan_string (const char *string);  
extern int config_lex_destroy(void);
extern int config_lex(YYSTYPE *);

%}

%start  start

%define api.pure
%parse-param {hash_t **hash}

%union {
	hash_t   *hash_items;
	hash_t    hash_item;
	char     *name;
	data_t    data;
}
%token NAME STRING ASSIGN TNULL
%type  <hash_items>  hash_items
%type  <hash_item>   hash_item
%type  <name>        hash_name  NAME STRING
%type  <data>        hash_value

%%

start : hash_items { *hash = $1; }

hash_items :
	/* empty */ {
		$$ = malloc(sizeof(hash_t));
		hash_assign_hash_end($$);
	}
	| hash_item {
		$$ = malloc(2 * sizeof(hash_t));
		hash_assign_hash_t   (&$$[0], &$1);
		hash_assign_hash_end (&$$[1]);
	}
	| hash_items ',' hash_item {
		size_t nelements = hash_get_nelements($1);
		$1 = realloc($1, (nelements + 1) * sizeof(hash_t));
		memmove(&$1[1], &$1[0], nelements * sizeof(hash_t));
		hash_assign_hash_t(&$1[0], &$3);
		$$ = $1;
	}
	;

hash_item : hash_name hash_value {
	$$.key = $1;
	data_assign_data_t(&$$.data, &$2);
};

hash_name :
          /* empty */  { $$ = NULL  }
	| NAME  ASSIGN { $$ = $1    }
	| TNULL ASSIGN { $$ = NULL; };

hash_value :
	  STRING             { data_assign_raw(&$$, TYPE_STRING, $1, strlen($1) + 1);  }  // fucking macro nesting
	| '{' hash_items '}' { data_assign_raw(&$$, TYPE_HASHT,  $2, 1 /*allocated*/); }; // no DATA_PTR_HASHT_FREE here
	| '(' NAME ')' STRING {
		ssize_t  retval;
		data_t   d_str = DATA_PTR_STRING($4, strlen($4)+1);
		
		/* convert string to needed data */
		data_convert_to_alloc(retval, data_type_from_string($2), &$$, &d_str, NULL);
		if(retval != 0){
			yyerror(hash, "failed convert data\n"); YYERROR;
		}
		
		free($2);
		free($4);
	  };

%%

void yyerror(hash_t **hash, const char *msg){
	(void)hash;
	error("config error: %s\n", msg);
}

hash_t *   configs_string_parse(char *string){ // {{{
	hash_t *new_hash = NULL;
	
	config__scan_string(string);
	
	yyparse(&new_hash);
	
	config_lex_destroy();
	return new_hash;
} // }}}

hash_t *   configs_file_parse(char *filename){ // {{{
	int     size = 0;
	hash_t *new_hash = NULL;
	char   *string;
	FILE   *f;
	
	if( (f = fopen(filename, "rb")) == NULL)
		return NULL;
	
	fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
	
	if( (string = malloc(size+1)) != NULL){
		if(fread(string, sizeof(char), size, f) == size){
			string[size] = '\0';
			new_hash = configs_string_parse(string);
		} 
		free(string);
	}
	fclose(f);
	return new_hash;
} // }}}

