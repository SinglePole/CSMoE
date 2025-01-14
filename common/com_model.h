﻿//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// com_model.h
#if !defined( COM_MODEL_H )
#define COM_MODEL_H
#if defined( _WIN32 )
#ifndef __MINGW32__
#pragma once
#endif /* not __MINGW32__ */
#endif

#include "bspfile.h"	// we need some declarations from it

#include "xash3d_types.h"

/*
==============================================================================
	ENGINE MODEL FORMAT
==============================================================================
*/

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2

#define ZISCALE		((float)0x8000)

#define MAX_MODEL_NAME		64
#define MAX_MODEL_SUFFIX		32

#define	MIPLEVELS			4
#define VERTEXSIZE		7
#define	MAXLIGHTMAPS		4
#define	NUM_AMBIENTS		4		// automatic ambient sounds

typedef enum
{
	mod_bad = -1,
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
} modtype_t;

// plane_t structure
typedef struct mplane_s
{
	vec3_c	normal;			// surface normal
	float	dist;			// closest appoach to origin
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

typedef struct
{
	vec3_c		position;
} mvertex_t;

typedef struct
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct texture_s
{
	char		name[16];
	unsigned	width, height;
	int		gl_texturenum;
	struct msurface_s* texturechain;	// for gl_texsort drawing
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s* anim_next;		// in the animation sequence
	struct texture_s* alternate_anims;	// bmodels in frame 1 use these
	unsigned short	fb_texturenum;	// auto-luma texturenum
	unsigned short	dt_texturenum;	// detail-texture binding
	unsigned int	unused[2];	// reserved
	unsigned short unused1;
	unsigned short	nm_texturenum;
} texture_t;

typedef struct
{
	float		vecs[2][4];		// [s/t] unit vectors in world space. 
								// [i][3] is the s/t offset relative to the origin.
								// s or t = dot(3Dpoint,vecs[i])+vecs[i][3]
	float		mipadjust;		// ?? mipmap limits for very small surfaces
	texture_t* texture;
	int			flags;			// sky or slime, no lightmap or 256 subdivision
} mtexinfo_t;

// 73 bytes per VBO vertex
// FIXME: align to 32 bytes
typedef struct glvert_s
{
	vec3_c		vertex;		// position
	vec3_c		normal;		// normal
	vec2_t		stcoord;		// ST texture coords
	vec2_t		lmcoord;		// ST lightmap coords
	vec2_t		sccoord;		// ST scissor coords (decals only) - for normalmap coords migration
	vec3_c		tangent;		// tangent
	vec3_c		binormal;		// binormal
	byte		color[4];		// colors per vertex
} glvert_t;

typedef struct vertex_s
{
    vec3_c xyz;
    vec2_t s1t1;
    vec2_t s2t2;
} vertex_t;
static_assert(sizeof(vertex_t) == VERTEXSIZE * sizeof(float));

typedef struct glpoly_s
{
	struct glpoly_s* next;
	struct glpoly_s* chain;
	int		numverts;
	int		flags;          		// for SURF_UNDERWATER
    vertex_t		verts[4];	// variable sized (xyz s1t1 s2t2)
} glpoly_t;

typedef struct mnode_s
{
	// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s* parent;

	// node specific
	mplane_t* plane;
	struct mnode_s* children[2];

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;

typedef struct msurface_s	msurface_t;
typedef struct decal_s		decal_t;

// JAY: Compress this as much as possible
struct decal_s
{
	decal_t* pnext;			// linked list for each surface
	msurface_t* psurface;		// Surface id for persistence / unlinking
	float		dx;				// Offsets into surface texture (in texture coordinates, so we don't need floats)
	float		dy;
	float		scale;		// Pixel scale
	short		texture;		// Decal texture
	byte		flags;			// Decal flags

	short		entityIndex;	// Entity this is attached to

	// Xash3D added
	vec3_c		position;		// location of the decal center in world space.
	vec3_c		saxis;		// direction of the s axis in world space
	struct msurfmesh_s* mesh;		// decal mesh in local space
	int		reserved[4];	// for future expansions
};

typedef struct mleaf_s
{
	// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s* parent;

	// leaf specific
	byte* compressed_vis;
	struct efrag_s* efrags;

	msurface_t** firstmarksurface;
	int			nummarksurfaces;
	byte* compressed_pas;
	byte		ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

struct msurface_s
{
	int		visframe;		// should be drawn when node is crossed

	mplane_t* plane;		// pointer to shared plane
	int		flags;		// see SURF_ #defines

	int		firstedge;	// look up in model->surfedges[], negative numbers
	int		numedges;		// are backwards edges

	short		texturemins[2];
	short		extents[2];

	int		light_s, light_t;	// gl lightmap coordinates

	glpoly_t* polys;		// multiple if warped
	struct msurface_s* texturechain;

	mtexinfo_t* texinfo;

	// lighting info
	int		dlightframe;	// last frame the surface was checked by an animated light
	int		dlightbits;	// dynamically generated. Indicates if the surface illumination
						// is modified by an animated light.

	int		lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	int		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	struct msurface_s* lightmapchain;		// for new dlights rendering (was cached_dlight)

	color24* samples;		// note: this is the actual lightmap data for this surface
	decal_t* pdecals;
};

typedef struct msurfmesh_s
{
	unsigned short	numVerts;
	unsigned short	numElems;		// ~ 20 000 vertex per one surface. Should be enough
	unsigned int	startVert;	// user-variable. may be used for construct world single-VBO
	unsigned int	startElem;	// user-variable. may be used for construct world single-VBO

	glvert_t* verts;		// vertexes array
	unsigned short* elems;		// indices		

	struct msurface_s* surf;		// pointer to parent surface. Just for consistency
	struct msurfmesh_s* next;		// temporary chain of subdivided surfaces
} msurfmesh_t;

// surface extradata stored in cache.data for all brushmodels
typedef struct mextrasurf_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// surface origin
	msurfmesh_t* mesh;		// VBO\VA ready surface mesh. Not used by engine but can be used by mod-makers

	int		dlight_s, dlight_t;	// gl lightmap coordinates for dynamic lightmaps

	int		mirrortexturenum;	// gl texnum
    matrix4x4		mirrormatrix;
	struct mextrasurf_s* mirrorchain;	// for gl_texsort drawing
	struct mextrasurf_s* detailchain;	// for detail textures drawing
	color24* deluxemap;	// note: this is the actual deluxemap data for this surface

	int		reserved[32];	// just for future expansions or mod-makers
} mextrasurf_t;

typedef struct hull_s
{
	dclipnode_t* clipnodes;
	mplane_t* planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_c		clip_mins;
	vec3_c		clip_maxs;
} hull_t;

#if !defined( CACHE_USER ) && !defined( QUAKEDEF_H )
#define CACHE_USER
typedef struct cache_user_s
{
	void* data;
} cache_user_t;
#endif

typedef struct model_s
{
	char		name[MAX_MODEL_NAME];
	qboolean	needload;		// bmodels and sprites don't cache normally

	modtype_t	type;
	int			numframes;
	//synctype_t	synctype;
	mempool_t* mempool;		// private mempool (was synctype)

	int			flags;

	//
	// volume occupied by the model
	//		
	vec3_c		mins, maxs;
	float		radius;

	//
	// brush model
	//
	int		firstmodelsurface;
	int		nummodelsurfaces;

	int			numsubmodels;
	dmodel_t* submodels;

	int			numplanes;
	mplane_t* planes;

	int			numleafs;		// number of visible leafs, not counting 0
	struct mleaf_s* leafs;

	int			numvertexes;
	mvertex_t* vertexes;

	int			numedges;
	medge_t* edges;

	int			numnodes;
	mnode_t* nodes;

	int			numtexinfo;
	mtexinfo_t* texinfo;

	int			numsurfaces;
	msurface_t* surfaces;

	int			numsurfedges;
	int* surfedges;

	int			numclipnodes;
	dclipnode_t* clipnodes;

	int			nummarksurfaces;
	msurface_t** marksurfaces;

	hull_t		hulls[MAX_MAP_HULLS];

	int			numtextures;
	texture_t** textures;

	byte* visdata;

	color24* lightdata;

	char* entities;

	//
	// additional model data
	//
	cache_user_t	cache;		// only access through Mod_Extradata

	char		suffix[MAX_MODEL_SUFFIX];

} model_t;

typedef struct alight_s
{
	int			ambientlight;	// clip at 128
	int			shadelight;		// clip at 192 - ambientlight
	vec3_t		color;
	vec3_t *plightvec;
} alight_t;

typedef struct auxvert_s
{
	float	fv[3];		// viewspace x, y
} auxvert_t;

#include "custom.h"

#define	MAX_INFO_STRING			256
#define	MAX_SCOREBOARDNAME		32
typedef struct player_info_s
{
	// User id on server
	int		userid;

	// User info string
	char	userinfo[MAX_INFO_STRING];

	// Name
	char	name[MAX_SCOREBOARDNAME];

	// Spectator or not, unused
	int		spectator;

	int		ping;
	int		packet_loss;

	// skin information
	char	model[64];
	int		topcolor;
	int		bottomcolor;

	// last frame rendered
	int		renderframe;

	// Gait frame estimation
	int		gaitsequence;
	float	gaitframe;
	float	gaityaw;
	vec3_c	prevgaitorigin;

	customization_t customdata;
} player_info_t;

//
// sprite representation in memory
//
typedef enum { SPR_SINGLE = 0, SPR_GROUP, SPR_ANGLED } spriteframetype_t;

typedef struct mspriteframe_s
{
	int		width;
	int		height;
	float		up, down, left, right;
	int		gl_texturenum;
} mspriteframe_t;

typedef struct
{
	int		numframes;
	float* intervals;
	mspriteframe_t* frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t	type;
	mspriteframe_t* frameptr;
} mspriteframedesc_t;

typedef struct
{
	short		type;
	short		texFormat;
	int		maxwidth;
	int		maxheight;
	int		numframes;
	int		radius;
	int		facecull;
	int		synctype;
	mspriteframedesc_t	frames[1];
} msprite_t;

#endif // #define COM_MODEL_H