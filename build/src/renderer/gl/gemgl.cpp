// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include "gemgl.h"

#include <core/typedefs.h>
#include <core/logging.h>

#include <stdio.h> // for sscanf
#include <string.h> // for strstr

// uncomment this to dump extensions upon context creation
//#define GEMINI_SHOW_EXTENSIONS 1

namespace renderer
{
	gemgl_interface_t gl;

	typedef const GLubyte * (GEMGLAPI gemgl_GLGETSTRINGPROC)( GLenum param );

//#define GEMGL_LOG(...) NULL_MACRO
#define GEMGL_LOG(...) LOGV(__VA_ARGS__)

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
				GEMGL_LOG( "%s: (%s) %i\n", errorMessage, msg, e );
			}
			else
			{
				GEMGL_LOG( "%s: %i\n", errorMessage, e );
			}
		}

		return e;
	} // gemgl_check_error

	void gemgl_parse_version(short& major, short& minor)
	{
		int major_version = 0;
		int minor_version = 0;

		gemgl_GLGETSTRINGPROC gl_get_string = (gemgl_GLGETSTRINGPROC)gemgl_findsymbol(gl, "glGetString");
		if ( gl_get_string )
		{
			assert(gl_get_string != nullptr);
			const GLubyte * version = gl_get_string( GL_VERSION );
			if ( !version )
			{
				LOGE( "glGetString( GL_VERSION ) returned NULL!\n" );
				gemgl_check_error( "gemgl_parse_version" );
				return;
			}

			// TODO@APP: This needs to be replaced with a better method of parsing
			// the OpenGL version.
			if ( sscanf( (const char*)version, "%i.%i", &major_version, &minor_version ) < 2 )
			{
				if (sscanf((const char*)version, "OpenGL ES %i.%i", &major_version, &minor_version) < 2)
				{
					LOGE( "Error parsing OpenGL version\n" );
					return;
				}
			}
		}
		else
		{
			LOGE( "Unable to retrieve glGetString pointer!\n" );
		}

		major = (short)major_version;
		minor = (short)minor_version;
		LOGV("GL version detected: %i.%i\n", major_version, minor_version);
	} // gemgl_parse_version


	int gemgl_startup(gemgl_interface_t& gl_interface)
	{
		// load the platform-specific GL library
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
		const char* lib_name = "";

#if defined(PLATFORM_WINDOWS)
		lib_name = "OpenGL32.dll";
#elif defined(PLATFORM_RASPBERRYPI) || defined(PLATFORM_GLES2_SUPPORT)
	#if defined(PLATFORM_RASPBERRYPI)
		lib_name = "/opt/vc/lib/libGLESv2.so";
	#else
		lib_name = "libGLESv2.so";
	#endif
#elif defined(PLATFORM_LINUX)
		lib_name = "libGL.so";
#else
	#error Unknown platform!
#endif

		LOGV("Loading gl driver \"%s\"...\n", lib_name);

		// If you hit this assert, gemgl_startup is being called
		// more than once -- or without calling gemgl_shutdown in between.
		assert(gl_interface.library == 0);

		gl_interface.library = platform::dylib_open(lib_name);
		if (gl_interface.library == 0)
		{
			LOGV("Could not load gl driver: \"%s\"\n", lib_name);
			return 0;
		}
#elif PLATFORM_APPLE
		if ( !gemgl_osx_startup() )
		{
			LOGV( "OSX startup failed!\n" );
			return 0;
		}
#endif
		return 0;
	}


	int gemgl_load_symbols(gemgl_interface_t& gl_interface)
	{
#if GEMGL_ENABLE_ES
		GEMGL_LINK( gl.Viewport, "glViewport", GEMGLFNVIEWPORT );
		GEMGL_LINK( gl.DepthRange, "glDepthRangef", GEMGLFNDEPTHRANGEF );
		GEMGL_LINK( gl.ClearStencil, "glClearStencil", GEMGLFNCLEARSTENCIL );
		GEMGL_LINK( gl.ClearDepth, "glClearDepth", GEMGLFNCLEARDEPTH );
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
#if PLATFORM_GLES2_SUPPORT // Android and iOS should have support for these.
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



#else // Desktop OpenGL
		GEMGL_LINK( gl.Viewport, "glViewport", GEMGLFNVIEWPORT );
		GEMGL_LINK( gl.DepthRange, "glDepthRange", GEMGLFNDEPTHRANGE );
		GEMGL_LINK( gl.ClearStencil, "glClearStencil", GEMGLFNCLEARSTENCIL );
		GEMGL_LINK( gl.ClearDepth, "glClearDepth", GEMGLFNCLEARDEPTH );
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

#if defined(PLATFORM_OPENGL_SUPPORT)
		GEMGL_LINK( gl.QueryCounter, "glQueryCounter", GEMGLFNQUERYCOUNTER);
		GEMGL_LINK( gl.GetQueryObjecti64v, "glGetQueryObjecti64v", GEMGLFNGETQUERYOBJECTI64V);
		GEMGL_LINK( gl.GetQueryObjectui64v, "glGetQueryObjectui64v", GEMGLFNGETQUERYOBJECTUI64V);
#endif

		GEMGL_LINK( gl.DrawBuffers, "glDrawBuffers", GEMGLFNDRAWBUFFERS);

#if PLATFORM_WINDOWS
		GEMGL_LINK( gl.SwapInterval, "wglSwapIntervalEXT", GEMGLSWAPINTERVAL );
#endif
#endif

		GEMGL_LINK( gl.GetString, "glGetString", GEMGLFNGETSTRING );
		GEMGL_LINK( gl.GetStringi, "glGetStringi", GEMGLFNGETSTRINGI );

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

			GLint total_extensions = -1;
// only supported in GL 3.0+
#if defined(GEMINI_SHOW_EXTENSIONS) && defined(GL_NUM_EXTENSIONS) // not available for GLES2

			gl.GetIntegerv(GL_NUM_EXTENSIONS, &total_extensions);
			gl.CheckError("GetIntegerv");

	//		LOGV( "GL_EXTENSIONS: %s\n", gl.GetString(GL_EXTENSIONS) );
	//		gl.CheckError( "glGetString" );

			if (total_extensions > 0)
			{
				LOGV("GL_EXTENSIONS: (%i)\n", total_extensions);
				for (GLuint i = 0; i < static_cast<GLuint>(total_extensions); ++i)
				{
					LOGV("[%i] - %s\n", i, gl.GetStringi(GL_EXTENSIONS, i));
				}
			}

			if (total_extensions == -1)
			{
				LOGV("GL_EXTENSIONS: %s\n", gl.GetString(GL_EXTENSIONS));
				gl.CheckError("GetString(GL_EXTENSIONS)");
			}
#endif
		}

		return 1;
	} // gl_startup

	void* gemgl_findsymbol(gemgl_interface_t& gl_interface, const char* name)
	{
		void* ptr = 0;

		// check OS specific GL function first, then check the linked library
#if PLATFORM_WINDOWS
		ptr = wglGetProcAddress( name );
#elif PLATFORM_RASPBERRYPI
		// fall through
#elif PLATFORM_LINUX
		ptr = (void*)glXGetProcAddress( (const GLubyte*)name );
#elif PLATFORM_APPLE
		ptr = gemgl_native_findsymbol( name );
#elif PLATFORM_ANDROID
		// fall through
#else
		#error Unknown platform!
#endif

#if PLATFORM_WINDOWS || PLATFORM_LINUX || PLATFORM_ANDROID
		if (!ptr)
		{
			ptr = platform::dylib_find(gl_interface.library, name);
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

	void gemgl_shutdown(gemgl_interface_t& gl_interface)
	{
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
		// If you hit this, gemgl_shutdown was called more than once.
		assert(gl_interface.library != nullptr);
		platform::dylib_close(gl_interface.library);
		gl_interface.library = nullptr;
#elif defined(PLATFORM_APPLE)
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

#if !TARGET_OS_IPHONE && !defined(PLATFORM_ANDROID)
			case GL_SAMPLER_1D			: return "GL_SAMPLER_1D"; break;
			case GL_SAMPLER_3D			: return "GL_SAMPLER_3D"; break;
			case GL_SAMPLER_1D_SHADOW	: return "GL_SAMPLER_1D_SHADOW"; break;
			case GL_SAMPLER_2D_SHADOW	: return "GL_SAMPLER_2D_SHADOW"; break;
#endif
		}

		return "Unknown type!";
	} // gemgl_uniform_to_string


	bool gemgl_find_extension(const char* extension)
	{
		bool found_extension = false;
		const GLubyte* extension_string = gl.GetString(GL_EXTENSIONS);
		found_extension = (core::str::strstr((const char*)extension_string, extension) != 0);

		return found_extension;
	} // gemgl_find_extension
} // namespace renderer
