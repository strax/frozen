#ifndef DATA_H
#define DATA_H

/* m4 {{{
m4_define(`REGISTER_TYPE', `
        m4_define(`TYPES_ARRAY', m4_defn(`TYPES_ARRAY')`$1,
	')
        m4_define(`TDEFS_ARRAY', m4_defn(`TDEFS_ARRAY')`#define $1 $1
	')
')
m4_define(`REGISTER_MACRO', `
	m4_define(`MACRO_ARRAY',
		m4_defn(`MACRO_ARRAY')
		`#define $1 { $2 }'
	)
')
m4_define(`REGISTER_STRUCT', `
	m4_define(`STRUCT_ARRAY',
		m4_defn(`STRUCT_ARRAY')
		`$1'
	)
')
m4_divert(-1)
m4_include(data_protos.m4)
m4_divert(0)
}}} */


enum data_type {
	TYPES_ARRAY()
	TYPE_INVALID = -1
};

enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
};

typedef size_t (*f_data_len)       (data_t *, data_ctx_t *);
typedef int    (*f_data_cmp)       (data_t *, data_ctx_t *, data_t *, data_ctx_t *);
typedef int    (*f_data_arith)     (char, data_t *, data_ctx_t *, data_t *, data_ctx_t *);

typedef ssize_t (*f_data_write)     (data_t *, data_ctx_t *, off_t, void *, size_t);
typedef ssize_t (*f_data_read)      (data_t *, data_ctx_t *, off_t, void **, size_t *);

typedef ssize_t (*f_data_convert)   (data_t *, data_ctx_t *, data_t *, data_ctx_t *);

TDEFS_ARRAY()
STRUCT_ARRAY()
MACRO_ARRAY()

#define DATA_INVALID  { TYPE_INVALID, NULL, 0 }
#define DEF_BUFFER_SIZE 1024

struct data_ctx_t {
	data_proto_t   *data_proto;
	void           *user_data;
};

struct data_t {
	data_type       type;
	void           *data_ptr;
	size_t          data_size;
};

struct data_proto_t {
	char *          type_str;
	data_type       type;
	size_type       size_type;
	size_t          fixed_size;
	
	f_data_len      func_len;
	f_data_cmp      func_cmp;
	f_data_arith    func_arithmetic;
	
	f_data_read     func_read;
	f_data_write    func_write;
	f_data_convert  func_convert;
};

extern data_proto_t  data_protos[];
extern size_t        data_protos_size;

/* api's */
API int                  data_type_is_valid     (data_type type);
API data_type            data_type_from_string  (char *string);
API char *               data_string_from_type  (data_type type);

API size_t               data_len               (data_t *data, data_ctx_t *data_ctx);
API int                  data_cmp               (data_t *data1, data_ctx_t *data1_ctx, data_t *data2, data_ctx_t *data2_ctx);
API int                  data_arithmetic        (char operator, data_t *operand1, data_ctx_t *operand1_ctx, data_t *operand2, data_ctx_t *operand2_ctx);

API void                 data_reinit            (data_t *dst, data_type type, void *data_ptr, size_t data_size);

API ssize_t              data_read              (data_t *src, data_ctx_t *src_ctx, off_t offset, void *buffer, size_t size);
API ssize_t              data_write             (data_t *dst, data_ctx_t *dst_ctx, off_t offset, void *buffer, size_t size);

API int                  data_convert           (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx);
API int                  data_transfer          (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx);
API ssize_t              data_copy              (data_t *dst, data_t *src);
API void                 data_free              (data_t *data);

API data_type            data_value_type        (data_t *data);
API void *               data_value_ptr         (data_t *data);
API size_t               data_value_len         (data_t *data);

#define data_alloc_local(_dst,_type,_size) { \
	(_dst)->type      = _type;           \
	(_dst)->data_size = _size;           \
	(_dst)->data_ptr  = alloca(_size);   \
}

#define data_copy_local(_dst,_src) {                                     \
	data_alloc_local(_dst, (_src)->type, (_src)->data_size);         \
	memcpy((_dst)->data_ptr, (_src)->data_ptr, (_src)->data_size);   \
}


#endif // DATA_H

/* vim: set filetype=c: */