#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <vector>
using namespace std;

#include <gemini/mem.h>
#include <gemini/core.h>
#include <gemini/core/filesystem.h>
#include <gemini/core/log.h>
#include <gemini/util/stackstring.h>
#include <gemini/core/xfile.h>
#include <gemini/util/fixedarray.h>
#include <gemini/util/arg.h>

#include <slim/xlog.h>

#include <json/json.h>



template< typename T>
void fill( vector<int>& v, T done )
{
	int i = 0;
	while(!done())
	{
		v.push_back(i++);
	}
}



void test_function()
{
//	[](){ printf("hello\n"); }();

	vector<int> v;
	
	// capture by reference
	fill(v, [&](){ return v.size() >= 8; } );


	fill(v,
		 [&](){ int sum = 0;
			 for_each(begin(v), end(v), [&](int i){ sum+= 1; });
			 return sum >= 10;
		 }
	 );
	 
	int loop = 0;
	for_each(begin(v), end(v), [&](int i){
		printf("%i -> %i\n", loop++, i);
	});
}


int main(int argc, char** argv)
{
	memory::startup();
	core::startup();
	test_function();
	
	core::shutdown();
	memory::shutdown();
	return 0;
}