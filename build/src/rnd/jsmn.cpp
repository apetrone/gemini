
#include "jsmn.h"
int test_jsmn_one()
{
	jsmntok_t tokens[256];

	jsmn_parser parser;
	jsmn_init( &parser );

	jsmnerr_t result = jsmn_parse( &parser, input1, tokens, 256 );
	if ( result != JSMN_SUCCESS )
	{
		fprintf( stderr, "Unable to parse json!\n" );
		return 0;
	}

	for( unsigned int a = 0; a < 256; ++a )
	{
		jsmntok_t * token = &tokens[a];
		if ( token->start != -1 )
		{
			printf( "token: %i\n", a );
			printf( "-> type: %i\n", token->type );
			printf( "-> start: %i\n", token->start );
			printf( "-> end: %i\n", token->end );
			printf( "-> size: %i\n", token->size );

			if ( token->type == JSMN_STRING )
			{
				input1[ token->end ] = '\0';
				printf( "-> %*s\n", token->end-token->start, input1+token->start );
			}
		}
	}

	return 1;
}
