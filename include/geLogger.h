#ifndef JE_LOGGER_H
#define JE_LOGGER_H
/*!
	@file jeLogger.h
	
	@author Gerald LePage (darkriftx)
	@brief Abstract logger class header

	@par License
	The contents of this file are subject to the Jet3D Public License       
	Version 1.02 (the "License"); you may not use this file except in         
	compliance with the License. You may obtain a copy of the License at       
	http://www.jet3d.com                                                        
                                                                             
	@par
	Software distributed under the License is distributed on an "AS IS"           
	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See           
	the License for the specific language governing rights and limitations          
	under the License.                                                               
                                                                                  
	@par
	The Original Code is Jet3D, released December 12, 1999.                            
	Copyright (C) 1996-1999 Eclipse Entertainment, L.L.C. All Rights Reserved           
*/

#include <iostream>
#include <sstream>
#include <ctime>

namespace g3d
{
	typedef enum LogThreshold
	{
		LogFatal = 0x0001,
		LogError = 0x0002,
		LogWarn = 0x0004,
		LogInfo = 0x0008,
		LogDebug = 0x0010,
	} LogThreshold;

	//Abstract base logging class
	class geLogger
	{
	private:
		geLogger();
		geLogger(const geLogger& rhs);
		geLogger& operator=(const geLogger& rhs);
	public:
		
	protected:
		std::string _name;
		unsigned short _logThreshold;
		bool _logOpen;

	protected:
		virtual bool openLog()=0;
		virtual bool flushLog()=0;
		virtual bool closeLog()=0;
		virtual void _logMessage(const LogThreshold& level, const std::string& message)=0;
		bool isLogOpen() const
		{
			return(_logOpen);
		}
		const char* getLogLevelName(const LogThreshold& level) const
		{
			switch(level)
			{
			case LogFatal : return("FATAL");
			case LogError : return("ERROR");
			case LogWarn  : return("WARN");
			case LogInfo  : return("INFO");
			case LogDebug : return("DEBUG");
			default: return("????");
			}
		}

		const char* getTimeStamp() const
		{
			time_t curTime = time(NULL);
			return(ctime(&curTime));
		}

	public:
		geLogger(const std::string& name,
			     const unsigned short& logThreshold) :
			_name(name),
			_logThreshold(logThreshold),
			_logOpen(false)
		{
		}

		virtual ~geLogger()
		{
		}

		void setLevelOn(const LogThreshold& level)
		{
			//Set the level on using the OR bitwise operator
			_logThreshold |= level;
		}

		void setLevelOff(const LogThreshold& level)
		{
			//Turn the level off using the exclusive OR bitwise operator
			//which sets to zero in the resultant operand any bit that was
			//set to one in both operands of the operation
			_logThreshold ^= level;
		}

		bool isLevelOn(const LogThreshold& level) const
		{
			return( (level & _logThreshold) != 0 );
		}

		void logMessage(const LogThreshold& level, const std::string& message)
		{
			if (isLevelOn(level) && isLogOpen())
			{
				_logMessage(level, message);
			}
		}

		void logMessage(const LogThreshold& level, const char* message)
		{
			if (isLevelOn(level) && isLogOpen())
			{
				_logMessage(level, std::string(message));
			}
		}

		void logMessage(const LogThreshold& level, std::ostringstream& oStr)
		{
			if (isLevelOn(level) && isLogOpen())
			{
				_logMessage(level, oStr.str());
			}
		}
	};

}


#endif //GE_LOGGER_H