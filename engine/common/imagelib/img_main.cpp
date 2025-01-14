/*
img_main.c - load & save various image formats
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "imagelib.h"

// global image variables
imglib_t	image;

typedef struct suffix_s
{
	const char	*suf;
	uint		flags;
	side_hint_t	hint;
} suffix_t;

static const suffix_t skybox_qv1[6] =
{
{ "ft", IMAGE_FLIP_X, CB_HINT_POSX },
{ "bk", IMAGE_FLIP_Y, CB_HINT_NEGX },
{ "up", IMAGE_ROT_90, CB_HINT_POSZ },
{ "dn", IMAGE_ROT_90, CB_HINT_NEGZ },
{ "rt", IMAGE_ROT_90, CB_HINT_POSY },
{ "lf", IMAGE_ROT270, CB_HINT_NEGY },
};

static const suffix_t skybox_qv2[6] =
{
{ "_ft", IMAGE_FLIP_X, CB_HINT_POSX },
{ "_bk", IMAGE_FLIP_Y, CB_HINT_NEGX },
{ "_up", IMAGE_ROT_90, CB_HINT_POSZ },
{ "_dn", IMAGE_ROT_90, CB_HINT_NEGZ },
{ "_rt", IMAGE_ROT_90, CB_HINT_POSY },
{ "_lf", IMAGE_ROT270, CB_HINT_NEGY },
};

static const suffix_t cubemap_v1[6] =
{
{ "posx", 0, CB_HINT_POSX },
{ "negx", 0, CB_HINT_NEGX },
{ "posy", 0, CB_HINT_POSY },
{ "negy", 0, CB_HINT_NEGY },
{ "posz", 0, CB_HINT_POSZ },
{ "negz", 0, CB_HINT_NEGZ },
};

static const suffix_t cubemap_v2[6] =
{
{ "px", 0, CB_HINT_POSX },
{ "nx", 0, CB_HINT_NEGX },
{ "py", 0, CB_HINT_POSY },
{ "ny", 0, CB_HINT_NEGY },
{ "pz", 0, CB_HINT_POSZ },
{ "nz", 0, CB_HINT_NEGZ },
};

typedef struct cubepack_s
{
	const char	*name;	// just for debug
	const suffix_t	*type;
} cubepack_t;

static const cubepack_t load_cubemap[] =
{
{ "3Ds Sky1", skybox_qv1 },
{ "3Ds Sky2", skybox_qv2 },
{ "3Ds Cube", cubemap_v2 },
{ "Tenebrae", cubemap_v1 },
{ NULL, NULL },
};

// soul of ImageLib - table of image format constants 
const bpc_desc_t PFDesc[] =
{
{PF_UNKNOWN,	"raw",	0x1908, 0, false },
{PF_INDEXED_24,	"pal 24",	0x1908, 1, false },
{PF_INDEXED_32,	"pal 32",	0x1908, 1, false },
{PF_RGBA_32,	"RGBA 32",0x1908, 4, false },
{PF_BGRA_32,	"BGRA 32",0x80E1, 4, false },
{PF_RGB_24,	"RGB 24",	0x1908, 3, false },
{PF_BGR_24,	"BGR 24",	0x80E0, 3, false },
{PF_DXT1, "DXT 1", 0x83F1, 4, true },
{PF_DXT3, "DXT 3", 0x83F2, 4, true },
{PF_DXT5, "DXT 5", 0x83F3, 4, true },
{PF_A_8,	"A 8",0x1906, 1, false },
{PF_ASTC_4x4,	"ASTC 4x4",0x93B0, 4, true },
{PF_ASTC_6x6,	"ASTC 6x6",0x93B4, 4, true },
{PF_ASTC_8x8,	"ASTC 8x8",0x93B7, 4, true },
{PF_ASTC_10x10,	"ASTC 10x10",0x93BB, 4, true },
{PF_ASTC_12x12,	"ASTC 12x12",0x93BD, 4, true },
};

void Image_Reset( void )
{
	// reset global variables
	image.width = image.height = image.depth = 0;
	image.source_width = image.source_height = 0;
	image.num_sides = image.flags = 0;
	image.source_type = image.num_mips = 0;
	image.type = PF_UNKNOWN;
	image.fogParams[0] = 0;
	image.fogParams[1] = 0;
	image.fogParams[2] = 0;
	image.fogParams[3] = 0;
	image.encode = 0;

	// pointers will be saved with prevoius picture struct
	// don't care about it
	image.palette = NULL;
	image.cubemap = NULL;
	image.rgba = NULL;
	image.ptr = 0;
	image.size = 0;
}

void FS_FreeImageInternal( rgbdata_t *pack );

image_ref Image_NewTemp()
{
	image_ref pack(new rgbdata_t(), FS_FreeImageInternal);
	return pack;
}

image_ref ImagePack( void )
{
	image_ref pack = Image_NewTemp();

	// clear any force flags
	image.force_flags = 0;

	if( image.cubemap && image.num_sides != 6 )
	{
		// this never be happens, just in case
		MsgDev( D_NOTE, "ImagePack: inconsistent cubemap pack %d\n", image.num_sides );
		pack.reset();
		return nullptr;
	}

	if( image.cubemap ) 
	{
		image.flags |= IMAGE_CUBEMAP;
		pack->buffer = image.cubemap;
		pack->width = image.source_width;
		pack->height = image.source_height;
		pack->type = image.source_type;
		pack->size = image.size * image.num_sides;
	}
	else 
	{
		pack->buffer = image.rgba;
		pack->width = image.width;
		pack->height = image.height;
		pack->depth = image.depth;
		pack->type = image.type;
		pack->size = image.size;
	}

	// copy fog params
	pack->fogParams[0] = image.fogParams[0];
	pack->fogParams[1] = image.fogParams[1];
	pack->fogParams[2] = image.fogParams[2];
	pack->fogParams[3] = image.fogParams[3];

	pack->flags = image.flags;
	pack->numMips = image.num_mips;
	pack->palette = image.palette;
	pack->encode = image.encode;
	
	return pack;
}

/*
================
FS_AddSideToPack

================
*/
qboolean FS_AddSideToPack( const char *name, int adjust_flags )
{
	byte	*out, *flipped;
	qboolean	resampled = false;
	
	// first side set average size for all cubemap sides!
	if( !image.cubemap )
	{
		image.source_width = image.width;
		image.source_height = image.height;
		image.source_type = image.type;
	}

	// keep constant size, render.dll expecting it
	image.size = image.source_width * image.source_height * 4;

	// mixing dds format with any existing ?
	if( image.type != image.source_type )
		return false;

	// flip image if needed
	flipped = Image_FlipInternal( image.rgba, &image.width, &image.height, image.source_type, adjust_flags );
	if( !flipped ) return false; // try to reasmple dxt?
	if( flipped != image.rgba ) image.rgba = Image_Copy( image.size );

	// resampling image if needed
	out = Image_ResampleInternal((uint *)image.rgba, image.width, image.height, image.source_width, image.source_height, image.source_type, &resampled );
	if( !out ) return false; // try to reasmple dxt?
	if( resampled ) image.rgba = Image_Copy( image.size );

	image.cubemap = (byte*)Mem_Realloc( host.imagepool, image.cubemap, image.ptr + image.size );
	Q_memcpy( image.cubemap + image.ptr, image.rgba, image.size ); // add new side

	Mem_Free( image.rgba );	// release source buffer
	image.ptr += image.size; 	// move to next
	image.num_sides++;		// bump sides count

	return true;
}

