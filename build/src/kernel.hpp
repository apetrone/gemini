namespace kernel
{
	// kernel error codes, returned by kernel::main()
	enum Error
	{
		NoError = 0,
		NotFound = -1,
		NoKernel = -2,
		NoInstance = -3,
		ConfigFailed = -4,
		StartupFailed = -5,
		CoreFailed = -6
	};

	// Kernel flags for device details
	enum
	{
		DeviceDesktop 					= (1 << 0), // 'device' is a desktop computer
		DeviceiPhone 					= (1 << 1), // set if this is an iPhone
		DeviceiPad 						= (1 << 2), // set if this is an iPad
		DeviceSupportsRetinaDisplay 	= (1 << 3), // set if this device supports retina
	};

	// status codes for config and startup return values
	enum
	{
		Failure = 0,
		Success = 1,
		NoWindow = -1
	};

	// parameters passed to callbacks
	struct Params
	{
		enum
		{
			Invalid = 0,
			GainFocusEvent,
			LostFocusEvent,
			ResizeEvent,
		};
		
		int argc;
		char ** argv;
		const char * error_message;
		const char * window_title;
		
		// kDevice constants above describe the current system
		unsigned char device_flags;
	
		// dimensions of the actual window in pixels
		unsigned short window_width;
		unsigned short window_height;
		
		// dimensions of the rendering area in pixels
		unsigned short render_width;
		unsigned short render_height;
		unsigned short prev_width;
		unsigned short prev_height;
		unsigned char event_type;
		bool is_active;
	};
	
	class IKernel
	{
	public:
		IKernel() {}
		virtual ~IKernel(){}

		bool isActive() const;
		void setInactive();
		Params & parameters();
		
		virtual int config( kernel::Params & params ) = 0; // return kSuccess, kFailure, or kNoWindow
		virtual int startup( kernel::Params & params ) = 0; // return kSuccess, kFailure, or kNoWindow
		virtual void tick( kernel::Params & params ) = 0; // called every frame
//		virtual void step( kernel::Params & params ) = 0; // called every step interval milliseconds
//		virtual void event( kernel::Params & event ) = 0;
		
		virtual void shutdown() = 0;
		virtual const char * classname() = 0;
	};


	typedef IKernel * (*Creator)();
	struct Registrar
	{
		Registrar( const char * name, Creator fn );
	};
	
	//
	// kernel registration / search
	#define DECLARE_KERNEL( className ) \
		public: static IKernel * create() { return new className; }\
		public: virtual const char * classname() { return #className; }

	#define IMPLEMENT_KERNEL( className ) \
		kernel::Registrar kr_##className( #className, className::create )
	
	Error main( int argc, char ** argv, const char * kernel_name );
	IKernel * instance();
};