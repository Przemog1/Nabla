// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_STL_LOADER_

#include "CSTLMeshFileLoader.h"
#include "SMesh.h"

#include "IReadFile.h"
#include "coreutil.h"
#include "os.h"
#include "SVertexManipulator.h"

#include <vector>

namespace irr
{
namespace scene
{

asset::IAsset* CSTLMeshFileLoader::loadAsset(io::IReadFile* _file, const asset::IAssetLoader::SAssetLoadParams& _params, asset::IAssetLoader::IAssetLoaderOverride* _override, uint32_t _hierarchyLevel)
{
	const long filesize = _file->getSize();
	if (filesize < 6) // we need a header
		return nullptr;

    bool hasColor = false;

	SCPUMesh* mesh = new SCPUMesh();
	ICPUMeshDataFormatDesc* desc = new ICPUMeshDataFormatDesc();
    {
	ICPUMeshBuffer* meshbuffer = new ICPUMeshBuffer();
	meshbuffer->setMeshDataAndFormat(desc);
	desc->drop();

	mesh->addMeshBuffer(meshbuffer);
	meshbuffer->drop();
    }

	bool binary = false;
	core::stringc token;
	if (getNextToken(_file, token) != "solid")
		binary = hasColor = true;

    core::vector<core::vectorSIMDf> positions, normals;
    core::vector<uint32_t> colors;
	if (binary)
	{
        if (_file->getSize() < 80)
        {
            mesh->drop();
            return nullptr;
        }
		_file->seek(80); // skip header
        uint32_t vtxCnt = 0u;
		_file->read(&vtxCnt, 4);
        positions.reserve(3*vtxCnt);
        normals.reserve(vtxCnt);
        colors.reserve(vtxCnt);
	}
	else
		goNextLine(_file); // skip header


	uint16_t attrib=0u;
	token.reserve(32);
	while (_file->getPos() < filesize)
	{
		if (!binary)
		{
			if (getNextToken(_file, token) != "facet")
			{
				if (token=="endsolid")
					break;
				mesh->drop();
				return nullptr;
			}
			if (getNextToken(_file, token) != "normal")
			{
				mesh->drop();
				return nullptr;
			}
		}

        {
        core::vectorSIMDf n;
		getNextVector(_file, n, binary);
        normals.push_back(n);
        }

		if (!binary)
		{
			if (getNextToken(_file, token) != "outer")
			{
				mesh->drop();
				return nullptr;
			}
			if (getNextToken(_file, token) != "loop")
			{
				mesh->drop();
				return nullptr;
			}
		}

        {
        core::vectorSIMDf p[3];
		for (uint32_t i = 0u; i < 3u; ++i)
		{
			if (!binary)
			{
				if (getNextToken(_file, token) != "vertex")
				{
					mesh->drop();
					return nullptr;
				}
			}
			getNextVector(_file, p[i], binary);
		}
        for (uint32_t i = 0u; i < 3u; ++i) // seems like in STL format vertices are ordered in clockwise manner...
            positions.push_back(p[2u-i]);
        }

		if (!binary)
		{
			if (getNextToken(_file, token) != "endloop")
			{
				mesh->drop();
				return nullptr;
			}
			if (getNextToken(_file, token) != "endfacet")
			{
				mesh->drop();
				return nullptr;
			}
		}
		else
		{
			_file->read(&attrib, 2);
		}

        if (hasColor && (attrib & 0x8000)) // assuming VisCam/SolidView non-standard trick to store color in 2 bytes of extra attribute
        {
            colors.push_back(video::A1R5G5B5toA8R8G8B8(attrib));
        }
        else
        {
            hasColor = false;
            colors.clear();
        }

		if ((normals.back() == core::vectorSIMDf()).all())
        {
			normals.back().set(
                core::plane3df(
                    (positions.rbegin()+2)->getAsVector3df(),
                    (positions.rbegin()+1)->getAsVector3df(),
                    (positions.rbegin()+0)->getAsVector3df()).Normal
            );
        }
	} // end while (_file->getPos() < filesize)

    const size_t vtxSize = hasColor ? (3 * sizeof(float) + 4 + 4) : (3 * sizeof(float) + 4);
	core::ICPUBuffer* vertexBuf = new core::ICPUBuffer(vtxSize*positions.size());

    uint32_t normal{};
    for (size_t i = 0u; i < positions.size(); ++i)
    {
        if (i%3 == 0)
            normal = quantizeNormal2_10_10_10(normals[i/3]);
        uint8_t* ptr = ((uint8_t*)(vertexBuf->getPointer())) + i*vtxSize;
        memcpy(ptr, positions[i].pointer, 3*4);
        ((uint32_t*)(ptr+12))[0] = normal;
        if (hasColor)
            memcpy(ptr+16, colors.data()+i/3, 4);
    }

	desc->mapVertexAttrBuffer(vertexBuf, EVAI_ATTR0, ECPA_THREE, ECT_FLOAT, vtxSize, 0);
	desc->mapVertexAttrBuffer(vertexBuf, EVAI_ATTR3, ECPA_FOUR, ECT_INT_2_10_10_10_REV, vtxSize, 12);
    if (hasColor)
	    desc->mapVertexAttrBuffer(vertexBuf, EVAI_ATTR1, ECPA_REVERSED_OR_BGRA, ECT_NORMALIZED_UNSIGNED_BYTE, vtxSize, 16);
	vertexBuf->drop();

	mesh->getMeshBuffer(0)->setIndexCount(positions.size());
    //mesh->getMeshBuffer(0)->setPrimitiveType(EPT_POINTS);
	mesh->recalculateBoundingBox(true);

	return mesh;
}

bool CSTLMeshFileLoader::isALoadableFileFormat(io::IReadFile * _file) const
{
    if (_file->getSize() <= 6u)
        return false;

    char header[6];
    const size_t prevPos = _file->getPos();
    _file->seek(0u);
    _file->read(header, 6u);
    _file->seek(prevPos);

    if (strncmp(header, "solid ", 6u) == 0)
        return true;
    else
        return _file->getSize() > 80u; // some other validation also needed here...
}

//! Read 3d vector of floats
void CSTLMeshFileLoader::getNextVector(io::IReadFile* file, core::vectorSIMDf& vec, bool binary) const
{
	if (binary)
	{
		file->read(&vec.X, 4);
		file->read(&vec.Y, 4);
		file->read(&vec.Z, 4);
	}
	else
	{
		goNextWord(file);
		core::stringc tmp;

		getNextToken(file, tmp);
		sscanf(tmp.c_str(),"%f",&vec.X);
		getNextToken(file, tmp);
		sscanf(tmp.c_str(),"%f",&vec.Y);
		getNextToken(file, tmp);
		sscanf(tmp.c_str(),"%f",&vec.Z);
	}
	vec.X=-vec.X;
}


//! Read next word
const core::stringc& CSTLMeshFileLoader::getNextToken(io::IReadFile* file, core::stringc& token) const
{
	goNextWord(file);
	uint8_t c;
	token = "";
	while(file->getPos() != file->getSize())
	{
		file->read(&c, 1);
		// found it, so leave
		if (core::isspace(c))
			break;
		token.append(c);
	}
	return token;
}


//! skip to next word
void CSTLMeshFileLoader::goNextWord(io::IReadFile* file) const
{
	uint8_t c;
	while(file->getPos() != file->getSize())
	{
		file->read(&c, 1);
		// found it, so leave
		if (!core::isspace(c))
		{
			file->seek(-1, true);
			break;
		}
	}
}


//! Read until line break is reached and stop at the next non-space character
void CSTLMeshFileLoader::goNextLine(io::IReadFile* file) const
{
	uint8_t c;
	// look for newline characters
	while(file->getPos() != file->getSize())
	{
		file->read(&c, 1);
		// found it, so leave
		if (c=='\n' || c=='\r')
			break;
	}
}

} // end namespace scene
} // end namespace irr


#endif // _IRR_COMPILE_WITH_STL_LOADER_