/*
================
FS_LoadImage

loading and unpack to rgba any known image
================
*/
image_ref FS_LoadImage( const char *filename, const byte *buffer, size_t size )
{
	const char	*ext = FS_FileExtension( filename );
	string		path, loadname, sidename;
	qboolean		anyformat = true;
	qboolean		gamedironly = true;
	int		i;
	fs_offset_t	filesize = 0;
	const loadpixformat_t *format;
	const cubepack_t	*cmap;
	byte		*f;

	Image_Reset(); // clear old image
	Q_strncpy( loadname, filename, sizeof( loadname ));

	if( Q_stricmp( ext, "" ))
	{
		// we needs to compare file extension with list of supported formats
		// and be sure what is real extension, not a filename with dot
		for( format = image.loadformats; format && format->formatstring; format++ )
		{
			if( !Q_stricmp( format->ext, ext ))
			{
				FS_StripExtension( loadname );
				anyformat = false;
				break;
			}
		}
	} 

	// HACKHACK: skip any checks, load file from buffer
	if( filename[0] == '#' && buffer && size ) goto load_internal;
	else if (buffer && !size && !Q_stricmp(ext, "bmp")) {
		if (Q_strchr(filename, '@') || Q_strchr(filename, '#')) goto load_internal_ext;
	}

search_fs:

	// now try all the formats in the selected list
	for( format = image.loadformats; format && format->formatstring; format++)
	{
		if( anyformat || !Q_stricmp( ext, format->ext ))
		{
			Q_sprintf( path, format->formatstring, loadname, "", format->ext );
			image.hint = format->hint;
			f = FS_LoadFile( path, &filesize, gamedironly );
			if( f && filesize > 0 )
			{
				if( format->loadfunc( path, f, (size_t)filesize ))
				{
					Mem_Free( f ); // release buffer
					return ImagePack(); // loaded
				}
				else Mem_Free(f); // release buffer 
			}
		}
	}

	if( gamedironly )
	{
		gamedironly = false;
		goto search_fs;
	}

	// check all cubemap sides with package suffix
	for( cmap = load_cubemap; cmap && cmap->type; cmap++ )
	{
		for( i = 0; i < 6; i++ )
		{
			// for support mixed cubemaps e.g. sky_ft.bmp, sky_rt.tga
			// NOTE: all loaders must keep sides in one format for all
			for( format = image.loadformats; format && format->formatstring; format++ )
			{
				if( anyformat || !Q_stricmp( ext, format->ext ))
				{
					Q_sprintf( path, format->formatstring, loadname, cmap->type[i].suf, format->ext );
					image.hint = (image_hint_t)cmap->type[i].hint; // side hint

					f = FS_LoadFile( path, &filesize, false );
					if( f && filesize > 0 )
					{
						// this name will be used only for tell user about problems 
						if( format->loadfunc( path, f, (size_t)filesize ))
						{         
							Q_snprintf( sidename, sizeof( sidename ), "%s%s.%s", loadname, cmap->type[i].suf, format->ext );
							if( FS_AddSideToPack( sidename, cmap->type[i].flags )) // process flags to flip some sides
							{
								Mem_Free( f );
								break; // loaded
							}
						}
						Mem_Free( f );
					}
				}
			}

			if( image.num_sides != i + 1 ) // check side
			{
				// first side not found, probably it's not cubemap
				// it contain info about image_type and dimensions, don't generate black cubemaps 
				if( !image.cubemap ) break;
				MsgDev( D_ERROR, "FS_LoadImage: couldn't load (%s%s), create black image\n", loadname, cmap->type[i].suf );

				// Mem_Alloc already filled memblock with 0x00, no need to do it again
				image.cubemap = (byte*)Mem_Realloc( host.imagepool, image.cubemap, image.ptr + image.size );
				image.ptr += image.size; // move to next
				image.num_sides++; // merge counter
			}
		}

		// make sure what all sides is loaded
		if( image.num_sides != 6 )
		{
			// unexpected errors ?
			if( image.cubemap )
				Mem_Free( image.cubemap );
			Image_Reset();
		}
		else break;
	}

	if( image.cubemap )
		return ImagePack(); // all done

load_internal:
	for( format = image.loadformats; format && format->formatstring; format++ )
	{
		if( anyformat || !Q_stricmp( ext, format->ext ))
		{
			image.hint = format->hint;
			if( buffer && size > 0  )
			{
				if( format->loadfunc( loadname, buffer, size ))
					return ImagePack(); // loaded
			}
		}
	}

load_internal_ext:
	if (!Q_stricmp(ext, "bmp"))
	{
		Q_sprintf(path, "%s.bmp", loadname);
		image.hint = IL_HINT_HL;
		f = FS_LoadFile(path, &filesize, gamedironly);
		if (f && filesize > 0)
		{
			if (Image_LoadMDL_BMP(path, f, (size_t)filesize, buffer))
			{
				Mem_Free(f); // release buffer
				return ImagePack(); // loaded
			}
			else Mem_Free(f); // release buffer 
		}
	}

	if (gamedironly)
	{
		gamedironly = false;
		goto load_internal_ext;
	}

	if( !image.loadformats || image.loadformats->ext == NULL )
		MsgDev( D_NOTE, "FS_LoadImage: imagelib offline\n" );
	else if( filename[0] != '#' )
		MsgDev( D_WARN, "FS_LoadImage: couldn't load \"%s\"\n", loadname );

	// clear any force flags
	image.force_flags = 0;

	return nullptr;
}

