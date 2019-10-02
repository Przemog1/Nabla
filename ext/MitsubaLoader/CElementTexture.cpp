#include "../../ext/MitsubaLoader/ParserUtil.h"
#include "../../ext/MitsubaLoader/CElementFactory.h"

#include "../../ext/MitsubaLoader/CGlobalMitsubaMetadata.h"

#include <functional>

namespace irr
{
namespace ext
{
namespace MitsubaLoader
{


template<>
IElement* CElementFactory::createElement<CElementTexture>(const char** _atts, ParserManager* _util)
{
	const char* type;
	const char* id;
	if (!IElement::getTypeAndIDStrings(type, id, _atts))
		return nullptr;

	static const core::unordered_map<std::string, CElementTexture::Type, core::CaseInsensitiveHash, core::CaseInsensitiveEquals> StringToType =
	{
		{"bitmap",			CElementTexture::Type::BITMAP},
		{"scale",			CElementTexture::Type::SCALE}
	};

	auto found = StringToType.find(type);
	if (found==StringToType.end())
	{
		ParserLog::invalidXMLFileStructure("unknown type");
		_IRR_DEBUG_BREAK_IF(false);
		return nullptr;
	}

	CElementTexture* obj = _util->objects.construct<CElementTexture>(id);
	if (!obj)
		return nullptr;

	obj->type = found->second;
	// defaults
	switch (obj->type)
	{
		case CElementTexture::Type::BITMAP:
			obj->bitmap = CElementTexture::Bitmap();
			break;
		case CElementTexture::Type::SCALE:
			obj->scale = CElementTexture::Scale();
			break;
		default:
			break;
	}
	return obj;
}

bool CElementTexture::addProperty(SNamedPropertyElement&& _property)
{
	if (type==CElementTexture::Type::SCALE)
	{
		if (_property.type!=SPropertyElementData::Type::FLOAT)
			return false;
		scale.scale = _property.fvalue;
		return true;
	}


	bool error = false;
	auto dispatch = [&](auto func) -> void
	{
		switch (type)
		{
			case CElementTexture::Type::BITMAP:
				func(bitmap);
				break;
			default:
				error = true;
				break;
		}
	};

#define SET_PROPERTY_TEMPLATE(MEMBER,PROPERTY_TYPE, ... )		[&]() -> void { \
		dispatch([&](auto& state) -> void { \
			IRR_PSEUDO_IF_CONSTEXPR_BEGIN(is_any_of<std::remove_reference<decltype(state)>::type,__VA_ARGS__>::value) \
			{ \
				if (_property.type!=PROPERTY_TYPE) { \
					error = true; \
					return; \
				} \
				state. ## MEMBER = _property.getProperty<PROPERTY_TYPE>(); \
			} \
			IRR_PSEUDO_IF_CONSTEXPR_END \
		}); \
	}

