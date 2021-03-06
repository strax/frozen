#include <libfrozen.h>
#include <dataproto.h>

#include <string/string_t.h>
#include <format/format_t.h>

static ssize_t       data_default_read          (data_t *data, fastcall_read *fargs){ // {{{
	fastcall_physicallen r_len = { { 3, ACTION_LOGICALLEN } };
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_len) != 0 || data_query(data, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EFAULT;
	
	if(r_len.length == 0)
		return -1; // EOF
	
	if(fargs->buffer == NULL || fargs->offset > r_len.length)
		return -EINVAL; // invalid range
	
	fargs->buffer_size = MIN(fargs->buffer_size, r_len.length - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(fargs->buffer, r_ptr.ptr + fargs->offset, fargs->buffer_size);
	return 0;
} // }}}
static ssize_t       data_default_write         (data_t *data, fastcall_write *fargs){ // {{{
	fastcall_physicallen r_len = { { 3, ACTION_LOGICALLEN } };
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_len) != 0 || data_query(data, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EFAULT;
	
	if(r_len.length == 0)
		return -1; // EOF
	
	if(fargs->buffer == NULL || fargs->offset > r_len.length)
		return -EINVAL; // invalid range
	
	fargs->buffer_size = MIN(fargs->buffer_size, r_len.length - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(r_ptr.ptr + fargs->offset, fargs->buffer, fargs->buffer_size);
	return 0;
} // }}}
static ssize_t       data_default_copy          (data_t *src, fastcall_copy *fargs){ // {{{
	fastcall_getdataptr r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(src, &r_ptr) != 0)
		return -EFAULT;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	if(r_ptr.ptr != NULL){
		fastcall_physicallen r_len = { { 3, ACTION_PHYSICALLEN } };
		if( data_query(src, &r_len) != 0)
			return -EFAULT;
		
		if( (fargs->dest->ptr = malloc(r_len.length)) == NULL)
			return -EFAULT;
		
		memcpy(fargs->dest->ptr, r_ptr.ptr, r_len.length);
	}else{
		fargs->dest->ptr = NULL;
	}
	
	fargs->dest->type = src->type;
	return 0;
} // }}}
static ssize_t       data_default_compare       (data_t *data1, fastcall_compare *fargs){ // {{{
	ssize_t                ret;
	char                   buffer1[DEF_BUFFER_SIZE], buffer2[DEF_BUFFER_SIZE];
	uintmax_t              buffer1_size, buffer2_size, cmp_size;
	uintmax_t              offset1, offset2, goffset1, goffset2;
	
	if(fargs->data2 == NULL)
		return -EINVAL;
	
	goffset1     = goffset2     = 0;
	buffer1_size = buffer2_size = 0;
	do {
		if(buffer1_size == 0){
			fastcall_read r_read = { { 5, ACTION_READ }, goffset1, &buffer1, sizeof(buffer1) };
			if( (ret = data_query(data1, &r_read)) < -1)
				return ret;
			
			if(ret == -1 && buffer2_size != 0){
				fargs->result = 1;
				break;
			}
			
			buffer1_size  = r_read.buffer_size;
			goffset1     += r_read.buffer_size;
			offset1       = 0;
		}
		if(buffer2_size == 0){
			fastcall_read r_read = { { 5, ACTION_READ }, goffset2, &buffer2, sizeof(buffer2) };
			if( (ret = data_query(fargs->data2, &r_read)) < -1)
				return ret;
			
			if(ret == -1 && buffer1_size != 0){
				fargs->result = 2;
				break;
			}
			if(ret == -1){
				fargs->result = 0;
				break;
			}
			
			buffer2_size  = r_read.buffer_size;
			goffset2     += r_read.buffer_size;
			offset2       = 0;
		}
		
		cmp_size = MIN(buffer1_size, buffer2_size);
		
		if( (ret = memcmp(buffer1 + offset1, buffer2 + offset2, cmp_size)) != 0){
			fargs->result = (ret < 0) ? 1 : 2;
			break;
		}
		
		offset1      += cmp_size;
		offset2      += cmp_size;
		buffer1_size -= cmp_size;
		buffer2_size -= cmp_size;
	}while(1);
	return 0;
} // }}}
static ssize_t       data_default_free          (data_t *data, fastcall_free *fargs){ // {{{
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EFAULT;
	
	if(r_ptr.ptr != NULL)
		free(r_ptr.ptr);
	return 0;
} // }}}
static ssize_t       data_default_getdataptr    (data_t *data, fastcall_getdataptr *fargs){ // {{{
	fargs->ptr = data->ptr;
	return 0;
} // }}}
static ssize_t       data_default_is_null       (data_t *data, fastcall_is_null *fargs){ // {{{
	fargs->is_null = (data->ptr == NULL) ? 1 : 0;
	return 0;
} // }}}
static ssize_t       data_default_init          (data_t *dst, fastcall_init *fargs){ // {{{
	data_t                 d_initstr         = DATA_STRING(fargs->string);
	
	fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(human) };
	return data_query(dst, &r_convert);
} // }}}
static ssize_t       data_default_transfer      (data_t *src, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;
	
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, fargs->dest, FORMAT(clean) };
	if( (ret = data_query(src, &r_convert)) != 0)
		return ret;
	
	if(fargs->header.nargs >= 4)
		fargs->transfered = r_convert.transfered;
	
	return 0;
} // }}}
static ssize_t       data_default_convert_to    (data_t *src, fastcall_convert_to *fargs){ // {{{
	char            buffer[DEF_BUFFER_SIZE];
	ssize_t         rret, wret;
	uintmax_t       roffset, woffset, transfered;
	uintmax_t       read;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	roffset = woffset = transfered = 0;
	
	// first read
	fastcall_read r_read = { { 5, ACTION_READ }, roffset, &buffer, sizeof(buffer) };
	if( (rret = data_query(src, &r_read)) < 0) // EOF return too
		return rret;
	
	read     = r_read.buffer_size;
	roffset += r_read.buffer_size;
	
	goto start;
	do {
		fastcall_read r_read = { { 5, ACTION_READ }, roffset, &buffer, sizeof(buffer) };
		if( (rret = data_query(src, &r_read)) < -1)
			return rret;
		
		if(rret == -1) // EOF from read side
			break;
		
		read     = r_read.buffer_size;
		roffset += r_read.buffer_size;

	start:;
		fastcall_write r_write = { { 5, ACTION_WRITE }, woffset, &buffer, read };
		if( (wret = data_query(fargs->dest, &r_write)) < -1)
			return wret;
		
		if(wret == -1) // EOF from write side
			break;
		
		transfered += r_write.buffer_size;
		woffset    += r_write.buffer_size;
	}while(1);
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	return 0;
} // }}}

data_proto_t default_t_proto = {
	.type            = TYPE_DEFAULTT,
	.type_str        = "",
	.api_type        = API_HANDLERS,
	.handlers        = {
		[ACTION_COPY]        = (f_data_func)&data_default_copy,
		[ACTION_COMPARE]     = (f_data_func)&data_default_compare,
		[ACTION_READ]        = (f_data_func)&data_default_read,
		[ACTION_WRITE]       = (f_data_func)&data_default_write,
		[ACTION_TRANSFER]    = (f_data_func)&data_default_transfer,
		[ACTION_CONVERT_TO]  = (f_data_func)&data_default_convert_to,
		[ACTION_FREE]        = (f_data_func)&data_default_free,
		[ACTION_GETDATAPTR]  = (f_data_func)&data_default_getdataptr,
		[ACTION_IS_NULL]     = (f_data_func)&data_default_is_null,
		[ACTION_INIT]        = (f_data_func)&data_default_init,
	}
};