/*
================
Image_Save

writes image as any known format
================
*/
qboolean FS_SaveImage( const char *filename, image_ref pix )
{
	const char	*ext = FS_FileExtension( filename );
	qboolean		anyformat = !Q_stricmp( ext, "" ) ? true : false;
	string		path, savename;
	const savepixformat_t *format;

	if( !pix || !pix->buffer || anyformat )
	{
		// clear any force flags
		image.force_flags = 0;
		return false;
	}

	Q_strncpy( savename, filename, sizeof( savename ));
	FS_StripExtension( savename ); // remove extension if needed

	if( pix->flags & (IMAGE_CUBEMAP|IMAGE_SKYBOX))
	{
		size_t		realSize = pix->size; // keep real pic size
		byte		*picBuffer; // to avoid corrupt memory on free data
		const suffix_t	*box;
		int		i;

		if( pix->flags & IMAGE_SKYBOX )
			box = skybox_qv1;
		else if( pix->flags & IMAGE_CUBEMAP )
			box = cubemap_v2;
		else
		{
			// clear any force flags
			image.force_flags = 0;
			return false;	// do not happens
		}

		pix->size /= 6; // now set as side size 
		picBuffer = pix->buffer;

		// save all sides seperately
		for( format = image.saveformats; format && format->formatstring; format++ )
		{
			if( !Q_stricmp( ext, format->ext ))
			{
				for( i = 0; i < 6; i++ )
				{
					Q_sprintf( path, format->formatstring, savename, box[i].suf, format->ext );
					if( !format->savefunc( path, pix )) break; // there were errors
					pix->buffer += pix->size; // move pointer
				}

				// restore pointers
				pix->size = realSize;
				pix->buffer = picBuffer;

				// clear any force flags
				image.force_flags = 0;

				return ( i == 6 );
			}
		}
	}
	else
	{
		for( format = image.saveformats; format && format->formatstring; format++ )
		{
			if( !Q_stricmp( ext, format->ext ))
			{
				Q_sprintf( path, format->formatstring, savename, "", format->ext );
				if( format->savefunc( path, pix ))
				{
					// clear any force flags
					image.force_flags = 0;
					return true; // saved
				}
			}
		}
	}

	// clear any force flags
	image.force_flags = 0;

	return false;
}

/*
================
Image_FreeImage

free RGBA buffer
================
*/
void FS_FreeImageInternal( rgbdata_t *pack )
{
	if( pack )
	{
		if(!(pack->flags & IMAGE_TEMP))
		{
			if( pack->buffer ) Mem_Free( pack->buffer );
			if( pack->palette ) Mem_Free( pack->palette );
		}
		delete pack;
	}
	// else MsgDev( D_WARN, "FS_FreeImage: trying to free NULL image\n" );
}

/*
================
FS_CopyImage

make an image copy
================
*/
image_ref FS_CopyImage( image_ref in )
{
	int	palSize = 0;

	if( !in ) return nullptr;

	image_ref out = Image_NewTemp();
	*out = *in;

	switch( in->type )
	{
	case PF_INDEXED_24:
		palSize = 768;
		break;
	case PF_INDEXED_32:
		palSize = 1024;
		break;
	}

	if( palSize )
	{
		out->palette = (byte *)Mem_Alloc( host.imagepool, palSize );
		Q_memcpy( out->palette, in->palette, palSize );
	}

	if( in->size )
	{
		out->buffer = (byte*)Mem_Alloc( host.imagepool, in->size );
		Q_memcpy( out->buffer, in->buffer, in->size );
	}

	return out;
}
