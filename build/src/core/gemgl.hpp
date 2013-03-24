#pragma once

#if _WIN32 || LINUX
	#include "xlib.h"
#endif

#if _WIN32
	#include <limits.h>
	#include <windows.h>
	#include <gl/gl.h>
	#include <glext.h>
	#include <wglext.h>
	#pragma comment( lib, "opengl32.lib" )
	#define GEMGLAPI WINAPI*
#elif LINUX
	#include <stdint.h>
	#include <GL/gl.h>
	#include <GL/glx.h>
//	#include <glxext.h>
	#define GEMGLAPI *
#elif __APPLE__
	#include <stdint.h>
	#include <TargetConditionals.h>

	#if TARGET_OS_IPHONE
		#include <OpenGLES/ES2/gl.h>
		#include <OpenGLES/ES2/glext.h>
	#elif TARGET_OS_MAC
		#include <OpenGL/gl3.h>
		#include <OpenGL/gl3ext.h>
		//#include <OpenGL/OpenGL.h>
		//#include <OpenGL/gl.h>
	#endif

	#define GEMGLAPI *
#endif


#include "renderer.hpp"

// print information regarding whether or not we correctly link opengl functions
#define GEMGL_DEBUG_SYMBOLS 1

#if __APPLE__ && (TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE) || PLATFORM_IS_RASPBERRYPI
	#define GEMGL_ENABLE_ES 1
#endif

#define GEMGL_LINK( name, fn, type )	name = (type) gemgl_findsymbol( gl, fn );

// ---------------------------------------

typedef GLuint GLObject;

