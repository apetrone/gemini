#include "gemgl.hpp"
//#include "platform.h"
#include <stdio.h> // for sscanf
#include <string.h> // for strstr
#include <xstr.h>
#include <log.h>

gemgl_interface_t gl;

typedef const GLubyte * (GEMGLAPI gemgl_GLGETSTRINGPROC)( GLenum param );

GLenum gemgl_check_error( const char * msg )
{
	GLenum e = glGetError();
	if ( e != GL_NO_ERROR )
	{
		const char * errorMessage = "gemgl_check_error";

		switch ( e )
		{
			case GL_INVALID_ENUM: errorMessage = "GL_INVALID_ENUM"; break;
			case GL_INVALID_OPERATION: errorMessage = "GL_INVALID_OPERATION"; break;
			case GL_INVALID_VALUE: errorMessage = "GL_INVALID_VALUE"; break;
			case GL_OUT_OF_MEMORY: errorMessage = "GL_OUT_OF_MEMORY"; break;
			default: break;
		}

		if ( msg != 0 )
		{
			LOGV( "%s: (%s) %i\n", errorMessage, msg, e );
		}
		else
		{
			LOGV( "%s: %i\n", errorMessage, e );
		}
	}
	
	return e;
} // gemgl_check_error

void gemgl_parse_version( int & major, int & minor, int renderer_type )
{
	gemgl_GLGETSTRINGPROC gl_get_string = (gemgl_GLGETSTRINGPROC)gemgl_findsymbol( gl, "glGetString" );
	if ( gl_get_string )
	{
		LOGV( "glGetString is %p\n", gl_get_string );
		const GLubyte * version = gl_get_string( GL_VERSION );
		if ( !version )
		{
			LOGE( "glGetString( GL_VERSION ) returned NULL!\n" );
			gemgl_check_error( "gemgl_parse_version" );
			return;
		}

#if GEMGL_ENABLE_ES
		// sscanf isn't working in iOS 5 Simulator. Well, there are multiple ways to skin a cat...
		if( !strstr( (const char*)version, "OpenGL ES 2.0" ) )
		{
			LOGE( "Error parsing OpenGL version\n" );
			return;
		}
		major = 2;
		minor = 0;
#else
		if ( sscanf( (const char*)version, "%d.%d", &major, &minor ) < 2 )
		{
			LOGE( "Error parsing OpenGL version\n" );
			return;
		}
#endif
	}
	else
	{
		LOGE( "Unable to retrieve glGetString pointer!\n" );
	}
} // gemgl_parse_version



