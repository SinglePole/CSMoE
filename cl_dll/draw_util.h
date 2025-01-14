/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#pragma once
#ifndef DRAW_UTIL_H
#define DRAW_UTIL_H
// Drawing primitives
namespace cl {
constexpr int DHN_DRAWZERO = 1;
constexpr int DHN_2DIGITS = 2;
constexpr int DHN_3DIGITS = 4;
constexpr int DHN_FILLZERO = 8;
constexpr int DHN_4DIGITS = 16;
constexpr int DHN_5DIGITS = 32;
constexpr int DHN_FROMRIGHT = 64;
constexpr int DHN_CENTERALIGNED = 128;
constexpr int MIN_ALPHA = 100;

constexpr unsigned long RGB_YELLOWISH = 0x00FFA000; //255,160,0
constexpr unsigned long RGB_REDISH = 0x00FF1010; //255,16,16
constexpr unsigned long RGB_GREENISH = 0x0000A000; //0,160,0
constexpr unsigned long RGB_WHITE = 0x00FFFFFF; //0,160,0
constexpr unsigned long RGB_LIGHTBLUE = 0x0072C5FF; //114, 197, 255

class DrawUtils
{
public:
	static int DrawHudNumber(int x, int y, int iFlags, int iNumber,
						 int r, int g, int b );

	static int DrawHudNumber2( int x, int y, bool DrawZero, int iDigits, int iNumber,
						   int r, int g, int b);

	static int DrawHudNumber2( int x, int y, int iNumber,
						   int r, int g, int b);

	static int DrawNEWHudNumber(int index, int iX, int iY, int number, int r, int g, int b, int a, int iDrawZero, int maxsize, int widthplus = 0);

	static int GetNEWHudNumberWidth(int type, int number, int iDrawZero, int maxsize, int widthplus = 0);

	static int DrawHudString(int x, int y, int iMaxX, const char *szString,
						 int r, int g, int b, float scale = 0.0f, bool drawing = false );

	static int DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString,
								 int r, int g, int b, float scale = 0.0f, bool drawing = false );

	static inline int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber,
								int r, int g, int b, float scale = 0.0f )
	{
		char szString[16];
		snprintf( szString, sizeof(szString), "%d", iNumber );
		return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b, scale );
	}

	static int HudStringLen( const char *szIt, float scale = 1 );

	// legacy shit came with Valve
	static inline int GetNumWidth(int iNumber, int iFlags)
	{
		if ( iFlags & ( DHN_3DIGITS ) )
			return 3;

		if ( iFlags & ( DHN_2DIGITS ) )
			return 2;

		if ( iNumber <= 0 )
			return iFlags & DHN_DRAWZERO ? 1 : 0;

		if ( iNumber < 10 )
			return 1;

		if ( iNumber < 100 )
			return 2;

		return 3;
	}

	static inline int DrawConsoleString(int x, int y, const char *string)
	{
		return gEngfuncs.pfnDrawConsoleString(x, y, (char*)string);
	}

	static inline void SetConsoleTextColor( float r, float g, float b )
	{
		gEngfuncs.pfnDrawSetTextColor(r, g, b);
	}

	static inline void SetConsoleTextColor( unsigned char r, unsigned char g, unsigned char b )
	{
		gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
	}

	static inline int ConsoleStringLen(  const char *szIt )
	{
		int _width;
		gEngfuncs.pfnDrawConsoleStringLen(szIt, &_width, NULL);
		return _width;
	}

	static inline void ConsoleStringSize( const char *szIt, int *width, int *height )
	{
		gEngfuncs.pfnDrawConsoleStringLen(szIt, width, height);
	}

	static inline int TextMessageDrawChar( int x, int y, int number, int r, int g, int b, float scale = 0.0f )
	{
		int ret;
		if( scale && g_iMobileAPIVersion )
			ret = gMobileAPI.pfnDrawScaledCharacter( x, y, number, r, g, b, scale ) / gHUD.m_flScale;
		else
			ret = gEngfuncs.pfnDrawCharacter( x, y, number, r, g, b );
		return ret;
	}

	static inline void UnpackRGB( int &r, int &g, int &b, const unsigned long ulRGB )
	{
		r = (ulRGB & 0xFF0000) >>16;
		g = (ulRGB & 0xFF00) >> 8;
		b = ulRGB & 0xFF;
	}

	static inline void ScaleColors( int &r, int &g, int &b, const int a )
	{
		r *= a / 255.0f;
		g *= a / 255.0f;
		b *= a / 255.0f;
	}

	static inline void DrawRectangle( int x, int y, int wide, int tall,
						   int r = 0, int g = 0, int b = 0, int a = 153,
						   bool drawStroke = true )
	{
		FillRGBABlend( x, y, wide, tall, r, g, b, a );
		if ( drawStroke )
		{
			// TODO: remove this hardcoded hardcore
			DrawOutlinedRect(x, y, wide, tall, 255, 140, 0, 255);
		}
	}

	static inline void DrawOutlinedRect(int x, int y, int wide, int tall,
		int r = 0, int g = 0, int b = 0, int a = 255)
	{
		FillRGBABlend(x + 1, y, wide - 1, 1, r, g, b, a);
		FillRGBABlend(x, y, 1, tall - 1, r, g, b, a);
		FillRGBABlend(x + wide - 1, y + 1, 1, tall - 1, r, g, b, a);
		FillRGBABlend(x, y + tall - 1, wide - 1, 1, r, g, b, a);
	}

	static void Draw2DQuad( float x1, float y1, float x2, float y2 );
	static void Draw2DQuadScaled(float x1, float y1, float x2, float y2) { return Draw2DQuad(x1 * gHUD.m_flScale, y1 * gHUD.m_flScale, x2 * gHUD.m_flScale, y2 * gHUD.m_flScale); }
};
}
#endif // DRAW_UTIL_H
