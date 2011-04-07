AC_DEFUN([FROZEN_DATA],[
	define([data_name],translit($1,_,-))
	define([data_action],ifelse($3,yes,disable,enable))
	AC_ARG_ENABLE($1,AS_HELP_STRING([--]data_action()[-data-]data_name(),data_action()[ data ]data_name()))
	
	if test "$3" = "yes"; then
		if test "${enable_$1}" = "yes"; then
			enable_$1="no"
		else
			enable_$1="yes"
		fi
	fi
	
	if test "${enable_$1}" = "yes"; then
		DATA_DIR_$1="$2"
		DATA_DEP_$1="$4"
		DATA_OBJECTS_$2="$DATA_OBJECTS_$2 libfrozen_data_$2_la-$1.lo"
		DATA_SELECTED="$DATA_SELECTED $1"
		DATA_HEADERS="$DATA_HEADERS $2/$1.h"
		
		AC_SUBST([DATA_OBJECTS_$2])
	fi
	
	if test "$DATA_MAKEFILE_$2" = ""; then
		GEN_MAKEFILES="$GEN_MAKEFILES src/libfrozen/data/$2/Makefile"
		DATA_DIST_DIRS="$DATA_DIST_DIRS $2"
		AC_SUBST([DATA_DIST_DIRS])
		DATA_MAKEFILE_$2=yes
		
		DATA_LIBS_SELECTED="$DATA_LIBS_SELECTED $2/libfrozen_data_$2.la"
		AC_SUBST([DATA_LIBS_SELECTED])
	fi
])

AC_DEFUN([FROZEN_DATA_END],[
	DATA_SELECTED_DEPS=$DATA_SELECTED
	for m in $DATA_SELECTED; do
		eval m_deps="\$DATA_DEP_$m"
		for d in $m_deps; do
			DATA_SELECTED_DEPS=$( echo $DATA_SELECTED_DEPS | sed "s|$d|$d $m|" | sed "s|$m||" )
			# add this module after dependency and remove first occurence of module, leaving inserted
		done;
	done;
	for m in $DATA_SELECTED_DEPS; do
		eval m_folder="\$DATA_DIR_$m"
		DATA_DIRS="$DATA_DIRS $m_folder"
		DATA_DIRS=$(echo $DATA_DIRS | sed "s|$m_folder|REPLACEMEBACK|" | sed "s|$m_folder||g" | sed "s|REPLACEMEBACK|$m_folder|" )
		
	done;
	AC_SUBST([DATA_DIRS])
	
	data_hdr_file=src/libfrozen/core/data_selected.h
	echo "/* Autogenerated file. Don't edit */" > $data_hdr_file
	echo "#ifndef DATA_SELECTED_H" >> $data_hdr_file
	echo "#define DATA_SELECTED_H" >> $data_hdr_file
	for h in $DATA_HEADERS; do
		echo "#include <$h>" >> $data_hdr_file
	done;
	echo "#ifdef DATA_C" >> $data_hdr_file
	echo "data_proto_t * data_protos[[]] = {" >> $data_hdr_file
	for h in $DATA_SELECTED; do
		h_upper=$( echo "$h" | tr "a-z" "A-Z" | sed "s#_##g")
		echo "   [[TYPE_$h_upper]] = &${h}_proto," >> $data_hdr_file
	done;
	echo "};" >> $data_hdr_file
	echo "size_t data_protos_size = (sizeof(data_protos)/sizeof(data_protos[[0]]));" >> $data_hdr_file
	echo "#endif" >> $data_hdr_file
	
	echo "#endif" >> $data_hdr_file
])