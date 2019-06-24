#ifndef GE_FILE_LOGGER_H
#define GE_FILE_LOGGER_H
/*!
	@file geFileLogger.h
	
	@author Gerald LePage (darkriftx)
	@brief File logger class header

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

#include <fstream>
#include "geLogger.h"

namespace g3d
{
	class geFileLogger : public geLogger
	{
	private:
		geFileLogger();
		geFileLogger(const geFileLogger& rhs);
		geFileLogger& operator=(const geFileLogger& rhs);
	private:
		std::ofstream _outputFile;
		std::string _directory;
	protected:
		virtual bool openLog()
		{
			if (_logOpen)return(false);
			std::ostringstream fName;
			fName << _directory << _name << ".log";
			_outputFile.open(fName.str().c_str());
			if (!_outputFile)return(false);			
			_logOpen = true;
			return(true);
		}

		virtual bool flushLog()
		{
			if (!_logOpen)return(false);
			_outputFile.flush();
			return(true);
		}

		virtual bool closeLog()
		{
			if (!_logOpen)return(false);
			_outputFile.close();
			return(true);
		}


		virtual void _logMessage(const LogThreshold& level, const std::string& message)
		{
			std::string logLevelName(getLogLevelName(level));
			std::string curTimeStamp(getTimeStamp());

			_outputFile << logLevelName << "\t" << curTimeStamp.substr(0, curTimeStamp.length()-2)  << "\t" << message << std::endl;
			flushLog();
		}

	public:
		geFileLogger(const std::string& name, 
			const std::string& directory, 
			const unsigned short& threshold) : geLogger(name, threshold),
			_outputFile(),
			_directory(directory)
		{
			openLog();
		}

		virtual ~geFileLogger()
		{
			flushLog();
			closeLog();
		}

	};

}

#endif //GE_FILE_LOGGER_H
