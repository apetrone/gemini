// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include "typedefs.h"
#include "xstr.h"
#include "log.h"
#include "filesystem.hpp"
//#include "memorystream.hpp"
#include "font.hpp"
#include "renderer.hpp"
#include "renderstream.hpp"
#include <stdlib.h>

#define STB_TRUETYPE_IMPLEMENTATION 1
#define STBTT_malloc(x,u)  malloc(x)
#define STBTT_free(x,u)    free(x)
#include "stb_truetype.h"

namespace font
{
	const unsigned int FONT_TEXTURE_WIDTH = 512;
	const unsigned int FONT_TEXTURE_HEIGHT = 512;
	const unsigned int FONT_CHARINFO_SIZE = 95;
	const unsigned int FONT_MAX_VERTICES = 8192;
	const unsigned int FONT_MAX_INDICES = 6172;
	const unsigned int FONT_CHAR_START = 32;

	
	struct FontVertex
	{
		float x, y, z;
		unsigned char r,g,b,a;
		float u, v;
	};

	struct ImageInfo
	{
		int width;
		int height;
		int pitch;
		int topOffset;
	};
	

	struct CharacterInfo
	{
		unsigned int glyph_index; // this character's glyph index
		short advance_x; // add advanceX and advanceY after drawing
		short advance_y;
		short left; // add left before drawing
		short top; // subtract top to offset origin at top, left
		short width;
		short height;
		short h_bearing_x;
		short h_bearing_y;
		short v_bearing_x;
		short v_bearing_y;
		float uv[4]; // uv rect
	}; // CharacterInfo

	// this class assumes an origin in the upper left hand corner and assumes bottom > top, right > left
	template <class _Type>
	struct rect
	{
		_Type left, top, right, bottom;

		rect() : left( _Type(0) ), top( _Type(0) ), right( _Type(0) ), bottom( _Type(0) ) {}
		rect( _Type _left, _Type _top, _Type _right, _Type _bottom ) :
		left(_left), top(_top), right(_right), bottom(_bottom) {}

		_Type getWidth() const
		{
			return ( right - left );
		}

		_Type getHeight() const
		{
			return ( bottom - top );
		}

		rect<_Type> operator- ( const rect<_Type> & other ) const
		{
			return rect<_Type> (left-other.left, top-other.top, right-other.top, bottom-other.bottom );
		}

		rect<_Type> operator- ( const rect<_Type> & other )
		{
			return rect<_Type> (left-other.left, top-other.top, right-other.top, bottom-other.bottom );
		}

		rect<_Type> operator+ ( const rect<_Type> & other ) const
		{
			return rect<_Type> (left+other.left, top+other.top, right+other.top, bottom+other.bottom );
		}

		bool FitsInside( const rect<_Type> & other ) const
		{
			if ( getWidth() <= other.getWidth() && getHeight() <= other.getHeight() )
				return true;

			return false;
		}


		void set( _Type _left = 0, _Type _top = 0, _Type _right = 0, _Type _bottom = 0 )
		{
			left = _left;
			top = _top;
			right = _right;
			bottom = _bottom;
		}
	};

	typedef rect<float> rectf;
	typedef rect<int> recti;

    struct AlignmentNode
    {
        AlignmentNode * child[2];

        int x, y;
        int width, height;
        int id;

        AlignmentNode()
        {
            //printf( "RNode(): %x\n", this );
            child[0] = child[1] = 0;
            id = -1;

            x = y = width = height = 0;
        }

        ~AlignmentNode()
        {
            //printf( "~RNode(): %x\n", this );
        }

        inline bool CanFitRect( const recti & r )
        {
            if ( r.getWidth() < width && r.getHeight() < height )
                return true;
            else
                return false;
        }

        inline bool IsLeaf() const
        {
            return (child[0] == 0 && child[1] == 0);
        }
    };

    struct PixelBGRA
    {
        unsigned char blue, green, red, alpha;
    };

	struct AlignmentGrid
	{
		~AlignmentGrid()
		{
			PurgeNode( root.child[0] );
			PurgeNode( root.child[1] );
		}

		AlignmentNode root;
		AlignmentNode * Insert( AlignmentNode * node, recti & r )
		{
			// create a new child node
			if ( !node->IsLeaf() )
			{
				AlignmentNode * newNode = Insert( node->child[0], r );
				if ( newNode != 0 )
				{
					return newNode;
				}

				newNode = Insert( node->child[1], r );

				if ( newNode != 0 )
				{
					return newNode;
				}

				return 0;
			}

			// if r does NOT fit inside the node, return 0
			if ( r.getWidth() > node->width || r.getHeight() > node->height )
			{
				return 0;
			}

			// split and create children
			int w = node->width - r.getWidth();
			int h = node->height - r.getHeight();

			node->child[0] = new AlignmentNode();
			node->child[1] = new AlignmentNode();

			if ( w <= h ) // split horizontally
			{
				node->child[0]->x = node->x + r.getWidth();
				node->child[0]->y = node->y;
				node->child[0]->width = w;
				node->child[0]->height = r.getHeight();

				node->child[1]->x = node->x;
				node->child[1]->y = node->y + r.getHeight();
				node->child[1]->width = node->width;
				node->child[1]->height = h;
			}
			else // split vertically
			{
				node->child[0]->x = node->x;
				node->child[0]->y = node->y + r.getHeight();
				node->child[0]->width = r.getWidth();
				node->child[0]->height = h;

				node->child[1]->x = node->x + r.getWidth();
				node->child[1]->y = node->y;
				node->child[1]->width = w;
				node->child[1]->height = node->height;
			}

            node->width = r.getWidth();
            node->height = r.getHeight();
            return node;
		} // Insert


