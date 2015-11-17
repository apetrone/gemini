
#include <string.h>
#include <yajl/yajl_tree.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>

char input1[] = "{\n\"name\": \"adam\"}";

int test_yajl_one()
{
	char error_buffer[1024] = {0};
	char input[] = "{\n\"name\": \"adam\"\n}";
	yajl_val node;

	node = yajl_tree_parse( (const char*)input, error_buffer, 1024 );
	if ( !node )
	{
		fprintf( stderr, "parse_error: " );
		if (strlen(error_buffer))
		{
			fprintf( stderr, " %s", error_buffer );
		}
		else
		{
			fprintf( stderr, "Unknown error!" );
		}
		fprintf( stderr, "\n" );
		return 1;
	}


	const char * path[] = {"name", 0};

	yajl_val value = yajl_tree_get( node, path, yajl_t_string );
	if ( value )
	{
		printf( "%s\n", YAJL_GET_STRING(value) );
	}
	else
	{

		printf( "yep, you're crazy.\n" );
	}

	yajl_tree_free( node );

	return 0;
}


static int reformat_null(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_null(g);
}

static int reformat_boolean(void * ctx, int boolean)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_bool(g, boolean);
}

static int reformat_number(void * ctx, const char * s, size_t l)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_number(g, s, l);
}

static int reformat_string(void * ctx, const unsigned char * stringVal,
                           size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_string(g, stringVal, stringLen);
}

static int reformat_map_key(void * ctx, const unsigned char * stringVal,
                            size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_string(g, stringVal, stringLen);
}

static int reformat_start_map(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_map_open(g);
}


static int reformat_end_map(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_map_close(g);
}

static int reformat_start_array(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_array_open(g);
}

static int reformat_end_array(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_array_close(g);
}

static yajl_callbacks callbacks = {
    reformat_null,
    reformat_boolean,
    NULL,
    NULL,
    reformat_number,
    reformat_string,
    reformat_start_map,
    reformat_map_key,
    reformat_end_map,
    reformat_start_array,
    reformat_end_array
};

int test_yajl_two()
{
	yajl_handle handle;
	yajl_gen gen;
	yajl_status status;

	gen = yajl_gen_alloc(0);

	yajl_gen_config( gen, yajl_gen_beautify, 1 );
	yajl_gen_config( gen, yajl_gen_validate_utf8, 1 );

	handle = yajl_alloc( &callbacks, 0, (void *)gen );

	yajl_config( handle, yajl_allow_comments, 1 );

	char input[] = "{\n\"name\": \"adam\"}";

	status = yajl_parse( handle, (const unsigned char*)input, sizeof(input) );
	status = yajl_complete_parse( handle );
	if ( status != yajl_status_ok )
	{
		fprintf( stderr, "Error parsing!\n" );
		unsigned char * str = yajl_get_error(handle, 1, (const unsigned char*)input, sizeof(input));
        fprintf(stderr, "%s", (const char *) str);
        yajl_free_error(handle, str);
	}

	yajl_gen_free( gen );
	yajl_free( handle );

	return 0;
}