int gemgl_startup( gemgl_interface_t & gl_interface, gemgl_config & config )
{
	int major = 0;
	int minor = 0;
	
	
#if _WIN32 || LINUX || __ANDROID__
	const char * libName = "";

#if _WIN32
	libName = "OpenGL32.dll";
#elif defined(PLATFORM_IS_RASPBERRYPI) || defined(PLATFORM_USE_GLES2)
	libName = "libGLESv2.so";
#elif LINUX
	libName = "libGL.so";
#endif

	LOGV( "Loading gl driver \"%s\"...\n", libName );

	if ( !xlib_open( &gl_interface.library, libName ) )
	{
		LOGV( "Could not load gl driver: \"%s\"\n", libName );
		return 0;
	}
#elif __APPLE__
	if ( !gemgl_osx_startup() )
	{
		LOGV( "OSX startup failed!\n" );
		return 0;
	}
#endif

	gemgl_parse_version( major, minor, config.type );
	

#if GEMGL_ENABLE_ES
#else
	if ( config.type == renderer::OpenGL )
	{
		if ( major < config.major_version || (major == config.major_version && minor < config.minor_version) )
		{
			LOGV( "Minimum OpenGL 3.2 required!\n" );
			return 0;
		}
	}
#endif

#if GEMGL_ENABLE_ES
	GEMGL_LINK( gl.Viewport, "glViewport", GEMGLFNVIEWPORT );
	GEMGL_LINK( gl.DepthRange, "glDepthRangef", GEMGLFNDEPTHRANGEF );
	GEMGL_LINK( gl.ClearColor, "glClearColor", GEMGLFNCLEARCOLOR );
	GEMGL_LINK( gl.Clear, "glClear", GEMGLFNCLEAR );
	GEMGL_LINK( gl.DrawArrays, "glDrawArrays", GEMGLFNDRAWARRAYS );
	GEMGL_LINK( gl.DrawElements, "glDrawElements", GEMGLFNDRAWELEMENTS );
	GEMGL_LINK( gl.LineWidth, "glLineWidth", GEMGLFNLINEWIDTH );
	GEMGL_LINK( gl.FrontFace, "glFrontFace", GEMGLFNFRONTFACE );
	GEMGL_LINK( gl.CullFace, "glCullFace", GEMGLFNCULLFACE );
	GEMGL_LINK( gl.Enable, "glEnable", GEMGLFNENABLE );
	GEMGL_LINK( gl.Disable, "glDisable", GEMGLFNDISABLE );
	GEMGL_LINK( gl.PolygonOffset, "glPolygonOffset", GEMGLFNPOLYGONOFFSET );
	GEMGL_LINK( gl.PixelStorei, "glPixelStorei", GEMGLFNPIXELSTOREI );
	GEMGL_LINK( gl.DepthMask, "glDepthMask", GEMGLFNDEPTHMASK );
	
	// textures
	GEMGL_LINK( gl.TexParameterf, "glTexParameterf", GEMGLFNTEXPARAMETERF );
	GEMGL_LINK( gl.TexParameterfv, "glTexParameterfv", GEMGLFNTEXPARAMETERFV );
	GEMGL_LINK( gl.TexParameteri, "glTexParameteri", GEMGLFNTEXPARAMETERI );
	GEMGL_LINK( gl.TexParameteriv, "glTexParameteriv", GEMGLFNTEXPARAMETERIV );
	GEMGL_LINK( gl.ActiveTexture, "glActiveTexture", GEMGLFNACTIVETEXTURE );
	GEMGL_LINK( gl.TexSubImage2D, "glTexSubImage2D", GEMGLFNTEXSUBIMAGE2D );
	GEMGL_LINK( gl.GenerateMipmap, "glGenerateMipmap", GEMGLFNGENERATEMIPMAP );		
	GEMGL_LINK( gl.TexImage2D, "glTexImage2D", GEMGLFNTEXIMAGE2D );
	GEMGL_LINK( gl.BindTexture, "glBindTexture", GEMGLFNBINDTEXTURE );
	GEMGL_LINK( gl.DeleteTextures, "glDeleteTextures", GEMGLFNDELETETEXTURES );
	GEMGL_LINK( gl.GenTextures, "glGenTextures", GEMGLFNGENTEXTURES );
	GEMGL_LINK( gl.IsTexture, "glIsTexture", GEMGLFNISTEXTURE );
	
	// attribs
	GEMGL_LINK( gl.VertexAttribPointer, "glVertexAttribPointer", GEMGLFNVERTEXATTRIBPOINTER );
	GEMGL_LINK( gl.EnableVertexAttribArray, "glEnableVertexAttribArray", GEMGLFNENABLEVERTEXATTRIBARRAY );
	GEMGL_LINK( gl.DisableVertexAttribArray, "glDisableVertexAttribArray", GEMGLFNDISABLEVERTEXATTRIBARRAY );
	GEMGL_LINK( gl.VertexAttrib1f, "glVertexAttrib1f", GEMGLFNVERTEXATTRIB1F );
	GEMGL_LINK( gl.VertexAttrib2f, "glVertexAttrib2f", GEMGLFNVERTEXATTRIB2F );
	GEMGL_LINK( gl.VertexAttrib3f, "glVertexAttrib3f", GEMGLFNVERTEXATTRIB3F );
	GEMGL_LINK( gl.VertexAttrib4f, "glVertexAttrib4f", GEMGLFNVERTEXATTRIB4F );
	GEMGL_LINK( gl.VertexAttrib1fv, "glVertexAttrib1fv", GEMGLFNVERTEXATTRIB1FV );
	GEMGL_LINK( gl.VertexAttrib2fv, "glVertexAttrib2fv", GEMGLFNVERTEXATTRIB2FV );
	GEMGL_LINK( gl.VertexAttrib3fv, "glVertexAttrib3fv", GEMGLFNVERTEXATTRIB3FV );
	GEMGL_LINK( gl.VertexAttrib4fv, "glVertexAttrib4fv", GEMGLFNVERTEXATTRIB4FV );
	
	// SHADERS AND PROGRAMS
	// shader objects
	GEMGL_LINK( gl.CreateShader, "glCreateShader", GEMGLFNCREATESHADER );
	GEMGL_LINK( gl.ShaderSource, "glShaderSource", GEMGLFNSHADERSOURCE );
	GEMGL_LINK( gl.CompileShader, "glCompileShader", GEMGLFNCOMPILESHADER );
	GEMGL_LINK( gl.ReleaseShaderCompiler, "glReleaseShaderCompiler", GEMGLFNRELEASESHADERCOMPILER );
	GEMGL_LINK( gl.DeleteShader, "glDeleteShader", GEMGLFNDELETESHADER );
	
	// loading shader binaries
	GEMGL_LINK( gl.ShaderBinary, "glShaderBinary", GEMGLFNSHADERBINARY );
	
	// program objects
	GEMGL_LINK( gl.CreateProgram, "glCreateProgram", GEMGLFNCREATEPROGRAM );
	GEMGL_LINK( gl.AttachShader, "glAttachShader", GEMGLFNATTACHSHADER );
	GEMGL_LINK( gl.DetachShader, "glDetachShader", GEMGLFNDETACHSHADER );
	GEMGL_LINK( gl.LinkProgram, "glLinkProgram", GEMGLFNLINKPROGRAM );
	GEMGL_LINK( gl.UseProgram, "glUseProgram", GEMGLFNUSEPROGRAM );
	GEMGL_LINK( gl.DeleteProgram, "glDeleteProgram", GEMGLFNDELETEPROGRAM );

	// SHADER VARIABLES
	
	// vertex attributes
	GEMGL_LINK( gl.GetActiveAttrib, "glGetActiveAttrib", GEMGLFNGETACTIVEATTRIB );
	GEMGL_LINK( gl.GetAttribLocation, "glGetAttribLocation", GEMGLFNGETATTRIBLOCATION );
	GEMGL_LINK( gl.BindAttribLocation, "glBindAttribLocation", GEMGLFNBINDATTRIBLOCATION );
	
	// uniform variables
	GEMGL_LINK( gl.GetUniformLocation, "glGetUniformLocation", GEMGLFNGETUNIFORMLOCATION );
	GEMGL_LINK( gl.GetActiveUniform, "glGetActiveUniform", GEMGLFNGETACTIVEUNIFORM );
	GEMGL_LINK( gl.GetActiveUniformsiv, "glGetActiveUniformsiv", GEMGLFNGETACTIVEUNIFORMSIV );
	GEMGL_LINK( gl.Uniform1f, "glUniform1f", GEMGLFNUNIFORM1F );
	GEMGL_LINK( gl.Uniform2f, "glUniform2f", GEMGLFNUNIFORM2F );
	GEMGL_LINK( gl.Uniform3f, "glUniform3f", GEMGLFNUNIFORM3F );
	GEMGL_LINK( gl.Uniform4f, "glUniform4f", GEMGLFNUNIFORM4F );
	GEMGL_LINK( gl.Uniform1i, "glUniform1i", GEMGLFNUNIFORM1I );
	GEMGL_LINK( gl.Uniform2i, "glUniform2i", GEMGLFNUNIFORM2I );
	GEMGL_LINK( gl.Uniform3i, "glUniform3i", GEMGLFNUNIFORM3I );
	GEMGL_LINK( gl.Uniform4i, "glUniform4i", GEMGLFNUNIFORM4I );
	GEMGL_LINK( gl.Uniform1fv, "glUniform1fv", GEMGLFNUNIFORM1FV );
	GEMGL_LINK( gl.Uniform2fv, "glUniform2fv", GEMGLFNUNIFORM2FV );
	GEMGL_LINK( gl.Uniform3fv, "glUniform3fv", GEMGLFNUNIFORM3FV );
	GEMGL_LINK( gl.Uniform4fv, "glUniform4fv", GEMGLFNUNIFORM4FV );
	GEMGL_LINK( gl.Uniform1iv, "glUniform1iv", GEMGLFNUNIFORM1IV );
	GEMGL_LINK( gl.Uniform2iv, "glUniform2iv", GEMGLFNUNIFORM2IV );
	GEMGL_LINK( gl.Uniform3iv, "glUniform3iv", GEMGLFNUNIFORM3IV );
	GEMGL_LINK( gl.Uniform4iv, "glUniform4iv", GEMGLFNUNIFORM4IV );
	GEMGL_LINK( gl.UniformMatrix2fv, "glUniformMatrix2fv", GEMGLFNUNIFORMMATRIX2FV );
	GEMGL_LINK( gl.UniformMatrix3fv, "glUniformMatrix3fv", GEMGLFNUNIFORMMATRIX3FV );
	GEMGL_LINK( gl.UniformMatrix4fv, "glUniformMatrix4fv", GEMGLFNUNIFORMMATRIX4FV );
	
	// shader execution
	GEMGL_LINK( gl.ValidateProgram, "glValidateProgram", GEMGLFNVALIDATEPROGRAM );
	
	// SHADER QUERIES
	// shader queries
	GEMGL_LINK( gl.IsShader, "glIsShader", GEMGLFNISSHADER );
	GEMGL_LINK( gl.GetShaderiv, "glGetShaderiv", GEMGLFNGETSHADERIV );
	GEMGL_LINK( gl.GetAttachedShaders, "glGetAttachedShaders", GEMGLFNGETATTACHEDSHADERS );
	GEMGL_LINK( gl.GetShaderInfoLog, "glGetShaderInfoLog", GEMGLFNGETSHADERINFOLOG );
	//GEMGL_LINK( gl.GetShaderSource, "glGetShaderSource", GEMGLFNGETSHADERSOURCE );
	
	// program queries
	GEMGL_LINK( gl.IsProgram, "glIsProgram", GEMGLFNISPROGRAM );
	GEMGL_LINK( gl.GetProgramiv, "glGetProgramiv", GEMGLFNGETPROGRAMIV );
	GEMGL_LINK( gl.GetProgramInfoLog, "glGetProgramInfoLog", GEMGLFNGETPROGRAMINFOLOG );
	

	// PER-FRAGMENT OPERATIONS
	// blending
	
	GEMGL_LINK( gl.BlendEquation, "glBlendEquation", GEMGLFNBLENDEQUATION );	
	GEMGL_LINK( gl.BlendEquationSeparate, "glBlendEquationSeparate", GEMGLFNBLENDEQUATIONSEPARATE );
	GEMGL_LINK( gl.BlendFuncSeparate, "glBlendFuncSeparate", GEMGLFNBLENDFUNCSEPARATE );
	GEMGL_LINK( gl.BlendFunc, "glBlendFunc", GEMGLFNBLENDFUNC );
	GEMGL_LINK( gl.BlendColor, "glBlendColor", GEMGLFNBLENDCOLOR );
	
	// vertex array objects
#if PLATFORM_IS_MOBILE // Android and iOS should have support for these.
	GEMGL_LINK( gl.GenVertexArrays, "glGenVertexArraysOES", GEMGLFNGENVERTEXARRAYS );		
	GEMGL_LINK( gl.BindVertexArray, "glBindVertexArrayOES", GEMGLFNBINDVERTEXARRAY );
	GEMGL_LINK( gl.DeleteVertexArrays, "glDeleteVertexArraysOES", GEMGLFNDELETEVERTEXARRAYS );
	GEMGL_LINK( gl.IsVertexArray, "glIsVertexArrayOES", GEMGLFNISVERTEXARRAY );
#endif

	// BUFFER OBJECTS
	GEMGL_LINK( gl.GenBuffers, "glGenBuffers", GEMGLFNGENBUFFERS );
	GEMGL_LINK( gl.BindBuffer, "glBindBuffer", GEMGLFNBINDBUFFER );
	GEMGL_LINK( gl.DeleteBuffers, "glDeleteBuffers", GEMGLFNDELETEBUFFERS );
	GEMGL_LINK( gl.IsBuffer, "glIsBuffer", GEMGLFNISBUFFER );
	GEMGL_LINK( gl.BufferData, "glBufferData", GEMGLFNBUFFERDATA );
	GEMGL_LINK( gl.BufferSubData, "glBufferSubData", GEMGLFNBUFFERSUBDATA );
	GEMGL_LINK( gl.GetBufferParameteriv, "glGetBufferParameteriv", GEMGLFNGETBUFFERPARAMETERIV );

	

#else
	GEMGL_LINK( gl.Viewport, "glViewport", GEMGLFNVIEWPORT );
	GEMGL_LINK( gl.DepthRange, "glDepthRange", GEMGLFNDEPTHRANGE );
	GEMGL_LINK( gl.ClearColor, "glClearColor", GEMGLFNCLEARCOLOR );
	GEMGL_LINK( gl.Clear, "glClear", GEMGLFNCLEAR );		
	GEMGL_LINK( gl.DrawArrays, "glDrawArrays", GEMGLFNDRAWARRAYS );
	GEMGL_LINK( gl.DrawElements, "glDrawElements", GEMGLFNDRAWELEMENTS );
	GEMGL_LINK( gl.PointSize, "glPointSize", GEMGLFNPOINTSIZE );
	GEMGL_LINK( gl.LineWidth, "glLineWidth", GEMGLFNLINEWIDTH );
	GEMGL_LINK( gl.FrontFace, "glFrontFace", GEMGLFNFRONTFACE );
	GEMGL_LINK( gl.CullFace, "glCullFace", GEMGLFNCULLFACE );
	GEMGL_LINK( gl.Enable, "glEnable", GEMGLFNENABLE );
	GEMGL_LINK( gl.Disable, "glDisable", GEMGLFNDISABLE );
	GEMGL_LINK( gl.PixelStorei, "glPixelStorei", GEMGLFNPIXELSTOREI );
	GEMGL_LINK( gl.DepthMask, "glDepthMask", GEMGLFNDEPTHMASK );

	// textures
	GEMGL_LINK( gl.TexParameterf, "glTexParameterf", GEMGLFNTEXPARAMETERF );
	GEMGL_LINK( gl.TexParameterfv, "glTexParameterfv", GEMGLFNTEXPARAMETERFV );
	GEMGL_LINK( gl.TexParameteri, "glTexParameteri", GEMGLFNTEXPARAMETERI );
	GEMGL_LINK( gl.TexParameteriv, "glTexParameteriv", GEMGLFNTEXPARAMETERIV );
	GEMGL_LINK( gl.TexImage1D, "glTexImage1D", GEMGLFNTEXIMAGE1D );
	GEMGL_LINK( gl.TexImage2D, "glTexImage2D", GEMGLFNTEXIMAGE2D );
	GEMGL_LINK( gl.GenerateMipmap, "glGenerateMipmap", GEMGLFNGENERATEMIPMAP );
	GEMGL_LINK( gl.BindTexture, "glBindTexture", GEMGLFNBINDTEXTURE );
	GEMGL_LINK( gl.DeleteTextures, "glDeleteTextures", GEMGLFNDELETETEXTURES );
	GEMGL_LINK( gl.GenTextures, "glGenTextures", GEMGLFNGENTEXTURES );
	GEMGL_LINK( gl.IsTexture, "glIsTexture", GEMGLFNISTEXTURE );
	GEMGL_LINK( gl.ActiveTexture, "glActiveTexture", GEMGLFNACTIVETEXTURE );
	GEMGL_LINK( gl.TexSubImage2D, "glTexSubImage2D", GEMGLFNTEXSUBIMAGE2D );

	// shaders
	GEMGL_LINK( gl.CreateProgram, "glCreateProgram", GEMGLFNCREATEPROGRAM );
	GEMGL_LINK( gl.DeleteProgram, "glDeleteProgram", GEMGLFNDELETEPROGRAM );
	GEMGL_LINK( gl.CreateShader, "glCreateShader", GEMGLFNCREATESHADER );
	GEMGL_LINK( gl.DeleteShader, "glDeleteShader", GEMGLFNDELETESHADER );
	GEMGL_LINK( gl.LinkProgram, "glLinkProgram", GEMGLFNLINKPROGRAM );
	GEMGL_LINK( gl.ShaderSource, "glShaderSource", GEMGLFNSHADERSOURCE );
	GEMGL_LINK( gl.UseProgram, "glUseProgram", GEMGLFNUSEPROGRAM );
	GEMGL_LINK( gl.CompileShader, "glCompileShader", GEMGLFNCOMPILESHADER );
	GEMGL_LINK( gl.AttachShader, "glAttachShader", GEMGLFNATTACHSHADER );
	GEMGL_LINK( gl.DetachShader, "glDetachShader", GEMGLFNDETACHSHADER );
	GEMGL_LINK( gl.IsShader, "glIsShader", GEMGLFNISSHADER );
	GEMGL_LINK( gl.GetShaderiv, "glGetShaderiv", GEMGLFNGETSHADERIV );
	GEMGL_LINK( gl.GetShaderInfoLog, "glGetShaderInfoLog", GEMGLFNGETSHADERINFOLOG );
	GEMGL_LINK( gl.GetProgramInfoLog, "glGetProgramInfoLog", GEMGLFNGETPROGRAMINFOLOG );
	// shader execution
	GEMGL_LINK( gl.ValidateProgram, "glValidateProgram", GEMGLFNVALIDATEPROGRAM );
	GEMGL_LINK( gl.IsProgram, "glIsProgram", GEMGLFNISPROGRAM );
	GEMGL_LINK( gl.GetProgramiv, "glGetProgramiv", GEMGLFNGETPROGRAMIV );
	GEMGL_LINK( gl.GetAttachedShaders, "glGetAttachedShaders", GEMGLFNGETATTACHEDSHADERS );
	
	// attrib
	GEMGL_LINK( gl.GetActiveAttrib, "glGetActiveAttrib", GEMGLFNGETACTIVEATTRIB );
	GEMGL_LINK( gl.GetAttribLocation, "glGetAttribLocation", GEMGLFNGETATTRIBLOCATION );
	GEMGL_LINK( gl.BindAttribLocation, "glBindAttribLocation", GEMGLFNBINDATTRIBLOCATION );
	GEMGL_LINK( gl.BindFragDataLocation, "glBindFragDataLocation", GEMGLFNBINDFRAGDATALOCATION );
	
	// uniforms
	GEMGL_LINK( gl.GetUniformLocation, "glGetUniformLocation", GEMGLFNGETUNIFORMLOCATION );
	GEMGL_LINK( gl.GetUniformBlockIndex, "glGetUniformBlockIndex", GEMGLFNGETUNIFORMBLOCKINDEX );
	GEMGL_LINK( gl.GetActiveUniformBlockName, "glGetActiveUniformBlockName", GEMGLFNGETACTIVEUNIFORMBLOCKNAME );
	GEMGL_LINK( gl.GetActiveUniformBlockiv, "glGetActiveUniformBlockiv", GEMGLFNGETACTIVEUNIFORMBLOCKIV );
	GEMGL_LINK( gl.GetUniformIndices, "glGetUniformIndices", GEMGLFNGETUNIFORMINDICES );
	GEMGL_LINK( gl.GetActiveUniformName, "glGetActiveUniformName", GEMGLFNGETACTIVEUNIFORMNAME );
	GEMGL_LINK( gl.GetActiveUniform, "glGetActiveUniform", GEMGLFNGETACTIVEUNIFORM );
	GEMGL_LINK( gl.GetActiveUniformsiv, "glGetActiveUniformsiv", GEMGLFNGETACTIVEUNIFORMSIV );
	
	// vertex array objects
	GEMGL_LINK( gl.BindVertexArray, "glBindVertexArray", GEMGLFNBINDVERTEXARRAY );
	GEMGL_LINK( gl.DeleteVertexArrays, "glDeleteVertexArrays", GEMGLFNDELETEVERTEXARRAYS );
	GEMGL_LINK( gl.GenVertexArrays, "glGenVertexArrays", GEMGLFNGENVERTEXARRAYS );
	GEMGL_LINK( gl.IsVertexArray, "glIsVertexArray", GEMGLFNISVERTEXARRAY );

	GEMGL_LINK( gl.VertexAttribPointer, "glVertexAttribPointer", GEMGLFNVERTEXATTRIBPOINTER );
	GEMGL_LINK( gl.EnableVertexAttribArray, "glEnableVertexAttribArray", GEMGLFNENABLEVERTEXATTRIBARRAY );
	GEMGL_LINK( gl.DisableVertexAttribArray, "glDisableVertexAttribArray", GEMGLFNDISABLEVERTEXATTRIBARRAY );

	GEMGL_LINK( gl.BindBuffer, "glBindBuffer", GEMGLFNBINDBUFFER );
	GEMGL_LINK( gl.BindBufferRange, "glBindBufferRange", GEMGLFNBINDBUFFERRANGE );
	GEMGL_LINK( gl.BindBufferBase, "glBindBufferBase", GEMGLFNBINDBUFFERBASE );
	GEMGL_LINK( gl.DeleteBuffers, "glDeleteBuffers", GEMGLFNDELETEBUFFERS );
	GEMGL_LINK( gl.GenBuffers, "glGenBuffers", GEMGLFNGENBUFFERS );
	GEMGL_LINK( gl.IsBuffer, "glIsBuffer", GEMGLFNISBUFFER );
	GEMGL_LINK( gl.BufferData, "glBufferData", GEMGLFNBUFFERDATA );
	GEMGL_LINK( gl.BufferSubData, "glBufferSubData", GEMGLFNBUFFERSUBDATA );
	GEMGL_LINK( gl.GetBufferSubData, "glGetBufferSubData", GEMGLFNGETBUFFERSUBDATA );
	GEMGL_LINK( gl.MapBuffer, "glMapBuffer", GEMGLFNMAPBUFFER );
	GEMGL_LINK( gl.UnmapBuffer, "glUnmapBuffer", GEMGLFNUNMAPBUFFER );
	GEMGL_LINK( gl.GetBufferParameteriv, "glGetBufferParameteriv", GEMGLFNGETBUFFERPARAMETERIV );
	GEMGL_LINK( gl.GetBufferPointerv, "glGetBufferPointerv", GEMGLFNGETBUFFERPOINTERV );

	// uniforms
	GEMGL_LINK( gl.Uniform1f, "glUniform1f", GEMGLFNUNIFORM1F );
	GEMGL_LINK( gl.Uniform2f, "glUniform2f", GEMGLFNUNIFORM2F );
	GEMGL_LINK( gl.Uniform3f, "glUniform3f", GEMGLFNUNIFORM3F );
	GEMGL_LINK( gl.Uniform4f, "glUniform4f", GEMGLFNUNIFORM4F );
	GEMGL_LINK( gl.Uniform1i, "glUniform1i", GEMGLFNUNIFORM1I );
	GEMGL_LINK( gl.Uniform2i, "glUniform2i", GEMGLFNUNIFORM2I );
	GEMGL_LINK( gl.Uniform3i, "glUniform3i", GEMGLFNUNIFORM3I );
	GEMGL_LINK( gl.Uniform4i, "glUniform4i", GEMGLFNUNIFORM4I );
	GEMGL_LINK( gl.Uniform1fv, "glUniform1fv", GEMGLFNUNIFORM1FV );
	GEMGL_LINK( gl.Uniform2fv, "glUniform2fv", GEMGLFNUNIFORM2FV );
	GEMGL_LINK( gl.Uniform3fv, "glUniform3fv", GEMGLFNUNIFORM3FV );
	GEMGL_LINK( gl.Uniform4fv, "glUniform4fv", GEMGLFNUNIFORM4FV );
	GEMGL_LINK( gl.Uniform1iv, "glUniform1iv", GEMGLFNUNIFORM1IV );
	GEMGL_LINK( gl.Uniform2iv, "glUniform2iv", GEMGLFNUNIFORM2IV );
	GEMGL_LINK( gl.Uniform3iv, "glUniform3iv", GEMGLFNUNIFORM3IV );
	GEMGL_LINK( gl.Uniform4iv, "glUniform4iv", GEMGLFNUNIFORM4IV );
	GEMGL_LINK( gl.UniformMatrix2fv, "glUniformMatrix2fv", GEMGLFNUNIFORMMATRIX2FV );
	GEMGL_LINK( gl.UniformMatrix3fv, "glUniformMatrix3fv", GEMGLFNUNIFORMMATRIX3FV );
	GEMGL_LINK( gl.UniformMatrix4fv, "glUniformMatrix4fv", GEMGLFNUNIFORMMATRIX4FV );

	
	// PER-FRAGMENT OPERATIONS
	// blending
	
	GEMGL_LINK( gl.BlendEquation, "glBlendEquation", GEMGLFNBLENDEQUATION );	
	GEMGL_LINK( gl.BlendEquationSeparate, "glBlendEquationSeparate", GEMGLFNBLENDEQUATIONSEPARATE );
	GEMGL_LINK( gl.BlendFuncSeparate, "glBlendFuncSeparate", GEMGLFNBLENDFUNCSEPARATE );
	GEMGL_LINK( gl.BlendFunc, "glBlendFunc", GEMGLFNBLENDFUNC );
	GEMGL_LINK( gl.BlendColor, "glBlendColor", GEMGLFNBLENDCOLOR );

	GEMGL_LINK( gl.BindFramebuffer, "glBindFramebuffer", GEMGLFNBINDFRAMEBUFFER );
	GEMGL_LINK( gl.DeleteFramebuffers, "glDeleteFramebuffers", GEMGLFNDELETEFRAMEBUFFERS );
	GEMGL_LINK( gl.GenFramebuffers, "glGenFramebuffers", GEMGLFNGENFRAMEBUFFERS );
	GEMGL_LINK( gl.BindRenderbuffer, "glBindRenderbuffer", GEMGLFNBINDRENDERBUFFER );
	GEMGL_LINK( gl.DeleteRenderbuffers, "glDeleteRenderbuffers", GEMGLFNDELETERENDERBUFFERS );
	GEMGL_LINK( gl.GenRenderbuffers, "glGenRenderbuffers", GEMGLFNGENRENDERBUFFERS );
	GEMGL_LINK( gl.RenderbufferStorageMultisample, "glRenderbufferStorageMultisample", GEMGLFNRENDERBUFFERSTORAGEMULTISAMPLE );
	GEMGL_LINK( gl.RenderbufferStorage, "glRenderbufferStorage", GEMGLFNRENDERBUFFERSTORAGE );
	GEMGL_LINK( gl.FramebufferRenderbuffer, "glFramebufferRenderbuffer", GEMGLFNFRAMEBUFFERRENDERBUFFER );
	GEMGL_LINK( gl.FramebufferTexture, "glFramebufferTexture", GEMGLFNFRAMEBUFFERTEXTURE );
	GEMGL_LINK( gl.FramebufferTexture3D, "glFramebufferTexture3D", GEMGLFNFRAMEBUFFERTEXTURE3D );
	GEMGL_LINK( gl.FramebufferTexture2D, "glFramebufferTexture2D", GEMGLFNFRAMEBUFFERTEXTURE2D );
	GEMGL_LINK( gl.FramebufferTexture1D, "glFramebufferTexture1D", GEMGLFNFRAMEBUFFERTEXTURE1D );
	GEMGL_LINK( gl.FramebufferTextureLayer, "glFramebufferTextureLayer", GEMGLFNFRAMEBUFFERTEXTURELAYER );
	GEMGL_LINK( gl.CheckFramebufferStatus, "glCheckFramebufferStatus", GEMGLFNCHECKFRAMEBUFFERSTATUS );
	GEMGL_LINK( gl.IsFramebuffer, "glIsFramebuffer", GEMGLFNISFRAMEBUFFER );

	GEMGL_LINK( gl.GetFramebufferAttachmentParameteriv, "glGetFramebufferAttachmentParameteriv", GEMGLFNGETFRAMEBUFFERATTACHMENTPARAMETERIV );
	
	GEMGL_LINK( gl.IsRenderbuffer, "glIsRenderbuffer", GEMGLFNISRENDERBUFFER );
	GEMGL_LINK( gl.GetRenderbufferParameteriv, "glGetRenderbufferParameteriv", GEMGLFNGETRENDERBUFFERPARAMETERIV );	

	
#if _WIN32
	GEMGL_LINK( gl.SwapInterval, "wglSwapIntervalEXT", GEMGLSWAPINTERVAL );
#endif
#endif

	GEMGL_LINK( gl.GetString, "glGetString", GEMGLFNGETSTRING );

	// state queries
	GEMGL_LINK( gl.GetBooleanv, "glGetBooleanv", GEMGLFNGETBOOLEANV );
	GEMGL_LINK( gl.GetIntegerv, "glGetIntegerv", GEMGLFNGETINTEGERV );
	GEMGL_LINK( gl.GetFloatv, "glGetFloatv", GEMGLFNGETFLOATV );

	// link internal functions
	gl.CheckError = gemgl_check_error;

	assert( gl.GetString != 0 );
	if ( gl.GetString )
	{
		LOGV( "GL_VENDOR: %s\n", gl.GetString(GL_VENDOR) );
		gl.CheckError( "glGetString" );
		
		LOGV( "GL_RENDERER: %s\n", gl.GetString(GL_RENDERER) );
		gl.CheckError( "glGetString" );
		
		LOGV( "GL_VERSION: %s\n", gl.GetString(GL_VERSION) );
		gl.CheckError( "glGetString" );
		
		LOGV( "GL_SHADING_LANGUAGE_VERSION: %s\n", gl.GetString(GL_SHADING_LANGUAGE_VERSION) );
		gl.CheckError( "glGetString" );
		
		LOGV( "GL_EXTENSIONS: %s\n", gl.GetString(GL_EXTENSIONS) );
		gl.CheckError( "glGetString" );
	}

	return 1;
} // gl_startup

