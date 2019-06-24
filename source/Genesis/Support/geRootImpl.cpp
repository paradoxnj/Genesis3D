/*
	@file geRootImpl.cpp
	@author Anthony Rufrano (paradoxnj)
	@brief Root class to create engine objects

	The contents of this file are subject to the Genesis3D Public License
	Version 1.01 (the "License"); you may not use this file except in
	compliance with the License. You may obtain a copy of the License at
	http://www.genesis3d.com

	Software distributed under the License is distributed on an "AS IS"
	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
	the License for the specific language governing rights and limitations
	under the License.

	The Original Code is Genesis3D, released March 25, 1999.
	Genesis3D Version 1.1 released November 15, 1999
	Copyright (C) 1999 WildTangent, Inc. All Rights Reserved
*/
#include "geRootImpl.h"

namespace g3d {

	GENESISAPI IgeRoot *geRoot_Create()
	{
		return new geRootImpl();
	}

	geRootImpl::geRootImpl() : m_pLog(nullptr)
	{
		m_pLog = new geFileLogger("Genesis3d", ".\\", LogFatal | LogError | LogWarn | LogInfo);
		assert(m_pLog);

		m_iRefCount = 1;

		logMessage(LogInfo, "Genesis3D Logging Started");
	}

	geRootImpl::~geRootImpl()
	{
		GE_SAFE_DELETE(m_pLog);
	}

	uint32 geRootImpl::addRef()
	{
		m_iRefCount++;
		return m_iRefCount;
	}

	uint32 geRootImpl::release()
	{
		m_iRefCount--;
		if (m_iRefCount == 0)
		{
			delete this;
			return 0;
		}

		return m_iRefCount;
	}

	geBoolean geRootImpl::logMessage(const LogThreshold &thresh, const std::string &strMessage)
	{
		if (!m_pLog)
			return GE_FALSE;

		m_pLog->logMessage(thresh, strMessage);
		return GE_TRUE;
	}
}