		void CopyImage( int startX, int startY, ImageInfo & dstInfo, unsigned char * dst, const unsigned char * src, ImageInfo & srcInfo, bool noaa = false )
		{
			int y;
			int x;
			int destX = startX;
			int destY = dstInfo.height - startY - srcInfo.height;
			int idx;
			PixelBGRA * rgb = 0;

			// rendered up-side down if bmp_top > 0
			for( y = srcInfo.height-1; y >= 0; y-- )
			{
				rgb = (PixelBGRA*) &dst[ destY * dstInfo.pitch ];
				destX = startX;
				for( x = 0; x < srcInfo.width; ++x )
				{
					idx = (x + (y * srcInfo.pitch));

					if ( !noaa )
					{
						// RGBA
						rgb[ destX ].red = rgb[ destX ].green = rgb[ destX ].blue = rgb[ destX ].alpha = src[ idx ];
					}
					else
					{
						// mono
						unsigned char v = src[ (y * srcInfo.pitch + (x>>3) ) ] & (256 >> ((x&7)+1));
						rgb[ destX ].red = rgb[ destX ].green = rgb[ destX ].blue = rgb[ destX ].alpha = (v ? 255 : 0);
					}
					++destX;
				}

				++destY;
			}
		}

		void PurgeNode( AlignmentNode * root )
		{
			if ( !root )
				return;

			if ( root->child[0] )
				PurgeNode( root->child[0] );

			if ( root->child[1] )
				PurgeNode( root->child[1] );

			delete root;
		}
	}; // AlignmentGrid


	struct SimpleFontHandle
	{
		bool noaa;

		unsigned int textureID;
		unsigned int textureWidth;
		unsigned int textureHeight;
		unsigned int textureBPP;
		unsigned char * texturePixels;
		
		unsigned int font_size;
		unsigned int font_height;
		unsigned int line_height;
		
		stbtt_fontinfo font_info;
		
		renderer::VertexStream * vertex_stream;

		AlignmentGrid grid;
		CharacterInfo char_info[ FONT_CHARINFO_SIZE ];

		void purge()
		{
		}
	}; // SimpleFontHandle

	namespace internal
	{
		unsigned int _font_id = 0;
		const unsigned int MAX_FONT_HANDLES = 8;
		SimpleFontHandle _font_handles[ MAX_FONT_HANDLES ];
		
		renderer::VertexStream _vertexstream;
		
		SimpleFontHandle * request_handle()
		{
			SimpleFontHandle * handle;
			
			assert( _font_id < MAX_FONT_HANDLES );
//			for( int i = 0; i < internal::MAX_FONT_HANDLES; ++i )
			{
				handle = &_font_handles[ _font_id++ ];
				if ( handle->font_info.numGlyphs == 0 )
				{
					return handle;
				}
			}
			
			return 0;
		}
	}; // namespace internal

	void startup()
	{
		for( int i = 0; i < internal::MAX_FONT_HANDLES; ++i )
		{
			memset( &internal::_font_handles[i], 0, sizeof(SimpleFontHandle) );
		}
		
		// fetch the correct shader for rendering
		// ...
		
		// initialize the vertex stream
		internal::_vertexstream.desc.add( renderer::VD_FLOAT3 );
		internal::_vertexstream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		
		internal::_vertexstream.create( FONT_MAX_VERTICES, FONT_MAX_INDICES, renderer::DRAW_INDEXED_TRIANGLES, renderer::BUFFER_STREAM );
		
	} // startup

	void shutdown()
	{
		// cleanup used memory here
		internal::_vertexstream.destroy();
	} // shutdown
	

	
	void draw_string( font::Handle fontid, int x, int y, const char * utf8, const Color & color )
	{
		RenderStream rs;

		// setup global rendering state
		rs.add_state( renderer::STATE_DEPTH_TEST, 0 );
		rs.add_state( renderer::STATE_BLEND, 1 );
		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		rs.run_commands();
		
		
		// draw characters
		// ...
		
		
		// restore state
		rs.rewind();
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.run_commands();
	} // draw_string
	
	unsigned int measure_height( font::Handle fontid )
	{
		return 0;
	} // measure_height
	
	unsigned int measure_width( font::Handle fontid, const char * str )
	{
		return 0;
	} // measure_width
	
	font::Handle load_font_from_memory( const void * data, unsigned int data_size, unsigned int point_size, bool antialiased, unsigned int hres, unsigned int vres )
	{
		SimpleFontHandle * fh = internal::request_handle();
		
		if ( !fh )
		{
			return font::Handle(0);
		}
		
		if ( !stbtt_InitFont( &fh->font_info, (const unsigned char*)data, 0 ) )
		{
			return font::Handle(0);
		}
		
		int ascent, descent, linegap, delta;
		stbtt_GetFontVMetrics( &fh->font_info, &ascent, &descent, &linegap );
		
		delta = (ascent-descent);
		
		fh->font_size = point_size;
		fh->line_height = (delta + linegap) / (float)delta;
		fh->font_height = fh->line_height * point_size;
		
		
		LOGV( "font height is: %i\n", fh->font_height );
		
		return font::Handle(0);
	} // load_font_from_memory

}; // namespace font
