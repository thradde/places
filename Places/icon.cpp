
#define STRICT
#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#include "shobjidl.h"
#include "shlguid.h"

#include "memdebug.h"

#include <SFML/Graphics.hpp>

#include <vector>
#include <set>
using namespace std;

#define ASSERT
#include "RfwString.h"
#include "exceptions.h"
#include "Mutex.h"
#include "stream.h"
#include "platform.h"
#include "generic.h"
#include "popup_menu.h"
#include "configuration.h"
#include "GetIcon.h"
#include "md5.h"
#include "bitmap_cache.h"
#include "icon.h"


// --------------------------------------------------------------------------------------------------------------------------------------------
//																Globals
// --------------------------------------------------------------------------------------------------------------------------------------------
static sf::Shader *_pShader;		// for drawing icons hi-lighted


// --------------------------------------------------------------------------------------------------------------------------------------------
//															InitIconShader()
// --------------------------------------------------------------------------------------------------------------------------------------------
void InitIconShader()
{
	_pShader = new sf::Shader();

#if 1
	// make brighter
	_pShader->loadFromMemory(
		"uniform sampler2D texture;"

		// The width and height of each pixel in texture coordinates
		"uniform float xscale;"
		"uniform float yscale;"

		"void main()"
		"{"
		"	vec4 bkg = vec4(0.0, 0.0, 0.0, 0.2);"
		"	vec2 texel = gl_TexCoord[0].st;"
		"	vec4 pixel = texture2D(texture, texel);"
		"	if (pixel.a < 0.05)"
		"		pixel = bkg;"
		/*
		"	else"
		"	{"
		"		if (texture2D(texture, texel + vec2(0.0, -yscale).a < 0.05)"
		"			pixel = (pixel + bkg) / 2.0;"
		"		else if (texture2D(texture, texel + vec2(0.0, yscale).a < 0.05)"
		"			pixel = (pixel + bkg) / 2.0;"
		"		else if (texture2D(texture, texel + vec2(-xscale, 0.0).a < 0.05)"
		"			pixel = (pixel + bkg) / 2.0;"
		"		else if (texture2D(texture, texel + vec2(xscale, 0.0).a < 0.05)"
		"			pixel = (pixel + bkg) / 2.0;"
		"		pixel = clamp(pixel, 0.0, 1.0);"
		"	}"
		*/
		/*
		"	else"
		"	{"
		"		pixel.rgb += vec3(0.2, 0.2, 0.2);"
		"		pixel.rgb = clamp(pixel.rgb, 0.0, 1.0);"
		"	}"
		*/
		"	gl_FragColor = gl_Color * pixel;"
		"}",
		sf::Shader::Fragment);
#else
	// Glow-Effect
	_pShader->loadFromMemory(
		"uniform sampler2D texture;"

		// The width and height of each pixel in texture coordinates
		"uniform float xscale;"
		"uniform float yscale;"

		"void main()"
		"{"
			// Current texture coordinate
		"	vec2 texel = gl_TexCoord[0].st;"
		"	vec4 pixel = texture2D(texture, texel);"
	
			// Larger constant = bigger glow
		"	float glow = 5.0 * ((xscale + yscale) / 2.0);"

			// The vector to contain the new, "bloomed" color value
		"	vec4 bloom = vec4(0);"

			// Loop over all the pixels on the texture in the area given by the constant in glow
		"	float count = 0;"
		"	for (float x = texel.x - glow; x < texel.x + glow; x += xscale)"
		"	{"
		"		for (float y = texel.y - glow; y < texel.y + glow; y += yscale)"
		"		{"
					// Add that pixel's value to the bloom vector
		//"			bloom += (texture2D(texture, vec2(x, y)) - 0.4) * 30.0;"
		"			bloom += texture2D(texture, vec2(x, y));"
					// Add 1 to the number of pixels sampled
		"			count++;"
		"		}"
		"	}"
			// Divide by the number of pixels sampled to average out the value
			// The constant being multiplied with count here will dim the bloom effect a bit, with higher values
			// Clamp the value between a 0.0 to 1.0 range
			"	bloom = clamp(bloom / (count * 2), 0.0, 1.0);"

			// Set the current fragment to the original texture pixel, with our bloom value added on
			"	gl_FragColor = gl_Color * (pixel + bloom);"
		"}",
		sf::Shader::Fragment);
#endif

	_pShader->setParameter("texture", sf::Shader::CurrentTexture);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::RenderTitleTextLayout()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::RenderTitleTextLayout()
{
	const int max_lines = gConfig.m_nIconTitleLines;
	RString title = m_strFullTitle;
	sf::Vector2f pos;
	m_Title.Reset();
	while (title.length() > 0 && m_Title.GetNumLines() < max_lines)
	{
		// eat leading blanks
		while (title.length() > 0 && title[0] == _T(' '))
			title.remove(0, 1);

		gConfig.m_pText->setString(title.c_str());
		gConfig.m_pText->setPosition(0, 0);

		size_t i;
		int last_width = 0;
		for (i = 0; i < title.length(); i++)
		{
			if (title[(int)i] == _T('\n'))
				break;

			/*RString tmp(title.Left(i + 1));
			gConfig.m_pText->setString(tmp.c_str());
			sf::FloatRect rc = gConfig.m_pText->getGlobalBounds();

			gConfig.m_pText->setString(title.c_str());*/
			pos = gConfig.m_pText->findCharacterPos(i + 1);		// it is the pos, so we need the start-pos of the next character
			if (pos.x > gConfig.m_nTextWidth)
				break;
			last_width = (int)pos.x;
		}

		if (i == 0)
			continue;	// break;

		int width = (int)pos.x;
		if (pos.x > gConfig.m_nTextWidth)
		{
			i--;
			width = last_width;
		}

		// when using the bounds of the sprite in DrawTitle():
		m_Title.AddLine(CIconTextLine(title.substr(0, i + 1), (gConfig.m_nIconSizeX - width) / 2));
		//m_Title.AddLine(CIconTextLine(title.substr(0, i + 1), width / 2));
		title.remove(0, i + 1);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::CreateOrAddCachedBitmap()
// Returns true, if the bitmap was changed.
// Remember to call bitmap_cache.Unref(m_pCachedBitmap) before calling this method!
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::CreateOrAddCachedBitmap(CBitmapCache &bitmap_cache)
{
	bitmap_cache.Unref(m_pCachedBitmap);

	if (!m_strIconPath.empty())
		m_pCachedBitmap = bitmap_cache.GetCacheItem(m_strIconPath, false);
	else
	{
		m_pCachedBitmap = bitmap_cache.GetCacheItem(m_strFilePath, true);
		
		// If it is a LNK-file, resolve the target name.
		// We do this, because executing a link is *much* slower than executing the resolved EXE directly.
		if (m_strFilePath.right(4) == _T(".lnk") || m_strFilePath.right(4) == _T(".LNK"))
		{
			RString target;
			if (SUCCEEDED(ResolveLink(NULL, m_strFilePath, target)))
				m_strFilePath = target;
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::CIcon()
// --------------------------------------------------------------------------------------------------------------------------------------------
CIcon::CIcon(CBitmapCache &bitmap_cache, const RString &strTitle, const RString &strFilePath, const RString &strIconPath)
	:	m_strFilePath(strFilePath),
		m_strIconPath(strIconPath),
		m_strFullTitle(strTitle),
		m_enState(enStateNormal),
		m_fTargetScale(1.f),
		m_MovedBy(0, 0),
		m_bIsAnimated(false),
		m_bIsUnlinked(false),
		m_fStepsToAnimateMove(0),
		m_bResynced(false),
		m_pCachedBitmap(nullptr)
{	
	// create or add cached bitmap
	CreateOrAddCachedBitmap(bitmap_cache);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::CIcon(Stream)
// --------------------------------------------------------------------------------------------------------------------------------------------
CIcon::CIcon(Stream &stream)
	:	m_enState(enStateNormal),
		m_fTargetScale(1.f),
		m_MovedBy(0, 0),
		m_bIsAnimated(false),
		m_bIsUnlinked(false),
		m_fStepsToAnimateMove(0),
		m_bResynced(false),
		m_pCachedBitmap(nullptr)
{
	m_strFilePath = stream.ReadString();
	m_strParameters = stream.ReadString();
//	m_bUseShellApi = stream.ReadBool();
	m_strIconPath = stream.ReadString();
	m_strFullTitle = stream.ReadString();

	int pixels_x = stream.ReadUInt();	// pixel resolution
	int pixels_y = stream.ReadUInt();	// pixel resolution

	m_Position.m_RasterPos.m_nCol = stream.ReadInt();
	m_Position.m_RasterPos.m_nRow = stream.ReadInt();
	m_Position.m_nPage = stream.ReadInt();

	CMD5Hash md5hash;
	md5hash.Read(stream);

	// link to cached bitmap
	CBitmapCache *bitmap_cache = (CBitmapCache *)stream.GetExtra();
	m_pCachedBitmap = bitmap_cache->GetCacheItem(md5hash);

	// create icon
	CreateIcon(pixels_x);
	RenderTitleTextLayout();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::CIcon(Stream)
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::Write(Stream &stream) const
{
	stream.WriteString(m_strFilePath);
	stream.WriteString(m_strParameters);
//	stream.WriteBool(m_bUseShellApi);
	stream.WriteString(m_strIconPath);
	stream.WriteString(m_strFullTitle);
	
	stream.WriteUInt(m_Sprite.getTexture()->getSize().x);	// pixel resolution
	stream.WriteUInt(m_Sprite.getTexture()->getSize().y);	// pixel resolution

	stream.WriteInt(m_Position.m_RasterPos.m_nCol);
	stream.WriteInt(m_Position.m_RasterPos.m_nRow);
	stream.WriteInt(m_Position.m_nPage);

	if (m_pCachedBitmap)
		m_pCachedBitmap->GetHash().Write(stream);
	else
	{
		CMD5Hash dummy;
		dummy.Write(stream);
	}
	
	// todo: scaled draw texture noch schreiben? oder in CIcon::CIcon(Stream &stream) ==> CreateIcon() aufrufen?
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::SetIconPath()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::SetIconPath(CBitmapCache &bitmap_cache, const RString &new_path)
{
	if (m_strIconPath == new_path)
		return;		// no change

	m_strIconPath = new_path;
	CreateOrAddCachedBitmap(bitmap_cache);
	CreateIcon(gConfig.m_nIconSizeX);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::SetFilePath()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::SetFilePath(CBitmapCache &bitmap_cache, const RString &new_path)
{
	if (m_strFilePath == new_path)
		return;		// no change
	m_strFilePath = new_path;
	if (m_strIconPath.empty())
	{
		bitmap_cache.Unref(m_pCachedBitmap);
		m_pCachedBitmap = bitmap_cache.GetCacheItem(m_strFilePath, true);
		CreateIcon(gConfig.m_nIconSizeX);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::SetFullTitle()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::SetFullTitle(const RString &title)
{
	if (m_strFullTitle == title)
		return;		// no change
	m_strFullTitle = title;
	RenderTitleTextLayout();
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::Resync()
//
// Checks, if the bitmap of the underlying target has changed and loads the new bitmap
// returns true, if changed
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIcon::Resync(CBitmapCache &bitmap_cache)
{
	if (m_bResynced)
		return false;

	m_bResynced = true;

	CBitmapCacheItem *new_item;
	if (!m_strIconPath.empty())
		new_item = bitmap_cache.GetCacheItem(m_strIconPath, false);
	else
		new_item = bitmap_cache.GetCacheItem(m_strFilePath, true);

	if (new_item != m_pCachedBitmap)
	{
		bitmap_cache.Unref(m_pCachedBitmap);
		m_pCachedBitmap = new_item;
		CreateIcon(gConfig.m_nIconSizeX);
		return true;
	}
	
	bitmap_cache.Unref(new_item);
	return false;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::CreateIcon()
//
// Creates a sprite which will be exactly scaled to the desired pixels.
// Chooses the best fitting texture.
// Renders the title text layout.
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::CreateIcon(int pixels, bool force_create)
{
	if (m_pCachedBitmap)
	{
		if (force_create || m_pCachedBitmap->GetDrawTexture().getSize().x != pixels)
				m_pCachedBitmap->CreateScaledDrawTexture(pixels);

		// assign texture to sprite and setup sprite
//		m_Sprite.setTexture(m_pCachedBitmap->GetDrawTexture(), true);
	}

	// if the real screen resolution is changed, it is essential to re-assign the texture to the sprite!
	// otherwise some icons and the positons of their titles are wrong.
	m_Sprite.setTexture(m_pCachedBitmap->GetDrawTexture(), true);
	m_Sprite.setOrigin((float)pixels / 2.f, (float)pixels / 2.f);
	m_Sprite.setColor(sf::Color(255, 255, 255, 255));
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::IsVisible()
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIcon::IsVisible(int view_left, int view_right)
{
	float left = m_Sprite.getGlobalBounds().left;
	float right = left + m_Sprite.getGlobalBounds().width;
	return left <= (float)view_right && right >= view_left;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::Draw()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::Draw(sf::RenderWindow &window)
{
	if (m_enState == enStateHighlight || m_enState == enStateDragging)
	{
		_pShader->setParameter("xscale", 1.0f / m_Sprite.getGlobalBounds().width * m_Sprite.getScale().x);
		_pShader->setParameter("yscale", 1.0f / m_Sprite.getGlobalBounds().height * m_Sprite.getScale().y);
		window.draw(m_Sprite, _pShader);
	}
	else
		window.draw(m_Sprite);
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::DrawTitle()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::DrawTitle(sf::RenderWindow &window, sf::Text &text)
{
	sf::FloatRect rc = m_Sprite.getGlobalBounds();
	sf::Vector2f pos = m_Sprite.getPosition();

#if 0
	// moving with the bottom of a scaled sprite
	// float y = floor(rc.top + rc.height + m_MovedBy.y + gConfig.ScaleCoord(6) + 0.5f);
#else
	float y = floor(pos.y + m_Sprite.getTextureRect().height / 2.f - m_MovedBy.y + gConfig.ScaleCoord(6) + 0.5f);
	//pos.x -= m_Sprite.getTextureRect().width / 2.f;
#endif

	float line_spacing = text.getFont()->getLineSpacing((int)(text.getCharacterSize())) - 1;	// -1 looks better

	/* Highlight the text - deactivated, doesn't look that good

	float org_y = y;
	if (m_enState == enStateHighlight || m_enState == enStateDragging)
	{
		// draw selection background 
		_foreach(it, m_Title.m_arTextLines)
		{
			text.setString(it->m_strText.c_str());
			float x = floor(rc.left - m_MovedBy.x + it->m_nOffset + 0.5f);

			text.setPosition(x, y);
			sf::FloatRect backgroundRect = text.getGlobalBounds();
			sf::RectangleShape background(sf::Vector2f(backgroundRect.width + 4, line_spacing + 2));
			background.setPosition(x - 1, y - 1);
			//background.setFillColor(sf::Color(0, 107, 215));
			background.setFillColor(sf::Color(255, 170, 21));
			window.draw(background);

			y += line_spacing;
		}
	}

	y = org_y;*/
	_foreach(it, m_Title.m_arTextLines)
	{
		text.setString(it->m_strText.c_str());
#if 1
		float x = floor(rc.left - m_MovedBy.x + it->m_nOffset + 0.5f);
#else
		float x = floor(pos.x - it->m_nOffset + 0.5f);
#endif
		text.setPosition(x + 1, y + 1);
		text.setColor(sf::Color(0x40, 0x40, 0x40));
		window.draw(text);

		text.setPosition(x, y);
		text.setColor(sf::Color(255, 255, 255));
		window.draw(text);

		//y += text.getCharacterSize() * 1.3f;
		y += line_spacing;
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::SetState()
// returns false, if requested state is already set
// --------------------------------------------------------------------------------------------------------------------------------------------
//const sf::Color colorHighlight(255, 255, 255, 208); //232));
const sf::Color colorHighlight(255, 255, 255, 128); //232));

bool CIcon::SetState(EIconState state, bool scale_now)
{
	if (m_enState == state)
		return false;

	m_fSpriteScaleInc = 0.02f;	// animate slowly

	switch(state)
	{
	case enStateNormal:
		m_fTargetScale = 1.f;
		if (m_enState == enStateOpen)
			m_fSpriteScaleInc = 0.125f; // animate fast
		m_Sprite.setColor(sf::Color(255, 255, 255, 255));
		break;

	case enStateHighlight:
		m_fTargetScale = 1.2f;
		//m_Sprite.setColor(colorHighlight);
		break;

	case enStateDragging:
		m_fTargetScale = 1.f + 16 * m_fSpriteScaleInc;
		//m_Sprite.setColor(colorHighlight);
		break;

	case enStateOpen:
		m_fSpriteScaleInc = 0.125f; // animate fast
		m_fTargetScale = 2.f;
		break;

	case enStateBlowup:
		m_fSpriteScaleInc = 0.125f; // animate fast
		m_fTargetScale = 4.f;
		break;
	}

	m_enState = state;

	if (scale_now)
	{
		sf::FloatRect rc_old = m_Sprite.getGlobalBounds();

		m_Sprite.setScale(m_fTargetScale, m_fTargetScale);

		sf::FloatRect rc_new = m_Sprite.getGlobalBounds();
		m_MovedBy.x += rc_new.left - rc_old.left;
		m_MovedBy.y += (rc_old.top + rc_old.height - (rc_new.top + rc_new.height));
	}

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::PerformScalingAnimation()
// returns false, if the animation sequence is finished
// --------------------------------------------------------------------------------------------------------------------------------------------
bool CIcon::PerformScalingAnimation()
{
	if (!IsScaling())
		return false;

	sf::FloatRect rc_old = m_Sprite.getGlobalBounds();
			
	float sprite_x_scale = m_Sprite.getScale().x;
	if (sprite_x_scale < m_fTargetScale)
	{
		sprite_x_scale += m_fSpriteScaleInc;
		if (sprite_x_scale > m_fTargetScale)
			sprite_x_scale = m_fTargetScale;
	}
	else
	{
		sprite_x_scale -= m_fSpriteScaleInc;
		if (sprite_x_scale < m_fTargetScale)
			sprite_x_scale = m_fTargetScale;
	}

	m_Sprite.setScale(sprite_x_scale, sprite_x_scale);

	sf::FloatRect rc_new = m_Sprite.getGlobalBounds();
	m_MovedBy.x += rc_new.left - rc_old.left;
	m_MovedBy.y += (rc_old.top + rc_old.height - (rc_new.top + rc_new.height));

	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//															CIcon::InitMoveAnimation()
// --------------------------------------------------------------------------------------------------------------------------------------------
void CIcon::InitMoveAnimation(float x, float y)
{
	sf::Vector2f cur_pos = GetSpritePosition();

	float dx = x - cur_pos.x;
	float dy = y - cur_pos.y;

	float abs_dx = fabs(dx);
	float abs_dy = fabs(dy);

	if (abs_dx > abs_dy)
		m_fStepsToAnimateMove = CalcAnimationSteps(abs_dx);
	else
		m_fStepsToAnimateMove = CalcAnimationSteps(abs_dy);

	m_fAnimateMoveX = dx / m_fStepsToAnimateMove;
	m_fAnimateMoveY = dy / m_fStepsToAnimateMove;
}