enum gemgl_renderer_type
{
	GEMGL_INVALID = 0,
	GEMGL_CORE_32,
	GEMGL_ES_20,	
};


	// ---------------------------------------
	typedef void (GEMGLAPI GEMGLFNVIEWPORT) ( GLint x, GLint y, GLsizei width, GLsizei height );
	typedef void (GEMGLAPI GEMGLFNDRAWARRAYS) ( GLenum mode, GLint first, GLsizei count );
	typedef void (GEMGLAPI GEMGLFNDRAWELEMENTS) ( GLenum mode, GLsizei count, GLenum type, GLvoid* indices );
	typedef void (GEMGLAPI GEMGLFNCLEARCOLOR) ( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
	typedef void (GEMGLAPI GEMGLFNCLEAR) ( GLbitfield mask );
	typedef void (GEMGLAPI GEMGLFNPOINTSIZE) ( GLfloat size );
	typedef void (GEMGLAPI GEMGLFNLINEWIDTH) ( GLfloat width );
	typedef void (GEMGLAPI GEMGLFNFRONTFACE) ( GLenum dir );
	typedef void (GEMGLAPI GEMGLFNCULLFACE) ( GLenum mode );
	typedef void (GEMGLAPI GEMGLFNENABLE) ( GLenum pname );
	typedef void (GEMGLAPI GEMGLFNDISABLE) ( GLenum pname );
	typedef void (GEMGLAPI GEMGLFNPOLYGONOFFSET) ( GLfloat factor, GLfloat units );
	typedef void (GEMGLAPI GEMGLFNPIXELSTOREI) ( GLenum pname, GLint param );

	typedef void (GEMGLAPI GEMGLFNTEXPARAMETERF) ( GLenum target, GLenum pname, GLfloat param );
	typedef void (GEMGLAPI GEMGLFNTEXPARAMETERFV) ( GLenum target, GLenum pname, const GLfloat* params );
	typedef void (GEMGLAPI GEMGLFNTEXPARAMETERI) ( GLenum target, GLenum pname, GLint param );
	typedef void (GEMGLAPI GEMGLFNTEXPARAMETERIV) ( GLenum target, GLenum pname, const GLint* params );
	typedef void (GEMGLAPI GEMGLFNTEXIMAGE1D) ( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid* pixels );
	typedef void (GEMGLAPI GEMGLFNTEXIMAGE2D) ( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid* pixels );
	typedef void (GEMGLAPI GEMGLFNBINDTEXTURE) ( GLenum target, GLuint texture );
	typedef void (GEMGLAPI GEMGLFNDELETETEXTURES) ( GLsizei n, GLuint* textures );
	typedef void (GEMGLAPI GEMGLFNGENTEXTURES) ( GLsizei n, GLuint* textures );
	typedef GLboolean (GEMGLAPI GEMGLFNISTEXTURE) ( GLuint texture );
	typedef void (GEMGLAPI GEMGLFNACTIVETEXTURE) ( GLenum texture );
	
#if GEMGL_ENABLE_ES
	typedef void (GEMGLAPI GEMGLFNDEPTHRANGEF) ( GLclampf n, GLclampf f );
#else
	typedef void (GEMGLAPI GEMGLFNDEPTHRANGE) ( GLclampd n, GLclampd f );
#endif

	typedef const GLubyte * (GEMGLAPI GEMGLFNGETSTRING) ( GLenum param );

	// ---------------------------------------
	typedef GLuint (GEMGLAPI GEMGLFNCREATEPROGRAM) ( void );
	typedef GLuint (GEMGLAPI GEMGLFNCREATESHADER) ( GLenum type );
	typedef void (GEMGLAPI GEMGLFNDELETEPROGRAM) ( GLObject program );
	//typedef void (GEMGLAPI GEMGLFNDELETEPROGRAMS) ( GLsizei n, GLuint* programs );
	typedef void (GEMGLAPI GEMGLFNDELETESHADER) ( GLObject shader );
    typedef void (GEMGLAPI GEMGLFNLINKPROGRAM) ( GLObject program );
	typedef void (GEMGLAPI GEMGLFNSHADERSOURCE) ( GLObject shader, GLsizei count, GLchar** string, GLint* length );
	typedef void (GEMGLAPI GEMGLFNUSEPROGRAM) ( GLObject program );
	typedef void (GEMGLAPI GEMGLFNCOMPILESHADER) ( GLObject shaderObj );
    typedef void (GEMGLAPI GEMGLFNATTACHSHADER) ( GLObject program, GLuint shader );
	typedef GLboolean (GEMGLAPI GEMGLFNISSHADER) (GLObject shader );
	typedef void (GEMGLAPI GEMGLFNGETSHADERIV) (GLObject shader, GLenum pname, GLint *params );
	typedef void (GEMGLAPI GEMGLFNGETSHADERINFOLOG) ( GLObject shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog );
	typedef void (GEMGLAPI GEMGLFNGETPROGRAMINFOLOG) (GLObject program, GLsizei bufSize, GLsizei * length, GLchar* infoLog );

	typedef GLboolean (GEMGLAPI GEMGLFNISPROGRAM) ( GLObject program );
	typedef void (GEMGLAPI GEMGLFNGETPROGRAMIV) (GLObject program, GLenum pname, GLint *params );
	typedef void (GEMGLAPI GEMGLFNVALIDATEPROGRAM) ( GLObject program );
	typedef void (GEMGLAPI GEMGLFNRELEASESHADERCOMPILER) ( void );
	typedef void (GEMGLAPI GEMGLFNSHADERBINARY) ( GLsizei count, const GLuint * shaders, GLenum binaryformat, const GLvoid * binary, GLsizei length );
	typedef void (GEMGLAPI GEMGLFNDETACHSHADER) ( GLObject program, GLuint shader );
	typedef void (GEMGLAPI GEMGLFNGETATTACHEDSHADERS) (GLObject program, GLsizei maxCount, GLsizei * count, GLuint * shaders );

	// attributes
	typedef void (GEMGLAPI GEMGLFNGETACTIVEATTRIB) ( GLObject program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name );
	typedef void (GEMGLAPI GEMGLFNGETATTRIBLOCATION) ( GLObject program, const GLchar * name );
	typedef void (GEMGLAPI GEMGLFNBINDATTRIBLOCATION) ( GLObject program, GLuint index, const GLchar* name );
	
	// uniforms
	typedef GLint (GEMGLAPI GEMGLFNGETUNIFORMLOCATION) ( GLObject program, const GLchar* name );
	typedef GLuint (GEMGLAPI GEMGLFNGETUNIFORMBLOCKINDEX) ( GLObject program, const GLchar * uniformBlockName );
	typedef void (GEMGLAPI GEMGLFNGETACTIVEUNIFORMBLOCKNAME) ( GLObject program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformBlockName );
	typedef void (GEMGLAPI GEMGLFNGETACTIVEUNIFORMBLOCKIV) ( GLObject program, GLuint uniformBlockIndex, GLenum pname, GLint * params );
	typedef void (GEMGLAPI GEMGLFNGETUNIFORMINDICES) ( GLObject program, GLsizei uniformCount, const GLchar ** uniformNames, GLuint * uniformIndices );
	typedef void (GEMGLAPI GEMGLFNGETACTIVEUNIFORMNAME) ( GLObject program, GLuint uniformIndex, GLsizei bufSize, GLsizei * length, GLchar * uniformName );
	typedef void (GEMGLAPI GEMGLFNGETACTIVEUNIFORM) ( GLObject program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name );
	typedef void (GEMGLAPI GEMGLFNGETACTIVEUNIFORMSIV) ( GLObject program, GLsizei uniformCount, const GLuint * uniformIndices, GLenum pname, GLint * params );
	
	
	typedef void (GEMGLAPI GEMGLFNBINDFRAGDATALOCATION) ( GLObject program, GLuint color, const GLchar* name );

	typedef void (GEMGLAPI GEMGLFNBINDVERTEXARRAY) ( GLuint array );
	typedef void (GEMGLAPI GEMGLFNDELETEVERTEXARRAYS) ( GLsizei n, GLuint* arrays );
	typedef void (GEMGLAPI GEMGLFNGENVERTEXARRAYS) ( GLsizei n, GLuint* arrays );
	typedef GLboolean (GEMGLAPI GEMGLFNISVERTEXARRAY) ( GLuint array );

	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIBPOINTER) ( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLvoid* pointer );
	typedef void (GEMGLAPI GEMGLFNENABLEVERTEXATTRIBARRAY) ( GLuint index );
	typedef void (GEMGLAPI GEMGLFNDISABLEVERTEXATTRIBARRAY) ( GLuint index );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB1F) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB2F) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB3F) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB4F) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB1FV) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB2FV) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB3FV) ( GLuint index, float * values );
	typedef void (GEMGLAPI GEMGLFNVERTEXATTRIB4FV) ( GLuint index, float * values );

	typedef void (GEMGLAPI GEMGLFNBINDBUFFER) ( GLenum target, GLuint buffer );
	typedef void (GEMGLAPI GEMGLFNBINDBUFFERRANGE) ( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
	typedef void (GEMGLAPI GEMGLFNBINDBUFFERBASE) ( GLenum target, GLuint index, GLuint buffer );	
	typedef void (GEMGLAPI GEMGLFNDELETEBUFFERS) ( GLsizei n, const GLuint* buffers );
	typedef void (GEMGLAPI GEMGLFNGENBUFFERS) ( GLsizei n, GLuint* buffers );
	typedef GLboolean (GEMGLAPI GEMGLFNISBUFFER) ( GLuint buffer );
	typedef void (GEMGLAPI GEMGLFNBUFFERDATA) ( GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage );
	typedef void (GEMGLAPI GEMGLFNBUFFERSUBDATA) ( GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data );
	typedef void (GEMGLAPI GEMGLFNGETBUFFERSUBDATA) ( GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data );
	typedef GLvoid* (GEMGLAPI GEMGLFNMAPBUFFER) ( GLenum target, GLenum access );
	typedef GLboolean (GEMGLAPI GEMGLFNUNMAPBUFFER) ( GLenum target );
	typedef void (GEMGLAPI GEMGLFNGETBUFFERPARAMETERIV) ( GLenum target, GLenum pname, GLint* params );
	typedef void (GEMGLAPI GEMGLFNGETBUFFERPOINTERV) ( GLenum target, GLenum pname, GLvoid** params );

	typedef void (GEMGLAPI GEMGLFNUNIFORM1F) ( GLint location, GLfloat v0 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM2F) ( GLint location, GLfloat v0, GLfloat v1 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM3F) ( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM4F) ( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM1I) ( GLint location, GLint v0 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM2I) ( GLint location, GLint v0, GLint v1 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM3I) ( GLint location, GLint v0, GLint v1, GLint v2 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM4I) ( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
	typedef void (GEMGLAPI GEMGLFNUNIFORM1FV) ( GLint location, GLsizei count, GLfloat* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM2FV) ( GLint location, GLsizei count, GLfloat* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM3FV) ( GLint location, GLsizei count, GLfloat* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM4FV) ( GLint location, GLsizei count, GLfloat* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM1IV) ( GLint location, GLsizei count, GLint* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM2IV) ( GLint location, GLsizei count, GLint* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM3IV) ( GLint location, GLsizei count, GLint* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORM4IV) ( GLint location, GLsizei count, GLint* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORMMATRIX2FV) ( GLint location, GLsizei count, GLboolean transpose, GLfloat* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORMMATRIX3FV) ( GLint location, GLsizei count, GLboolean transpose, GLfloat* value );
	typedef void (GEMGLAPI GEMGLFNUNIFORMMATRIX4FV) ( GLint location, GLsizei count, GLboolean transpose, GLfloat* value );

	typedef void (GEMGLAPI GEMGLFNGENERATEMIPMAP) ( GLenum target );
	
	// blending
	typedef void (GEMGLAPI GEMGLFNBLENDEQUATION) ( GLenum mode );
	typedef void (GEMGLAPI GEMGLFNBLENDEQUATIONSEPARATE) ( GLenum modeRGB, GLenum modeAlpha );
	typedef void (GEMGLAPI GEMGLFNBLENDFUNCSEPARATE) ( GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
	typedef void (GEMGLAPI GEMGLFNBLENDFUNC) ( GLenum src, GLenum dst );
	typedef void (GEMGLAPI GEMGLFNBLENDCOLOR) ( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
	

#if _WIN32
	typedef int (GEMGLAPI GEMGLSWAPINTERVAL) (int interval);
	typedef int (GEMGLAPI GEMGLGETSWAPINTERVAL) (void);
#endif

	

	// frame buffers
	typedef void (GEMGLAPI GEMGLFNBINDFRAMEBUFFER) ( GLenum target, GLuint framebuffer );
	typedef void (GEMGLAPI GEMGLFNDELETEFRAMEBUFFERS) ( GLsizei, GLuint * framebuffers );
	typedef void (GEMGLAPI GEMGLFNGENFRAMEBUFFERS) ( GLsizei n, GLuint * ids );
	
	// render buffers
	typedef void (GEMGLAPI GEMGLFNBINDRENDERBUFFER) ( GLenum target, GLuint renderbuffer );
	typedef void (GEMGLAPI GEMGLFNDELETERENDERBUFFERS) ( GLsizei n, GLuint * renderbuffers );
	typedef void (GEMGLAPI GEMGLFNGENRENDERBUFFERS) ( GLsizei n, GLuint * renderbuffers );
	typedef void (GEMGLAPI GEMGLFNRENDERBUFFERSTORAGEMULTISAMPLE) ( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
	typedef void (GEMGLAPI GEMGLFNRENDERBUFFERSTORAGE) ( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
	
	// attaching texture images to a framebuffer
	typedef void (GEMGLAPI GEMGLFNFRAMEBUFFERRENDERBUFFER) ( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
	typedef void (GEMGLAPI GEMGLFNFRAMEBUFFERTEXTURE) ( GLenum target, GLenum attachment, GLuint texture, GLint level );
	typedef void (GEMGLAPI GEMGLFNFRAMEBUFFERTEXTURE3D) ( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
	typedef void (GEMGLAPI GEMGLFNFRAMEBUFFERTEXTURE2D) ( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
	typedef void (GEMGLAPI GEMGLFNFRAMEBUFFERTEXTURE1D) ( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
	typedef void (GEMGLAPI GEMGLFNFRAMEBUFFERTEXTURELAYER) ( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );	
	
	typedef GLenum (GEMGLAPI GEMGLFNCHECKFRAMEBUFFERSTATUS) ( GLenum target );
	typedef GLboolean (GEMGLAPI GEMGLFNISFRAMEBUFFER) ( GLuint framebuffer );
	
	typedef void (GEMGLAPI GEMGLFNGETFRAMEBUFFERATTACHMENTPARAMETERIV) ( GLenum target, GLenum attachment, GLenum pname, GLint * params );
	
	typedef GLboolean (GEMGLAPI GEMGLFNISRENDERBUFFER) ( GLuint renderbuffer );
	typedef void (GEMGLAPI GEMGLFNGETRENDERBUFFERPARAMETERIV) ( GLenum target, GLenum pname, GLint * params );
	

    typedef GLenum (*GEMGL_CHECKERROR)( const char * );

	typedef struct
	{
#if GEMGL_ENABLE_ES
		GEMGLFNVIEWPORT Viewport;
		GEMGLFNDEPTHRANGEF DepthRange;
		GEMGLFNCLEARCOLOR ClearColor;
		GEMGLFNCLEAR Clear;
		GEMGLFNDRAWARRAYS DrawArrays;
		GEMGLFNDRAWELEMENTS DrawElements;
		GEMGLFNLINEWIDTH LineWidth;
		GEMGLFNFRONTFACE FrontFace;
		GEMGLFNCULLFACE CullFace;
		GEMGLFNENABLE Enable;
		GEMGLFNDISABLE Disable;
		GEMGLFNPOLYGONOFFSET PolygonOffset;
		GEMGLFNPIXELSTOREI PixelStorei;
		
		// ReadPixels

		// BUFFER OBJECTS
		GEMGLFNBINDBUFFER BindBuffer;		
		GEMGLFNDELETEBUFFERS DeleteBuffers;
		GEMGLFNGENBUFFERS GenBuffers;
		GEMGLFNISBUFFER IsBuffer;
		GEMGLFNBUFFERDATA BufferData;
		GEMGLFNBUFFERSUBDATA BufferSubData;
		GEMGLFNGETBUFFERPARAMETERIV GetBufferParameteriv;
		
		// textures
		GEMGLFNACTIVETEXTURE ActiveTexture;
		GEMGLFNTEXIMAGE2D TexImage2D;
		//CopyTexImage2D
		//TexSubImage2D
		//CopyTexSubImage2D
		//CompressedTexImage2D
		//CompressedTexSubImage2D
		//GetTexParameterifv
		GEMGLFNGENERATEMIPMAP GenerateMipmap;
		GEMGLFNTEXPARAMETERF TexParameterf;
		GEMGLFNTEXPARAMETERFV TexParameterfv;
		GEMGLFNTEXPARAMETERI TexParameteri;
		GEMGLFNTEXPARAMETERIV TexParameteriv;
		GEMGLFNBINDTEXTURE BindTexture;
		GEMGLFNDELETETEXTURES DeleteTextures;
		GEMGLFNGENTEXTURES GenTextures;
		GEMGLFNISTEXTURE IsTexture;
		
		
		// attribs
		GEMGLFNVERTEXATTRIBPOINTER VertexAttribPointer;
		GEMGLFNENABLEVERTEXATTRIBARRAY EnableVertexAttribArray;
		GEMGLFNDISABLEVERTEXATTRIBARRAY DisableVertexAttribArray;	
		GEMGLFNVERTEXATTRIB1F VertexAttrib1f;
		GEMGLFNVERTEXATTRIB1F VertexAttrib2f;
		GEMGLFNVERTEXATTRIB1F VertexAttrib3f;
		GEMGLFNVERTEXATTRIB1F VertexAttrib4f;
		GEMGLFNVERTEXATTRIB1F VertexAttrib1fv;
		GEMGLFNVERTEXATTRIB1F VertexAttrib2fv;
		GEMGLFNVERTEXATTRIB1F VertexAttrib3fv;
		GEMGLFNVERTEXATTRIB1F VertexAttrib4fv;
	
		// SHADERS AND PROGRAMS
		// shader objects
		GEMGLFNCREATESHADER CreateShader;
		GEMGLFNSHADERSOURCE ShaderSource;
		GEMGLFNCOMPILESHADER CompileShader;
		GEMGLFNRELEASESHADERCOMPILER ReleaseShaderCompiler;
		GEMGLFNDELETESHADER DeleteShader;
		
		// loading shader binaries
		GEMGLFNSHADERBINARY ShaderBinary;
		
		// program objects		
        GEMGLFNCREATEPROGRAM CreateProgram;
		GEMGLFNATTACHSHADER AttachShader;
		GEMGLFNDETACHSHADER DetachShader;
		GEMGLFNLINKPROGRAM LinkProgram;
		GEMGLFNUSEPROGRAM UseProgram;
        GEMGLFNDELETEPROGRAM DeleteProgram;

		// SHADER VARIABLES
		// vertex attributes
		GEMGLFNGETACTIVEATTRIB GetActiveAttrib;
		GEMGLFNGETATTRIBLOCATION GetAttribLocation;
		GEMGLFNBINDATTRIBLOCATION BindAttribLocation;		
		
		// uniform variables
		GEMGLFNGETUNIFORMLOCATION GetUniformLocation;
		GEMGLFNGETACTIVEUNIFORM GetActiveUniform;
		GEMGLFNGETACTIVEUNIFORMSIV GetActiveUniformsiv;		
		GEMGLFNUNIFORM1F Uniform1f;
		GEMGLFNUNIFORM2F Uniform2f;
		GEMGLFNUNIFORM3F Uniform3f;
		GEMGLFNUNIFORM4F Uniform4f;
		GEMGLFNUNIFORM1I Uniform1i;
		GEMGLFNUNIFORM2I Uniform2i;
		GEMGLFNUNIFORM3I Uniform3i;
		GEMGLFNUNIFORM4I Uniform4i;
		GEMGLFNUNIFORM1FV Uniform1fv;
		GEMGLFNUNIFORM2FV Uniform2fv;
		GEMGLFNUNIFORM3FV Uniform3fv;
		GEMGLFNUNIFORM4FV Uniform4fv;
		GEMGLFNUNIFORM1IV Uniform1iv;
		GEMGLFNUNIFORM2IV Uniform2iv;
		GEMGLFNUNIFORM3IV Uniform3iv;
		GEMGLFNUNIFORM4IV Uniform4iv;
		GEMGLFNUNIFORMMATRIX2FV UniformMatrix2fv;
		GEMGLFNUNIFORMMATRIX3FV UniformMatrix3fv;
		GEMGLFNUNIFORMMATRIX4FV UniformMatrix4fv;
		
		// shader execution
		GEMGLFNVALIDATEPROGRAM ValidateProgram;
		
		// shader queries
		GEMGLFNISSHADER IsShader;
		GEMGLFNGETSHADERIV GetShaderiv;
		GEMGLFNGETATTACHEDSHADERS GetAttachedShaders;
		GEMGLFNGETSHADERINFOLOG GetShaderInfoLog;
		//GEMGLFNGETSHADERSOURCE GetShaderSource;
		
		// program queries
		GEMGLFNISPROGRAM IsProgram;
		GEMGLFNGETPROGRAMIV GetProgramiv;
		GEMGLFNGETPROGRAMINFOLOG GetProgramInfoLog;

		// PER-FRAGMENT OPERATIONS
		
		// blending
		GEMGLFNBLENDEQUATION BlendEquation;
		GEMGLFNBLENDEQUATIONSEPARATE BlendEquationSeparate;
		GEMGLFNBLENDFUNCSEPARATE BlendFuncSeparate;
		GEMGLFNBLENDFUNC BlendFunc;
		GEMGLFNBLENDCOLOR BlendColor;
		
		// VERTEX ARRAY OBJECTS
		GEMGLFNGENVERTEXARRAYS GenVertexArrays;
		GEMGLFNBINDVERTEXARRAY BindVertexArray;
		GEMGLFNDELETEVERTEXARRAYS DeleteVertexArrays;
		GEMGLFNISVERTEXARRAY IsVertexArray;

		
		
#else
		//
		GEMGLFNVIEWPORT Viewport;
		GEMGLFNDEPTHRANGE DepthRange;
		GEMGLFNDRAWARRAYS DrawArrays;
		GEMGLFNDRAWELEMENTS DrawElements;
		GEMGLFNCLEARCOLOR ClearColor;
		GEMGLFNCLEAR Clear;
		GEMGLFNPOINTSIZE PointSize;
		GEMGLFNLINEWIDTH LineWidth;
		GEMGLFNFRONTFACE FrontFace;
		GEMGLFNCULLFACE CullFace;
		GEMGLFNENABLE Enable;
		GEMGLFNDISABLE Disable;
		GEMGLFNPIXELSTOREI PixelStorei;
		
		// textures
		GEMGLFNTEXPARAMETERF TexParameterf;
		GEMGLFNTEXPARAMETERFV TexParameterfv;
		GEMGLFNTEXPARAMETERI TexParameteri;
		GEMGLFNTEXPARAMETERIV TexParameteriv;
		GEMGLFNTEXIMAGE1D TexImage1D;
		GEMGLFNTEXIMAGE2D TexImage2D;
		GEMGLFNBINDTEXTURE BindTexture;
		GEMGLFNDELETETEXTURES DeleteTextures;
		GEMGLFNGENTEXTURES GenTextures;
		GEMGLFNISTEXTURE IsTexture;
		GEMGLFNACTIVETEXTURE ActiveTexture;
		GEMGLFNGENERATEMIPMAP GenerateMipmap;

		// shaders
        GEMGLFNCREATEPROGRAM CreateProgram;
        GEMGLFNDELETEPROGRAM DeleteProgram;
        GEMGLFNCREATESHADER CreateShader;
        GEMGLFNDELETESHADER DeleteShader;
        GEMGLFNLINKPROGRAM LinkProgram;
        GEMGLFNSHADERSOURCE ShaderSource;
        GEMGLFNUSEPROGRAM UseProgram;
        GEMGLFNCOMPILESHADER CompileShader;
        GEMGLFNATTACHSHADER AttachShader;
		GEMGLFNDETACHSHADER DetachShader;
		GEMGLFNISSHADER IsShader;
		GEMGLFNGETSHADERIV GetShaderiv;
        GEMGLFNGETSHADERINFOLOG GetShaderInfoLog;
		GEMGLFNGETPROGRAMINFOLOG GetProgramInfoLog;
		// shader execution
		GEMGLFNVALIDATEPROGRAM ValidateProgram;
		
		GEMGLFNISPROGRAM IsProgram;
		GEMGLFNGETPROGRAMIV GetProgramiv;
		GEMGLFNGETATTACHEDSHADERS GetAttachedShaders;

		GEMGLFNBINDATTRIBLOCATION BindAttribLocation;
		GEMGLFNBINDFRAGDATALOCATION BindFragDataLocation;

		// vertex arrays
		GEMGLFNBINDVERTEXARRAY BindVertexArray;
		GEMGLFNDELETEVERTEXARRAYS DeleteVertexArrays;
		GEMGLFNGENVERTEXARRAYS GenVertexArrays;
		GEMGLFNISVERTEXARRAY IsVertexArray;

		// attribs
		GEMGLFNGETACTIVEATTRIB GetActiveAttrib;
		GEMGLFNVERTEXATTRIBPOINTER VertexAttribPointer;
		GEMGLFNENABLEVERTEXATTRIBARRAY EnableVertexAttribArray;
		GEMGLFNDISABLEVERTEXATTRIBARRAY DisableVertexAttribArray;

		// uniform variables
		GEMGLFNGETUNIFORMLOCATION GetUniformLocation;
		GEMGLFNGETUNIFORMBLOCKINDEX GetUniformBlockIndex;
		GEMGLFNGETACTIVEUNIFORMBLOCKNAME GetActiveUniformBlockName;
		GEMGLFNGETACTIVEUNIFORMBLOCKIV GetActiveUniformBlockiv;
		GEMGLFNGETUNIFORMINDICES GetUniformIndices;
		GEMGLFNGETACTIVEUNIFORMNAME GetActiveUniformName;
		GEMGLFNGETACTIVEUNIFORM GetActiveUniform;
		GEMGLFNGETACTIVEUNIFORMSIV GetActiveUniformsiv;
		
		// buffers
		GEMGLFNBINDBUFFER BindBuffer;
		GEMGLFNBINDBUFFERRANGE BindBufferRange;
		GEMGLFNBINDBUFFERBASE BindBufferBase;		
		GEMGLFNDELETEBUFFERS DeleteBuffers;
		GEMGLFNGENBUFFERS GenBuffers;
		GEMGLFNISBUFFER IsBuffer;
		GEMGLFNBUFFERDATA BufferData;
		GEMGLFNBUFFERSUBDATA BufferSubData;
		GEMGLFNGETBUFFERSUBDATA GetBufferSubData;
		GEMGLFNMAPBUFFER MapBuffer;
		GEMGLFNUNMAPBUFFER UnmapBuffer;
		GEMGLFNGETBUFFERPARAMETERIV GetBufferParameteriv;
		GEMGLFNGETBUFFERPOINTERV GetBufferPointerv;

		// uniforms
		GEMGLFNUNIFORM1F Uniform1f;
		GEMGLFNUNIFORM2F Uniform2f;
		GEMGLFNUNIFORM3F Uniform3f;
		GEMGLFNUNIFORM4F Uniform4f;
		GEMGLFNUNIFORM1I Uniform1i;
		GEMGLFNUNIFORM2I Uniform2i;
		GEMGLFNUNIFORM3I Uniform3i;
		GEMGLFNUNIFORM4I Uniform4i;
		GEMGLFNUNIFORM1FV Uniform1fv;
		GEMGLFNUNIFORM2FV Uniform2fv;
		GEMGLFNUNIFORM3FV Uniform3fv;
		GEMGLFNUNIFORM4FV Uniform4fv;
		GEMGLFNUNIFORM1IV Uniform1iv;
		GEMGLFNUNIFORM2IV Uniform2iv;
		GEMGLFNUNIFORM3IV Uniform3iv;
		GEMGLFNUNIFORM4IV Uniform4iv;
		GEMGLFNUNIFORMMATRIX2FV UniformMatrix2fv;
		GEMGLFNUNIFORMMATRIX3FV UniformMatrix3fv;
		GEMGLFNUNIFORMMATRIX4FV UniformMatrix4fv;

		// PER-FRAGMENT OPERATIONS
		
		// blending
		GEMGLFNBLENDEQUATION BlendEquation;
		GEMGLFNBLENDEQUATIONSEPARATE BlendEquationSeparate;
		GEMGLFNBLENDFUNCSEPARATE BlendFuncSeparate;
		GEMGLFNBLENDFUNC BlendFunc;
		GEMGLFNBLENDCOLOR BlendColor;

	#if _WIN32
		GEMGLSWAPINTERVAL SwapInterval;
		GEMGLGETSWAPINTERVAL GetSwapInterval;
	#endif

		
		GEMGLFNBINDFRAMEBUFFER BindFramebuffer;
		GEMGLFNDELETEFRAMEBUFFERS DeleteFramebuffers;
		GEMGLFNGENFRAMEBUFFERS GenFramebuffers;
		
		GEMGLFNBINDRENDERBUFFER BindRenderbuffer;
		GEMGLFNDELETERENDERBUFFERS DeleteRenderBuffers;
		GEMGLFNGENRENDERBUFFERS GenRenderbuffers;
		GEMGLFNRENDERBUFFERSTORAGEMULTISAMPLE RenderbufferStorageMultisample;
		GEMGLFNRENDERBUFFERSTORAGE RenderbufferStorage;
		
		GEMGLFNFRAMEBUFFERRENDERBUFFER FramebufferRenderbuffer;
		GEMGLFNFRAMEBUFFERTEXTURE FramebufferTexture;
		GEMGLFNFRAMEBUFFERTEXTURE3D FramebufferTexture3D;
		GEMGLFNFRAMEBUFFERTEXTURE2D FramebufferTexture2D;
		GEMGLFNFRAMEBUFFERTEXTURE1D FramebufferTexture1D;
		GEMGLFNFRAMEBUFFERTEXTURELAYER FramebufferTextureLayer;
		
		GEMGLFNCHECKFRAMEBUFFERSTATUS CheckFramebufferStatus;
		GEMGLFNISFRAMEBUFFER IsFramebuffer;
		
		GEMGLFNGETFRAMEBUFFERATTACHMENTPARAMETERIV GetFramebufferAttachmentParameteriv;
		
		GEMGLFNISRENDERBUFFER IsRenderbuffer;
		GEMGLFNGETRENDERBUFFERPARAMETERIV GetRenderbufferParameteriv;	
		
		
		
#endif
		GEMGLFNGETSTRING GetString;
        GEMGL_CHECKERROR CheckError;


#if _WIN32 || LINUX
		xlib_t library;
#endif
	} gemgl_interface_t;

	struct gemgl_config
	{
		renderer::DriverType type;
		
		short major_version;
		short minor_version;
	}; // gemgl_config

	extern gemgl_interface_t gl;

	// init GL interface
	int gemgl_startup( gemgl_interface_t & gl_interface, gemgl_config & config );
	void gemgl_shutdown( gemgl_interface_t & gl_interface );
	void * gemgl_findsymbol( gemgl_interface_t & gl_interface, const char * symbol_name );
	const char * gemgl_uniform_to_string( GLenum type );

#if __APPLE__
	int gemgl_osx_startup( void );
	void gemgl_osx_shutdown( void );
	void * gemgl_native_findsymbol( const char * name );
#endif
