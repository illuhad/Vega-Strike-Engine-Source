/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
 * 
 * http://vegastrike.sourceforge.net/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "vegastrike.h"
#include "gfxlib.h"
#include "gl_globals.h"
#include "vs_globals.h"
const int  MAX_TEXTURES = 256;

struct GLTexture{
  unsigned char *texture;
  GLubyte * palette;
  unsigned int width;
  unsigned int height;
  int texturestage;
  GLuint name;
  GFXBOOL alive;
  TEXTUREFORMAT textureformat;
  GLenum targets;
  enum FILTER mipmapped;
  GFXBOOL shared_palette;
  
	GLTexture ()
	{
		alive = GFXFALSE;
		name = 0;
		width =0;
		height = 0;
		texture = NULL;
		palette = NULL;
		shared_palette = GFXTRUE;
	}
	~GLTexture()
	{
	  if (palette) {
			delete [] palette;
			palette = NULL;
	  }
	}
};
//static GLTexture *textures=NULL;
//static GLEnum * targets=NULL;

static GLTexture textures[MAX_TEXTURES];
static int activetexture[4]={-1,-1};
static void ConvertPalette(unsigned char *dest, unsigned char *src)
{
  for(int a=0; a<256; a++, dest+=4, src+=4) {
		memcpy(dest, src, 3);
		dest[3] = 255;
  }

}