void * gemgl_findsymbol( gemgl_interface_t & gl_interface, const char * name )
{
	void * ptr = 0;

	// check OS specific GL function first, then check the linked library
#if _WIN32
	ptr = wglGetProcAddress( name );
#elif PLATFORM_IS_RASPBERRYPI
	// fall through
#elif LINUX
	ptr = (void*)glXGetProcAddress( (const GLubyte*)name );
#elif __APPLE__
	ptr = gemgl_native_findsymbol( name );
#elif __ANDROID__
	// fall through
#else
	#error Unknown platform!
#endif

#if _WIN32 || LINUX || __ANDROID__
	if ( !ptr )
	{
		ptr = xlib_find_symbol( &gl_interface.library, name );
	}
#endif

#if GEMGL_DEBUG_SYMBOLS
	if ( !ptr )
	{
		LOGV( "Could not locate symbol \"%s\"!\n", name );
	}
	else
	{
//		LOGV( "Linked function %s -> %lx\n", name, (long unsigned int)ptr );
	}
#endif
	return ptr;
} // gem_glfindsymbol

void gemgl_shutdown( gemgl_interface_t & gl_interface  )
{
#if _WIN32 || LINUX || __ANDROID__
	xlib_close( &gl_interface.library );
#elif __APPLE__
	gemgl_osx_shutdown();
#endif
} // gemgl_shutdown