	auto processFilename = [&]() -> void
	{ 
		dispatch([&](auto& state) -> void {
			using state_type = std::remove_reference<decltype(state)>::type;

			IRR_PSEUDO_IF_CONSTEXPR_BEGIN(std::is_same<state_type,Bitmap>::value)
			{
				bitmap.filename = std::move(_property);
			}
			IRR_PSEUDO_IF_CONSTEXPR_END
		});
	};
	auto getWrapMode = [&]() -> Bitmap::WRAP_MODE {
		static const core::unordered_map<std::string, Bitmap::WRAP_MODE, core::CaseInsensitiveHash, core::CaseInsensitiveEquals> StringToWrap =
		{
			{"repeat",	Bitmap::WRAP_MODE::REPEAT},
			{"mirror",	Bitmap::WRAP_MODE::MIRROR},
			{"clamp",	Bitmap::WRAP_MODE::CLAMP},
			{"zero",	Bitmap::WRAP_MODE::ZERO},
			{"one",		Bitmap::WRAP_MODE::ONE}
		};
		auto found = StringToWrap.end();
		if (_property.type == SPropertyElementData::Type::STRING)
			found = StringToWrap.find(_property.getProperty<SPropertyElementData::Type::STRING>());
		if (found != StringToWrap.end())
			return found->second;
		return Bitmap::WRAP_MODE::REPEAT;
	};
	auto processWrapMode = [&]() -> void
	{ 
		dispatch([&](auto& state) -> void {
			using state_type = std::remove_reference<decltype(state)>::type;

			IRR_PSEUDO_IF_CONSTEXPR_BEGIN(std::is_same<state_type,Bitmap>::value)
			{
				auto value = getWrapMode();
				state.wrapModeU = value;
				state.wrapModeV = value;
			}
			IRR_PSEUDO_IF_CONSTEXPR_END
		});
	};
	auto processWrapModeU =  [&]() -> void
	{ 
		dispatch([&](auto& state) -> void {
			using state_type = std::remove_reference<decltype(state)>::type;

			IRR_PSEUDO_IF_CONSTEXPR_BEGIN(std::is_same<state_type,Bitmap>::value)
			{
				state.wrapModeU = getWrapMode();
			}
			IRR_PSEUDO_IF_CONSTEXPR_END
		});
	};
	auto processWrapModeV =  [&]() -> void
	{ 
		dispatch([&](auto& state) -> void {
			using state_type = std::remove_reference<decltype(state)>::type;

			IRR_PSEUDO_IF_CONSTEXPR_BEGIN(std::is_same<state_type,Bitmap>::value)
			{
				state.wrapModeV = getWrapMode();
			}
			IRR_PSEUDO_IF_CONSTEXPR_END
		});
	};
	auto processGamma = SET_PROPERTY_TEMPLATE(gamma,SPropertyElementData::Type::FLOAT,Bitmap);
	auto processFilterType = [&]() -> void
	{ 
		dispatch([&](auto& state) -> void {
			using state_type = std::remove_reference<decltype(state)>::type;

			IRR_PSEUDO_IF_CONSTEXPR_BEGIN(std::is_same<state_type,Bitmap>::value)
			{
				static const core::unordered_map<std::string,Bitmap::FILTER_TYPE,core::CaseInsensitiveHash,core::CaseInsensitiveEquals> StringToType =
				{
					{"ewa",			Bitmap::FILTER_TYPE::EWA},
					{"trilinear",	Bitmap::FILTER_TYPE::TRILINEAR},
					{"nearest",		Bitmap::FILTER_TYPE::NEAREST}
				};
				auto found = StringToType.end();
				if (_property.type==SPropertyElementData::Type::STRING)
					found = StringToType.find(_property.getProperty<SPropertyElementData::Type::STRING>());
				if (found!=StringToType.end())
				{
					error = true;
					return;
				}
				state.filterType = found->second;
			}
			IRR_PSEUDO_IF_CONSTEXPR_END
		});
	};
	auto processMaxAnisotropy = SET_PROPERTY_TEMPLATE(maxAnisotropy,SPropertyElementData::Type::FLOAT,Bitmap);
	//auto processCache = SET_PROPERTY_TEMPLATE(cache,SPropertyElementData::Type::BOOLEAN,Bitmap);
	auto processUoffset = SET_PROPERTY_TEMPLATE(uoffset,SPropertyElementData::Type::FLOAT,Bitmap);
	auto processVoffset = SET_PROPERTY_TEMPLATE(voffset,SPropertyElementData::Type::FLOAT,Bitmap);
	auto processUscale = SET_PROPERTY_TEMPLATE(uscale,SPropertyElementData::Type::FLOAT,Bitmap);
	auto processVscale = SET_PROPERTY_TEMPLATE(vscale,SPropertyElementData::Type::FLOAT,Bitmap);
	//auto processChannel = ;


	static const core::unordered_map<std::string, std::function<void()>, core::CaseInsensitiveHash, core::CaseInsensitiveEquals> SetPropertyMap =
	{
		{"filename",		processFilename},
		{"wrapMode",		processWrapMode},
		{"wrapModeU",		processWrapModeU},
		{"wrapModeV",		processWrapModeV},
		{"gamma",			processGamma},
		{"filterType",		processFilterType},
		{"maxAnisotropy",	processMaxAnisotropy},
		//{"cache",			processCache},
		{"uoffset",			processUoffset},
		{"voffset",			processVoffset},
		{"uscale",			processUscale},
		{"vscale",			processVscale}//,
		//{"channel",		processChannel)
	};
	
	auto found = SetPropertyMap.find(_property.name);
	if (found==SetPropertyMap.end())
	{
		_IRR_DEBUG_BREAK_IF(true);
		ParserLog::invalidXMLFileStructure("No BSDF can have such property set with name: "+_property.name);
		return false;
	}

	found->second();
	return !error;
}


bool CElementTexture::processChildData(IElement* _child, const char* name)
{
	if (!_child)
		return true;

	switch (_child->getType())
	{
		case IElement::Type::TEXTURE:
			{
				auto _texture = static_cast<CElementTexture*>(_child);
				switch (type)
				{
					case Type::SCALE:
						scale.texture = _texture;
						break;
					default:
						_IRR_DEBUG_BREAK_IF(true);
						ParserLog::invalidXMLFileStructure("No supported texture can have a texture as child element, except for \"scale\"");
						return false;
						break;
				}
			}
			break;
		default:
			return false;
			break;
	}
	return true;
}

bool CElementTexture::onEndTag(asset::IAssetLoader::IAssetLoaderOverride* _override, CGlobalMitsubaMetadata* globalMetadata)
{
	if (type == Type::INVALID)
	{
		ParserLog::invalidXMLFileStructure(getLogName() + ": type not specified");
		_IRR_DEBUG_BREAK_IF(true);
		return true;
	}
	
	// TODO: Validation
	{
	}

	return true;
}

}
}
}