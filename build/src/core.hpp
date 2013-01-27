#pragma once



namespace core
{
	// generic error struct instead of trying to organize various int values across API calls.
	struct Error
	{
		enum
		{
			Failure = 0xBADDAE, // non-recoverable error, bad day :(
			Warning, // unexpected result, will proceed
		};
		
		int status;
		const char * message;
		
		Error( int error_status, const char * error_message = 0 );
		bool failed() const { return status == Failure; }
	};
	
	Error startup();
	void shutdown();
};