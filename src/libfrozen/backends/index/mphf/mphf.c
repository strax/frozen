#define MPHF_C

#include <libfrozen.h>
#include <mphf.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_mphf index/mphf
 */
/**
 * @ingroup mod_backend_mphf
 * @page page_mphf_info Description
 *
 * Minimal Perfect Hash Function. This module can be used to create perfect hash tables for given set. As drawback -
 * table in most cases need to be rebuilded frequently. Can be used in enviroment critical to space and execution time.
 * Also, reads are more preferable than writes (actually, not write, but new item creation). 
 */
/**
 * @ingroup mod_backend_mphf
 * @page page_mphf_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "index/mphf",
 *              type                    = <see algo>,          # algorithm to use in index
 *              input                   = (hash_key_t)'key',    # input key for index
 *              output                  = (hash_key_t)'offset', # output key name for index
 *              <algo specific parameters>
 * }
 * @endcode
 *
 * Algorithms avaliable:
 * @li chm_imp - rewritten from scratch CHM algorithm described in 
 *               "An optimal algorithm for generating minimal perfect hash functions.", Z.J. Czech, G. Havas, and B.S. Majewski.
 *               http://cmph.sourceforge.net/papers/chm92.pdf
 */ 

#define EMODULE              10

typedef struct mphf_userdata {
	mphf_t               mphf;
	mphf_proto_t        *mphf_proto;
	uintmax_t            broken;

	hash_key_t           input;
	hash_key_t           output;

} mphf_userdata;

static mphf_proto_t *   mphf_string_to_proto(char *string){ // {{{
	if(string == NULL)                 return NULL;
	if(strcmp(string, "chm_imp") == 0) return &mphf_protos[MPHF_TYPE_CHM_IMP];
	if(strcmp(string, "bdz_imp") == 0) return &mphf_protos[MPHF_TYPE_BDZ_IMP];
	return NULL;
} // }}}
static ssize_t          mphf_configure_any(backend_t *backend, config_t *config, request_t *fork_req){ // {{{
	ssize_t          ret;
	uintmax_t        fork_only       = 0;
	char            *mphf_type_str   = NULL;
	mphf_userdata   *userdata        = (mphf_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT,  mphf_type_str,    config, HK(type));
	hash_data_copy(ret, TYPE_UINTT,    fork_only,        config, HK(fork_only));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,  config, HK(input));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->output, config, HK(output));
       
	if(fork_only == 1 && fork_req == NULL)
		return 0;

	if( (userdata->mphf_proto = mphf_string_to_proto(mphf_type_str)) == NULL)
		return error("backend mphf parameter mphf_type invalid or not supplied");
	
	memset(&userdata->mphf, 0, sizeof(userdata->mphf));
	
	userdata->mphf.config     = config;
	userdata->broken          = 0;

	if(fork_req == NULL){
		if( (ret = userdata->mphf_proto->func_load(&userdata->mphf)) < 0)
			return ret;
	}else{
		if( (ret = userdata->mphf_proto->func_fork(&userdata->mphf, fork_req)) < 0)
			return ret;
	}
	return 0;
} // }}}

static int mphf_init(backend_t *backend){ // {{{
	mphf_userdata *userdata = backend->userdata = calloc(1, sizeof(mphf_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->input  = HK(key);
	userdata->output = HK(offset);
	return 0;
} // }}}
static int mphf_destroy(backend_t *backend){ // {{{
	intmax_t       ret;
	mphf_userdata *userdata = (mphf_userdata *)backend->userdata;
	
	if(userdata->mphf_proto != NULL){
		if( (ret = userdata->mphf_proto->func_unload(&userdata->mphf)) < 0)
			return ret;
	}

	free(userdata);
	return 0;
} // }}}
static int mphf_configure(backend_t *backend, config_t *config){ // {{{
	return mphf_configure_any(backend, config, NULL);
} // }}}
static int mphf_fork(backend_t *backend, backend_t *parent, request_t *request){ // {{{
	return mphf_configure_any(backend, backend->config, request);
} // }}}

static ssize_t mphf_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t               ret;
	uint32_t              action;
	uintmax_t             d_input;
	uintmax_t            *d_output;
	data_t               *data_output;
	mphf_userdata        *userdata           = (mphf_userdata *)backend->userdata;

	hash_data_copy(ret, TYPE_UINT32T, action, request, HK(action));
	if(ret != 0)
		return -ENOSYS;
	
	// rebuilds
	if(action == ACTION_REBUILD){
		if( (ret = userdata->mphf_proto->func_rebuild(&userdata->mphf)) < 0)
			return ret;
		
		userdata->broken = 0;
		return -EBADF;
	}
	if(userdata->broken != 0)
		return -EBADF;
	//
	
	hash_data_copy(ret, TYPE_UINTT, d_input, request, userdata->input);
	if(ret != 0)
		return -EINVAL;
	
	data_output = hash_data_find(request, userdata->output);
	if(data_output == NULL)
		return -EINVAL;
	
	d_output = data_output->ptr;
	
	switch(action){
		case ACTION_CREATE:
			if( (ret = userdata->mphf_proto->func_insert(
				&userdata->mphf, 
				d_input,
				*d_output
			)) < 0){
				if(ret == -EBADF)
					userdata->broken = 1;
				return ret;
			}
			break;
		case ACTION_WRITE:
			if( (ret = userdata->mphf_proto->func_update(
				&userdata->mphf, 
				d_input,
				*d_output
			)) < 0){
				if(ret == -EBADF)
					userdata->broken = 1;
				return ret;
			}
			break;
		case ACTION_READ:
			switch( (ret = userdata->mphf_proto->func_query(
				&userdata->mphf,
				d_input,
				d_output
			))){
				case MPHF_QUERY_NOTFOUND: return -ENOENT;
				case MPHF_QUERY_FOUND:    break;
				default:                  return ret;
			};
			break;
		case ACTION_DELETE:
			if( (ret = userdata->mphf_proto->func_delete(
				&userdata->mphf, 
				d_input
			)) < 0){
				if(ret == -EBADF)
					userdata->broken = 1;
				return ret;
			}
			
			break;
	}
	return 0;
} // }}}

backend_t mphf_proto = {
	.class          = "index/mphf",
	.supported_api  = API_HASH,
	.func_init      = &mphf_init,
	.func_configure = &mphf_configure,
	.func_fork      = &mphf_fork,
	.func_destroy   = &mphf_destroy,
	.backend_type_hash = {
		.func_handler = &mphf_handler
	}
};

