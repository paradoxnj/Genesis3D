/*
	@file geRoot.h
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
#pragma once

#include <string>

#include "BaseType.h"
#include "geLogger.h"

namespace g3d {

	class IgeRoot : virtual public IgeUnknown
	{
	protected:
		virtual ~IgeRoot() {}

	public:
		virtual geBoolean logMessage(const LogThreshold &threshold, const std::string &strMessage) = 0;
	};
}