const char * gemgl_uniform_to_string( GLenum type )
{
	switch( type )
	{
		case GL_FLOAT				: return "GL_FLOAT"; break;
		case GL_FLOAT_VEC2			: return "GL_FLOAT_VEC2"; break;
		case GL_FLOAT_VEC3			: return "GL_FLOAT_VEC3"; break;
		case GL_FLOAT_VEC4			: return "GL_FLOAT_VEC4"; break;
		case GL_INT					: return "GL_INT"; break;
		case GL_INT_VEC2			: return "GL_INT_VEC2"; break;
		case GL_INT_VEC3			: return "GL_INT_VEC3"; break;
		case GL_INT_VEC4			: return "GL_INT_VEC4"; break;
		case GL_BOOL				: return "GL_BOOL"; break;
		case GL_BOOL_VEC2			: return "GL_BOOL_VEC2"; break;
		case GL_BOOL_VEC3			: return "GL_BOOL_VEC3"; break;
		case GL_BOOL_VEC4			: return "GL_BOOL_VEC4"; break;
		case GL_FLOAT_MAT2			: return "GL_FLOAT_MAT2"; break;
		case GL_FLOAT_MAT3			: return "GL_FLOAT_MAT3"; break;
		case GL_FLOAT_MAT4			: return "GL_FLOAT_MAT4"; break;
		case GL_SAMPLER_2D			: return "GL_SAMPLER_2D"; break;
		case GL_SAMPLER_CUBE		: return "GL_SAMPLER_CUBE"; break;
			
#if !TARGET_OS_IPHONE && !defined(__ANDROID__)
		case GL_SAMPLER_1D			: return "GL_SAMPLER_1D"; break;					
		case GL_SAMPLER_3D			: return "GL_SAMPLER_3D"; break;					
		case GL_SAMPLER_1D_SHADOW	: return "GL_SAMPLER_1D_SHADOW"; break;
		case GL_SAMPLER_2D_SHADOW	: return "GL_SAMPLER_2D_SHADOW"; break;
#endif
	}
	
	return "Unknown type!";
} // gemgl_uniform_to_string


bool gemgl_find_extension( const char * extension )
{
	bool found_extension = false;
	const GLubyte * extension_string = gl.GetString( GL_EXTENSIONS );
	found_extension = (xstr_str( (const char*)extension_string, extension ) != 0);
	
	return found_extension;
} // gemgl_find_extension
