/*
	@file geSingleton.h
	@author Anthony Rufrano (paradoxnj)
	@brief Templated singleton class

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

#include <assert.h>
#include "BaseType.h"

namespace g3d {

template <typename T> class geSingleton
{
protected:
	static T* s_Singleton;

private:
	geSingleton(const geSingleton<T> &);
	geSingleton& operator=(const geSingleton<T> &);

public:
	geSingleton()
	{
//		assert(!s_Singleton);
		s_Singleton = static_cast<T*>(this);
	}

	virtual ~geSingleton()
	{
		assert(s_Singleton);
		s_Singleton = 0;
	}

	static T& getSingleton()
	{
		assert(s_Singleton);
		return *s_Singleton;
	}

	static T* getSingletonPtr()
	{
		return s_Singleton;
	}
};

template <typename T> T* geSingleton<T>::s_Singleton = 0;

} // namespace g3d
