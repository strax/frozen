#include <libfrozen.h>
#define EMODULE 6

typedef struct incap_userdata {
	hash_key_t     key;
	hash_key_t     key_out;
	hash_key_t     key_to;
	hash_key_t     key_from;
	hash_key_t     count;
	hash_key_t     size;
	DT_SIZET       multiply_as_sizet;
	DT_OFFT        multiply_as_offt;
	data_t         multiply_as_sizet_data;
	data_t         multiply_as_offt_data;
} incap_userdata;

static int incap_init(chain_t *chain){ // {{{
	if((chain->userdata = calloc(1, sizeof(incap_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int incap_destroy(chain_t *chain){ // {{{
	incap_userdata *userdata = (incap_userdata *)chain->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int incap_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t          ret;
	DT_STRING        key_str       = "offset";
	DT_STRING        key_out_str   = "offset_out";
	DT_STRING        key_to_str    = "offset_to";
	DT_STRING        key_from_str  = "offset_from";
	DT_STRING        count_str     = "buffer";
	DT_STRING        size_str      = "size";
	DT_OFFT          multiply      = 1;
	incap_userdata  *userdata      = (incap_userdata *)chain->userdata;
	
	hash_data_copy(ret, TYPE_STRING, key_str,       config, HK(key));
	hash_data_copy(ret, TYPE_STRING, key_out_str,   config, HK(key_out));
	hash_data_copy(ret, TYPE_STRING, key_to_str,    config, HK(key_to));
	hash_data_copy(ret, TYPE_STRING, key_from_str,  config, HK(key_from));
	hash_data_copy(ret, TYPE_STRING, count_str,     config, HK(count));
	hash_data_copy(ret, TYPE_STRING, size_str,      config, HK(size));
	hash_data_copy(ret, TYPE_OFFT,   multiply,      config, HK(multiply));
	
	data_t multiply_as_offt_data = DATA_PTR_OFFT(&userdata->multiply_as_offt);
	memcpy(&userdata->multiply_as_offt_data, &multiply_as_offt_data, sizeof(multiply_as_offt_data));
	data_t multiply_as_sizet_data = DATA_PTR_SIZET(&userdata->multiply_as_sizet);
	memcpy(&userdata->multiply_as_sizet_data, &multiply_as_sizet_data, sizeof(multiply_as_sizet_data));
	
	userdata->key      = hash_string_to_key(key_str);
	userdata->key_out  = hash_string_to_key(key_out_str);
	userdata->key_to   = hash_string_to_key(key_to_str);
	userdata->key_from = hash_string_to_key(key_from_str);
	userdata->count    = hash_string_to_key(count_str);
	userdata->size     = hash_string_to_key(size_str);
	userdata->multiply_as_sizet = multiply;
	userdata->multiply_as_offt  = multiply;
	
	if(multiply == 0)
		return error("chain incapsulate parameter multiply invalid");
	
	return 0;
} // }}}

static ssize_t incap_backend_createwrite(chain_t *chain, request_t *request){
	ssize_t          ret;
	size_t           size, new_size;
	data_t          *offset;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	hash_data_find(request, userdata->size, &offset, &offset_ctx);
	if(offset != NULL){
		// round size
		size     = GET_TYPE_SIZET(offset);
		new_size =     size / userdata->multiply_as_sizet;
		new_size = new_size * userdata->multiply_as_sizet;
		if( (size % userdata->multiply_as_sizet) != 0)
			new_size += userdata->multiply_as_sizet;
		
		data_write(offset, offset_ctx, 0, &new_size, sizeof(new_size));
	}
	
	hash_data_find(request, userdata->key, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('*', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	ret = chain_next_query(chain, request);
	
	hash_data_find(request, userdata->key_out, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('/', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	return ret;
}

static ssize_t incap_backend_read(chain_t *chain, request_t *request){
	data_t          *offset;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	hash_data_find(request, userdata->key, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('*', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	return chain_next_query(chain, request);
}

static ssize_t incap_backend_move(chain_t *chain, request_t *request){
	data_t          *offset;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	hash_data_find(request, userdata->key_to, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('*', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	hash_data_find(request, userdata->key_from, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('*', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	return chain_next_query(chain, request);
}

static ssize_t incap_backend_count(chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *offset;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	ret = chain_next_query(chain, request);
	
	hash_data_find(request, userdata->count, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('/', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	return ret;
}

static ssize_t incap_backend_custom(chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *offset;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	hash_data_find(request, userdata->key, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('*', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	ret = chain_next_query(chain, request);
	
	hash_data_find(request, userdata->key_out, &offset, &offset_ctx);
	if(offset != NULL)
		data_arithmetic('/', offset, offset_ctx, &userdata->multiply_as_offt_data, NULL);
	
	return ret;
	
}

static chain_t chain_incap = {
	"incapsulate",
	CHAIN_TYPE_CRWD,
	.func_init      = &incap_init,
	.func_configure = &incap_configure,
	.func_destroy   = &incap_destroy,
	{{
		.func_create = &incap_backend_createwrite,
		.func_set    = &incap_backend_createwrite,
		.func_get    = &incap_backend_read,
		.func_delete = &incap_backend_read,
		.func_move   = &incap_backend_move,
		.func_count  = &incap_backend_count,
		.func_custom = &incap_backend_custom
	}}
};
CHAIN_REGISTER(chain_incap)