GFXBOOL /*GFXDRVAPI*/ GFXCreateTexture(int width, int height, TEXTUREFORMAT textureformat, int *handle, char *palette , int texturestage, enum FILTER mipmap, enum TEXTURE_TARGET texture_target)
{
  //  if (!textures) {
  //    textures = new GLTexture [MAX_TEXTURES]; //if the dynamically allocated array is not made... make it
  //    targets = new GLEnum [MAX_TEXTURES];
  //  }
  if (g_game.Multitexture)
    {
		switch (texturestage )
		{
		case 0:
			glActiveTextureARB(GL_TEXTURE0_ARB);
			break;
		case 1:
			glActiveTextureARB(GL_TEXTURE1_ARB);
			break;
		//case 3:  ... 3 pass... are you insane? well look who's talking to himself! oh.. good point :)
		default:
			glActiveTextureARB(GL_TEXTURE0_ARB);
			break;
		}
	}
	
	*handle = 0;

	while (*handle<MAX_TEXTURES&&textures[*handle].alive)
		(*handle)++;
	if (*handle==MAX_TEXTURES)
		return GFXFALSE;
	GLenum WrapMode;
	switch (texture_target) {
	case TEXTURE2D: textures [*handle].targets=GL_TEXTURE_2D;
	  WrapMode = GL_REPEAT;
	  break;
	case CUBEMAP: textures [*handle].targets=GL_TEXTURE_CUBE_MAP_EXT;
	  WrapMode = GL_CLAMP;
	  fprintf (stderr, "stage %d, wid %d, hei %d",texturestage,width,height);
	  break;
	}
	
	textures[*handle].name = *handle; //for those libs with stubbed out handle gen't
	//fprintf (stderr,"Texture Handle %d",*handle);
	textures[*handle].alive = GFXTRUE;
	textures[*handle].texturestage = texturestage;
	textures[*handle].mipmapped = mipmap;
	glGenTextures (1,&textures[*handle].name);
	
	glTexParameteri(textures[*handle].targets, GL_TEXTURE_WRAP_S, WrapMode);
	glTexParameteri(textures[*handle].targets, GL_TEXTURE_WRAP_T, WrapMode);

	

	if (textures[*handle].mipmapped&(TRILINEAR|MIPMAP)&&g_game.mipmap>=2) {
	  glTexParameteri (textures[*handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  if (textures[*handle].mipmapped&TRILINEAR) {
	    glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	  }else {
	    glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	  }
	} else {
	  if (textures[*handle].mipmapped==NEAREST||g_game.mipmap==0) {
	    glTexParameteri (textures[*handle].targets, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	  } else {
	    glTexParameteri (textures[*handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  }
	}
	glTexParameterf (textures[*handle].targets,GL_TEXTURE_PRIORITY,.5);
	textures[*handle].width = width;
	textures[*handle].height = height;
	if (palette&&textureformat == PALETTE8)
	{
		textures[*handle].palette = new GLubyte [1024];
		ConvertPalette(textures[*handle].palette, (unsigned char *)palette);
		//memcpy (textures[*handle].palette,palette,768);
	}
	textures[*handle].textureformat = textureformat;

	return GFXTRUE;
}
void GFXPrioritizeTexture (unsigned int handle, float priority) {
  glPrioritizeTextures (1,&handle,&priority); 
}
void /*GFXDRVAPI*/ GFXAttachPalette (unsigned char *palette, int handle)
{
	ConvertPalette(textures[handle].palette, palette);
	//memcpy (textures[handle].palette,palette,768);
}
GFXBOOL /*GFXDRVAPI*/ GFXTransferTexture (unsigned char *buffer, int handle,  enum TEXTURE_IMAGE_TARGET imagetarget)
{	
  GLenum image2D;
  switch (imagetarget) {
  case TEXTURE_2D:
    image2D = GL_TEXTURE_2D;
    break;
  case CUBEMAP_POSITIVE_X:
    image2D = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT;
    break;
  case CUBEMAP_NEGATIVE_X:
    image2D=GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT;
    break;
  case CUBEMAP_POSITIVE_Y:
    image2D = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT;
    break;
  case CUBEMAP_NEGATIVE_Y:
    image2D=GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT;
    break;
  case CUBEMAP_POSITIVE_Z:
    image2D = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT;
    break;
  case CUBEMAP_NEGATIVE_Z:
    image2D=GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT;
    break;
  }
  if (image2D!=GL_TEXTURE_2D) {
    fprintf (stderr, "gotcha %d", imagetarget);
  }	
	//probably want to set a server state here
  	glBindTexture(textures[handle].targets, 0);
	glDeleteTextures(1, &textures[handle].name);
	glGenTextures(1, &textures[handle].name);
	glBindTexture(textures[handle].targets, textures[handle].name);
	//	glTexParameteri(textures[handle].targets, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//	glTexParameteri(textures[handle].targets, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if ((textures[handle].mipmapped&(TRILINEAR|MIPMAP))&&g_game.mipmap>=2) {
	  glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  if (textures[handle].mipmapped&TRILINEAR) {
	    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	  }else {
	    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	  }
	} else {
	  if (textures[handle].mipmapped==NEAREST||g_game.mipmap==0) {
	    glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	  } else {
	    glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	  }
	}
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	int error;
	textures[handle].texture = buffer;
	
	switch(textures[handle].textureformat)
	{
	case DUMMY:
	case RGB24:
	  fprintf (stderr,"RGB24 bitmaps not yet supported");
	  break;
	case RGB32:
	  if (textures[handle].mipmapped&&g_game.mipmap)
	    gluBuild2DMipmaps(image2D, 3, textures[handle].width, textures[handle].height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  else
	    glTexImage2D(image2D, 0, 3, textures[handle].width, textures[handle].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  break;
	case RGBA32:
	  if (textures[handle].mipmapped&&g_game.mipmap)
	    gluBuild2DMipmaps(image2D, 4, textures[handle].width, textures[handle].height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  else
	    glTexImage2D(image2D, 0, 4, textures[handle].width, textures[handle].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  break;
	case RGBA16:
	  if (textures[handle].mipmapped&&g_game.mipmap)
	    gluBuild2DMipmaps(image2D, GL_RGBA16, textures[handle].width, textures[handle].height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  else
	    glTexImage2D(image2D, 0, GL_RGBA16, textures[handle].width, textures[handle].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  break;
	case RGB16:
	  if (textures[handle].mipmapped&&g_game.mipmap)
	    gluBuild2DMipmaps(image2D, GL_RGB16, textures[handle].width, textures[handle].height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  else
	    glTexImage2D(image2D, 0, GL_RGB16, textures[handle].width, textures[handle].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	  break;
	case PALETTE8:
		if (g_game.PaletteExt)
		{
			  textures[handle].shared_palette = GFXFALSE;
			  fprintf (stderr,"texture error 0\n");
				glColorTable(textures[handle].targets, 
					     GL_RGBA, 
					     256, 
					     GL_RGBA, 
					     GL_UNSIGNED_BYTE, 
					     textures[handle].palette);
				error = glGetError();
				if (error)
					return GFXFALSE;
				
				//}
			//memset(buffer, 0, textures[handle].width*textures[handle].height);
				if (textures[handle].mipmapped&&g_game.mipmap)
				  gluBuild2DMipmaps(image2D, GL_COLOR_INDEX8_EXT, textures[handle].width, textures[handle].height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buffer);
				else
				  glTexImage2D(image2D, 0, GL_COLOR_INDEX8_EXT, textures[handle].width, textures[handle].height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buffer);
				break;
				error = glGetError();
				if (error) 
				  return GFXFALSE;
		}
		else
		{
			int nsize = 4*textures[handle].height*textures[handle].width;
			unsigned char * tbuf = NULL;
			tbuf = new unsigned char [nsize];
			textures[handle].texture = tbuf;
			int j =0;
			for (int i=0; i< nsize; i+=4)
			{
				tbuf[i] = textures[handle].palette[4*buffer[j]];
				tbuf[i+1] = textures[handle].palette[4*buffer[j]+1];
				tbuf[i+2] = textures[handle].palette[4*buffer[j]+2];
				tbuf[i+3]= textures[handle].palette[4*buffer[j]+3];//used to be 255
				j ++;
			}
			if (textures[handle].mipmapped&&g_game.mipmap)
			  gluBuild2DMipmaps(image2D, 4, textures[handle].width, textures[handle].height, GL_RGBA, GL_UNSIGNED_BYTE, tbuf);
			else
			  glTexImage2D(image2D, 0, 4, textures[handle].width, textures[handle].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tbuf);

			//unashiehized			glTexImage2D(image2D,0,3,textures[handle].width, textures[handle].height,0,GL_RGBA, GL_UNSIGNED_BYTE, tbuf);
			delete [] tbuf;
			//delete [] buffer;
			//buffer = tbuf;
			
		}
		break;
	}

	//glBindTexture(textures[handle].targets, textures[handle].name);
	return GFXTRUE;

}
GFXBOOL /*GFXDRVAPI*/ GFXDeleteTexture (int handle)
{
	
	if (textures[handle].texture)
	{
		textures[handle].texture = NULL;
		glDeleteTextures(1, &textures[handle].name);
	}
	if (textures[handle].palette) {
		delete [] textures[handle].palette;
		textures[handle].palette=0;
	}
	textures[handle].alive = GFXFALSE;
	return GFXTRUE;
}


GFXBOOL /*GFXDRVAPI*/ GFXSelectTexture(int handle, int stage)
{
  //FIXME? is this legit?
  if (activetexture[stage]==handle)
    return GFXTRUE;
  else
    activetexture[stage] = handle;

	if (g_game.Multitexture)
	{
		switch(textures[handle].texturestage)
		{
		case 0:
		  glActiveTextureARB(GL_TEXTURE0_ARB);	
		  break;
		case 1:
		  glActiveTextureARB(GL_TEXTURE1_ARB);			
		  break;
		default:
			glActiveTextureARB(GL_TEXTURE0_ARB);			
			break;
		}

		glBindTexture(textures[handle].targets, textures[handle].name);

		if ((textures[handle].mipmapped&(TRILINEAR|MIPMAP))&&g_game.mipmap>=2) {
		  glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		  if (textures[handle].mipmapped&TRILINEAR) {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		  }else {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		  }
		} else {
		  if (textures[handle].mipmapped==NEAREST||g_game.mipmap==0) {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		  } else {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  }
		}

		if(g_game.PaletteExt&&textures[handle].textureformat == PALETTE8) {
		  //memset(textures[handle].palette, 255, 1024);
		}

		//float ccolor[4] = {1.0,1.0,1.0,1.0};
		switch(textures[handle].texturestage)
		{
		case 0:
			glActiveTextureARB(GL_TEXTURE0_ARB);	
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			break;
		case 1:
			glActiveTextureARB(GL_TEXTURE0_ARB);	
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glActiveTextureARB(GL_TEXTURE1_ARB);	
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnable (textures[handle].targets);		
			break;
		default:
			glActiveTextureARB(GL_TEXTURE0_ARB);		
			glEnable (textures[handle].targets);		
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			break;
		}

	}
	else
	{
		Stage0Texture = GFXTRUE;

		if (textures[handle].texturestage)
		{
			Stage1Texture = GFXTRUE;
			Stage1TextureName = textures[handle].name;
		}	
		else
		{
			Stage1Texture = GFXFALSE;
			glEnable (textures[handle].targets);
			glBindTexture(textures[handle].targets, textures[handle].name);
			Stage0TextureName = textures[handle].name;
		}
		if (textures[handle].mipmapped&(TRILINEAR|MIPMAP)&&g_game.mipmap>=2) {
		  glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		  if (textures[handle].mipmapped&TRILINEAR) {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		  }else {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		  }
		} else {
		  if (textures[handle].mipmapped==NEAREST||g_game.mipmap==0) {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		  } else {
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  }
		}
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		//FIXME VEGASTRIKE//REMOVED BY DANNYglTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, ccolor);
	}
	return GFXTRUE;
}

